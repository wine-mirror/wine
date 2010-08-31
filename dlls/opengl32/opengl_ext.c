
/* Auto-generated file... Do not edit ! */

#include "config.h"
#include "opengl_ext.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(opengl);

enum extensions
{
    EXT_glActiveProgramEXT,
    EXT_glActiveShaderProgram,
    EXT_glActiveStencilFaceEXT,
    EXT_glActiveTexture,
    EXT_glActiveTextureARB,
    EXT_glActiveVaryingNV,
    EXT_glAlphaFragmentOp1ATI,
    EXT_glAlphaFragmentOp2ATI,
    EXT_glAlphaFragmentOp3ATI,
    EXT_glApplyTextureEXT,
    EXT_glAreProgramsResidentNV,
    EXT_glAreTexturesResidentEXT,
    EXT_glArrayElementEXT,
    EXT_glArrayObjectATI,
    EXT_glAsyncMarkerSGIX,
    EXT_glAttachObjectARB,
    EXT_glAttachShader,
    EXT_glBeginConditionalRender,
    EXT_glBeginConditionalRenderNV,
    EXT_glBeginFragmentShaderATI,
    EXT_glBeginOcclusionQueryNV,
    EXT_glBeginPerfMonitorAMD,
    EXT_glBeginQuery,
    EXT_glBeginQueryARB,
    EXT_glBeginQueryIndexed,
    EXT_glBeginTransformFeedback,
    EXT_glBeginTransformFeedbackEXT,
    EXT_glBeginTransformFeedbackNV,
    EXT_glBeginVertexShaderEXT,
    EXT_glBeginVideoCaptureNV,
    EXT_glBindAttribLocation,
    EXT_glBindAttribLocationARB,
    EXT_glBindBuffer,
    EXT_glBindBufferARB,
    EXT_glBindBufferBase,
    EXT_glBindBufferBaseEXT,
    EXT_glBindBufferBaseNV,
    EXT_glBindBufferOffsetEXT,
    EXT_glBindBufferOffsetNV,
    EXT_glBindBufferRange,
    EXT_glBindBufferRangeEXT,
    EXT_glBindBufferRangeNV,
    EXT_glBindFragDataLocation,
    EXT_glBindFragDataLocationEXT,
    EXT_glBindFragDataLocationIndexed,
    EXT_glBindFragmentShaderATI,
    EXT_glBindFramebuffer,
    EXT_glBindFramebufferEXT,
    EXT_glBindImageTextureEXT,
    EXT_glBindLightParameterEXT,
    EXT_glBindMaterialParameterEXT,
    EXT_glBindMultiTextureEXT,
    EXT_glBindParameterEXT,
    EXT_glBindProgramARB,
    EXT_glBindProgramNV,
    EXT_glBindProgramPipeline,
    EXT_glBindRenderbuffer,
    EXT_glBindRenderbufferEXT,
    EXT_glBindSampler,
    EXT_glBindTexGenParameterEXT,
    EXT_glBindTextureEXT,
    EXT_glBindTextureUnitParameterEXT,
    EXT_glBindTransformFeedback,
    EXT_glBindTransformFeedbackNV,
    EXT_glBindVertexArray,
    EXT_glBindVertexArrayAPPLE,
    EXT_glBindVertexShaderEXT,
    EXT_glBindVideoCaptureStreamBufferNV,
    EXT_glBindVideoCaptureStreamTextureNV,
    EXT_glBinormal3bEXT,
    EXT_glBinormal3bvEXT,
    EXT_glBinormal3dEXT,
    EXT_glBinormal3dvEXT,
    EXT_glBinormal3fEXT,
    EXT_glBinormal3fvEXT,
    EXT_glBinormal3iEXT,
    EXT_glBinormal3ivEXT,
    EXT_glBinormal3sEXT,
    EXT_glBinormal3svEXT,
    EXT_glBinormalPointerEXT,
    EXT_glBlendColor,
    EXT_glBlendColorEXT,
    EXT_glBlendEquation,
    EXT_glBlendEquationEXT,
    EXT_glBlendEquationIndexedAMD,
    EXT_glBlendEquationSeparate,
    EXT_glBlendEquationSeparateEXT,
    EXT_glBlendEquationSeparateIndexedAMD,
    EXT_glBlendEquationSeparatei,
    EXT_glBlendEquationSeparateiARB,
    EXT_glBlendEquationi,
    EXT_glBlendEquationiARB,
    EXT_glBlendFuncIndexedAMD,
    EXT_glBlendFuncSeparate,
    EXT_glBlendFuncSeparateEXT,
    EXT_glBlendFuncSeparateINGR,
    EXT_glBlendFuncSeparateIndexedAMD,
    EXT_glBlendFuncSeparatei,
    EXT_glBlendFuncSeparateiARB,
    EXT_glBlendFunci,
    EXT_glBlendFunciARB,
    EXT_glBlitFramebuffer,
    EXT_glBlitFramebufferEXT,
    EXT_glBufferAddressRangeNV,
    EXT_glBufferData,
    EXT_glBufferDataARB,
    EXT_glBufferParameteriAPPLE,
    EXT_glBufferRegionEnabled,
    EXT_glBufferSubData,
    EXT_glBufferSubDataARB,
    EXT_glCheckFramebufferStatus,
    EXT_glCheckFramebufferStatusEXT,
    EXT_glCheckNamedFramebufferStatusEXT,
    EXT_glClampColor,
    EXT_glClampColorARB,
    EXT_glClearBufferfi,
    EXT_glClearBufferfv,
    EXT_glClearBufferiv,
    EXT_glClearBufferuiv,
    EXT_glClearColorIiEXT,
    EXT_glClearColorIuiEXT,
    EXT_glClearDepthdNV,
    EXT_glClearDepthf,
    EXT_glClientActiveTexture,
    EXT_glClientActiveTextureARB,
    EXT_glClientActiveVertexStreamATI,
    EXT_glClientAttribDefaultEXT,
    EXT_glClientWaitSync,
    EXT_glColor3fVertex3fSUN,
    EXT_glColor3fVertex3fvSUN,
    EXT_glColor3hNV,
    EXT_glColor3hvNV,
    EXT_glColor4fNormal3fVertex3fSUN,
    EXT_glColor4fNormal3fVertex3fvSUN,
    EXT_glColor4hNV,
    EXT_glColor4hvNV,
    EXT_glColor4ubVertex2fSUN,
    EXT_glColor4ubVertex2fvSUN,
    EXT_glColor4ubVertex3fSUN,
    EXT_glColor4ubVertex3fvSUN,
    EXT_glColorFormatNV,
    EXT_glColorFragmentOp1ATI,
    EXT_glColorFragmentOp2ATI,
    EXT_glColorFragmentOp3ATI,
    EXT_glColorMaskIndexedEXT,
    EXT_glColorMaski,
    EXT_glColorP3ui,
    EXT_glColorP3uiv,
    EXT_glColorP4ui,
    EXT_glColorP4uiv,
    EXT_glColorPointerEXT,
    EXT_glColorPointerListIBM,
    EXT_glColorPointervINTEL,
    EXT_glColorSubTable,
    EXT_glColorSubTableEXT,
    EXT_glColorTable,
    EXT_glColorTableEXT,
    EXT_glColorTableParameterfv,
    EXT_glColorTableParameterfvSGI,
    EXT_glColorTableParameteriv,
    EXT_glColorTableParameterivSGI,
    EXT_glColorTableSGI,
    EXT_glCombinerInputNV,
    EXT_glCombinerOutputNV,
    EXT_glCombinerParameterfNV,
    EXT_glCombinerParameterfvNV,
    EXT_glCombinerParameteriNV,
    EXT_glCombinerParameterivNV,
    EXT_glCombinerStageParameterfvNV,
    EXT_glCompileShader,
    EXT_glCompileShaderARB,
    EXT_glCompileShaderIncludeARB,
    EXT_glCompressedMultiTexImage1DEXT,
    EXT_glCompressedMultiTexImage2DEXT,
    EXT_glCompressedMultiTexImage3DEXT,
    EXT_glCompressedMultiTexSubImage1DEXT,
    EXT_glCompressedMultiTexSubImage2DEXT,
    EXT_glCompressedMultiTexSubImage3DEXT,
    EXT_glCompressedTexImage1D,
    EXT_glCompressedTexImage1DARB,
    EXT_glCompressedTexImage2D,
    EXT_glCompressedTexImage2DARB,
    EXT_glCompressedTexImage3D,
    EXT_glCompressedTexImage3DARB,
    EXT_glCompressedTexSubImage1D,
    EXT_glCompressedTexSubImage1DARB,
    EXT_glCompressedTexSubImage2D,
    EXT_glCompressedTexSubImage2DARB,
    EXT_glCompressedTexSubImage3D,
    EXT_glCompressedTexSubImage3DARB,
    EXT_glCompressedTextureImage1DEXT,
    EXT_glCompressedTextureImage2DEXT,
    EXT_glCompressedTextureImage3DEXT,
    EXT_glCompressedTextureSubImage1DEXT,
    EXT_glCompressedTextureSubImage2DEXT,
    EXT_glCompressedTextureSubImage3DEXT,
    EXT_glConvolutionFilter1D,
    EXT_glConvolutionFilter1DEXT,
    EXT_glConvolutionFilter2D,
    EXT_glConvolutionFilter2DEXT,
    EXT_glConvolutionParameterf,
    EXT_glConvolutionParameterfEXT,
    EXT_glConvolutionParameterfv,
    EXT_glConvolutionParameterfvEXT,
    EXT_glConvolutionParameteri,
    EXT_glConvolutionParameteriEXT,
    EXT_glConvolutionParameteriv,
    EXT_glConvolutionParameterivEXT,
    EXT_glCopyBufferSubData,
    EXT_glCopyColorSubTable,
    EXT_glCopyColorSubTableEXT,
    EXT_glCopyColorTable,
    EXT_glCopyColorTableSGI,
    EXT_glCopyConvolutionFilter1D,
    EXT_glCopyConvolutionFilter1DEXT,
    EXT_glCopyConvolutionFilter2D,
    EXT_glCopyConvolutionFilter2DEXT,
    EXT_glCopyImageSubDataNV,
    EXT_glCopyMultiTexImage1DEXT,
    EXT_glCopyMultiTexImage2DEXT,
    EXT_glCopyMultiTexSubImage1DEXT,
    EXT_glCopyMultiTexSubImage2DEXT,
    EXT_glCopyMultiTexSubImage3DEXT,
    EXT_glCopyTexImage1DEXT,
    EXT_glCopyTexImage2DEXT,
    EXT_glCopyTexSubImage1DEXT,
    EXT_glCopyTexSubImage2DEXT,
    EXT_glCopyTexSubImage3D,
    EXT_glCopyTexSubImage3DEXT,
    EXT_glCopyTextureImage1DEXT,
    EXT_glCopyTextureImage2DEXT,
    EXT_glCopyTextureSubImage1DEXT,
    EXT_glCopyTextureSubImage2DEXT,
    EXT_glCopyTextureSubImage3DEXT,
    EXT_glCreateProgram,
    EXT_glCreateProgramObjectARB,
    EXT_glCreateShader,
    EXT_glCreateShaderObjectARB,
    EXT_glCreateShaderProgramEXT,
    EXT_glCreateShaderProgramv,
    EXT_glCreateSyncFromCLeventARB,
    EXT_glCullParameterdvEXT,
    EXT_glCullParameterfvEXT,
    EXT_glCurrentPaletteMatrixARB,
    EXT_glDebugMessageCallbackAMD,
    EXT_glDebugMessageCallbackARB,
    EXT_glDebugMessageControlARB,
    EXT_glDebugMessageEnableAMD,
    EXT_glDebugMessageInsertAMD,
    EXT_glDebugMessageInsertARB,
    EXT_glDeformSGIX,
    EXT_glDeformationMap3dSGIX,
    EXT_glDeformationMap3fSGIX,
    EXT_glDeleteAsyncMarkersSGIX,
    EXT_glDeleteBufferRegion,
    EXT_glDeleteBuffers,
    EXT_glDeleteBuffersARB,
    EXT_glDeleteFencesAPPLE,
    EXT_glDeleteFencesNV,
    EXT_glDeleteFragmentShaderATI,
    EXT_glDeleteFramebuffers,
    EXT_glDeleteFramebuffersEXT,
    EXT_glDeleteNamedStringARB,
    EXT_glDeleteNamesAMD,
    EXT_glDeleteObjectARB,
    EXT_glDeleteObjectBufferATI,
    EXT_glDeleteOcclusionQueriesNV,
    EXT_glDeletePerfMonitorsAMD,
    EXT_glDeleteProgram,
    EXT_glDeleteProgramPipelines,
    EXT_glDeleteProgramsARB,
    EXT_glDeleteProgramsNV,
    EXT_glDeleteQueries,
    EXT_glDeleteQueriesARB,
    EXT_glDeleteRenderbuffers,
    EXT_glDeleteRenderbuffersEXT,
    EXT_glDeleteSamplers,
    EXT_glDeleteShader,
    EXT_glDeleteSync,
    EXT_glDeleteTexturesEXT,
    EXT_glDeleteTransformFeedbacks,
    EXT_glDeleteTransformFeedbacksNV,
    EXT_glDeleteVertexArrays,
    EXT_glDeleteVertexArraysAPPLE,
    EXT_glDeleteVertexShaderEXT,
    EXT_glDepthBoundsEXT,
    EXT_glDepthBoundsdNV,
    EXT_glDepthRangeArrayv,
    EXT_glDepthRangeIndexed,
    EXT_glDepthRangedNV,
    EXT_glDepthRangef,
    EXT_glDetachObjectARB,
    EXT_glDetachShader,
    EXT_glDetailTexFuncSGIS,
    EXT_glDisableClientStateIndexedEXT,
    EXT_glDisableIndexedEXT,
    EXT_glDisableVariantClientStateEXT,
    EXT_glDisableVertexAttribAPPLE,
    EXT_glDisableVertexAttribArray,
    EXT_glDisableVertexAttribArrayARB,
    EXT_glDisablei,
    EXT_glDrawArraysEXT,
    EXT_glDrawArraysIndirect,
    EXT_glDrawArraysInstanced,
    EXT_glDrawArraysInstancedARB,
    EXT_glDrawArraysInstancedEXT,
    EXT_glDrawBufferRegion,
    EXT_glDrawBuffers,
    EXT_glDrawBuffersARB,
    EXT_glDrawBuffersATI,
    EXT_glDrawElementArrayAPPLE,
    EXT_glDrawElementArrayATI,
    EXT_glDrawElementsBaseVertex,
    EXT_glDrawElementsIndirect,
    EXT_glDrawElementsInstanced,
    EXT_glDrawElementsInstancedARB,
    EXT_glDrawElementsInstancedBaseVertex,
    EXT_glDrawElementsInstancedEXT,
    EXT_glDrawMeshArraysSUN,
    EXT_glDrawRangeElementArrayAPPLE,
    EXT_glDrawRangeElementArrayATI,
    EXT_glDrawRangeElements,
    EXT_glDrawRangeElementsBaseVertex,
    EXT_glDrawRangeElementsEXT,
    EXT_glDrawTransformFeedback,
    EXT_glDrawTransformFeedbackNV,
    EXT_glDrawTransformFeedbackStream,
    EXT_glEdgeFlagFormatNV,
    EXT_glEdgeFlagPointerEXT,
    EXT_glEdgeFlagPointerListIBM,
    EXT_glElementPointerAPPLE,
    EXT_glElementPointerATI,
    EXT_glEnableClientStateIndexedEXT,
    EXT_glEnableIndexedEXT,
    EXT_glEnableVariantClientStateEXT,
    EXT_glEnableVertexAttribAPPLE,
    EXT_glEnableVertexAttribArray,
    EXT_glEnableVertexAttribArrayARB,
    EXT_glEnablei,
    EXT_glEndConditionalRender,
    EXT_glEndConditionalRenderNV,
    EXT_glEndFragmentShaderATI,
    EXT_glEndOcclusionQueryNV,
    EXT_glEndPerfMonitorAMD,
    EXT_glEndQuery,
    EXT_glEndQueryARB,
    EXT_glEndQueryIndexed,
    EXT_glEndTransformFeedback,
    EXT_glEndTransformFeedbackEXT,
    EXT_glEndTransformFeedbackNV,
    EXT_glEndVertexShaderEXT,
    EXT_glEndVideoCaptureNV,
    EXT_glEvalMapsNV,
    EXT_glExecuteProgramNV,
    EXT_glExtractComponentEXT,
    EXT_glFenceSync,
    EXT_glFinalCombinerInputNV,
    EXT_glFinishAsyncSGIX,
    EXT_glFinishFenceAPPLE,
    EXT_glFinishFenceNV,
    EXT_glFinishObjectAPPLE,
    EXT_glFinishTextureSUNX,
    EXT_glFlushMappedBufferRange,
    EXT_glFlushMappedBufferRangeAPPLE,
    EXT_glFlushMappedNamedBufferRangeEXT,
    EXT_glFlushPixelDataRangeNV,
    EXT_glFlushRasterSGIX,
    EXT_glFlushVertexArrayRangeAPPLE,
    EXT_glFlushVertexArrayRangeNV,
    EXT_glFogCoordFormatNV,
    EXT_glFogCoordPointer,
    EXT_glFogCoordPointerEXT,
    EXT_glFogCoordPointerListIBM,
    EXT_glFogCoordd,
    EXT_glFogCoorddEXT,
    EXT_glFogCoorddv,
    EXT_glFogCoorddvEXT,
    EXT_glFogCoordf,
    EXT_glFogCoordfEXT,
    EXT_glFogCoordfv,
    EXT_glFogCoordfvEXT,
    EXT_glFogCoordhNV,
    EXT_glFogCoordhvNV,
    EXT_glFogFuncSGIS,
    EXT_glFragmentColorMaterialSGIX,
    EXT_glFragmentLightModelfSGIX,
    EXT_glFragmentLightModelfvSGIX,
    EXT_glFragmentLightModeliSGIX,
    EXT_glFragmentLightModelivSGIX,
    EXT_glFragmentLightfSGIX,
    EXT_glFragmentLightfvSGIX,
    EXT_glFragmentLightiSGIX,
    EXT_glFragmentLightivSGIX,
    EXT_glFragmentMaterialfSGIX,
    EXT_glFragmentMaterialfvSGIX,
    EXT_glFragmentMaterialiSGIX,
    EXT_glFragmentMaterialivSGIX,
    EXT_glFrameTerminatorGREMEDY,
    EXT_glFrameZoomSGIX,
    EXT_glFramebufferDrawBufferEXT,
    EXT_glFramebufferDrawBuffersEXT,
    EXT_glFramebufferReadBufferEXT,
    EXT_glFramebufferRenderbuffer,
    EXT_glFramebufferRenderbufferEXT,
    EXT_glFramebufferTexture,
    EXT_glFramebufferTexture1D,
    EXT_glFramebufferTexture1DEXT,
    EXT_glFramebufferTexture2D,
    EXT_glFramebufferTexture2DEXT,
    EXT_glFramebufferTexture3D,
    EXT_glFramebufferTexture3DEXT,
    EXT_glFramebufferTextureARB,
    EXT_glFramebufferTextureEXT,
    EXT_glFramebufferTextureFaceARB,
    EXT_glFramebufferTextureFaceEXT,
    EXT_glFramebufferTextureLayer,
    EXT_glFramebufferTextureLayerARB,
    EXT_glFramebufferTextureLayerEXT,
    EXT_glFreeObjectBufferATI,
    EXT_glGenAsyncMarkersSGIX,
    EXT_glGenBuffers,
    EXT_glGenBuffersARB,
    EXT_glGenFencesAPPLE,
    EXT_glGenFencesNV,
    EXT_glGenFragmentShadersATI,
    EXT_glGenFramebuffers,
    EXT_glGenFramebuffersEXT,
    EXT_glGenNamesAMD,
    EXT_glGenOcclusionQueriesNV,
    EXT_glGenPerfMonitorsAMD,
    EXT_glGenProgramPipelines,
    EXT_glGenProgramsARB,
    EXT_glGenProgramsNV,
    EXT_glGenQueries,
    EXT_glGenQueriesARB,
    EXT_glGenRenderbuffers,
    EXT_glGenRenderbuffersEXT,
    EXT_glGenSamplers,
    EXT_glGenSymbolsEXT,
    EXT_glGenTexturesEXT,
    EXT_glGenTransformFeedbacks,
    EXT_glGenTransformFeedbacksNV,
    EXT_glGenVertexArrays,
    EXT_glGenVertexArraysAPPLE,
    EXT_glGenVertexShadersEXT,
    EXT_glGenerateMipmap,
    EXT_glGenerateMipmapEXT,
    EXT_glGenerateMultiTexMipmapEXT,
    EXT_glGenerateTextureMipmapEXT,
    EXT_glGetActiveAttrib,
    EXT_glGetActiveAttribARB,
    EXT_glGetActiveSubroutineName,
    EXT_glGetActiveSubroutineUniformName,
    EXT_glGetActiveSubroutineUniformiv,
    EXT_glGetActiveUniform,
    EXT_glGetActiveUniformARB,
    EXT_glGetActiveUniformBlockName,
    EXT_glGetActiveUniformBlockiv,
    EXT_glGetActiveUniformName,
    EXT_glGetActiveUniformsiv,
    EXT_glGetActiveVaryingNV,
    EXT_glGetArrayObjectfvATI,
    EXT_glGetArrayObjectivATI,
    EXT_glGetAttachedObjectsARB,
    EXT_glGetAttachedShaders,
    EXT_glGetAttribLocation,
    EXT_glGetAttribLocationARB,
    EXT_glGetBooleanIndexedvEXT,
    EXT_glGetBooleani_v,
    EXT_glGetBufferParameteri64v,
    EXT_glGetBufferParameteriv,
    EXT_glGetBufferParameterivARB,
    EXT_glGetBufferParameterui64vNV,
    EXT_glGetBufferPointerv,
    EXT_glGetBufferPointervARB,
    EXT_glGetBufferSubData,
    EXT_glGetBufferSubDataARB,
    EXT_glGetColorTable,
    EXT_glGetColorTableEXT,
    EXT_glGetColorTableParameterfv,
    EXT_glGetColorTableParameterfvEXT,
    EXT_glGetColorTableParameterfvSGI,
    EXT_glGetColorTableParameteriv,
    EXT_glGetColorTableParameterivEXT,
    EXT_glGetColorTableParameterivSGI,
    EXT_glGetColorTableSGI,
    EXT_glGetCombinerInputParameterfvNV,
    EXT_glGetCombinerInputParameterivNV,
    EXT_glGetCombinerOutputParameterfvNV,
    EXT_glGetCombinerOutputParameterivNV,
    EXT_glGetCombinerStageParameterfvNV,
    EXT_glGetCompressedMultiTexImageEXT,
    EXT_glGetCompressedTexImage,
    EXT_glGetCompressedTexImageARB,
    EXT_glGetCompressedTextureImageEXT,
    EXT_glGetConvolutionFilter,
    EXT_glGetConvolutionFilterEXT,
    EXT_glGetConvolutionParameterfv,
    EXT_glGetConvolutionParameterfvEXT,
    EXT_glGetConvolutionParameteriv,
    EXT_glGetConvolutionParameterivEXT,
    EXT_glGetDebugMessageLogAMD,
    EXT_glGetDebugMessageLogARB,
    EXT_glGetDetailTexFuncSGIS,
    EXT_glGetDoubleIndexedvEXT,
    EXT_glGetDoublei_v,
    EXT_glGetFenceivNV,
    EXT_glGetFinalCombinerInputParameterfvNV,
    EXT_glGetFinalCombinerInputParameterivNV,
    EXT_glGetFloatIndexedvEXT,
    EXT_glGetFloati_v,
    EXT_glGetFogFuncSGIS,
    EXT_glGetFragDataIndex,
    EXT_glGetFragDataLocation,
    EXT_glGetFragDataLocationEXT,
    EXT_glGetFragmentLightfvSGIX,
    EXT_glGetFragmentLightivSGIX,
    EXT_glGetFragmentMaterialfvSGIX,
    EXT_glGetFragmentMaterialivSGIX,
    EXT_glGetFramebufferAttachmentParameteriv,
    EXT_glGetFramebufferAttachmentParameterivEXT,
    EXT_glGetFramebufferParameterivEXT,
    EXT_glGetGraphicsResetStatusARB,
    EXT_glGetHandleARB,
    EXT_glGetHistogram,
    EXT_glGetHistogramEXT,
    EXT_glGetHistogramParameterfv,
    EXT_glGetHistogramParameterfvEXT,
    EXT_glGetHistogramParameteriv,
    EXT_glGetHistogramParameterivEXT,
    EXT_glGetImageTransformParameterfvHP,
    EXT_glGetImageTransformParameterivHP,
    EXT_glGetInfoLogARB,
    EXT_glGetInstrumentsSGIX,
    EXT_glGetInteger64i_v,
    EXT_glGetInteger64v,
    EXT_glGetIntegerIndexedvEXT,
    EXT_glGetIntegeri_v,
    EXT_glGetIntegerui64i_vNV,
    EXT_glGetIntegerui64vNV,
    EXT_glGetInvariantBooleanvEXT,
    EXT_glGetInvariantFloatvEXT,
    EXT_glGetInvariantIntegervEXT,
    EXT_glGetListParameterfvSGIX,
    EXT_glGetListParameterivSGIX,
    EXT_glGetLocalConstantBooleanvEXT,
    EXT_glGetLocalConstantFloatvEXT,
    EXT_glGetLocalConstantIntegervEXT,
    EXT_glGetMapAttribParameterfvNV,
    EXT_glGetMapAttribParameterivNV,
    EXT_glGetMapControlPointsNV,
    EXT_glGetMapParameterfvNV,
    EXT_glGetMapParameterivNV,
    EXT_glGetMinmax,
    EXT_glGetMinmaxEXT,
    EXT_glGetMinmaxParameterfv,
    EXT_glGetMinmaxParameterfvEXT,
    EXT_glGetMinmaxParameteriv,
    EXT_glGetMinmaxParameterivEXT,
    EXT_glGetMultiTexEnvfvEXT,
    EXT_glGetMultiTexEnvivEXT,
    EXT_glGetMultiTexGendvEXT,
    EXT_glGetMultiTexGenfvEXT,
    EXT_glGetMultiTexGenivEXT,
    EXT_glGetMultiTexImageEXT,
    EXT_glGetMultiTexLevelParameterfvEXT,
    EXT_glGetMultiTexLevelParameterivEXT,
    EXT_glGetMultiTexParameterIivEXT,
    EXT_glGetMultiTexParameterIuivEXT,
    EXT_glGetMultiTexParameterfvEXT,
    EXT_glGetMultiTexParameterivEXT,
    EXT_glGetMultisamplefv,
    EXT_glGetMultisamplefvNV,
    EXT_glGetNamedBufferParameterivEXT,
    EXT_glGetNamedBufferParameterui64vNV,
    EXT_glGetNamedBufferPointervEXT,
    EXT_glGetNamedBufferSubDataEXT,
    EXT_glGetNamedFramebufferAttachmentParameterivEXT,
    EXT_glGetNamedProgramLocalParameterIivEXT,
    EXT_glGetNamedProgramLocalParameterIuivEXT,
    EXT_glGetNamedProgramLocalParameterdvEXT,
    EXT_glGetNamedProgramLocalParameterfvEXT,
    EXT_glGetNamedProgramStringEXT,
    EXT_glGetNamedProgramivEXT,
    EXT_glGetNamedRenderbufferParameterivEXT,
    EXT_glGetNamedStringARB,
    EXT_glGetNamedStringivARB,
    EXT_glGetObjectBufferfvATI,
    EXT_glGetObjectBufferivATI,
    EXT_glGetObjectParameterfvARB,
    EXT_glGetObjectParameterivAPPLE,
    EXT_glGetObjectParameterivARB,
    EXT_glGetOcclusionQueryivNV,
    EXT_glGetOcclusionQueryuivNV,
    EXT_glGetPerfMonitorCounterDataAMD,
    EXT_glGetPerfMonitorCounterInfoAMD,
    EXT_glGetPerfMonitorCounterStringAMD,
    EXT_glGetPerfMonitorCountersAMD,
    EXT_glGetPerfMonitorGroupStringAMD,
    EXT_glGetPerfMonitorGroupsAMD,
    EXT_glGetPixelTexGenParameterfvSGIS,
    EXT_glGetPixelTexGenParameterivSGIS,
    EXT_glGetPointerIndexedvEXT,
    EXT_glGetPointervEXT,
    EXT_glGetProgramBinary,
    EXT_glGetProgramEnvParameterIivNV,
    EXT_glGetProgramEnvParameterIuivNV,
    EXT_glGetProgramEnvParameterdvARB,
    EXT_glGetProgramEnvParameterfvARB,
    EXT_glGetProgramInfoLog,
    EXT_glGetProgramLocalParameterIivNV,
    EXT_glGetProgramLocalParameterIuivNV,
    EXT_glGetProgramLocalParameterdvARB,
    EXT_glGetProgramLocalParameterfvARB,
    EXT_glGetProgramNamedParameterdvNV,
    EXT_glGetProgramNamedParameterfvNV,
    EXT_glGetProgramParameterdvNV,
    EXT_glGetProgramParameterfvNV,
    EXT_glGetProgramPipelineInfoLog,
    EXT_glGetProgramPipelineiv,
    EXT_glGetProgramStageiv,
    EXT_glGetProgramStringARB,
    EXT_glGetProgramStringNV,
    EXT_glGetProgramSubroutineParameteruivNV,
    EXT_glGetProgramiv,
    EXT_glGetProgramivARB,
    EXT_glGetProgramivNV,
    EXT_glGetQueryIndexediv,
    EXT_glGetQueryObjecti64v,
    EXT_glGetQueryObjecti64vEXT,
    EXT_glGetQueryObjectiv,
    EXT_glGetQueryObjectivARB,
    EXT_glGetQueryObjectui64v,
    EXT_glGetQueryObjectui64vEXT,
    EXT_glGetQueryObjectuiv,
    EXT_glGetQueryObjectuivARB,
    EXT_glGetQueryiv,
    EXT_glGetQueryivARB,
    EXT_glGetRenderbufferParameteriv,
    EXT_glGetRenderbufferParameterivEXT,
    EXT_glGetSamplerParameterIiv,
    EXT_glGetSamplerParameterIuiv,
    EXT_glGetSamplerParameterfv,
    EXT_glGetSamplerParameteriv,
    EXT_glGetSeparableFilter,
    EXT_glGetSeparableFilterEXT,
    EXT_glGetShaderInfoLog,
    EXT_glGetShaderPrecisionFormat,
    EXT_glGetShaderSource,
    EXT_glGetShaderSourceARB,
    EXT_glGetShaderiv,
    EXT_glGetSharpenTexFuncSGIS,
    EXT_glGetStringi,
    EXT_glGetSubroutineIndex,
    EXT_glGetSubroutineUniformLocation,
    EXT_glGetSynciv,
    EXT_glGetTexBumpParameterfvATI,
    EXT_glGetTexBumpParameterivATI,
    EXT_glGetTexFilterFuncSGIS,
    EXT_glGetTexParameterIiv,
    EXT_glGetTexParameterIivEXT,
    EXT_glGetTexParameterIuiv,
    EXT_glGetTexParameterIuivEXT,
    EXT_glGetTexParameterPointervAPPLE,
    EXT_glGetTextureImageEXT,
    EXT_glGetTextureLevelParameterfvEXT,
    EXT_glGetTextureLevelParameterivEXT,
    EXT_glGetTextureParameterIivEXT,
    EXT_glGetTextureParameterIuivEXT,
    EXT_glGetTextureParameterfvEXT,
    EXT_glGetTextureParameterivEXT,
    EXT_glGetTrackMatrixivNV,
    EXT_glGetTransformFeedbackVarying,
    EXT_glGetTransformFeedbackVaryingEXT,
    EXT_glGetTransformFeedbackVaryingNV,
    EXT_glGetUniformBlockIndex,
    EXT_glGetUniformBufferSizeEXT,
    EXT_glGetUniformIndices,
    EXT_glGetUniformLocation,
    EXT_glGetUniformLocationARB,
    EXT_glGetUniformOffsetEXT,
    EXT_glGetUniformSubroutineuiv,
    EXT_glGetUniformdv,
    EXT_glGetUniformfv,
    EXT_glGetUniformfvARB,
    EXT_glGetUniformi64vNV,
    EXT_glGetUniformiv,
    EXT_glGetUniformivARB,
    EXT_glGetUniformui64vNV,
    EXT_glGetUniformuiv,
    EXT_glGetUniformuivEXT,
    EXT_glGetVariantArrayObjectfvATI,
    EXT_glGetVariantArrayObjectivATI,
    EXT_glGetVariantBooleanvEXT,
    EXT_glGetVariantFloatvEXT,
    EXT_glGetVariantIntegervEXT,
    EXT_glGetVariantPointervEXT,
    EXT_glGetVaryingLocationNV,
    EXT_glGetVertexAttribArrayObjectfvATI,
    EXT_glGetVertexAttribArrayObjectivATI,
    EXT_glGetVertexAttribIiv,
    EXT_glGetVertexAttribIivEXT,
    EXT_glGetVertexAttribIuiv,
    EXT_glGetVertexAttribIuivEXT,
    EXT_glGetVertexAttribLdv,
    EXT_glGetVertexAttribLdvEXT,
    EXT_glGetVertexAttribLi64vNV,
    EXT_glGetVertexAttribLui64vNV,
    EXT_glGetVertexAttribPointerv,
    EXT_glGetVertexAttribPointervARB,
    EXT_glGetVertexAttribPointervNV,
    EXT_glGetVertexAttribdv,
    EXT_glGetVertexAttribdvARB,
    EXT_glGetVertexAttribdvNV,
    EXT_glGetVertexAttribfv,
    EXT_glGetVertexAttribfvARB,
    EXT_glGetVertexAttribfvNV,
    EXT_glGetVertexAttribiv,
    EXT_glGetVertexAttribivARB,
    EXT_glGetVertexAttribivNV,
    EXT_glGetVideoCaptureStreamdvNV,
    EXT_glGetVideoCaptureStreamfvNV,
    EXT_glGetVideoCaptureStreamivNV,
    EXT_glGetVideoCaptureivNV,
    EXT_glGetVideoi64vNV,
    EXT_glGetVideoivNV,
    EXT_glGetVideoui64vNV,
    EXT_glGetVideouivNV,
    EXT_glGetnColorTableARB,
    EXT_glGetnCompressedTexImageARB,
    EXT_glGetnConvolutionFilterARB,
    EXT_glGetnHistogramARB,
    EXT_glGetnMapdvARB,
    EXT_glGetnMapfvARB,
    EXT_glGetnMapivARB,
    EXT_glGetnMinmaxARB,
    EXT_glGetnPixelMapfvARB,
    EXT_glGetnPixelMapuivARB,
    EXT_glGetnPixelMapusvARB,
    EXT_glGetnPolygonStippleARB,
    EXT_glGetnSeparableFilterARB,
    EXT_glGetnTexImageARB,
    EXT_glGetnUniformdvARB,
    EXT_glGetnUniformfvARB,
    EXT_glGetnUniformivARB,
    EXT_glGetnUniformuivARB,
    EXT_glGlobalAlphaFactorbSUN,
    EXT_glGlobalAlphaFactordSUN,
    EXT_glGlobalAlphaFactorfSUN,
    EXT_glGlobalAlphaFactoriSUN,
    EXT_glGlobalAlphaFactorsSUN,
    EXT_glGlobalAlphaFactorubSUN,
    EXT_glGlobalAlphaFactoruiSUN,
    EXT_glGlobalAlphaFactorusSUN,
    EXT_glHintPGI,
    EXT_glHistogram,
    EXT_glHistogramEXT,
    EXT_glIglooInterfaceSGIX,
    EXT_glImageTransformParameterfHP,
    EXT_glImageTransformParameterfvHP,
    EXT_glImageTransformParameteriHP,
    EXT_glImageTransformParameterivHP,
    EXT_glIndexFormatNV,
    EXT_glIndexFuncEXT,
    EXT_glIndexMaterialEXT,
    EXT_glIndexPointerEXT,
    EXT_glIndexPointerListIBM,
    EXT_glInsertComponentEXT,
    EXT_glInstrumentsBufferSGIX,
    EXT_glIsAsyncMarkerSGIX,
    EXT_glIsBuffer,
    EXT_glIsBufferARB,
    EXT_glIsBufferResidentNV,
    EXT_glIsEnabledIndexedEXT,
    EXT_glIsEnabledi,
    EXT_glIsFenceAPPLE,
    EXT_glIsFenceNV,
    EXT_glIsFramebuffer,
    EXT_glIsFramebufferEXT,
    EXT_glIsNameAMD,
    EXT_glIsNamedBufferResidentNV,
    EXT_glIsNamedStringARB,
    EXT_glIsObjectBufferATI,
    EXT_glIsOcclusionQueryNV,
    EXT_glIsProgram,
    EXT_glIsProgramARB,
    EXT_glIsProgramNV,
    EXT_glIsProgramPipeline,
    EXT_glIsQuery,
    EXT_glIsQueryARB,
    EXT_glIsRenderbuffer,
    EXT_glIsRenderbufferEXT,
    EXT_glIsSampler,
    EXT_glIsShader,
    EXT_glIsSync,
    EXT_glIsTextureEXT,
    EXT_glIsTransformFeedback,
    EXT_glIsTransformFeedbackNV,
    EXT_glIsVariantEnabledEXT,
    EXT_glIsVertexArray,
    EXT_glIsVertexArrayAPPLE,
    EXT_glIsVertexAttribEnabledAPPLE,
    EXT_glLightEnviSGIX,
    EXT_glLinkProgram,
    EXT_glLinkProgramARB,
    EXT_glListParameterfSGIX,
    EXT_glListParameterfvSGIX,
    EXT_glListParameteriSGIX,
    EXT_glListParameterivSGIX,
    EXT_glLoadIdentityDeformationMapSGIX,
    EXT_glLoadProgramNV,
    EXT_glLoadTransposeMatrixd,
    EXT_glLoadTransposeMatrixdARB,
    EXT_glLoadTransposeMatrixf,
    EXT_glLoadTransposeMatrixfARB,
    EXT_glLockArraysEXT,
    EXT_glMTexCoord2fSGIS,
    EXT_glMTexCoord2fvSGIS,
    EXT_glMakeBufferNonResidentNV,
    EXT_glMakeBufferResidentNV,
    EXT_glMakeNamedBufferNonResidentNV,
    EXT_glMakeNamedBufferResidentNV,
    EXT_glMapBuffer,
    EXT_glMapBufferARB,
    EXT_glMapBufferRange,
    EXT_glMapControlPointsNV,
    EXT_glMapNamedBufferEXT,
    EXT_glMapNamedBufferRangeEXT,
    EXT_glMapObjectBufferATI,
    EXT_glMapParameterfvNV,
    EXT_glMapParameterivNV,
    EXT_glMapVertexAttrib1dAPPLE,
    EXT_glMapVertexAttrib1fAPPLE,
    EXT_glMapVertexAttrib2dAPPLE,
    EXT_glMapVertexAttrib2fAPPLE,
    EXT_glMatrixFrustumEXT,
    EXT_glMatrixIndexPointerARB,
    EXT_glMatrixIndexubvARB,
    EXT_glMatrixIndexuivARB,
    EXT_glMatrixIndexusvARB,
    EXT_glMatrixLoadIdentityEXT,
    EXT_glMatrixLoadTransposedEXT,
    EXT_glMatrixLoadTransposefEXT,
    EXT_glMatrixLoaddEXT,
    EXT_glMatrixLoadfEXT,
    EXT_glMatrixMultTransposedEXT,
    EXT_glMatrixMultTransposefEXT,
    EXT_glMatrixMultdEXT,
    EXT_glMatrixMultfEXT,
    EXT_glMatrixOrthoEXT,
    EXT_glMatrixPopEXT,
    EXT_glMatrixPushEXT,
    EXT_glMatrixRotatedEXT,
    EXT_glMatrixRotatefEXT,
    EXT_glMatrixScaledEXT,
    EXT_glMatrixScalefEXT,
    EXT_glMatrixTranslatedEXT,
    EXT_glMatrixTranslatefEXT,
    EXT_glMemoryBarrierEXT,
    EXT_glMinSampleShading,
    EXT_glMinSampleShadingARB,
    EXT_glMinmax,
    EXT_glMinmaxEXT,
    EXT_glMultTransposeMatrixd,
    EXT_glMultTransposeMatrixdARB,
    EXT_glMultTransposeMatrixf,
    EXT_glMultTransposeMatrixfARB,
    EXT_glMultiDrawArrays,
    EXT_glMultiDrawArraysEXT,
    EXT_glMultiDrawElementArrayAPPLE,
    EXT_glMultiDrawElements,
    EXT_glMultiDrawElementsBaseVertex,
    EXT_glMultiDrawElementsEXT,
    EXT_glMultiDrawRangeElementArrayAPPLE,
    EXT_glMultiModeDrawArraysIBM,
    EXT_glMultiModeDrawElementsIBM,
    EXT_glMultiTexBufferEXT,
    EXT_glMultiTexCoord1d,
    EXT_glMultiTexCoord1dARB,
    EXT_glMultiTexCoord1dSGIS,
    EXT_glMultiTexCoord1dv,
    EXT_glMultiTexCoord1dvARB,
    EXT_glMultiTexCoord1dvSGIS,
    EXT_glMultiTexCoord1f,
    EXT_glMultiTexCoord1fARB,
    EXT_glMultiTexCoord1fSGIS,
    EXT_glMultiTexCoord1fv,
    EXT_glMultiTexCoord1fvARB,
    EXT_glMultiTexCoord1fvSGIS,
    EXT_glMultiTexCoord1hNV,
    EXT_glMultiTexCoord1hvNV,
    EXT_glMultiTexCoord1i,
    EXT_glMultiTexCoord1iARB,
    EXT_glMultiTexCoord1iSGIS,
    EXT_glMultiTexCoord1iv,
    EXT_glMultiTexCoord1ivARB,
    EXT_glMultiTexCoord1ivSGIS,
    EXT_glMultiTexCoord1s,
    EXT_glMultiTexCoord1sARB,
    EXT_glMultiTexCoord1sSGIS,
    EXT_glMultiTexCoord1sv,
    EXT_glMultiTexCoord1svARB,
    EXT_glMultiTexCoord1svSGIS,
    EXT_glMultiTexCoord2d,
    EXT_glMultiTexCoord2dARB,
    EXT_glMultiTexCoord2dSGIS,
    EXT_glMultiTexCoord2dv,
    EXT_glMultiTexCoord2dvARB,
    EXT_glMultiTexCoord2dvSGIS,
    EXT_glMultiTexCoord2f,
    EXT_glMultiTexCoord2fARB,
    EXT_glMultiTexCoord2fSGIS,
    EXT_glMultiTexCoord2fv,
    EXT_glMultiTexCoord2fvARB,
    EXT_glMultiTexCoord2fvSGIS,
    EXT_glMultiTexCoord2hNV,
    EXT_glMultiTexCoord2hvNV,
    EXT_glMultiTexCoord2i,
    EXT_glMultiTexCoord2iARB,
    EXT_glMultiTexCoord2iSGIS,
    EXT_glMultiTexCoord2iv,
    EXT_glMultiTexCoord2ivARB,
    EXT_glMultiTexCoord2ivSGIS,
    EXT_glMultiTexCoord2s,
    EXT_glMultiTexCoord2sARB,
    EXT_glMultiTexCoord2sSGIS,
    EXT_glMultiTexCoord2sv,
    EXT_glMultiTexCoord2svARB,
    EXT_glMultiTexCoord2svSGIS,
    EXT_glMultiTexCoord3d,
    EXT_glMultiTexCoord3dARB,
    EXT_glMultiTexCoord3dSGIS,
    EXT_glMultiTexCoord3dv,
    EXT_glMultiTexCoord3dvARB,
    EXT_glMultiTexCoord3dvSGIS,
    EXT_glMultiTexCoord3f,
    EXT_glMultiTexCoord3fARB,
    EXT_glMultiTexCoord3fSGIS,
    EXT_glMultiTexCoord3fv,
    EXT_glMultiTexCoord3fvARB,
    EXT_glMultiTexCoord3fvSGIS,
    EXT_glMultiTexCoord3hNV,
    EXT_glMultiTexCoord3hvNV,
    EXT_glMultiTexCoord3i,
    EXT_glMultiTexCoord3iARB,
    EXT_glMultiTexCoord3iSGIS,
    EXT_glMultiTexCoord3iv,
    EXT_glMultiTexCoord3ivARB,
    EXT_glMultiTexCoord3ivSGIS,
    EXT_glMultiTexCoord3s,
    EXT_glMultiTexCoord3sARB,
    EXT_glMultiTexCoord3sSGIS,
    EXT_glMultiTexCoord3sv,
    EXT_glMultiTexCoord3svARB,
    EXT_glMultiTexCoord3svSGIS,
    EXT_glMultiTexCoord4d,
    EXT_glMultiTexCoord4dARB,
    EXT_glMultiTexCoord4dSGIS,
    EXT_glMultiTexCoord4dv,
    EXT_glMultiTexCoord4dvARB,
    EXT_glMultiTexCoord4dvSGIS,
    EXT_glMultiTexCoord4f,
    EXT_glMultiTexCoord4fARB,
    EXT_glMultiTexCoord4fSGIS,
    EXT_glMultiTexCoord4fv,
    EXT_glMultiTexCoord4fvARB,
    EXT_glMultiTexCoord4fvSGIS,
    EXT_glMultiTexCoord4hNV,
    EXT_glMultiTexCoord4hvNV,
    EXT_glMultiTexCoord4i,
    EXT_glMultiTexCoord4iARB,
    EXT_glMultiTexCoord4iSGIS,
    EXT_glMultiTexCoord4iv,
    EXT_glMultiTexCoord4ivARB,
    EXT_glMultiTexCoord4ivSGIS,
    EXT_glMultiTexCoord4s,
    EXT_glMultiTexCoord4sARB,
    EXT_glMultiTexCoord4sSGIS,
    EXT_glMultiTexCoord4sv,
    EXT_glMultiTexCoord4svARB,
    EXT_glMultiTexCoord4svSGIS,
    EXT_glMultiTexCoordP1ui,
    EXT_glMultiTexCoordP1uiv,
    EXT_glMultiTexCoordP2ui,
    EXT_glMultiTexCoordP2uiv,
    EXT_glMultiTexCoordP3ui,
    EXT_glMultiTexCoordP3uiv,
    EXT_glMultiTexCoordP4ui,
    EXT_glMultiTexCoordP4uiv,
    EXT_glMultiTexCoordPointerEXT,
    EXT_glMultiTexCoordPointerSGIS,
    EXT_glMultiTexEnvfEXT,
    EXT_glMultiTexEnvfvEXT,
    EXT_glMultiTexEnviEXT,
    EXT_glMultiTexEnvivEXT,
    EXT_glMultiTexGendEXT,
    EXT_glMultiTexGendvEXT,
    EXT_glMultiTexGenfEXT,
    EXT_glMultiTexGenfvEXT,
    EXT_glMultiTexGeniEXT,
    EXT_glMultiTexGenivEXT,
    EXT_glMultiTexImage1DEXT,
    EXT_glMultiTexImage2DEXT,
    EXT_glMultiTexImage3DEXT,
    EXT_glMultiTexParameterIivEXT,
    EXT_glMultiTexParameterIuivEXT,
    EXT_glMultiTexParameterfEXT,
    EXT_glMultiTexParameterfvEXT,
    EXT_glMultiTexParameteriEXT,
    EXT_glMultiTexParameterivEXT,
    EXT_glMultiTexRenderbufferEXT,
    EXT_glMultiTexSubImage1DEXT,
    EXT_glMultiTexSubImage2DEXT,
    EXT_glMultiTexSubImage3DEXT,
    EXT_glNamedBufferDataEXT,
    EXT_glNamedBufferSubDataEXT,
    EXT_glNamedCopyBufferSubDataEXT,
    EXT_glNamedFramebufferRenderbufferEXT,
    EXT_glNamedFramebufferTexture1DEXT,
    EXT_glNamedFramebufferTexture2DEXT,
    EXT_glNamedFramebufferTexture3DEXT,
    EXT_glNamedFramebufferTextureEXT,
    EXT_glNamedFramebufferTextureFaceEXT,
    EXT_glNamedFramebufferTextureLayerEXT,
    EXT_glNamedProgramLocalParameter4dEXT,
    EXT_glNamedProgramLocalParameter4dvEXT,
    EXT_glNamedProgramLocalParameter4fEXT,
    EXT_glNamedProgramLocalParameter4fvEXT,
    EXT_glNamedProgramLocalParameterI4iEXT,
    EXT_glNamedProgramLocalParameterI4ivEXT,
    EXT_glNamedProgramLocalParameterI4uiEXT,
    EXT_glNamedProgramLocalParameterI4uivEXT,
    EXT_glNamedProgramLocalParameters4fvEXT,
    EXT_glNamedProgramLocalParametersI4ivEXT,
    EXT_glNamedProgramLocalParametersI4uivEXT,
    EXT_glNamedProgramStringEXT,
    EXT_glNamedRenderbufferStorageEXT,
    EXT_glNamedRenderbufferStorageMultisampleCoverageEXT,
    EXT_glNamedRenderbufferStorageMultisampleEXT,
    EXT_glNamedStringARB,
    EXT_glNewBufferRegion,
    EXT_glNewObjectBufferATI,
    EXT_glNormal3fVertex3fSUN,
    EXT_glNormal3fVertex3fvSUN,
    EXT_glNormal3hNV,
    EXT_glNormal3hvNV,
    EXT_glNormalFormatNV,
    EXT_glNormalP3ui,
    EXT_glNormalP3uiv,
    EXT_glNormalPointerEXT,
    EXT_glNormalPointerListIBM,
    EXT_glNormalPointervINTEL,
    EXT_glNormalStream3bATI,
    EXT_glNormalStream3bvATI,
    EXT_glNormalStream3dATI,
    EXT_glNormalStream3dvATI,
    EXT_glNormalStream3fATI,
    EXT_glNormalStream3fvATI,
    EXT_glNormalStream3iATI,
    EXT_glNormalStream3ivATI,
    EXT_glNormalStream3sATI,
    EXT_glNormalStream3svATI,
    EXT_glObjectPurgeableAPPLE,
    EXT_glObjectUnpurgeableAPPLE,
    EXT_glPNTrianglesfATI,
    EXT_glPNTrianglesiATI,
    EXT_glPassTexCoordATI,
    EXT_glPatchParameterfv,
    EXT_glPatchParameteri,
    EXT_glPauseTransformFeedback,
    EXT_glPauseTransformFeedbackNV,
    EXT_glPixelDataRangeNV,
    EXT_glPixelTexGenParameterfSGIS,
    EXT_glPixelTexGenParameterfvSGIS,
    EXT_glPixelTexGenParameteriSGIS,
    EXT_glPixelTexGenParameterivSGIS,
    EXT_glPixelTexGenSGIX,
    EXT_glPixelTransformParameterfEXT,
    EXT_glPixelTransformParameterfvEXT,
    EXT_glPixelTransformParameteriEXT,
    EXT_glPixelTransformParameterivEXT,
    EXT_glPointParameterf,
    EXT_glPointParameterfARB,
    EXT_glPointParameterfEXT,
    EXT_glPointParameterfSGIS,
    EXT_glPointParameterfv,
    EXT_glPointParameterfvARB,
    EXT_glPointParameterfvEXT,
    EXT_glPointParameterfvSGIS,
    EXT_glPointParameteri,
    EXT_glPointParameteriNV,
    EXT_glPointParameteriv,
    EXT_glPointParameterivNV,
    EXT_glPollAsyncSGIX,
    EXT_glPollInstrumentsSGIX,
    EXT_glPolygonOffsetEXT,
    EXT_glPresentFrameDualFillNV,
    EXT_glPresentFrameKeyedNV,
    EXT_glPrimitiveRestartIndex,
    EXT_glPrimitiveRestartIndexNV,
    EXT_glPrimitiveRestartNV,
    EXT_glPrioritizeTexturesEXT,
    EXT_glProgramBinary,
    EXT_glProgramBufferParametersIivNV,
    EXT_glProgramBufferParametersIuivNV,
    EXT_glProgramBufferParametersfvNV,
    EXT_glProgramEnvParameter4dARB,
    EXT_glProgramEnvParameter4dvARB,
    EXT_glProgramEnvParameter4fARB,
    EXT_glProgramEnvParameter4fvARB,
    EXT_glProgramEnvParameterI4iNV,
    EXT_glProgramEnvParameterI4ivNV,
    EXT_glProgramEnvParameterI4uiNV,
    EXT_glProgramEnvParameterI4uivNV,
    EXT_glProgramEnvParameters4fvEXT,
    EXT_glProgramEnvParametersI4ivNV,
    EXT_glProgramEnvParametersI4uivNV,
    EXT_glProgramLocalParameter4dARB,
    EXT_glProgramLocalParameter4dvARB,
    EXT_glProgramLocalParameter4fARB,
    EXT_glProgramLocalParameter4fvARB,
    EXT_glProgramLocalParameterI4iNV,
    EXT_glProgramLocalParameterI4ivNV,
    EXT_glProgramLocalParameterI4uiNV,
    EXT_glProgramLocalParameterI4uivNV,
    EXT_glProgramLocalParameters4fvEXT,
    EXT_glProgramLocalParametersI4ivNV,
    EXT_glProgramLocalParametersI4uivNV,
    EXT_glProgramNamedParameter4dNV,
    EXT_glProgramNamedParameter4dvNV,
    EXT_glProgramNamedParameter4fNV,
    EXT_glProgramNamedParameter4fvNV,
    EXT_glProgramParameter4dNV,
    EXT_glProgramParameter4dvNV,
    EXT_glProgramParameter4fNV,
    EXT_glProgramParameter4fvNV,
    EXT_glProgramParameteri,
    EXT_glProgramParameteriARB,
    EXT_glProgramParameteriEXT,
    EXT_glProgramParameters4dvNV,
    EXT_glProgramParameters4fvNV,
    EXT_glProgramStringARB,
    EXT_glProgramSubroutineParametersuivNV,
    EXT_glProgramUniform1d,
    EXT_glProgramUniform1dEXT,
    EXT_glProgramUniform1dv,
    EXT_glProgramUniform1dvEXT,
    EXT_glProgramUniform1f,
    EXT_glProgramUniform1fEXT,
    EXT_glProgramUniform1fv,
    EXT_glProgramUniform1fvEXT,
    EXT_glProgramUniform1i,
    EXT_glProgramUniform1i64NV,
    EXT_glProgramUniform1i64vNV,
    EXT_glProgramUniform1iEXT,
    EXT_glProgramUniform1iv,
    EXT_glProgramUniform1ivEXT,
    EXT_glProgramUniform1ui,
    EXT_glProgramUniform1ui64NV,
    EXT_glProgramUniform1ui64vNV,
    EXT_glProgramUniform1uiEXT,
    EXT_glProgramUniform1uiv,
    EXT_glProgramUniform1uivEXT,
    EXT_glProgramUniform2d,
    EXT_glProgramUniform2dEXT,
    EXT_glProgramUniform2dv,
    EXT_glProgramUniform2dvEXT,
    EXT_glProgramUniform2f,
    EXT_glProgramUniform2fEXT,
    EXT_glProgramUniform2fv,
    EXT_glProgramUniform2fvEXT,
    EXT_glProgramUniform2i,
    EXT_glProgramUniform2i64NV,
    EXT_glProgramUniform2i64vNV,
    EXT_glProgramUniform2iEXT,
    EXT_glProgramUniform2iv,
    EXT_glProgramUniform2ivEXT,
    EXT_glProgramUniform2ui,
    EXT_glProgramUniform2ui64NV,
    EXT_glProgramUniform2ui64vNV,
    EXT_glProgramUniform2uiEXT,
    EXT_glProgramUniform2uiv,
    EXT_glProgramUniform2uivEXT,
    EXT_glProgramUniform3d,
    EXT_glProgramUniform3dEXT,
    EXT_glProgramUniform3dv,
    EXT_glProgramUniform3dvEXT,
    EXT_glProgramUniform3f,
    EXT_glProgramUniform3fEXT,
    EXT_glProgramUniform3fv,
    EXT_glProgramUniform3fvEXT,
    EXT_glProgramUniform3i,
    EXT_glProgramUniform3i64NV,
    EXT_glProgramUniform3i64vNV,
    EXT_glProgramUniform3iEXT,
    EXT_glProgramUniform3iv,
    EXT_glProgramUniform3ivEXT,
    EXT_glProgramUniform3ui,
    EXT_glProgramUniform3ui64NV,
    EXT_glProgramUniform3ui64vNV,
    EXT_glProgramUniform3uiEXT,
    EXT_glProgramUniform3uiv,
    EXT_glProgramUniform3uivEXT,
    EXT_glProgramUniform4d,
    EXT_glProgramUniform4dEXT,
    EXT_glProgramUniform4dv,
    EXT_glProgramUniform4dvEXT,
    EXT_glProgramUniform4f,
    EXT_glProgramUniform4fEXT,
    EXT_glProgramUniform4fv,
    EXT_glProgramUniform4fvEXT,
    EXT_glProgramUniform4i,
    EXT_glProgramUniform4i64NV,
    EXT_glProgramUniform4i64vNV,
    EXT_glProgramUniform4iEXT,
    EXT_glProgramUniform4iv,
    EXT_glProgramUniform4ivEXT,
    EXT_glProgramUniform4ui,
    EXT_glProgramUniform4ui64NV,
    EXT_glProgramUniform4ui64vNV,
    EXT_glProgramUniform4uiEXT,
    EXT_glProgramUniform4uiv,
    EXT_glProgramUniform4uivEXT,
    EXT_glProgramUniformMatrix2dv,
    EXT_glProgramUniformMatrix2dvEXT,
    EXT_glProgramUniformMatrix2fv,
    EXT_glProgramUniformMatrix2fvEXT,
    EXT_glProgramUniformMatrix2x3dv,
    EXT_glProgramUniformMatrix2x3dvEXT,
    EXT_glProgramUniformMatrix2x3fv,
    EXT_glProgramUniformMatrix2x3fvEXT,
    EXT_glProgramUniformMatrix2x4dv,
    EXT_glProgramUniformMatrix2x4dvEXT,
    EXT_glProgramUniformMatrix2x4fv,
    EXT_glProgramUniformMatrix2x4fvEXT,
    EXT_glProgramUniformMatrix3dv,
    EXT_glProgramUniformMatrix3dvEXT,
    EXT_glProgramUniformMatrix3fv,
    EXT_glProgramUniformMatrix3fvEXT,
    EXT_glProgramUniformMatrix3x2dv,
    EXT_glProgramUniformMatrix3x2dvEXT,
    EXT_glProgramUniformMatrix3x2fv,
    EXT_glProgramUniformMatrix3x2fvEXT,
    EXT_glProgramUniformMatrix3x4dv,
    EXT_glProgramUniformMatrix3x4dvEXT,
    EXT_glProgramUniformMatrix3x4fv,
    EXT_glProgramUniformMatrix3x4fvEXT,
    EXT_glProgramUniformMatrix4dv,
    EXT_glProgramUniformMatrix4dvEXT,
    EXT_glProgramUniformMatrix4fv,
    EXT_glProgramUniformMatrix4fvEXT,
    EXT_glProgramUniformMatrix4x2dv,
    EXT_glProgramUniformMatrix4x2dvEXT,
    EXT_glProgramUniformMatrix4x2fv,
    EXT_glProgramUniformMatrix4x2fvEXT,
    EXT_glProgramUniformMatrix4x3dv,
    EXT_glProgramUniformMatrix4x3dvEXT,
    EXT_glProgramUniformMatrix4x3fv,
    EXT_glProgramUniformMatrix4x3fvEXT,
    EXT_glProgramUniformui64NV,
    EXT_glProgramUniformui64vNV,
    EXT_glProgramVertexLimitNV,
    EXT_glProvokingVertex,
    EXT_glProvokingVertexEXT,
    EXT_glPushClientAttribDefaultEXT,
    EXT_glQueryCounter,
    EXT_glReadBufferRegion,
    EXT_glReadInstrumentsSGIX,
    EXT_glReadnPixelsARB,
    EXT_glReferencePlaneSGIX,
    EXT_glReleaseShaderCompiler,
    EXT_glRenderbufferStorage,
    EXT_glRenderbufferStorageEXT,
    EXT_glRenderbufferStorageMultisample,
    EXT_glRenderbufferStorageMultisampleCoverageNV,
    EXT_glRenderbufferStorageMultisampleEXT,
    EXT_glReplacementCodePointerSUN,
    EXT_glReplacementCodeubSUN,
    EXT_glReplacementCodeubvSUN,
    EXT_glReplacementCodeuiColor3fVertex3fSUN,
    EXT_glReplacementCodeuiColor3fVertex3fvSUN,
    EXT_glReplacementCodeuiColor4fNormal3fVertex3fSUN,
    EXT_glReplacementCodeuiColor4fNormal3fVertex3fvSUN,
    EXT_glReplacementCodeuiColor4ubVertex3fSUN,
    EXT_glReplacementCodeuiColor4ubVertex3fvSUN,
    EXT_glReplacementCodeuiNormal3fVertex3fSUN,
    EXT_glReplacementCodeuiNormal3fVertex3fvSUN,
    EXT_glReplacementCodeuiSUN,
    EXT_glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fSUN,
    EXT_glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fvSUN,
    EXT_glReplacementCodeuiTexCoord2fNormal3fVertex3fSUN,
    EXT_glReplacementCodeuiTexCoord2fNormal3fVertex3fvSUN,
    EXT_glReplacementCodeuiTexCoord2fVertex3fSUN,
    EXT_glReplacementCodeuiTexCoord2fVertex3fvSUN,
    EXT_glReplacementCodeuiVertex3fSUN,
    EXT_glReplacementCodeuiVertex3fvSUN,
    EXT_glReplacementCodeuivSUN,
    EXT_glReplacementCodeusSUN,
    EXT_glReplacementCodeusvSUN,
    EXT_glRequestResidentProgramsNV,
    EXT_glResetHistogram,
    EXT_glResetHistogramEXT,
    EXT_glResetMinmax,
    EXT_glResetMinmaxEXT,
    EXT_glResizeBuffersMESA,
    EXT_glResumeTransformFeedback,
    EXT_glResumeTransformFeedbackNV,
    EXT_glSampleCoverage,
    EXT_glSampleCoverageARB,
    EXT_glSampleMapATI,
    EXT_glSampleMaskEXT,
    EXT_glSampleMaskIndexedNV,
    EXT_glSampleMaskSGIS,
    EXT_glSampleMaski,
    EXT_glSamplePatternEXT,
    EXT_glSamplePatternSGIS,
    EXT_glSamplerParameterIiv,
    EXT_glSamplerParameterIuiv,
    EXT_glSamplerParameterf,
    EXT_glSamplerParameterfv,
    EXT_glSamplerParameteri,
    EXT_glSamplerParameteriv,
    EXT_glScissorArrayv,
    EXT_glScissorIndexed,
    EXT_glScissorIndexedv,
    EXT_glSecondaryColor3b,
    EXT_glSecondaryColor3bEXT,
    EXT_glSecondaryColor3bv,
    EXT_glSecondaryColor3bvEXT,
    EXT_glSecondaryColor3d,
    EXT_glSecondaryColor3dEXT,
    EXT_glSecondaryColor3dv,
    EXT_glSecondaryColor3dvEXT,
    EXT_glSecondaryColor3f,
    EXT_glSecondaryColor3fEXT,
    EXT_glSecondaryColor3fv,
    EXT_glSecondaryColor3fvEXT,
    EXT_glSecondaryColor3hNV,
    EXT_glSecondaryColor3hvNV,
    EXT_glSecondaryColor3i,
    EXT_glSecondaryColor3iEXT,
    EXT_glSecondaryColor3iv,
    EXT_glSecondaryColor3ivEXT,
    EXT_glSecondaryColor3s,
    EXT_glSecondaryColor3sEXT,
    EXT_glSecondaryColor3sv,
    EXT_glSecondaryColor3svEXT,
    EXT_glSecondaryColor3ub,
    EXT_glSecondaryColor3ubEXT,
    EXT_glSecondaryColor3ubv,
    EXT_glSecondaryColor3ubvEXT,
    EXT_glSecondaryColor3ui,
    EXT_glSecondaryColor3uiEXT,
    EXT_glSecondaryColor3uiv,
    EXT_glSecondaryColor3uivEXT,
    EXT_glSecondaryColor3us,
    EXT_glSecondaryColor3usEXT,
    EXT_glSecondaryColor3usv,
    EXT_glSecondaryColor3usvEXT,
    EXT_glSecondaryColorFormatNV,
    EXT_glSecondaryColorP3ui,
    EXT_glSecondaryColorP3uiv,
    EXT_glSecondaryColorPointer,
    EXT_glSecondaryColorPointerEXT,
    EXT_glSecondaryColorPointerListIBM,
    EXT_glSelectPerfMonitorCountersAMD,
    EXT_glSelectTextureCoordSetSGIS,
    EXT_glSelectTextureSGIS,
    EXT_glSeparableFilter2D,
    EXT_glSeparableFilter2DEXT,
    EXT_glSetFenceAPPLE,
    EXT_glSetFenceNV,
    EXT_glSetFragmentShaderConstantATI,
    EXT_glSetInvariantEXT,
    EXT_glSetLocalConstantEXT,
    EXT_glShaderBinary,
    EXT_glShaderOp1EXT,
    EXT_glShaderOp2EXT,
    EXT_glShaderOp3EXT,
    EXT_glShaderSource,
    EXT_glShaderSourceARB,
    EXT_glSharpenTexFuncSGIS,
    EXT_glSpriteParameterfSGIX,
    EXT_glSpriteParameterfvSGIX,
    EXT_glSpriteParameteriSGIX,
    EXT_glSpriteParameterivSGIX,
    EXT_glStartInstrumentsSGIX,
    EXT_glStencilClearTagEXT,
    EXT_glStencilFuncSeparate,
    EXT_glStencilFuncSeparateATI,
    EXT_glStencilMaskSeparate,
    EXT_glStencilOpSeparate,
    EXT_glStencilOpSeparateATI,
    EXT_glStopInstrumentsSGIX,
    EXT_glStringMarkerGREMEDY,
    EXT_glSwizzleEXT,
    EXT_glTagSampleBufferSGIX,
    EXT_glTangent3bEXT,
    EXT_glTangent3bvEXT,
    EXT_glTangent3dEXT,
    EXT_glTangent3dvEXT,
    EXT_glTangent3fEXT,
    EXT_glTangent3fvEXT,
    EXT_glTangent3iEXT,
    EXT_glTangent3ivEXT,
    EXT_glTangent3sEXT,
    EXT_glTangent3svEXT,
    EXT_glTangentPointerEXT,
    EXT_glTbufferMask3DFX,
    EXT_glTessellationFactorAMD,
    EXT_glTessellationModeAMD,
    EXT_glTestFenceAPPLE,
    EXT_glTestFenceNV,
    EXT_glTestObjectAPPLE,
    EXT_glTexBuffer,
    EXT_glTexBufferARB,
    EXT_glTexBufferEXT,
    EXT_glTexBumpParameterfvATI,
    EXT_glTexBumpParameterivATI,
    EXT_glTexCoord1hNV,
    EXT_glTexCoord1hvNV,
    EXT_glTexCoord2fColor3fVertex3fSUN,
    EXT_glTexCoord2fColor3fVertex3fvSUN,
    EXT_glTexCoord2fColor4fNormal3fVertex3fSUN,
    EXT_glTexCoord2fColor4fNormal3fVertex3fvSUN,
    EXT_glTexCoord2fColor4ubVertex3fSUN,
    EXT_glTexCoord2fColor4ubVertex3fvSUN,
    EXT_glTexCoord2fNormal3fVertex3fSUN,
    EXT_glTexCoord2fNormal3fVertex3fvSUN,
    EXT_glTexCoord2fVertex3fSUN,
    EXT_glTexCoord2fVertex3fvSUN,
    EXT_glTexCoord2hNV,
    EXT_glTexCoord2hvNV,
    EXT_glTexCoord3hNV,
    EXT_glTexCoord3hvNV,
    EXT_glTexCoord4fColor4fNormal3fVertex4fSUN,
    EXT_glTexCoord4fColor4fNormal3fVertex4fvSUN,
    EXT_glTexCoord4fVertex4fSUN,
    EXT_glTexCoord4fVertex4fvSUN,
    EXT_glTexCoord4hNV,
    EXT_glTexCoord4hvNV,
    EXT_glTexCoordFormatNV,
    EXT_glTexCoordP1ui,
    EXT_glTexCoordP1uiv,
    EXT_glTexCoordP2ui,
    EXT_glTexCoordP2uiv,
    EXT_glTexCoordP3ui,
    EXT_glTexCoordP3uiv,
    EXT_glTexCoordP4ui,
    EXT_glTexCoordP4uiv,
    EXT_glTexCoordPointerEXT,
    EXT_glTexCoordPointerListIBM,
    EXT_glTexCoordPointervINTEL,
    EXT_glTexFilterFuncSGIS,
    EXT_glTexImage2DMultisample,
    EXT_glTexImage3D,
    EXT_glTexImage3DEXT,
    EXT_glTexImage3DMultisample,
    EXT_glTexImage4DSGIS,
    EXT_glTexParameterIiv,
    EXT_glTexParameterIivEXT,
    EXT_glTexParameterIuiv,
    EXT_glTexParameterIuivEXT,
    EXT_glTexRenderbufferNV,
    EXT_glTexSubImage1DEXT,
    EXT_glTexSubImage2DEXT,
    EXT_glTexSubImage3D,
    EXT_glTexSubImage3DEXT,
    EXT_glTexSubImage4DSGIS,
    EXT_glTextureBarrierNV,
    EXT_glTextureBufferEXT,
    EXT_glTextureColorMaskSGIS,
    EXT_glTextureImage1DEXT,
    EXT_glTextureImage2DEXT,
    EXT_glTextureImage3DEXT,
    EXT_glTextureLightEXT,
    EXT_glTextureMaterialEXT,
    EXT_glTextureNormalEXT,
    EXT_glTextureParameterIivEXT,
    EXT_glTextureParameterIuivEXT,
    EXT_glTextureParameterfEXT,
    EXT_glTextureParameterfvEXT,
    EXT_glTextureParameteriEXT,
    EXT_glTextureParameterivEXT,
    EXT_glTextureRangeAPPLE,
    EXT_glTextureRenderbufferEXT,
    EXT_glTextureSubImage1DEXT,
    EXT_glTextureSubImage2DEXT,
    EXT_glTextureSubImage3DEXT,
    EXT_glTrackMatrixNV,
    EXT_glTransformFeedbackAttribsNV,
    EXT_glTransformFeedbackStreamAttribsNV,
    EXT_glTransformFeedbackVaryings,
    EXT_glTransformFeedbackVaryingsEXT,
    EXT_glTransformFeedbackVaryingsNV,
    EXT_glUniform1d,
    EXT_glUniform1dv,
    EXT_glUniform1f,
    EXT_glUniform1fARB,
    EXT_glUniform1fv,
    EXT_glUniform1fvARB,
    EXT_glUniform1i,
    EXT_glUniform1i64NV,
    EXT_glUniform1i64vNV,
    EXT_glUniform1iARB,
    EXT_glUniform1iv,
    EXT_glUniform1ivARB,
    EXT_glUniform1ui,
    EXT_glUniform1ui64NV,
    EXT_glUniform1ui64vNV,
    EXT_glUniform1uiEXT,
    EXT_glUniform1uiv,
    EXT_glUniform1uivEXT,
    EXT_glUniform2d,
    EXT_glUniform2dv,
    EXT_glUniform2f,
    EXT_glUniform2fARB,
    EXT_glUniform2fv,
    EXT_glUniform2fvARB,
    EXT_glUniform2i,
    EXT_glUniform2i64NV,
    EXT_glUniform2i64vNV,
    EXT_glUniform2iARB,
    EXT_glUniform2iv,
    EXT_glUniform2ivARB,
    EXT_glUniform2ui,
    EXT_glUniform2ui64NV,
    EXT_glUniform2ui64vNV,
    EXT_glUniform2uiEXT,
    EXT_glUniform2uiv,
    EXT_glUniform2uivEXT,
    EXT_glUniform3d,
    EXT_glUniform3dv,
    EXT_glUniform3f,
    EXT_glUniform3fARB,
    EXT_glUniform3fv,
    EXT_glUniform3fvARB,
    EXT_glUniform3i,
    EXT_glUniform3i64NV,
    EXT_glUniform3i64vNV,
    EXT_glUniform3iARB,
    EXT_glUniform3iv,
    EXT_glUniform3ivARB,
    EXT_glUniform3ui,
    EXT_glUniform3ui64NV,
    EXT_glUniform3ui64vNV,
    EXT_glUniform3uiEXT,
    EXT_glUniform3uiv,
    EXT_glUniform3uivEXT,
    EXT_glUniform4d,
    EXT_glUniform4dv,
    EXT_glUniform4f,
    EXT_glUniform4fARB,
    EXT_glUniform4fv,
    EXT_glUniform4fvARB,
    EXT_glUniform4i,
    EXT_glUniform4i64NV,
    EXT_glUniform4i64vNV,
    EXT_glUniform4iARB,
    EXT_glUniform4iv,
    EXT_glUniform4ivARB,
    EXT_glUniform4ui,
    EXT_glUniform4ui64NV,
    EXT_glUniform4ui64vNV,
    EXT_glUniform4uiEXT,
    EXT_glUniform4uiv,
    EXT_glUniform4uivEXT,
    EXT_glUniformBlockBinding,
    EXT_glUniformBufferEXT,
    EXT_glUniformMatrix2dv,
    EXT_glUniformMatrix2fv,
    EXT_glUniformMatrix2fvARB,
    EXT_glUniformMatrix2x3dv,
    EXT_glUniformMatrix2x3fv,
    EXT_glUniformMatrix2x4dv,
    EXT_glUniformMatrix2x4fv,
    EXT_glUniformMatrix3dv,
    EXT_glUniformMatrix3fv,
    EXT_glUniformMatrix3fvARB,
    EXT_glUniformMatrix3x2dv,
    EXT_glUniformMatrix3x2fv,
    EXT_glUniformMatrix3x4dv,
    EXT_glUniformMatrix3x4fv,
    EXT_glUniformMatrix4dv,
    EXT_glUniformMatrix4fv,
    EXT_glUniformMatrix4fvARB,
    EXT_glUniformMatrix4x2dv,
    EXT_glUniformMatrix4x2fv,
    EXT_glUniformMatrix4x3dv,
    EXT_glUniformMatrix4x3fv,
    EXT_glUniformSubroutinesuiv,
    EXT_glUniformui64NV,
    EXT_glUniformui64vNV,
    EXT_glUnlockArraysEXT,
    EXT_glUnmapBuffer,
    EXT_glUnmapBufferARB,
    EXT_glUnmapNamedBufferEXT,
    EXT_glUnmapObjectBufferATI,
    EXT_glUpdateObjectBufferATI,
    EXT_glUseProgram,
    EXT_glUseProgramObjectARB,
    EXT_glUseProgramStages,
    EXT_glUseShaderProgramEXT,
    EXT_glVDPAUFiniNV,
    EXT_glVDPAUGetSurfaceivNV,
    EXT_glVDPAUInitNV,
    EXT_glVDPAUIsSurfaceNV,
    EXT_glVDPAUMapSurfacesNV,
    EXT_glVDPAURegisterOutputSurfaceNV,
    EXT_glVDPAURegisterVideoSurfaceNV,
    EXT_glVDPAUSurfaceAccessNV,
    EXT_glVDPAUUnmapSurfacesNV,
    EXT_glVDPAUUnregisterSurfaceNV,
    EXT_glValidateProgram,
    EXT_glValidateProgramARB,
    EXT_glValidateProgramPipeline,
    EXT_glVariantArrayObjectATI,
    EXT_glVariantPointerEXT,
    EXT_glVariantbvEXT,
    EXT_glVariantdvEXT,
    EXT_glVariantfvEXT,
    EXT_glVariantivEXT,
    EXT_glVariantsvEXT,
    EXT_glVariantubvEXT,
    EXT_glVariantuivEXT,
    EXT_glVariantusvEXT,
    EXT_glVertex2hNV,
    EXT_glVertex2hvNV,
    EXT_glVertex3hNV,
    EXT_glVertex3hvNV,
    EXT_glVertex4hNV,
    EXT_glVertex4hvNV,
    EXT_glVertexArrayParameteriAPPLE,
    EXT_glVertexArrayRangeAPPLE,
    EXT_glVertexArrayRangeNV,
    EXT_glVertexArrayVertexAttribLOffsetEXT,
    EXT_glVertexAttrib1d,
    EXT_glVertexAttrib1dARB,
    EXT_glVertexAttrib1dNV,
    EXT_glVertexAttrib1dv,
    EXT_glVertexAttrib1dvARB,
    EXT_glVertexAttrib1dvNV,
    EXT_glVertexAttrib1f,
    EXT_glVertexAttrib1fARB,
    EXT_glVertexAttrib1fNV,
    EXT_glVertexAttrib1fv,
    EXT_glVertexAttrib1fvARB,
    EXT_glVertexAttrib1fvNV,
    EXT_glVertexAttrib1hNV,
    EXT_glVertexAttrib1hvNV,
    EXT_glVertexAttrib1s,
    EXT_glVertexAttrib1sARB,
    EXT_glVertexAttrib1sNV,
    EXT_glVertexAttrib1sv,
    EXT_glVertexAttrib1svARB,
    EXT_glVertexAttrib1svNV,
    EXT_glVertexAttrib2d,
    EXT_glVertexAttrib2dARB,
    EXT_glVertexAttrib2dNV,
    EXT_glVertexAttrib2dv,
    EXT_glVertexAttrib2dvARB,
    EXT_glVertexAttrib2dvNV,
    EXT_glVertexAttrib2f,
    EXT_glVertexAttrib2fARB,
    EXT_glVertexAttrib2fNV,
    EXT_glVertexAttrib2fv,
    EXT_glVertexAttrib2fvARB,
    EXT_glVertexAttrib2fvNV,
    EXT_glVertexAttrib2hNV,
    EXT_glVertexAttrib2hvNV,
    EXT_glVertexAttrib2s,
    EXT_glVertexAttrib2sARB,
    EXT_glVertexAttrib2sNV,
    EXT_glVertexAttrib2sv,
    EXT_glVertexAttrib2svARB,
    EXT_glVertexAttrib2svNV,
    EXT_glVertexAttrib3d,
    EXT_glVertexAttrib3dARB,
    EXT_glVertexAttrib3dNV,
    EXT_glVertexAttrib3dv,
    EXT_glVertexAttrib3dvARB,
    EXT_glVertexAttrib3dvNV,
    EXT_glVertexAttrib3f,
    EXT_glVertexAttrib3fARB,
    EXT_glVertexAttrib3fNV,
    EXT_glVertexAttrib3fv,
    EXT_glVertexAttrib3fvARB,
    EXT_glVertexAttrib3fvNV,
    EXT_glVertexAttrib3hNV,
    EXT_glVertexAttrib3hvNV,
    EXT_glVertexAttrib3s,
    EXT_glVertexAttrib3sARB,
    EXT_glVertexAttrib3sNV,
    EXT_glVertexAttrib3sv,
    EXT_glVertexAttrib3svARB,
    EXT_glVertexAttrib3svNV,
    EXT_glVertexAttrib4Nbv,
    EXT_glVertexAttrib4NbvARB,
    EXT_glVertexAttrib4Niv,
    EXT_glVertexAttrib4NivARB,
    EXT_glVertexAttrib4Nsv,
    EXT_glVertexAttrib4NsvARB,
    EXT_glVertexAttrib4Nub,
    EXT_glVertexAttrib4NubARB,
    EXT_glVertexAttrib4Nubv,
    EXT_glVertexAttrib4NubvARB,
    EXT_glVertexAttrib4Nuiv,
    EXT_glVertexAttrib4NuivARB,
    EXT_glVertexAttrib4Nusv,
    EXT_glVertexAttrib4NusvARB,
    EXT_glVertexAttrib4bv,
    EXT_glVertexAttrib4bvARB,
    EXT_glVertexAttrib4d,
    EXT_glVertexAttrib4dARB,
    EXT_glVertexAttrib4dNV,
    EXT_glVertexAttrib4dv,
    EXT_glVertexAttrib4dvARB,
    EXT_glVertexAttrib4dvNV,
    EXT_glVertexAttrib4f,
    EXT_glVertexAttrib4fARB,
    EXT_glVertexAttrib4fNV,
    EXT_glVertexAttrib4fv,
    EXT_glVertexAttrib4fvARB,
    EXT_glVertexAttrib4fvNV,
    EXT_glVertexAttrib4hNV,
    EXT_glVertexAttrib4hvNV,
    EXT_glVertexAttrib4iv,
    EXT_glVertexAttrib4ivARB,
    EXT_glVertexAttrib4s,
    EXT_glVertexAttrib4sARB,
    EXT_glVertexAttrib4sNV,
    EXT_glVertexAttrib4sv,
    EXT_glVertexAttrib4svARB,
    EXT_glVertexAttrib4svNV,
    EXT_glVertexAttrib4ubNV,
    EXT_glVertexAttrib4ubv,
    EXT_glVertexAttrib4ubvARB,
    EXT_glVertexAttrib4ubvNV,
    EXT_glVertexAttrib4uiv,
    EXT_glVertexAttrib4uivARB,
    EXT_glVertexAttrib4usv,
    EXT_glVertexAttrib4usvARB,
    EXT_glVertexAttribArrayObjectATI,
    EXT_glVertexAttribDivisor,
    EXT_glVertexAttribDivisorARB,
    EXT_glVertexAttribFormatNV,
    EXT_glVertexAttribI1i,
    EXT_glVertexAttribI1iEXT,
    EXT_glVertexAttribI1iv,
    EXT_glVertexAttribI1ivEXT,
    EXT_glVertexAttribI1ui,
    EXT_glVertexAttribI1uiEXT,
    EXT_glVertexAttribI1uiv,
    EXT_glVertexAttribI1uivEXT,
    EXT_glVertexAttribI2i,
    EXT_glVertexAttribI2iEXT,
    EXT_glVertexAttribI2iv,
    EXT_glVertexAttribI2ivEXT,
    EXT_glVertexAttribI2ui,
    EXT_glVertexAttribI2uiEXT,
    EXT_glVertexAttribI2uiv,
    EXT_glVertexAttribI2uivEXT,
    EXT_glVertexAttribI3i,
    EXT_glVertexAttribI3iEXT,
    EXT_glVertexAttribI3iv,
    EXT_glVertexAttribI3ivEXT,
    EXT_glVertexAttribI3ui,
    EXT_glVertexAttribI3uiEXT,
    EXT_glVertexAttribI3uiv,
    EXT_glVertexAttribI3uivEXT,
    EXT_glVertexAttribI4bv,
    EXT_glVertexAttribI4bvEXT,
    EXT_glVertexAttribI4i,
    EXT_glVertexAttribI4iEXT,
    EXT_glVertexAttribI4iv,
    EXT_glVertexAttribI4ivEXT,
    EXT_glVertexAttribI4sv,
    EXT_glVertexAttribI4svEXT,
    EXT_glVertexAttribI4ubv,
    EXT_glVertexAttribI4ubvEXT,
    EXT_glVertexAttribI4ui,
    EXT_glVertexAttribI4uiEXT,
    EXT_glVertexAttribI4uiv,
    EXT_glVertexAttribI4uivEXT,
    EXT_glVertexAttribI4usv,
    EXT_glVertexAttribI4usvEXT,
    EXT_glVertexAttribIFormatNV,
    EXT_glVertexAttribIPointer,
    EXT_glVertexAttribIPointerEXT,
    EXT_glVertexAttribL1d,
    EXT_glVertexAttribL1dEXT,
    EXT_glVertexAttribL1dv,
    EXT_glVertexAttribL1dvEXT,
    EXT_glVertexAttribL1i64NV,
    EXT_glVertexAttribL1i64vNV,
    EXT_glVertexAttribL1ui64NV,
    EXT_glVertexAttribL1ui64vNV,
    EXT_glVertexAttribL2d,
    EXT_glVertexAttribL2dEXT,
    EXT_glVertexAttribL2dv,
    EXT_glVertexAttribL2dvEXT,
    EXT_glVertexAttribL2i64NV,
    EXT_glVertexAttribL2i64vNV,
    EXT_glVertexAttribL2ui64NV,
    EXT_glVertexAttribL2ui64vNV,
    EXT_glVertexAttribL3d,
    EXT_glVertexAttribL3dEXT,
    EXT_glVertexAttribL3dv,
    EXT_glVertexAttribL3dvEXT,
    EXT_glVertexAttribL3i64NV,
    EXT_glVertexAttribL3i64vNV,
    EXT_glVertexAttribL3ui64NV,
    EXT_glVertexAttribL3ui64vNV,
    EXT_glVertexAttribL4d,
    EXT_glVertexAttribL4dEXT,
    EXT_glVertexAttribL4dv,
    EXT_glVertexAttribL4dvEXT,
    EXT_glVertexAttribL4i64NV,
    EXT_glVertexAttribL4i64vNV,
    EXT_glVertexAttribL4ui64NV,
    EXT_glVertexAttribL4ui64vNV,
    EXT_glVertexAttribLFormatNV,
    EXT_glVertexAttribLPointer,
    EXT_glVertexAttribLPointerEXT,
    EXT_glVertexAttribP1ui,
    EXT_glVertexAttribP1uiv,
    EXT_glVertexAttribP2ui,
    EXT_glVertexAttribP2uiv,
    EXT_glVertexAttribP3ui,
    EXT_glVertexAttribP3uiv,
    EXT_glVertexAttribP4ui,
    EXT_glVertexAttribP4uiv,
    EXT_glVertexAttribPointer,
    EXT_glVertexAttribPointerARB,
    EXT_glVertexAttribPointerNV,
    EXT_glVertexAttribs1dvNV,
    EXT_glVertexAttribs1fvNV,
    EXT_glVertexAttribs1hvNV,
    EXT_glVertexAttribs1svNV,
    EXT_glVertexAttribs2dvNV,
    EXT_glVertexAttribs2fvNV,
    EXT_glVertexAttribs2hvNV,
    EXT_glVertexAttribs2svNV,
    EXT_glVertexAttribs3dvNV,
    EXT_glVertexAttribs3fvNV,
    EXT_glVertexAttribs3hvNV,
    EXT_glVertexAttribs3svNV,
    EXT_glVertexAttribs4dvNV,
    EXT_glVertexAttribs4fvNV,
    EXT_glVertexAttribs4hvNV,
    EXT_glVertexAttribs4svNV,
    EXT_glVertexAttribs4ubvNV,
    EXT_glVertexBlendARB,
    EXT_glVertexBlendEnvfATI,
    EXT_glVertexBlendEnviATI,
    EXT_glVertexFormatNV,
    EXT_glVertexP2ui,
    EXT_glVertexP2uiv,
    EXT_glVertexP3ui,
    EXT_glVertexP3uiv,
    EXT_glVertexP4ui,
    EXT_glVertexP4uiv,
    EXT_glVertexPointerEXT,
    EXT_glVertexPointerListIBM,
    EXT_glVertexPointervINTEL,
    EXT_glVertexStream1dATI,
    EXT_glVertexStream1dvATI,
    EXT_glVertexStream1fATI,
    EXT_glVertexStream1fvATI,
    EXT_glVertexStream1iATI,
    EXT_glVertexStream1ivATI,
    EXT_glVertexStream1sATI,
    EXT_glVertexStream1svATI,
    EXT_glVertexStream2dATI,
    EXT_glVertexStream2dvATI,
    EXT_glVertexStream2fATI,
    EXT_glVertexStream2fvATI,
    EXT_glVertexStream2iATI,
    EXT_glVertexStream2ivATI,
    EXT_glVertexStream2sATI,
    EXT_glVertexStream2svATI,
    EXT_glVertexStream3dATI,
    EXT_glVertexStream3dvATI,
    EXT_glVertexStream3fATI,
    EXT_glVertexStream3fvATI,
    EXT_glVertexStream3iATI,
    EXT_glVertexStream3ivATI,
    EXT_glVertexStream3sATI,
    EXT_glVertexStream3svATI,
    EXT_glVertexStream4dATI,
    EXT_glVertexStream4dvATI,
    EXT_glVertexStream4fATI,
    EXT_glVertexStream4fvATI,
    EXT_glVertexStream4iATI,
    EXT_glVertexStream4ivATI,
    EXT_glVertexStream4sATI,
    EXT_glVertexStream4svATI,
    EXT_glVertexWeightPointerEXT,
    EXT_glVertexWeightfEXT,
    EXT_glVertexWeightfvEXT,
    EXT_glVertexWeighthNV,
    EXT_glVertexWeighthvNV,
    EXT_glVideoCaptureNV,
    EXT_glVideoCaptureStreamParameterdvNV,
    EXT_glVideoCaptureStreamParameterfvNV,
    EXT_glVideoCaptureStreamParameterivNV,
    EXT_glViewportArrayv,
    EXT_glViewportIndexedf,
    EXT_glViewportIndexedfv,
    EXT_glWaitSync,
    EXT_glWeightPointerARB,
    EXT_glWeightbvARB,
    EXT_glWeightdvARB,
    EXT_glWeightfvARB,
    EXT_glWeightivARB,
    EXT_glWeightsvARB,
    EXT_glWeightubvARB,
    EXT_glWeightuivARB,
    EXT_glWeightusvARB,
    EXT_glWindowPos2d,
    EXT_glWindowPos2dARB,
    EXT_glWindowPos2dMESA,
    EXT_glWindowPos2dv,
    EXT_glWindowPos2dvARB,
    EXT_glWindowPos2dvMESA,
    EXT_glWindowPos2f,
    EXT_glWindowPos2fARB,
    EXT_glWindowPos2fMESA,
    EXT_glWindowPos2fv,
    EXT_glWindowPos2fvARB,
    EXT_glWindowPos2fvMESA,
    EXT_glWindowPos2i,
    EXT_glWindowPos2iARB,
    EXT_glWindowPos2iMESA,
    EXT_glWindowPos2iv,
    EXT_glWindowPos2ivARB,
    EXT_glWindowPos2ivMESA,
    EXT_glWindowPos2s,
    EXT_glWindowPos2sARB,
    EXT_glWindowPos2sMESA,
    EXT_glWindowPos2sv,
    EXT_glWindowPos2svARB,
    EXT_glWindowPos2svMESA,
    EXT_glWindowPos3d,
    EXT_glWindowPos3dARB,
    EXT_glWindowPos3dMESA,
    EXT_glWindowPos3dv,
    EXT_glWindowPos3dvARB,
    EXT_glWindowPos3dvMESA,
    EXT_glWindowPos3f,
    EXT_glWindowPos3fARB,
    EXT_glWindowPos3fMESA,
    EXT_glWindowPos3fv,
    EXT_glWindowPos3fvARB,
    EXT_glWindowPos3fvMESA,
    EXT_glWindowPos3i,
    EXT_glWindowPos3iARB,
    EXT_glWindowPos3iMESA,
    EXT_glWindowPos3iv,
    EXT_glWindowPos3ivARB,
    EXT_glWindowPos3ivMESA,
    EXT_glWindowPos3s,
    EXT_glWindowPos3sARB,
    EXT_glWindowPos3sMESA,
    EXT_glWindowPos3sv,
    EXT_glWindowPos3svARB,
    EXT_glWindowPos3svMESA,
    EXT_glWindowPos4dMESA,
    EXT_glWindowPos4dvMESA,
    EXT_glWindowPos4fMESA,
    EXT_glWindowPos4fvMESA,
    EXT_glWindowPos4iMESA,
    EXT_glWindowPos4ivMESA,
    EXT_glWindowPos4sMESA,
    EXT_glWindowPos4svMESA,
    EXT_glWriteMaskEXT,
    NB_EXTENSIONS
};

const int extension_registry_size = NB_EXTENSIONS;
void *extension_funcs[NB_EXTENSIONS];

/* The thunks themselves....*/
static void WINAPI wine_glActiveProgramEXT( GLuint program ) {
  void (*func_glActiveProgramEXT)( GLuint ) = extension_funcs[EXT_glActiveProgramEXT];
  TRACE("(%d)\n", program );
  ENTER_GL();
  func_glActiveProgramEXT( program );
  LEAVE_GL();
}

static void WINAPI wine_glActiveShaderProgram( GLuint pipeline, GLuint program ) {
  void (*func_glActiveShaderProgram)( GLuint, GLuint ) = extension_funcs[EXT_glActiveShaderProgram];
  TRACE("(%d, %d)\n", pipeline, program );
  ENTER_GL();
  func_glActiveShaderProgram( pipeline, program );
  LEAVE_GL();
}

static void WINAPI wine_glActiveStencilFaceEXT( GLenum face ) {
  void (*func_glActiveStencilFaceEXT)( GLenum ) = extension_funcs[EXT_glActiveStencilFaceEXT];
  TRACE("(%d)\n", face );
  ENTER_GL();
  func_glActiveStencilFaceEXT( face );
  LEAVE_GL();
}

static void WINAPI wine_glActiveTexture( GLenum texture ) {
  void (*func_glActiveTexture)( GLenum ) = extension_funcs[EXT_glActiveTexture];
  TRACE("(%d)\n", texture );
  ENTER_GL();
  func_glActiveTexture( texture );
  LEAVE_GL();
}

static void WINAPI wine_glActiveTextureARB( GLenum texture ) {
  void (*func_glActiveTextureARB)( GLenum ) = extension_funcs[EXT_glActiveTextureARB];
  TRACE("(%d)\n", texture );
  ENTER_GL();
  func_glActiveTextureARB( texture );
  LEAVE_GL();
}

static void WINAPI wine_glActiveVaryingNV( GLuint program, char* name ) {
  void (*func_glActiveVaryingNV)( GLuint, char* ) = extension_funcs[EXT_glActiveVaryingNV];
  TRACE("(%d, %p)\n", program, name );
  ENTER_GL();
  func_glActiveVaryingNV( program, name );
  LEAVE_GL();
}

static void WINAPI wine_glAlphaFragmentOp1ATI( GLenum op, GLuint dst, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod ) {
  void (*func_glAlphaFragmentOp1ATI)( GLenum, GLuint, GLuint, GLuint, GLuint, GLuint ) = extension_funcs[EXT_glAlphaFragmentOp1ATI];
  TRACE("(%d, %d, %d, %d, %d, %d)\n", op, dst, dstMod, arg1, arg1Rep, arg1Mod );
  ENTER_GL();
  func_glAlphaFragmentOp1ATI( op, dst, dstMod, arg1, arg1Rep, arg1Mod );
  LEAVE_GL();
}

static void WINAPI wine_glAlphaFragmentOp2ATI( GLenum op, GLuint dst, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod, GLuint arg2, GLuint arg2Rep, GLuint arg2Mod ) {
  void (*func_glAlphaFragmentOp2ATI)( GLenum, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint ) = extension_funcs[EXT_glAlphaFragmentOp2ATI];
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %d)\n", op, dst, dstMod, arg1, arg1Rep, arg1Mod, arg2, arg2Rep, arg2Mod );
  ENTER_GL();
  func_glAlphaFragmentOp2ATI( op, dst, dstMod, arg1, arg1Rep, arg1Mod, arg2, arg2Rep, arg2Mod );
  LEAVE_GL();
}

static void WINAPI wine_glAlphaFragmentOp3ATI( GLenum op, GLuint dst, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod, GLuint arg2, GLuint arg2Rep, GLuint arg2Mod, GLuint arg3, GLuint arg3Rep, GLuint arg3Mod ) {
  void (*func_glAlphaFragmentOp3ATI)( GLenum, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint ) = extension_funcs[EXT_glAlphaFragmentOp3ATI];
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d)\n", op, dst, dstMod, arg1, arg1Rep, arg1Mod, arg2, arg2Rep, arg2Mod, arg3, arg3Rep, arg3Mod );
  ENTER_GL();
  func_glAlphaFragmentOp3ATI( op, dst, dstMod, arg1, arg1Rep, arg1Mod, arg2, arg2Rep, arg2Mod, arg3, arg3Rep, arg3Mod );
  LEAVE_GL();
}

static void WINAPI wine_glApplyTextureEXT( GLenum mode ) {
  void (*func_glApplyTextureEXT)( GLenum ) = extension_funcs[EXT_glApplyTextureEXT];
  TRACE("(%d)\n", mode );
  ENTER_GL();
  func_glApplyTextureEXT( mode );
  LEAVE_GL();
}

static GLboolean WINAPI wine_glAreProgramsResidentNV( GLsizei n, GLuint* programs, GLboolean* residences ) {
  GLboolean ret_value;
  GLboolean (*func_glAreProgramsResidentNV)( GLsizei, GLuint*, GLboolean* ) = extension_funcs[EXT_glAreProgramsResidentNV];
  TRACE("(%d, %p, %p)\n", n, programs, residences );
  ENTER_GL();
  ret_value = func_glAreProgramsResidentNV( n, programs, residences );
  LEAVE_GL();
  return ret_value;
}

static GLboolean WINAPI wine_glAreTexturesResidentEXT( GLsizei n, GLuint* textures, GLboolean* residences ) {
  GLboolean ret_value;
  GLboolean (*func_glAreTexturesResidentEXT)( GLsizei, GLuint*, GLboolean* ) = extension_funcs[EXT_glAreTexturesResidentEXT];
  TRACE("(%d, %p, %p)\n", n, textures, residences );
  ENTER_GL();
  ret_value = func_glAreTexturesResidentEXT( n, textures, residences );
  LEAVE_GL();
  return ret_value;
}

static void WINAPI wine_glArrayElementEXT( GLint i ) {
  void (*func_glArrayElementEXT)( GLint ) = extension_funcs[EXT_glArrayElementEXT];
  TRACE("(%d)\n", i );
  ENTER_GL();
  func_glArrayElementEXT( i );
  LEAVE_GL();
}

static void WINAPI wine_glArrayObjectATI( GLenum array, GLint size, GLenum type, GLsizei stride, GLuint buffer, GLuint offset ) {
  void (*func_glArrayObjectATI)( GLenum, GLint, GLenum, GLsizei, GLuint, GLuint ) = extension_funcs[EXT_glArrayObjectATI];
  TRACE("(%d, %d, %d, %d, %d, %d)\n", array, size, type, stride, buffer, offset );
  ENTER_GL();
  func_glArrayObjectATI( array, size, type, stride, buffer, offset );
  LEAVE_GL();
}

static void WINAPI wine_glAsyncMarkerSGIX( GLuint marker ) {
  void (*func_glAsyncMarkerSGIX)( GLuint ) = extension_funcs[EXT_glAsyncMarkerSGIX];
  TRACE("(%d)\n", marker );
  ENTER_GL();
  func_glAsyncMarkerSGIX( marker );
  LEAVE_GL();
}

static void WINAPI wine_glAttachObjectARB( unsigned int containerObj, unsigned int obj ) {
  void (*func_glAttachObjectARB)( unsigned int, unsigned int ) = extension_funcs[EXT_glAttachObjectARB];
  TRACE("(%d, %d)\n", containerObj, obj );
  ENTER_GL();
  func_glAttachObjectARB( containerObj, obj );
  LEAVE_GL();
}

static void WINAPI wine_glAttachShader( GLuint program, GLuint shader ) {
  void (*func_glAttachShader)( GLuint, GLuint ) = extension_funcs[EXT_glAttachShader];
  TRACE("(%d, %d)\n", program, shader );
  ENTER_GL();
  func_glAttachShader( program, shader );
  LEAVE_GL();
}

static void WINAPI wine_glBeginConditionalRender( GLuint id, GLenum mode ) {
  void (*func_glBeginConditionalRender)( GLuint, GLenum ) = extension_funcs[EXT_glBeginConditionalRender];
  TRACE("(%d, %d)\n", id, mode );
  ENTER_GL();
  func_glBeginConditionalRender( id, mode );
  LEAVE_GL();
}

static void WINAPI wine_glBeginConditionalRenderNV( GLuint id, GLenum mode ) {
  void (*func_glBeginConditionalRenderNV)( GLuint, GLenum ) = extension_funcs[EXT_glBeginConditionalRenderNV];
  TRACE("(%d, %d)\n", id, mode );
  ENTER_GL();
  func_glBeginConditionalRenderNV( id, mode );
  LEAVE_GL();
}

static void WINAPI wine_glBeginFragmentShaderATI( void ) {
  void (*func_glBeginFragmentShaderATI)( void ) = extension_funcs[EXT_glBeginFragmentShaderATI];
  TRACE("()\n");
  ENTER_GL();
  func_glBeginFragmentShaderATI( );
  LEAVE_GL();
}

static void WINAPI wine_glBeginOcclusionQueryNV( GLuint id ) {
  void (*func_glBeginOcclusionQueryNV)( GLuint ) = extension_funcs[EXT_glBeginOcclusionQueryNV];
  TRACE("(%d)\n", id );
  ENTER_GL();
  func_glBeginOcclusionQueryNV( id );
  LEAVE_GL();
}

static void WINAPI wine_glBeginPerfMonitorAMD( GLuint monitor ) {
  void (*func_glBeginPerfMonitorAMD)( GLuint ) = extension_funcs[EXT_glBeginPerfMonitorAMD];
  TRACE("(%d)\n", monitor );
  ENTER_GL();
  func_glBeginPerfMonitorAMD( monitor );
  LEAVE_GL();
}

static void WINAPI wine_glBeginQuery( GLenum target, GLuint id ) {
  void (*func_glBeginQuery)( GLenum, GLuint ) = extension_funcs[EXT_glBeginQuery];
  TRACE("(%d, %d)\n", target, id );
  ENTER_GL();
  func_glBeginQuery( target, id );
  LEAVE_GL();
}

static void WINAPI wine_glBeginQueryARB( GLenum target, GLuint id ) {
  void (*func_glBeginQueryARB)( GLenum, GLuint ) = extension_funcs[EXT_glBeginQueryARB];
  TRACE("(%d, %d)\n", target, id );
  ENTER_GL();
  func_glBeginQueryARB( target, id );
  LEAVE_GL();
}

static void WINAPI wine_glBeginQueryIndexed( GLenum target, GLuint index, GLuint id ) {
  void (*func_glBeginQueryIndexed)( GLenum, GLuint, GLuint ) = extension_funcs[EXT_glBeginQueryIndexed];
  TRACE("(%d, %d, %d)\n", target, index, id );
  ENTER_GL();
  func_glBeginQueryIndexed( target, index, id );
  LEAVE_GL();
}

static void WINAPI wine_glBeginTransformFeedback( GLenum primitiveMode ) {
  void (*func_glBeginTransformFeedback)( GLenum ) = extension_funcs[EXT_glBeginTransformFeedback];
  TRACE("(%d)\n", primitiveMode );
  ENTER_GL();
  func_glBeginTransformFeedback( primitiveMode );
  LEAVE_GL();
}

static void WINAPI wine_glBeginTransformFeedbackEXT( GLenum primitiveMode ) {
  void (*func_glBeginTransformFeedbackEXT)( GLenum ) = extension_funcs[EXT_glBeginTransformFeedbackEXT];
  TRACE("(%d)\n", primitiveMode );
  ENTER_GL();
  func_glBeginTransformFeedbackEXT( primitiveMode );
  LEAVE_GL();
}

static void WINAPI wine_glBeginTransformFeedbackNV( GLenum primitiveMode ) {
  void (*func_glBeginTransformFeedbackNV)( GLenum ) = extension_funcs[EXT_glBeginTransformFeedbackNV];
  TRACE("(%d)\n", primitiveMode );
  ENTER_GL();
  func_glBeginTransformFeedbackNV( primitiveMode );
  LEAVE_GL();
}

static void WINAPI wine_glBeginVertexShaderEXT( void ) {
  void (*func_glBeginVertexShaderEXT)( void ) = extension_funcs[EXT_glBeginVertexShaderEXT];
  TRACE("()\n");
  ENTER_GL();
  func_glBeginVertexShaderEXT( );
  LEAVE_GL();
}

static void WINAPI wine_glBeginVideoCaptureNV( GLuint video_capture_slot ) {
  void (*func_glBeginVideoCaptureNV)( GLuint ) = extension_funcs[EXT_glBeginVideoCaptureNV];
  TRACE("(%d)\n", video_capture_slot );
  ENTER_GL();
  func_glBeginVideoCaptureNV( video_capture_slot );
  LEAVE_GL();
}

static void WINAPI wine_glBindAttribLocation( GLuint program, GLuint index, char* name ) {
  void (*func_glBindAttribLocation)( GLuint, GLuint, char* ) = extension_funcs[EXT_glBindAttribLocation];
  TRACE("(%d, %d, %p)\n", program, index, name );
  ENTER_GL();
  func_glBindAttribLocation( program, index, name );
  LEAVE_GL();
}

static void WINAPI wine_glBindAttribLocationARB( unsigned int programObj, GLuint index, char* name ) {
  void (*func_glBindAttribLocationARB)( unsigned int, GLuint, char* ) = extension_funcs[EXT_glBindAttribLocationARB];
  TRACE("(%d, %d, %p)\n", programObj, index, name );
  ENTER_GL();
  func_glBindAttribLocationARB( programObj, index, name );
  LEAVE_GL();
}

static void WINAPI wine_glBindBuffer( GLenum target, GLuint buffer ) {
  void (*func_glBindBuffer)( GLenum, GLuint ) = extension_funcs[EXT_glBindBuffer];
  TRACE("(%d, %d)\n", target, buffer );
  ENTER_GL();
  func_glBindBuffer( target, buffer );
  LEAVE_GL();
}

static void WINAPI wine_glBindBufferARB( GLenum target, GLuint buffer ) {
  void (*func_glBindBufferARB)( GLenum, GLuint ) = extension_funcs[EXT_glBindBufferARB];
  TRACE("(%d, %d)\n", target, buffer );
  ENTER_GL();
  func_glBindBufferARB( target, buffer );
  LEAVE_GL();
}

static void WINAPI wine_glBindBufferBase( GLenum target, GLuint index, GLuint buffer ) {
  void (*func_glBindBufferBase)( GLenum, GLuint, GLuint ) = extension_funcs[EXT_glBindBufferBase];
  TRACE("(%d, %d, %d)\n", target, index, buffer );
  ENTER_GL();
  func_glBindBufferBase( target, index, buffer );
  LEAVE_GL();
}

static void WINAPI wine_glBindBufferBaseEXT( GLenum target, GLuint index, GLuint buffer ) {
  void (*func_glBindBufferBaseEXT)( GLenum, GLuint, GLuint ) = extension_funcs[EXT_glBindBufferBaseEXT];
  TRACE("(%d, %d, %d)\n", target, index, buffer );
  ENTER_GL();
  func_glBindBufferBaseEXT( target, index, buffer );
  LEAVE_GL();
}

static void WINAPI wine_glBindBufferBaseNV( GLenum target, GLuint index, GLuint buffer ) {
  void (*func_glBindBufferBaseNV)( GLenum, GLuint, GLuint ) = extension_funcs[EXT_glBindBufferBaseNV];
  TRACE("(%d, %d, %d)\n", target, index, buffer );
  ENTER_GL();
  func_glBindBufferBaseNV( target, index, buffer );
  LEAVE_GL();
}

static void WINAPI wine_glBindBufferOffsetEXT( GLenum target, GLuint index, GLuint buffer, INT_PTR offset ) {
  void (*func_glBindBufferOffsetEXT)( GLenum, GLuint, GLuint, INT_PTR ) = extension_funcs[EXT_glBindBufferOffsetEXT];
  TRACE("(%d, %d, %d, %ld)\n", target, index, buffer, offset );
  ENTER_GL();
  func_glBindBufferOffsetEXT( target, index, buffer, offset );
  LEAVE_GL();
}

static void WINAPI wine_glBindBufferOffsetNV( GLenum target, GLuint index, GLuint buffer, INT_PTR offset ) {
  void (*func_glBindBufferOffsetNV)( GLenum, GLuint, GLuint, INT_PTR ) = extension_funcs[EXT_glBindBufferOffsetNV];
  TRACE("(%d, %d, %d, %ld)\n", target, index, buffer, offset );
  ENTER_GL();
  func_glBindBufferOffsetNV( target, index, buffer, offset );
  LEAVE_GL();
}

static void WINAPI wine_glBindBufferRange( GLenum target, GLuint index, GLuint buffer, INT_PTR offset, INT_PTR size ) {
  void (*func_glBindBufferRange)( GLenum, GLuint, GLuint, INT_PTR, INT_PTR ) = extension_funcs[EXT_glBindBufferRange];
  TRACE("(%d, %d, %d, %ld, %ld)\n", target, index, buffer, offset, size );
  ENTER_GL();
  func_glBindBufferRange( target, index, buffer, offset, size );
  LEAVE_GL();
}

static void WINAPI wine_glBindBufferRangeEXT( GLenum target, GLuint index, GLuint buffer, INT_PTR offset, INT_PTR size ) {
  void (*func_glBindBufferRangeEXT)( GLenum, GLuint, GLuint, INT_PTR, INT_PTR ) = extension_funcs[EXT_glBindBufferRangeEXT];
  TRACE("(%d, %d, %d, %ld, %ld)\n", target, index, buffer, offset, size );
  ENTER_GL();
  func_glBindBufferRangeEXT( target, index, buffer, offset, size );
  LEAVE_GL();
}

static void WINAPI wine_glBindBufferRangeNV( GLenum target, GLuint index, GLuint buffer, INT_PTR offset, INT_PTR size ) {
  void (*func_glBindBufferRangeNV)( GLenum, GLuint, GLuint, INT_PTR, INT_PTR ) = extension_funcs[EXT_glBindBufferRangeNV];
  TRACE("(%d, %d, %d, %ld, %ld)\n", target, index, buffer, offset, size );
  ENTER_GL();
  func_glBindBufferRangeNV( target, index, buffer, offset, size );
  LEAVE_GL();
}

static void WINAPI wine_glBindFragDataLocation( GLuint program, GLuint color, char* name ) {
  void (*func_glBindFragDataLocation)( GLuint, GLuint, char* ) = extension_funcs[EXT_glBindFragDataLocation];
  TRACE("(%d, %d, %p)\n", program, color, name );
  ENTER_GL();
  func_glBindFragDataLocation( program, color, name );
  LEAVE_GL();
}

static void WINAPI wine_glBindFragDataLocationEXT( GLuint program, GLuint color, char* name ) {
  void (*func_glBindFragDataLocationEXT)( GLuint, GLuint, char* ) = extension_funcs[EXT_glBindFragDataLocationEXT];
  TRACE("(%d, %d, %p)\n", program, color, name );
  ENTER_GL();
  func_glBindFragDataLocationEXT( program, color, name );
  LEAVE_GL();
}

static void WINAPI wine_glBindFragDataLocationIndexed( GLuint program, GLuint colorNumber, GLuint index, char* name ) {
  void (*func_glBindFragDataLocationIndexed)( GLuint, GLuint, GLuint, char* ) = extension_funcs[EXT_glBindFragDataLocationIndexed];
  TRACE("(%d, %d, %d, %p)\n", program, colorNumber, index, name );
  ENTER_GL();
  func_glBindFragDataLocationIndexed( program, colorNumber, index, name );
  LEAVE_GL();
}

static void WINAPI wine_glBindFragmentShaderATI( GLuint id ) {
  void (*func_glBindFragmentShaderATI)( GLuint ) = extension_funcs[EXT_glBindFragmentShaderATI];
  TRACE("(%d)\n", id );
  ENTER_GL();
  func_glBindFragmentShaderATI( id );
  LEAVE_GL();
}

static void WINAPI wine_glBindFramebuffer( GLenum target, GLuint framebuffer ) {
  void (*func_glBindFramebuffer)( GLenum, GLuint ) = extension_funcs[EXT_glBindFramebuffer];
  TRACE("(%d, %d)\n", target, framebuffer );
  ENTER_GL();
  func_glBindFramebuffer( target, framebuffer );
  LEAVE_GL();
}

static void WINAPI wine_glBindFramebufferEXT( GLenum target, GLuint framebuffer ) {
  void (*func_glBindFramebufferEXT)( GLenum, GLuint ) = extension_funcs[EXT_glBindFramebufferEXT];
  TRACE("(%d, %d)\n", target, framebuffer );
  ENTER_GL();
  func_glBindFramebufferEXT( target, framebuffer );
  LEAVE_GL();
}

static void WINAPI wine_glBindImageTextureEXT( GLuint index, GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum access, GLint format ) {
  void (*func_glBindImageTextureEXT)( GLuint, GLuint, GLint, GLboolean, GLint, GLenum, GLint ) = extension_funcs[EXT_glBindImageTextureEXT];
  TRACE("(%d, %d, %d, %d, %d, %d, %d)\n", index, texture, level, layered, layer, access, format );
  ENTER_GL();
  func_glBindImageTextureEXT( index, texture, level, layered, layer, access, format );
  LEAVE_GL();
}

static GLuint WINAPI wine_glBindLightParameterEXT( GLenum light, GLenum value ) {
  GLuint ret_value;
  GLuint (*func_glBindLightParameterEXT)( GLenum, GLenum ) = extension_funcs[EXT_glBindLightParameterEXT];
  TRACE("(%d, %d)\n", light, value );
  ENTER_GL();
  ret_value = func_glBindLightParameterEXT( light, value );
  LEAVE_GL();
  return ret_value;
}

static GLuint WINAPI wine_glBindMaterialParameterEXT( GLenum face, GLenum value ) {
  GLuint ret_value;
  GLuint (*func_glBindMaterialParameterEXT)( GLenum, GLenum ) = extension_funcs[EXT_glBindMaterialParameterEXT];
  TRACE("(%d, %d)\n", face, value );
  ENTER_GL();
  ret_value = func_glBindMaterialParameterEXT( face, value );
  LEAVE_GL();
  return ret_value;
}

static void WINAPI wine_glBindMultiTextureEXT( GLenum texunit, GLenum target, GLuint texture ) {
  void (*func_glBindMultiTextureEXT)( GLenum, GLenum, GLuint ) = extension_funcs[EXT_glBindMultiTextureEXT];
  TRACE("(%d, %d, %d)\n", texunit, target, texture );
  ENTER_GL();
  func_glBindMultiTextureEXT( texunit, target, texture );
  LEAVE_GL();
}

static GLuint WINAPI wine_glBindParameterEXT( GLenum value ) {
  GLuint ret_value;
  GLuint (*func_glBindParameterEXT)( GLenum ) = extension_funcs[EXT_glBindParameterEXT];
  TRACE("(%d)\n", value );
  ENTER_GL();
  ret_value = func_glBindParameterEXT( value );
  LEAVE_GL();
  return ret_value;
}

static void WINAPI wine_glBindProgramARB( GLenum target, GLuint program ) {
  void (*func_glBindProgramARB)( GLenum, GLuint ) = extension_funcs[EXT_glBindProgramARB];
  TRACE("(%d, %d)\n", target, program );
  ENTER_GL();
  func_glBindProgramARB( target, program );
  LEAVE_GL();
}

static void WINAPI wine_glBindProgramNV( GLenum target, GLuint id ) {
  void (*func_glBindProgramNV)( GLenum, GLuint ) = extension_funcs[EXT_glBindProgramNV];
  TRACE("(%d, %d)\n", target, id );
  ENTER_GL();
  func_glBindProgramNV( target, id );
  LEAVE_GL();
}

static void WINAPI wine_glBindProgramPipeline( GLuint pipeline ) {
  void (*func_glBindProgramPipeline)( GLuint ) = extension_funcs[EXT_glBindProgramPipeline];
  TRACE("(%d)\n", pipeline );
  ENTER_GL();
  func_glBindProgramPipeline( pipeline );
  LEAVE_GL();
}

static void WINAPI wine_glBindRenderbuffer( GLenum target, GLuint renderbuffer ) {
  void (*func_glBindRenderbuffer)( GLenum, GLuint ) = extension_funcs[EXT_glBindRenderbuffer];
  TRACE("(%d, %d)\n", target, renderbuffer );
  ENTER_GL();
  func_glBindRenderbuffer( target, renderbuffer );
  LEAVE_GL();
}

static void WINAPI wine_glBindRenderbufferEXT( GLenum target, GLuint renderbuffer ) {
  void (*func_glBindRenderbufferEXT)( GLenum, GLuint ) = extension_funcs[EXT_glBindRenderbufferEXT];
  TRACE("(%d, %d)\n", target, renderbuffer );
  ENTER_GL();
  func_glBindRenderbufferEXT( target, renderbuffer );
  LEAVE_GL();
}

static void WINAPI wine_glBindSampler( GLuint unit, GLuint sampler ) {
  void (*func_glBindSampler)( GLuint, GLuint ) = extension_funcs[EXT_glBindSampler];
  TRACE("(%d, %d)\n", unit, sampler );
  ENTER_GL();
  func_glBindSampler( unit, sampler );
  LEAVE_GL();
}

static GLuint WINAPI wine_glBindTexGenParameterEXT( GLenum unit, GLenum coord, GLenum value ) {
  GLuint ret_value;
  GLuint (*func_glBindTexGenParameterEXT)( GLenum, GLenum, GLenum ) = extension_funcs[EXT_glBindTexGenParameterEXT];
  TRACE("(%d, %d, %d)\n", unit, coord, value );
  ENTER_GL();
  ret_value = func_glBindTexGenParameterEXT( unit, coord, value );
  LEAVE_GL();
  return ret_value;
}

static void WINAPI wine_glBindTextureEXT( GLenum target, GLuint texture ) {
  void (*func_glBindTextureEXT)( GLenum, GLuint ) = extension_funcs[EXT_glBindTextureEXT];
  TRACE("(%d, %d)\n", target, texture );
  ENTER_GL();
  func_glBindTextureEXT( target, texture );
  LEAVE_GL();
}

static GLuint WINAPI wine_glBindTextureUnitParameterEXT( GLenum unit, GLenum value ) {
  GLuint ret_value;
  GLuint (*func_glBindTextureUnitParameterEXT)( GLenum, GLenum ) = extension_funcs[EXT_glBindTextureUnitParameterEXT];
  TRACE("(%d, %d)\n", unit, value );
  ENTER_GL();
  ret_value = func_glBindTextureUnitParameterEXT( unit, value );
  LEAVE_GL();
  return ret_value;
}

static void WINAPI wine_glBindTransformFeedback( GLenum target, GLuint id ) {
  void (*func_glBindTransformFeedback)( GLenum, GLuint ) = extension_funcs[EXT_glBindTransformFeedback];
  TRACE("(%d, %d)\n", target, id );
  ENTER_GL();
  func_glBindTransformFeedback( target, id );
  LEAVE_GL();
}

static void WINAPI wine_glBindTransformFeedbackNV( GLenum target, GLuint id ) {
  void (*func_glBindTransformFeedbackNV)( GLenum, GLuint ) = extension_funcs[EXT_glBindTransformFeedbackNV];
  TRACE("(%d, %d)\n", target, id );
  ENTER_GL();
  func_glBindTransformFeedbackNV( target, id );
  LEAVE_GL();
}

static void WINAPI wine_glBindVertexArray( GLuint array ) {
  void (*func_glBindVertexArray)( GLuint ) = extension_funcs[EXT_glBindVertexArray];
  TRACE("(%d)\n", array );
  ENTER_GL();
  func_glBindVertexArray( array );
  LEAVE_GL();
}

static void WINAPI wine_glBindVertexArrayAPPLE( GLuint array ) {
  void (*func_glBindVertexArrayAPPLE)( GLuint ) = extension_funcs[EXT_glBindVertexArrayAPPLE];
  TRACE("(%d)\n", array );
  ENTER_GL();
  func_glBindVertexArrayAPPLE( array );
  LEAVE_GL();
}

static void WINAPI wine_glBindVertexShaderEXT( GLuint id ) {
  void (*func_glBindVertexShaderEXT)( GLuint ) = extension_funcs[EXT_glBindVertexShaderEXT];
  TRACE("(%d)\n", id );
  ENTER_GL();
  func_glBindVertexShaderEXT( id );
  LEAVE_GL();
}

static void WINAPI wine_glBindVideoCaptureStreamBufferNV( GLuint video_capture_slot, GLuint stream, GLenum frame_region, INT_PTR offset ) {
  void (*func_glBindVideoCaptureStreamBufferNV)( GLuint, GLuint, GLenum, INT_PTR ) = extension_funcs[EXT_glBindVideoCaptureStreamBufferNV];
  TRACE("(%d, %d, %d, %ld)\n", video_capture_slot, stream, frame_region, offset );
  ENTER_GL();
  func_glBindVideoCaptureStreamBufferNV( video_capture_slot, stream, frame_region, offset );
  LEAVE_GL();
}

static void WINAPI wine_glBindVideoCaptureStreamTextureNV( GLuint video_capture_slot, GLuint stream, GLenum frame_region, GLenum target, GLuint texture ) {
  void (*func_glBindVideoCaptureStreamTextureNV)( GLuint, GLuint, GLenum, GLenum, GLuint ) = extension_funcs[EXT_glBindVideoCaptureStreamTextureNV];
  TRACE("(%d, %d, %d, %d, %d)\n", video_capture_slot, stream, frame_region, target, texture );
  ENTER_GL();
  func_glBindVideoCaptureStreamTextureNV( video_capture_slot, stream, frame_region, target, texture );
  LEAVE_GL();
}

static void WINAPI wine_glBinormal3bEXT( GLbyte bx, GLbyte by, GLbyte bz ) {
  void (*func_glBinormal3bEXT)( GLbyte, GLbyte, GLbyte ) = extension_funcs[EXT_glBinormal3bEXT];
  TRACE("(%d, %d, %d)\n", bx, by, bz );
  ENTER_GL();
  func_glBinormal3bEXT( bx, by, bz );
  LEAVE_GL();
}

static void WINAPI wine_glBinormal3bvEXT( GLbyte* v ) {
  void (*func_glBinormal3bvEXT)( GLbyte* ) = extension_funcs[EXT_glBinormal3bvEXT];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glBinormal3bvEXT( v );
  LEAVE_GL();
}

static void WINAPI wine_glBinormal3dEXT( GLdouble bx, GLdouble by, GLdouble bz ) {
  void (*func_glBinormal3dEXT)( GLdouble, GLdouble, GLdouble ) = extension_funcs[EXT_glBinormal3dEXT];
  TRACE("(%f, %f, %f)\n", bx, by, bz );
  ENTER_GL();
  func_glBinormal3dEXT( bx, by, bz );
  LEAVE_GL();
}

static void WINAPI wine_glBinormal3dvEXT( GLdouble* v ) {
  void (*func_glBinormal3dvEXT)( GLdouble* ) = extension_funcs[EXT_glBinormal3dvEXT];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glBinormal3dvEXT( v );
  LEAVE_GL();
}

static void WINAPI wine_glBinormal3fEXT( GLfloat bx, GLfloat by, GLfloat bz ) {
  void (*func_glBinormal3fEXT)( GLfloat, GLfloat, GLfloat ) = extension_funcs[EXT_glBinormal3fEXT];
  TRACE("(%f, %f, %f)\n", bx, by, bz );
  ENTER_GL();
  func_glBinormal3fEXT( bx, by, bz );
  LEAVE_GL();
}

static void WINAPI wine_glBinormal3fvEXT( GLfloat* v ) {
  void (*func_glBinormal3fvEXT)( GLfloat* ) = extension_funcs[EXT_glBinormal3fvEXT];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glBinormal3fvEXT( v );
  LEAVE_GL();
}

static void WINAPI wine_glBinormal3iEXT( GLint bx, GLint by, GLint bz ) {
  void (*func_glBinormal3iEXT)( GLint, GLint, GLint ) = extension_funcs[EXT_glBinormal3iEXT];
  TRACE("(%d, %d, %d)\n", bx, by, bz );
  ENTER_GL();
  func_glBinormal3iEXT( bx, by, bz );
  LEAVE_GL();
}

static void WINAPI wine_glBinormal3ivEXT( GLint* v ) {
  void (*func_glBinormal3ivEXT)( GLint* ) = extension_funcs[EXT_glBinormal3ivEXT];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glBinormal3ivEXT( v );
  LEAVE_GL();
}

static void WINAPI wine_glBinormal3sEXT( GLshort bx, GLshort by, GLshort bz ) {
  void (*func_glBinormal3sEXT)( GLshort, GLshort, GLshort ) = extension_funcs[EXT_glBinormal3sEXT];
  TRACE("(%d, %d, %d)\n", bx, by, bz );
  ENTER_GL();
  func_glBinormal3sEXT( bx, by, bz );
  LEAVE_GL();
}

static void WINAPI wine_glBinormal3svEXT( GLshort* v ) {
  void (*func_glBinormal3svEXT)( GLshort* ) = extension_funcs[EXT_glBinormal3svEXT];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glBinormal3svEXT( v );
  LEAVE_GL();
}

static void WINAPI wine_glBinormalPointerEXT( GLenum type, GLsizei stride, GLvoid* pointer ) {
  void (*func_glBinormalPointerEXT)( GLenum, GLsizei, GLvoid* ) = extension_funcs[EXT_glBinormalPointerEXT];
  TRACE("(%d, %d, %p)\n", type, stride, pointer );
  ENTER_GL();
  func_glBinormalPointerEXT( type, stride, pointer );
  LEAVE_GL();
}

static void WINAPI wine_glBlendColor( GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha ) {
  void (*func_glBlendColor)( GLclampf, GLclampf, GLclampf, GLclampf ) = extension_funcs[EXT_glBlendColor];
  TRACE("(%f, %f, %f, %f)\n", red, green, blue, alpha );
  ENTER_GL();
  func_glBlendColor( red, green, blue, alpha );
  LEAVE_GL();
}

static void WINAPI wine_glBlendColorEXT( GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha ) {
  void (*func_glBlendColorEXT)( GLclampf, GLclampf, GLclampf, GLclampf ) = extension_funcs[EXT_glBlendColorEXT];
  TRACE("(%f, %f, %f, %f)\n", red, green, blue, alpha );
  ENTER_GL();
  func_glBlendColorEXT( red, green, blue, alpha );
  LEAVE_GL();
}

static void WINAPI wine_glBlendEquation( GLenum mode ) {
  void (*func_glBlendEquation)( GLenum ) = extension_funcs[EXT_glBlendEquation];
  TRACE("(%d)\n", mode );
  ENTER_GL();
  func_glBlendEquation( mode );
  LEAVE_GL();
}

static void WINAPI wine_glBlendEquationEXT( GLenum mode ) {
  void (*func_glBlendEquationEXT)( GLenum ) = extension_funcs[EXT_glBlendEquationEXT];
  TRACE("(%d)\n", mode );
  ENTER_GL();
  func_glBlendEquationEXT( mode );
  LEAVE_GL();
}

static void WINAPI wine_glBlendEquationIndexedAMD( GLuint buf, GLenum mode ) {
  void (*func_glBlendEquationIndexedAMD)( GLuint, GLenum ) = extension_funcs[EXT_glBlendEquationIndexedAMD];
  TRACE("(%d, %d)\n", buf, mode );
  ENTER_GL();
  func_glBlendEquationIndexedAMD( buf, mode );
  LEAVE_GL();
}

static void WINAPI wine_glBlendEquationSeparate( GLenum modeRGB, GLenum modeAlpha ) {
  void (*func_glBlendEquationSeparate)( GLenum, GLenum ) = extension_funcs[EXT_glBlendEquationSeparate];
  TRACE("(%d, %d)\n", modeRGB, modeAlpha );
  ENTER_GL();
  func_glBlendEquationSeparate( modeRGB, modeAlpha );
  LEAVE_GL();
}

static void WINAPI wine_glBlendEquationSeparateEXT( GLenum modeRGB, GLenum modeAlpha ) {
  void (*func_glBlendEquationSeparateEXT)( GLenum, GLenum ) = extension_funcs[EXT_glBlendEquationSeparateEXT];
  TRACE("(%d, %d)\n", modeRGB, modeAlpha );
  ENTER_GL();
  func_glBlendEquationSeparateEXT( modeRGB, modeAlpha );
  LEAVE_GL();
}

static void WINAPI wine_glBlendEquationSeparateIndexedAMD( GLuint buf, GLenum modeRGB, GLenum modeAlpha ) {
  void (*func_glBlendEquationSeparateIndexedAMD)( GLuint, GLenum, GLenum ) = extension_funcs[EXT_glBlendEquationSeparateIndexedAMD];
  TRACE("(%d, %d, %d)\n", buf, modeRGB, modeAlpha );
  ENTER_GL();
  func_glBlendEquationSeparateIndexedAMD( buf, modeRGB, modeAlpha );
  LEAVE_GL();
}

static void WINAPI wine_glBlendEquationSeparatei( GLuint buf, GLenum modeRGB, GLenum modeAlpha ) {
  void (*func_glBlendEquationSeparatei)( GLuint, GLenum, GLenum ) = extension_funcs[EXT_glBlendEquationSeparatei];
  TRACE("(%d, %d, %d)\n", buf, modeRGB, modeAlpha );
  ENTER_GL();
  func_glBlendEquationSeparatei( buf, modeRGB, modeAlpha );
  LEAVE_GL();
}

static void WINAPI wine_glBlendEquationSeparateiARB( GLuint buf, GLenum modeRGB, GLenum modeAlpha ) {
  void (*func_glBlendEquationSeparateiARB)( GLuint, GLenum, GLenum ) = extension_funcs[EXT_glBlendEquationSeparateiARB];
  TRACE("(%d, %d, %d)\n", buf, modeRGB, modeAlpha );
  ENTER_GL();
  func_glBlendEquationSeparateiARB( buf, modeRGB, modeAlpha );
  LEAVE_GL();
}

static void WINAPI wine_glBlendEquationi( GLuint buf, GLenum mode ) {
  void (*func_glBlendEquationi)( GLuint, GLenum ) = extension_funcs[EXT_glBlendEquationi];
  TRACE("(%d, %d)\n", buf, mode );
  ENTER_GL();
  func_glBlendEquationi( buf, mode );
  LEAVE_GL();
}

static void WINAPI wine_glBlendEquationiARB( GLuint buf, GLenum mode ) {
  void (*func_glBlendEquationiARB)( GLuint, GLenum ) = extension_funcs[EXT_glBlendEquationiARB];
  TRACE("(%d, %d)\n", buf, mode );
  ENTER_GL();
  func_glBlendEquationiARB( buf, mode );
  LEAVE_GL();
}

static void WINAPI wine_glBlendFuncIndexedAMD( GLuint buf, GLenum src, GLenum dst ) {
  void (*func_glBlendFuncIndexedAMD)( GLuint, GLenum, GLenum ) = extension_funcs[EXT_glBlendFuncIndexedAMD];
  TRACE("(%d, %d, %d)\n", buf, src, dst );
  ENTER_GL();
  func_glBlendFuncIndexedAMD( buf, src, dst );
  LEAVE_GL();
}

static void WINAPI wine_glBlendFuncSeparate( GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha ) {
  void (*func_glBlendFuncSeparate)( GLenum, GLenum, GLenum, GLenum ) = extension_funcs[EXT_glBlendFuncSeparate];
  TRACE("(%d, %d, %d, %d)\n", sfactorRGB, dfactorRGB, sfactorAlpha, dfactorAlpha );
  ENTER_GL();
  func_glBlendFuncSeparate( sfactorRGB, dfactorRGB, sfactorAlpha, dfactorAlpha );
  LEAVE_GL();
}

static void WINAPI wine_glBlendFuncSeparateEXT( GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha ) {
  void (*func_glBlendFuncSeparateEXT)( GLenum, GLenum, GLenum, GLenum ) = extension_funcs[EXT_glBlendFuncSeparateEXT];
  TRACE("(%d, %d, %d, %d)\n", sfactorRGB, dfactorRGB, sfactorAlpha, dfactorAlpha );
  ENTER_GL();
  func_glBlendFuncSeparateEXT( sfactorRGB, dfactorRGB, sfactorAlpha, dfactorAlpha );
  LEAVE_GL();
}

static void WINAPI wine_glBlendFuncSeparateINGR( GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha ) {
  void (*func_glBlendFuncSeparateINGR)( GLenum, GLenum, GLenum, GLenum ) = extension_funcs[EXT_glBlendFuncSeparateINGR];
  TRACE("(%d, %d, %d, %d)\n", sfactorRGB, dfactorRGB, sfactorAlpha, dfactorAlpha );
  ENTER_GL();
  func_glBlendFuncSeparateINGR( sfactorRGB, dfactorRGB, sfactorAlpha, dfactorAlpha );
  LEAVE_GL();
}

static void WINAPI wine_glBlendFuncSeparateIndexedAMD( GLuint buf, GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha ) {
  void (*func_glBlendFuncSeparateIndexedAMD)( GLuint, GLenum, GLenum, GLenum, GLenum ) = extension_funcs[EXT_glBlendFuncSeparateIndexedAMD];
  TRACE("(%d, %d, %d, %d, %d)\n", buf, srcRGB, dstRGB, srcAlpha, dstAlpha );
  ENTER_GL();
  func_glBlendFuncSeparateIndexedAMD( buf, srcRGB, dstRGB, srcAlpha, dstAlpha );
  LEAVE_GL();
}

static void WINAPI wine_glBlendFuncSeparatei( GLuint buf, GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha ) {
  void (*func_glBlendFuncSeparatei)( GLuint, GLenum, GLenum, GLenum, GLenum ) = extension_funcs[EXT_glBlendFuncSeparatei];
  TRACE("(%d, %d, %d, %d, %d)\n", buf, srcRGB, dstRGB, srcAlpha, dstAlpha );
  ENTER_GL();
  func_glBlendFuncSeparatei( buf, srcRGB, dstRGB, srcAlpha, dstAlpha );
  LEAVE_GL();
}

static void WINAPI wine_glBlendFuncSeparateiARB( GLuint buf, GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha ) {
  void (*func_glBlendFuncSeparateiARB)( GLuint, GLenum, GLenum, GLenum, GLenum ) = extension_funcs[EXT_glBlendFuncSeparateiARB];
  TRACE("(%d, %d, %d, %d, %d)\n", buf, srcRGB, dstRGB, srcAlpha, dstAlpha );
  ENTER_GL();
  func_glBlendFuncSeparateiARB( buf, srcRGB, dstRGB, srcAlpha, dstAlpha );
  LEAVE_GL();
}

static void WINAPI wine_glBlendFunci( GLuint buf, GLenum src, GLenum dst ) {
  void (*func_glBlendFunci)( GLuint, GLenum, GLenum ) = extension_funcs[EXT_glBlendFunci];
  TRACE("(%d, %d, %d)\n", buf, src, dst );
  ENTER_GL();
  func_glBlendFunci( buf, src, dst );
  LEAVE_GL();
}

static void WINAPI wine_glBlendFunciARB( GLuint buf, GLenum src, GLenum dst ) {
  void (*func_glBlendFunciARB)( GLuint, GLenum, GLenum ) = extension_funcs[EXT_glBlendFunciARB];
  TRACE("(%d, %d, %d)\n", buf, src, dst );
  ENTER_GL();
  func_glBlendFunciARB( buf, src, dst );
  LEAVE_GL();
}

static void WINAPI wine_glBlitFramebuffer( GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter ) {
  void (*func_glBlitFramebuffer)( GLint, GLint, GLint, GLint, GLint, GLint, GLint, GLint, GLbitfield, GLenum ) = extension_funcs[EXT_glBlitFramebuffer];
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %d, %d)\n", srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter );
  ENTER_GL();
  func_glBlitFramebuffer( srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter );
  LEAVE_GL();
}

static void WINAPI wine_glBlitFramebufferEXT( GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter ) {
  void (*func_glBlitFramebufferEXT)( GLint, GLint, GLint, GLint, GLint, GLint, GLint, GLint, GLbitfield, GLenum ) = extension_funcs[EXT_glBlitFramebufferEXT];
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %d, %d)\n", srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter );
  ENTER_GL();
  func_glBlitFramebufferEXT( srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter );
  LEAVE_GL();
}

static void WINAPI wine_glBufferAddressRangeNV( GLenum pname, GLuint index, UINT64 address, INT_PTR length ) {
  void (*func_glBufferAddressRangeNV)( GLenum, GLuint, UINT64, INT_PTR ) = extension_funcs[EXT_glBufferAddressRangeNV];
  TRACE("(%d, %d, %s, %ld)\n", pname, index, wine_dbgstr_longlong(address), length );
  ENTER_GL();
  func_glBufferAddressRangeNV( pname, index, address, length );
  LEAVE_GL();
}

static void WINAPI wine_glBufferData( GLenum target, INT_PTR size, GLvoid* data, GLenum usage ) {
  void (*func_glBufferData)( GLenum, INT_PTR, GLvoid*, GLenum ) = extension_funcs[EXT_glBufferData];
  TRACE("(%d, %ld, %p, %d)\n", target, size, data, usage );
  ENTER_GL();
  func_glBufferData( target, size, data, usage );
  LEAVE_GL();
}

static void WINAPI wine_glBufferDataARB( GLenum target, INT_PTR size, GLvoid* data, GLenum usage ) {
  void (*func_glBufferDataARB)( GLenum, INT_PTR, GLvoid*, GLenum ) = extension_funcs[EXT_glBufferDataARB];
  TRACE("(%d, %ld, %p, %d)\n", target, size, data, usage );
  ENTER_GL();
  func_glBufferDataARB( target, size, data, usage );
  LEAVE_GL();
}

static void WINAPI wine_glBufferParameteriAPPLE( GLenum target, GLenum pname, GLint param ) {
  void (*func_glBufferParameteriAPPLE)( GLenum, GLenum, GLint ) = extension_funcs[EXT_glBufferParameteriAPPLE];
  TRACE("(%d, %d, %d)\n", target, pname, param );
  ENTER_GL();
  func_glBufferParameteriAPPLE( target, pname, param );
  LEAVE_GL();
}

static GLuint WINAPI wine_glBufferRegionEnabled( void ) {
  GLuint ret_value;
  GLuint (*func_glBufferRegionEnabled)( void ) = extension_funcs[EXT_glBufferRegionEnabled];
  TRACE("()\n");
  ENTER_GL();
  ret_value = func_glBufferRegionEnabled( );
  LEAVE_GL();
  return ret_value;
}

static void WINAPI wine_glBufferSubData( GLenum target, INT_PTR offset, INT_PTR size, GLvoid* data ) {
  void (*func_glBufferSubData)( GLenum, INT_PTR, INT_PTR, GLvoid* ) = extension_funcs[EXT_glBufferSubData];
  TRACE("(%d, %ld, %ld, %p)\n", target, offset, size, data );
  ENTER_GL();
  func_glBufferSubData( target, offset, size, data );
  LEAVE_GL();
}

static void WINAPI wine_glBufferSubDataARB( GLenum target, INT_PTR offset, INT_PTR size, GLvoid* data ) {
  void (*func_glBufferSubDataARB)( GLenum, INT_PTR, INT_PTR, GLvoid* ) = extension_funcs[EXT_glBufferSubDataARB];
  TRACE("(%d, %ld, %ld, %p)\n", target, offset, size, data );
  ENTER_GL();
  func_glBufferSubDataARB( target, offset, size, data );
  LEAVE_GL();
}

static GLenum WINAPI wine_glCheckFramebufferStatus( GLenum target ) {
  GLenum ret_value;
  GLenum (*func_glCheckFramebufferStatus)( GLenum ) = extension_funcs[EXT_glCheckFramebufferStatus];
  TRACE("(%d)\n", target );
  ENTER_GL();
  ret_value = func_glCheckFramebufferStatus( target );
  LEAVE_GL();
  return ret_value;
}

static GLenum WINAPI wine_glCheckFramebufferStatusEXT( GLenum target ) {
  GLenum ret_value;
  GLenum (*func_glCheckFramebufferStatusEXT)( GLenum ) = extension_funcs[EXT_glCheckFramebufferStatusEXT];
  TRACE("(%d)\n", target );
  ENTER_GL();
  ret_value = func_glCheckFramebufferStatusEXT( target );
  LEAVE_GL();
  return ret_value;
}

static GLenum WINAPI wine_glCheckNamedFramebufferStatusEXT( GLuint framebuffer, GLenum target ) {
  GLenum ret_value;
  GLenum (*func_glCheckNamedFramebufferStatusEXT)( GLuint, GLenum ) = extension_funcs[EXT_glCheckNamedFramebufferStatusEXT];
  TRACE("(%d, %d)\n", framebuffer, target );
  ENTER_GL();
  ret_value = func_glCheckNamedFramebufferStatusEXT( framebuffer, target );
  LEAVE_GL();
  return ret_value;
}

static void WINAPI wine_glClampColor( GLenum target, GLenum clamp ) {
  void (*func_glClampColor)( GLenum, GLenum ) = extension_funcs[EXT_glClampColor];
  TRACE("(%d, %d)\n", target, clamp );
  ENTER_GL();
  func_glClampColor( target, clamp );
  LEAVE_GL();
}

static void WINAPI wine_glClampColorARB( GLenum target, GLenum clamp ) {
  void (*func_glClampColorARB)( GLenum, GLenum ) = extension_funcs[EXT_glClampColorARB];
  TRACE("(%d, %d)\n", target, clamp );
  ENTER_GL();
  func_glClampColorARB( target, clamp );
  LEAVE_GL();
}

static void WINAPI wine_glClearBufferfi( GLenum buffer, GLint drawbuffer, GLfloat depth, GLint stencil ) {
  void (*func_glClearBufferfi)( GLenum, GLint, GLfloat, GLint ) = extension_funcs[EXT_glClearBufferfi];
  TRACE("(%d, %d, %f, %d)\n", buffer, drawbuffer, depth, stencil );
  ENTER_GL();
  func_glClearBufferfi( buffer, drawbuffer, depth, stencil );
  LEAVE_GL();
}

static void WINAPI wine_glClearBufferfv( GLenum buffer, GLint drawbuffer, GLfloat* value ) {
  void (*func_glClearBufferfv)( GLenum, GLint, GLfloat* ) = extension_funcs[EXT_glClearBufferfv];
  TRACE("(%d, %d, %p)\n", buffer, drawbuffer, value );
  ENTER_GL();
  func_glClearBufferfv( buffer, drawbuffer, value );
  LEAVE_GL();
}

static void WINAPI wine_glClearBufferiv( GLenum buffer, GLint drawbuffer, GLint* value ) {
  void (*func_glClearBufferiv)( GLenum, GLint, GLint* ) = extension_funcs[EXT_glClearBufferiv];
  TRACE("(%d, %d, %p)\n", buffer, drawbuffer, value );
  ENTER_GL();
  func_glClearBufferiv( buffer, drawbuffer, value );
  LEAVE_GL();
}

static void WINAPI wine_glClearBufferuiv( GLenum buffer, GLint drawbuffer, GLuint* value ) {
  void (*func_glClearBufferuiv)( GLenum, GLint, GLuint* ) = extension_funcs[EXT_glClearBufferuiv];
  TRACE("(%d, %d, %p)\n", buffer, drawbuffer, value );
  ENTER_GL();
  func_glClearBufferuiv( buffer, drawbuffer, value );
  LEAVE_GL();
}

static void WINAPI wine_glClearColorIiEXT( GLint red, GLint green, GLint blue, GLint alpha ) {
  void (*func_glClearColorIiEXT)( GLint, GLint, GLint, GLint ) = extension_funcs[EXT_glClearColorIiEXT];
  TRACE("(%d, %d, %d, %d)\n", red, green, blue, alpha );
  ENTER_GL();
  func_glClearColorIiEXT( red, green, blue, alpha );
  LEAVE_GL();
}

static void WINAPI wine_glClearColorIuiEXT( GLuint red, GLuint green, GLuint blue, GLuint alpha ) {
  void (*func_glClearColorIuiEXT)( GLuint, GLuint, GLuint, GLuint ) = extension_funcs[EXT_glClearColorIuiEXT];
  TRACE("(%d, %d, %d, %d)\n", red, green, blue, alpha );
  ENTER_GL();
  func_glClearColorIuiEXT( red, green, blue, alpha );
  LEAVE_GL();
}

static void WINAPI wine_glClearDepthdNV( GLdouble depth ) {
  void (*func_glClearDepthdNV)( GLdouble ) = extension_funcs[EXT_glClearDepthdNV];
  TRACE("(%f)\n", depth );
  ENTER_GL();
  func_glClearDepthdNV( depth );
  LEAVE_GL();
}

static void WINAPI wine_glClearDepthf( GLclampf d ) {
  void (*func_glClearDepthf)( GLclampf ) = extension_funcs[EXT_glClearDepthf];
  TRACE("(%f)\n", d );
  ENTER_GL();
  func_glClearDepthf( d );
  LEAVE_GL();
}

static void WINAPI wine_glClientActiveTexture( GLenum texture ) {
  void (*func_glClientActiveTexture)( GLenum ) = extension_funcs[EXT_glClientActiveTexture];
  TRACE("(%d)\n", texture );
  ENTER_GL();
  func_glClientActiveTexture( texture );
  LEAVE_GL();
}

static void WINAPI wine_glClientActiveTextureARB( GLenum texture ) {
  void (*func_glClientActiveTextureARB)( GLenum ) = extension_funcs[EXT_glClientActiveTextureARB];
  TRACE("(%d)\n", texture );
  ENTER_GL();
  func_glClientActiveTextureARB( texture );
  LEAVE_GL();
}

static void WINAPI wine_glClientActiveVertexStreamATI( GLenum stream ) {
  void (*func_glClientActiveVertexStreamATI)( GLenum ) = extension_funcs[EXT_glClientActiveVertexStreamATI];
  TRACE("(%d)\n", stream );
  ENTER_GL();
  func_glClientActiveVertexStreamATI( stream );
  LEAVE_GL();
}

static void WINAPI wine_glClientAttribDefaultEXT( GLbitfield mask ) {
  void (*func_glClientAttribDefaultEXT)( GLbitfield ) = extension_funcs[EXT_glClientAttribDefaultEXT];
  TRACE("(%d)\n", mask );
  ENTER_GL();
  func_glClientAttribDefaultEXT( mask );
  LEAVE_GL();
}

static GLenum WINAPI wine_glClientWaitSync( GLvoid* sync, GLbitfield flags, UINT64 timeout ) {
  GLenum ret_value;
  GLenum (*func_glClientWaitSync)( GLvoid*, GLbitfield, UINT64 ) = extension_funcs[EXT_glClientWaitSync];
  TRACE("(%p, %d, %s)\n", sync, flags, wine_dbgstr_longlong(timeout) );
  ENTER_GL();
  ret_value = func_glClientWaitSync( sync, flags, timeout );
  LEAVE_GL();
  return ret_value;
}

static void WINAPI wine_glColor3fVertex3fSUN( GLfloat r, GLfloat g, GLfloat b, GLfloat x, GLfloat y, GLfloat z ) {
  void (*func_glColor3fVertex3fSUN)( GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat ) = extension_funcs[EXT_glColor3fVertex3fSUN];
  TRACE("(%f, %f, %f, %f, %f, %f)\n", r, g, b, x, y, z );
  ENTER_GL();
  func_glColor3fVertex3fSUN( r, g, b, x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glColor3fVertex3fvSUN( GLfloat* c, GLfloat* v ) {
  void (*func_glColor3fVertex3fvSUN)( GLfloat*, GLfloat* ) = extension_funcs[EXT_glColor3fVertex3fvSUN];
  TRACE("(%p, %p)\n", c, v );
  ENTER_GL();
  func_glColor3fVertex3fvSUN( c, v );
  LEAVE_GL();
}

static void WINAPI wine_glColor3hNV( unsigned short red, unsigned short green, unsigned short blue ) {
  void (*func_glColor3hNV)( unsigned short, unsigned short, unsigned short ) = extension_funcs[EXT_glColor3hNV];
  TRACE("(%d, %d, %d)\n", red, green, blue );
  ENTER_GL();
  func_glColor3hNV( red, green, blue );
  LEAVE_GL();
}

static void WINAPI wine_glColor3hvNV( unsigned short* v ) {
  void (*func_glColor3hvNV)( unsigned short* ) = extension_funcs[EXT_glColor3hvNV];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glColor3hvNV( v );
  LEAVE_GL();
}

static void WINAPI wine_glColor4fNormal3fVertex3fSUN( GLfloat r, GLfloat g, GLfloat b, GLfloat a, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z ) {
  void (*func_glColor4fNormal3fVertex3fSUN)( GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat ) = extension_funcs[EXT_glColor4fNormal3fVertex3fSUN];
  TRACE("(%f, %f, %f, %f, %f, %f, %f, %f, %f, %f)\n", r, g, b, a, nx, ny, nz, x, y, z );
  ENTER_GL();
  func_glColor4fNormal3fVertex3fSUN( r, g, b, a, nx, ny, nz, x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glColor4fNormal3fVertex3fvSUN( GLfloat* c, GLfloat* n, GLfloat* v ) {
  void (*func_glColor4fNormal3fVertex3fvSUN)( GLfloat*, GLfloat*, GLfloat* ) = extension_funcs[EXT_glColor4fNormal3fVertex3fvSUN];
  TRACE("(%p, %p, %p)\n", c, n, v );
  ENTER_GL();
  func_glColor4fNormal3fVertex3fvSUN( c, n, v );
  LEAVE_GL();
}

static void WINAPI wine_glColor4hNV( unsigned short red, unsigned short green, unsigned short blue, unsigned short alpha ) {
  void (*func_glColor4hNV)( unsigned short, unsigned short, unsigned short, unsigned short ) = extension_funcs[EXT_glColor4hNV];
  TRACE("(%d, %d, %d, %d)\n", red, green, blue, alpha );
  ENTER_GL();
  func_glColor4hNV( red, green, blue, alpha );
  LEAVE_GL();
}

static void WINAPI wine_glColor4hvNV( unsigned short* v ) {
  void (*func_glColor4hvNV)( unsigned short* ) = extension_funcs[EXT_glColor4hvNV];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glColor4hvNV( v );
  LEAVE_GL();
}

static void WINAPI wine_glColor4ubVertex2fSUN( GLubyte r, GLubyte g, GLubyte b, GLubyte a, GLfloat x, GLfloat y ) {
  void (*func_glColor4ubVertex2fSUN)( GLubyte, GLubyte, GLubyte, GLubyte, GLfloat, GLfloat ) = extension_funcs[EXT_glColor4ubVertex2fSUN];
  TRACE("(%d, %d, %d, %d, %f, %f)\n", r, g, b, a, x, y );
  ENTER_GL();
  func_glColor4ubVertex2fSUN( r, g, b, a, x, y );
  LEAVE_GL();
}

static void WINAPI wine_glColor4ubVertex2fvSUN( GLubyte* c, GLfloat* v ) {
  void (*func_glColor4ubVertex2fvSUN)( GLubyte*, GLfloat* ) = extension_funcs[EXT_glColor4ubVertex2fvSUN];
  TRACE("(%p, %p)\n", c, v );
  ENTER_GL();
  func_glColor4ubVertex2fvSUN( c, v );
  LEAVE_GL();
}

static void WINAPI wine_glColor4ubVertex3fSUN( GLubyte r, GLubyte g, GLubyte b, GLubyte a, GLfloat x, GLfloat y, GLfloat z ) {
  void (*func_glColor4ubVertex3fSUN)( GLubyte, GLubyte, GLubyte, GLubyte, GLfloat, GLfloat, GLfloat ) = extension_funcs[EXT_glColor4ubVertex3fSUN];
  TRACE("(%d, %d, %d, %d, %f, %f, %f)\n", r, g, b, a, x, y, z );
  ENTER_GL();
  func_glColor4ubVertex3fSUN( r, g, b, a, x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glColor4ubVertex3fvSUN( GLubyte* c, GLfloat* v ) {
  void (*func_glColor4ubVertex3fvSUN)( GLubyte*, GLfloat* ) = extension_funcs[EXT_glColor4ubVertex3fvSUN];
  TRACE("(%p, %p)\n", c, v );
  ENTER_GL();
  func_glColor4ubVertex3fvSUN( c, v );
  LEAVE_GL();
}

static void WINAPI wine_glColorFormatNV( GLint size, GLenum type, GLsizei stride ) {
  void (*func_glColorFormatNV)( GLint, GLenum, GLsizei ) = extension_funcs[EXT_glColorFormatNV];
  TRACE("(%d, %d, %d)\n", size, type, stride );
  ENTER_GL();
  func_glColorFormatNV( size, type, stride );
  LEAVE_GL();
}

static void WINAPI wine_glColorFragmentOp1ATI( GLenum op, GLuint dst, GLuint dstMask, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod ) {
  void (*func_glColorFragmentOp1ATI)( GLenum, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint ) = extension_funcs[EXT_glColorFragmentOp1ATI];
  TRACE("(%d, %d, %d, %d, %d, %d, %d)\n", op, dst, dstMask, dstMod, arg1, arg1Rep, arg1Mod );
  ENTER_GL();
  func_glColorFragmentOp1ATI( op, dst, dstMask, dstMod, arg1, arg1Rep, arg1Mod );
  LEAVE_GL();
}

static void WINAPI wine_glColorFragmentOp2ATI( GLenum op, GLuint dst, GLuint dstMask, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod, GLuint arg2, GLuint arg2Rep, GLuint arg2Mod ) {
  void (*func_glColorFragmentOp2ATI)( GLenum, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint ) = extension_funcs[EXT_glColorFragmentOp2ATI];
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %d, %d)\n", op, dst, dstMask, dstMod, arg1, arg1Rep, arg1Mod, arg2, arg2Rep, arg2Mod );
  ENTER_GL();
  func_glColorFragmentOp2ATI( op, dst, dstMask, dstMod, arg1, arg1Rep, arg1Mod, arg2, arg2Rep, arg2Mod );
  LEAVE_GL();
}

static void WINAPI wine_glColorFragmentOp3ATI( GLenum op, GLuint dst, GLuint dstMask, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod, GLuint arg2, GLuint arg2Rep, GLuint arg2Mod, GLuint arg3, GLuint arg3Rep, GLuint arg3Mod ) {
  void (*func_glColorFragmentOp3ATI)( GLenum, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint ) = extension_funcs[EXT_glColorFragmentOp3ATI];
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d)\n", op, dst, dstMask, dstMod, arg1, arg1Rep, arg1Mod, arg2, arg2Rep, arg2Mod, arg3, arg3Rep, arg3Mod );
  ENTER_GL();
  func_glColorFragmentOp3ATI( op, dst, dstMask, dstMod, arg1, arg1Rep, arg1Mod, arg2, arg2Rep, arg2Mod, arg3, arg3Rep, arg3Mod );
  LEAVE_GL();
}

static void WINAPI wine_glColorMaskIndexedEXT( GLuint index, GLboolean r, GLboolean g, GLboolean b, GLboolean a ) {
  void (*func_glColorMaskIndexedEXT)( GLuint, GLboolean, GLboolean, GLboolean, GLboolean ) = extension_funcs[EXT_glColorMaskIndexedEXT];
  TRACE("(%d, %d, %d, %d, %d)\n", index, r, g, b, a );
  ENTER_GL();
  func_glColorMaskIndexedEXT( index, r, g, b, a );
  LEAVE_GL();
}

static void WINAPI wine_glColorMaski( GLuint index, GLboolean r, GLboolean g, GLboolean b, GLboolean a ) {
  void (*func_glColorMaski)( GLuint, GLboolean, GLboolean, GLboolean, GLboolean ) = extension_funcs[EXT_glColorMaski];
  TRACE("(%d, %d, %d, %d, %d)\n", index, r, g, b, a );
  ENTER_GL();
  func_glColorMaski( index, r, g, b, a );
  LEAVE_GL();
}

static void WINAPI wine_glColorP3ui( GLenum type, GLuint color ) {
  void (*func_glColorP3ui)( GLenum, GLuint ) = extension_funcs[EXT_glColorP3ui];
  TRACE("(%d, %d)\n", type, color );
  ENTER_GL();
  func_glColorP3ui( type, color );
  LEAVE_GL();
}

static void WINAPI wine_glColorP3uiv( GLenum type, GLuint* color ) {
  void (*func_glColorP3uiv)( GLenum, GLuint* ) = extension_funcs[EXT_glColorP3uiv];
  TRACE("(%d, %p)\n", type, color );
  ENTER_GL();
  func_glColorP3uiv( type, color );
  LEAVE_GL();
}

static void WINAPI wine_glColorP4ui( GLenum type, GLuint color ) {
  void (*func_glColorP4ui)( GLenum, GLuint ) = extension_funcs[EXT_glColorP4ui];
  TRACE("(%d, %d)\n", type, color );
  ENTER_GL();
  func_glColorP4ui( type, color );
  LEAVE_GL();
}

static void WINAPI wine_glColorP4uiv( GLenum type, GLuint* color ) {
  void (*func_glColorP4uiv)( GLenum, GLuint* ) = extension_funcs[EXT_glColorP4uiv];
  TRACE("(%d, %p)\n", type, color );
  ENTER_GL();
  func_glColorP4uiv( type, color );
  LEAVE_GL();
}

static void WINAPI wine_glColorPointerEXT( GLint size, GLenum type, GLsizei stride, GLsizei count, GLvoid* pointer ) {
  void (*func_glColorPointerEXT)( GLint, GLenum, GLsizei, GLsizei, GLvoid* ) = extension_funcs[EXT_glColorPointerEXT];
  TRACE("(%d, %d, %d, %d, %p)\n", size, type, stride, count, pointer );
  ENTER_GL();
  func_glColorPointerEXT( size, type, stride, count, pointer );
  LEAVE_GL();
}

static void WINAPI wine_glColorPointerListIBM( GLint size, GLenum type, GLint stride, GLvoid** pointer, GLint ptrstride ) {
  void (*func_glColorPointerListIBM)( GLint, GLenum, GLint, GLvoid**, GLint ) = extension_funcs[EXT_glColorPointerListIBM];
  TRACE("(%d, %d, %d, %p, %d)\n", size, type, stride, pointer, ptrstride );
  ENTER_GL();
  func_glColorPointerListIBM( size, type, stride, pointer, ptrstride );
  LEAVE_GL();
}

static void WINAPI wine_glColorPointervINTEL( GLint size, GLenum type, GLvoid** pointer ) {
  void (*func_glColorPointervINTEL)( GLint, GLenum, GLvoid** ) = extension_funcs[EXT_glColorPointervINTEL];
  TRACE("(%d, %d, %p)\n", size, type, pointer );
  ENTER_GL();
  func_glColorPointervINTEL( size, type, pointer );
  LEAVE_GL();
}

static void WINAPI wine_glColorSubTable( GLenum target, GLsizei start, GLsizei count, GLenum format, GLenum type, GLvoid* data ) {
  void (*func_glColorSubTable)( GLenum, GLsizei, GLsizei, GLenum, GLenum, GLvoid* ) = extension_funcs[EXT_glColorSubTable];
  TRACE("(%d, %d, %d, %d, %d, %p)\n", target, start, count, format, type, data );
  ENTER_GL();
  func_glColorSubTable( target, start, count, format, type, data );
  LEAVE_GL();
}

static void WINAPI wine_glColorSubTableEXT( GLenum target, GLsizei start, GLsizei count, GLenum format, GLenum type, GLvoid* data ) {
  void (*func_glColorSubTableEXT)( GLenum, GLsizei, GLsizei, GLenum, GLenum, GLvoid* ) = extension_funcs[EXT_glColorSubTableEXT];
  TRACE("(%d, %d, %d, %d, %d, %p)\n", target, start, count, format, type, data );
  ENTER_GL();
  func_glColorSubTableEXT( target, start, count, format, type, data );
  LEAVE_GL();
}

static void WINAPI wine_glColorTable( GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, GLvoid* table ) {
  void (*func_glColorTable)( GLenum, GLenum, GLsizei, GLenum, GLenum, GLvoid* ) = extension_funcs[EXT_glColorTable];
  TRACE("(%d, %d, %d, %d, %d, %p)\n", target, internalformat, width, format, type, table );
  ENTER_GL();
  func_glColorTable( target, internalformat, width, format, type, table );
  LEAVE_GL();
}

static void WINAPI wine_glColorTableEXT( GLenum target, GLenum internalFormat, GLsizei width, GLenum format, GLenum type, GLvoid* table ) {
  void (*func_glColorTableEXT)( GLenum, GLenum, GLsizei, GLenum, GLenum, GLvoid* ) = extension_funcs[EXT_glColorTableEXT];
  TRACE("(%d, %d, %d, %d, %d, %p)\n", target, internalFormat, width, format, type, table );
  ENTER_GL();
  func_glColorTableEXT( target, internalFormat, width, format, type, table );
  LEAVE_GL();
}

static void WINAPI wine_glColorTableParameterfv( GLenum target, GLenum pname, GLfloat* params ) {
  void (*func_glColorTableParameterfv)( GLenum, GLenum, GLfloat* ) = extension_funcs[EXT_glColorTableParameterfv];
  TRACE("(%d, %d, %p)\n", target, pname, params );
  ENTER_GL();
  func_glColorTableParameterfv( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glColorTableParameterfvSGI( GLenum target, GLenum pname, GLfloat* params ) {
  void (*func_glColorTableParameterfvSGI)( GLenum, GLenum, GLfloat* ) = extension_funcs[EXT_glColorTableParameterfvSGI];
  TRACE("(%d, %d, %p)\n", target, pname, params );
  ENTER_GL();
  func_glColorTableParameterfvSGI( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glColorTableParameteriv( GLenum target, GLenum pname, GLint* params ) {
  void (*func_glColorTableParameteriv)( GLenum, GLenum, GLint* ) = extension_funcs[EXT_glColorTableParameteriv];
  TRACE("(%d, %d, %p)\n", target, pname, params );
  ENTER_GL();
  func_glColorTableParameteriv( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glColorTableParameterivSGI( GLenum target, GLenum pname, GLint* params ) {
  void (*func_glColorTableParameterivSGI)( GLenum, GLenum, GLint* ) = extension_funcs[EXT_glColorTableParameterivSGI];
  TRACE("(%d, %d, %p)\n", target, pname, params );
  ENTER_GL();
  func_glColorTableParameterivSGI( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glColorTableSGI( GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, GLvoid* table ) {
  void (*func_glColorTableSGI)( GLenum, GLenum, GLsizei, GLenum, GLenum, GLvoid* ) = extension_funcs[EXT_glColorTableSGI];
  TRACE("(%d, %d, %d, %d, %d, %p)\n", target, internalformat, width, format, type, table );
  ENTER_GL();
  func_glColorTableSGI( target, internalformat, width, format, type, table );
  LEAVE_GL();
}

static void WINAPI wine_glCombinerInputNV( GLenum stage, GLenum portion, GLenum variable, GLenum input, GLenum mapping, GLenum componentUsage ) {
  void (*func_glCombinerInputNV)( GLenum, GLenum, GLenum, GLenum, GLenum, GLenum ) = extension_funcs[EXT_glCombinerInputNV];
  TRACE("(%d, %d, %d, %d, %d, %d)\n", stage, portion, variable, input, mapping, componentUsage );
  ENTER_GL();
  func_glCombinerInputNV( stage, portion, variable, input, mapping, componentUsage );
  LEAVE_GL();
}

static void WINAPI wine_glCombinerOutputNV( GLenum stage, GLenum portion, GLenum abOutput, GLenum cdOutput, GLenum sumOutput, GLenum scale, GLenum bias, GLboolean abDotProduct, GLboolean cdDotProduct, GLboolean muxSum ) {
  void (*func_glCombinerOutputNV)( GLenum, GLenum, GLenum, GLenum, GLenum, GLenum, GLenum, GLboolean, GLboolean, GLboolean ) = extension_funcs[EXT_glCombinerOutputNV];
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %d, %d)\n", stage, portion, abOutput, cdOutput, sumOutput, scale, bias, abDotProduct, cdDotProduct, muxSum );
  ENTER_GL();
  func_glCombinerOutputNV( stage, portion, abOutput, cdOutput, sumOutput, scale, bias, abDotProduct, cdDotProduct, muxSum );
  LEAVE_GL();
}

static void WINAPI wine_glCombinerParameterfNV( GLenum pname, GLfloat param ) {
  void (*func_glCombinerParameterfNV)( GLenum, GLfloat ) = extension_funcs[EXT_glCombinerParameterfNV];
  TRACE("(%d, %f)\n", pname, param );
  ENTER_GL();
  func_glCombinerParameterfNV( pname, param );
  LEAVE_GL();
}

static void WINAPI wine_glCombinerParameterfvNV( GLenum pname, GLfloat* params ) {
  void (*func_glCombinerParameterfvNV)( GLenum, GLfloat* ) = extension_funcs[EXT_glCombinerParameterfvNV];
  TRACE("(%d, %p)\n", pname, params );
  ENTER_GL();
  func_glCombinerParameterfvNV( pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glCombinerParameteriNV( GLenum pname, GLint param ) {
  void (*func_glCombinerParameteriNV)( GLenum, GLint ) = extension_funcs[EXT_glCombinerParameteriNV];
  TRACE("(%d, %d)\n", pname, param );
  ENTER_GL();
  func_glCombinerParameteriNV( pname, param );
  LEAVE_GL();
}

static void WINAPI wine_glCombinerParameterivNV( GLenum pname, GLint* params ) {
  void (*func_glCombinerParameterivNV)( GLenum, GLint* ) = extension_funcs[EXT_glCombinerParameterivNV];
  TRACE("(%d, %p)\n", pname, params );
  ENTER_GL();
  func_glCombinerParameterivNV( pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glCombinerStageParameterfvNV( GLenum stage, GLenum pname, GLfloat* params ) {
  void (*func_glCombinerStageParameterfvNV)( GLenum, GLenum, GLfloat* ) = extension_funcs[EXT_glCombinerStageParameterfvNV];
  TRACE("(%d, %d, %p)\n", stage, pname, params );
  ENTER_GL();
  func_glCombinerStageParameterfvNV( stage, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glCompileShader( GLuint shader ) {
  void (*func_glCompileShader)( GLuint ) = extension_funcs[EXT_glCompileShader];
  TRACE("(%d)\n", shader );
  ENTER_GL();
  func_glCompileShader( shader );
  LEAVE_GL();
}

static void WINAPI wine_glCompileShaderARB( unsigned int shaderObj ) {
  void (*func_glCompileShaderARB)( unsigned int ) = extension_funcs[EXT_glCompileShaderARB];
  TRACE("(%d)\n", shaderObj );
  ENTER_GL();
  func_glCompileShaderARB( shaderObj );
  LEAVE_GL();
}

static void WINAPI wine_glCompileShaderIncludeARB( GLuint shader, GLsizei count, char** path, GLint* length ) {
  void (*func_glCompileShaderIncludeARB)( GLuint, GLsizei, char**, GLint* ) = extension_funcs[EXT_glCompileShaderIncludeARB];
  TRACE("(%d, %d, %p, %p)\n", shader, count, path, length );
  ENTER_GL();
  func_glCompileShaderIncludeARB( shader, count, path, length );
  LEAVE_GL();
}

static void WINAPI wine_glCompressedMultiTexImage1DEXT( GLenum texunit, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imageSize, GLvoid* bits ) {
  void (*func_glCompressedMultiTexImage1DEXT)( GLenum, GLenum, GLint, GLenum, GLsizei, GLint, GLsizei, GLvoid* ) = extension_funcs[EXT_glCompressedMultiTexImage1DEXT];
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %p)\n", texunit, target, level, internalformat, width, border, imageSize, bits );
  ENTER_GL();
  func_glCompressedMultiTexImage1DEXT( texunit, target, level, internalformat, width, border, imageSize, bits );
  LEAVE_GL();
}

static void WINAPI wine_glCompressedMultiTexImage2DEXT( GLenum texunit, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, GLvoid* bits ) {
  void (*func_glCompressedMultiTexImage2DEXT)( GLenum, GLenum, GLint, GLenum, GLsizei, GLsizei, GLint, GLsizei, GLvoid* ) = extension_funcs[EXT_glCompressedMultiTexImage2DEXT];
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %p)\n", texunit, target, level, internalformat, width, height, border, imageSize, bits );
  ENTER_GL();
  func_glCompressedMultiTexImage2DEXT( texunit, target, level, internalformat, width, height, border, imageSize, bits );
  LEAVE_GL();
}

static void WINAPI wine_glCompressedMultiTexImage3DEXT( GLenum texunit, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, GLvoid* bits ) {
  void (*func_glCompressedMultiTexImage3DEXT)( GLenum, GLenum, GLint, GLenum, GLsizei, GLsizei, GLsizei, GLint, GLsizei, GLvoid* ) = extension_funcs[EXT_glCompressedMultiTexImage3DEXT];
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %d, %p)\n", texunit, target, level, internalformat, width, height, depth, border, imageSize, bits );
  ENTER_GL();
  func_glCompressedMultiTexImage3DEXT( texunit, target, level, internalformat, width, height, depth, border, imageSize, bits );
  LEAVE_GL();
}

static void WINAPI wine_glCompressedMultiTexSubImage1DEXT( GLenum texunit, GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, GLvoid* bits ) {
  void (*func_glCompressedMultiTexSubImage1DEXT)( GLenum, GLenum, GLint, GLint, GLsizei, GLenum, GLsizei, GLvoid* ) = extension_funcs[EXT_glCompressedMultiTexSubImage1DEXT];
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %p)\n", texunit, target, level, xoffset, width, format, imageSize, bits );
  ENTER_GL();
  func_glCompressedMultiTexSubImage1DEXT( texunit, target, level, xoffset, width, format, imageSize, bits );
  LEAVE_GL();
}

static void WINAPI wine_glCompressedMultiTexSubImage2DEXT( GLenum texunit, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, GLvoid* bits ) {
  void (*func_glCompressedMultiTexSubImage2DEXT)( GLenum, GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLsizei, GLvoid* ) = extension_funcs[EXT_glCompressedMultiTexSubImage2DEXT];
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %d, %p)\n", texunit, target, level, xoffset, yoffset, width, height, format, imageSize, bits );
  ENTER_GL();
  func_glCompressedMultiTexSubImage2DEXT( texunit, target, level, xoffset, yoffset, width, height, format, imageSize, bits );
  LEAVE_GL();
}

static void WINAPI wine_glCompressedMultiTexSubImage3DEXT( GLenum texunit, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, GLvoid* bits ) {
  void (*func_glCompressedMultiTexSubImage3DEXT)( GLenum, GLenum, GLint, GLint, GLint, GLint, GLsizei, GLsizei, GLsizei, GLenum, GLsizei, GLvoid* ) = extension_funcs[EXT_glCompressedMultiTexSubImage3DEXT];
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %p)\n", texunit, target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, bits );
  ENTER_GL();
  func_glCompressedMultiTexSubImage3DEXT( texunit, target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, bits );
  LEAVE_GL();
}

static void WINAPI wine_glCompressedTexImage1D( GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imageSize, GLvoid* data ) {
  void (*func_glCompressedTexImage1D)( GLenum, GLint, GLenum, GLsizei, GLint, GLsizei, GLvoid* ) = extension_funcs[EXT_glCompressedTexImage1D];
  TRACE("(%d, %d, %d, %d, %d, %d, %p)\n", target, level, internalformat, width, border, imageSize, data );
  ENTER_GL();
  func_glCompressedTexImage1D( target, level, internalformat, width, border, imageSize, data );
  LEAVE_GL();
}

static void WINAPI wine_glCompressedTexImage1DARB( GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imageSize, GLvoid* data ) {
  void (*func_glCompressedTexImage1DARB)( GLenum, GLint, GLenum, GLsizei, GLint, GLsizei, GLvoid* ) = extension_funcs[EXT_glCompressedTexImage1DARB];
  TRACE("(%d, %d, %d, %d, %d, %d, %p)\n", target, level, internalformat, width, border, imageSize, data );
  ENTER_GL();
  func_glCompressedTexImage1DARB( target, level, internalformat, width, border, imageSize, data );
  LEAVE_GL();
}

static void WINAPI wine_glCompressedTexImage2D( GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, GLvoid* data ) {
  void (*func_glCompressedTexImage2D)( GLenum, GLint, GLenum, GLsizei, GLsizei, GLint, GLsizei, GLvoid* ) = extension_funcs[EXT_glCompressedTexImage2D];
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %p)\n", target, level, internalformat, width, height, border, imageSize, data );
  ENTER_GL();
  func_glCompressedTexImage2D( target, level, internalformat, width, height, border, imageSize, data );
  LEAVE_GL();
}

static void WINAPI wine_glCompressedTexImage2DARB( GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, GLvoid* data ) {
  void (*func_glCompressedTexImage2DARB)( GLenum, GLint, GLenum, GLsizei, GLsizei, GLint, GLsizei, GLvoid* ) = extension_funcs[EXT_glCompressedTexImage2DARB];
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %p)\n", target, level, internalformat, width, height, border, imageSize, data );
  ENTER_GL();
  func_glCompressedTexImage2DARB( target, level, internalformat, width, height, border, imageSize, data );
  LEAVE_GL();
}

static void WINAPI wine_glCompressedTexImage3D( GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, GLvoid* data ) {
  void (*func_glCompressedTexImage3D)( GLenum, GLint, GLenum, GLsizei, GLsizei, GLsizei, GLint, GLsizei, GLvoid* ) = extension_funcs[EXT_glCompressedTexImage3D];
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %p)\n", target, level, internalformat, width, height, depth, border, imageSize, data );
  ENTER_GL();
  func_glCompressedTexImage3D( target, level, internalformat, width, height, depth, border, imageSize, data );
  LEAVE_GL();
}

static void WINAPI wine_glCompressedTexImage3DARB( GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, GLvoid* data ) {
  void (*func_glCompressedTexImage3DARB)( GLenum, GLint, GLenum, GLsizei, GLsizei, GLsizei, GLint, GLsizei, GLvoid* ) = extension_funcs[EXT_glCompressedTexImage3DARB];
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %p)\n", target, level, internalformat, width, height, depth, border, imageSize, data );
  ENTER_GL();
  func_glCompressedTexImage3DARB( target, level, internalformat, width, height, depth, border, imageSize, data );
  LEAVE_GL();
}

static void WINAPI wine_glCompressedTexSubImage1D( GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, GLvoid* data ) {
  void (*func_glCompressedTexSubImage1D)( GLenum, GLint, GLint, GLsizei, GLenum, GLsizei, GLvoid* ) = extension_funcs[EXT_glCompressedTexSubImage1D];
  TRACE("(%d, %d, %d, %d, %d, %d, %p)\n", target, level, xoffset, width, format, imageSize, data );
  ENTER_GL();
  func_glCompressedTexSubImage1D( target, level, xoffset, width, format, imageSize, data );
  LEAVE_GL();
}

static void WINAPI wine_glCompressedTexSubImage1DARB( GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, GLvoid* data ) {
  void (*func_glCompressedTexSubImage1DARB)( GLenum, GLint, GLint, GLsizei, GLenum, GLsizei, GLvoid* ) = extension_funcs[EXT_glCompressedTexSubImage1DARB];
  TRACE("(%d, %d, %d, %d, %d, %d, %p)\n", target, level, xoffset, width, format, imageSize, data );
  ENTER_GL();
  func_glCompressedTexSubImage1DARB( target, level, xoffset, width, format, imageSize, data );
  LEAVE_GL();
}

static void WINAPI wine_glCompressedTexSubImage2D( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, GLvoid* data ) {
  void (*func_glCompressedTexSubImage2D)( GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLsizei, GLvoid* ) = extension_funcs[EXT_glCompressedTexSubImage2D];
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %p)\n", target, level, xoffset, yoffset, width, height, format, imageSize, data );
  ENTER_GL();
  func_glCompressedTexSubImage2D( target, level, xoffset, yoffset, width, height, format, imageSize, data );
  LEAVE_GL();
}

static void WINAPI wine_glCompressedTexSubImage2DARB( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, GLvoid* data ) {
  void (*func_glCompressedTexSubImage2DARB)( GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLsizei, GLvoid* ) = extension_funcs[EXT_glCompressedTexSubImage2DARB];
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %p)\n", target, level, xoffset, yoffset, width, height, format, imageSize, data );
  ENTER_GL();
  func_glCompressedTexSubImage2DARB( target, level, xoffset, yoffset, width, height, format, imageSize, data );
  LEAVE_GL();
}

static void WINAPI wine_glCompressedTexSubImage3D( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, GLvoid* data ) {
  void (*func_glCompressedTexSubImage3D)( GLenum, GLint, GLint, GLint, GLint, GLsizei, GLsizei, GLsizei, GLenum, GLsizei, GLvoid* ) = extension_funcs[EXT_glCompressedTexSubImage3D];
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %p)\n", target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, data );
  ENTER_GL();
  func_glCompressedTexSubImage3D( target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, data );
  LEAVE_GL();
}

static void WINAPI wine_glCompressedTexSubImage3DARB( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, GLvoid* data ) {
  void (*func_glCompressedTexSubImage3DARB)( GLenum, GLint, GLint, GLint, GLint, GLsizei, GLsizei, GLsizei, GLenum, GLsizei, GLvoid* ) = extension_funcs[EXT_glCompressedTexSubImage3DARB];
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %p)\n", target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, data );
  ENTER_GL();
  func_glCompressedTexSubImage3DARB( target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, data );
  LEAVE_GL();
}

static void WINAPI wine_glCompressedTextureImage1DEXT( GLuint texture, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imageSize, GLvoid* bits ) {
  void (*func_glCompressedTextureImage1DEXT)( GLuint, GLenum, GLint, GLenum, GLsizei, GLint, GLsizei, GLvoid* ) = extension_funcs[EXT_glCompressedTextureImage1DEXT];
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %p)\n", texture, target, level, internalformat, width, border, imageSize, bits );
  ENTER_GL();
  func_glCompressedTextureImage1DEXT( texture, target, level, internalformat, width, border, imageSize, bits );
  LEAVE_GL();
}

static void WINAPI wine_glCompressedTextureImage2DEXT( GLuint texture, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, GLvoid* bits ) {
  void (*func_glCompressedTextureImage2DEXT)( GLuint, GLenum, GLint, GLenum, GLsizei, GLsizei, GLint, GLsizei, GLvoid* ) = extension_funcs[EXT_glCompressedTextureImage2DEXT];
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %p)\n", texture, target, level, internalformat, width, height, border, imageSize, bits );
  ENTER_GL();
  func_glCompressedTextureImage2DEXT( texture, target, level, internalformat, width, height, border, imageSize, bits );
  LEAVE_GL();
}

static void WINAPI wine_glCompressedTextureImage3DEXT( GLuint texture, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, GLvoid* bits ) {
  void (*func_glCompressedTextureImage3DEXT)( GLuint, GLenum, GLint, GLenum, GLsizei, GLsizei, GLsizei, GLint, GLsizei, GLvoid* ) = extension_funcs[EXT_glCompressedTextureImage3DEXT];
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %d, %p)\n", texture, target, level, internalformat, width, height, depth, border, imageSize, bits );
  ENTER_GL();
  func_glCompressedTextureImage3DEXT( texture, target, level, internalformat, width, height, depth, border, imageSize, bits );
  LEAVE_GL();
}

static void WINAPI wine_glCompressedTextureSubImage1DEXT( GLuint texture, GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, GLvoid* bits ) {
  void (*func_glCompressedTextureSubImage1DEXT)( GLuint, GLenum, GLint, GLint, GLsizei, GLenum, GLsizei, GLvoid* ) = extension_funcs[EXT_glCompressedTextureSubImage1DEXT];
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %p)\n", texture, target, level, xoffset, width, format, imageSize, bits );
  ENTER_GL();
  func_glCompressedTextureSubImage1DEXT( texture, target, level, xoffset, width, format, imageSize, bits );
  LEAVE_GL();
}

static void WINAPI wine_glCompressedTextureSubImage2DEXT( GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, GLvoid* bits ) {
  void (*func_glCompressedTextureSubImage2DEXT)( GLuint, GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLsizei, GLvoid* ) = extension_funcs[EXT_glCompressedTextureSubImage2DEXT];
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %d, %p)\n", texture, target, level, xoffset, yoffset, width, height, format, imageSize, bits );
  ENTER_GL();
  func_glCompressedTextureSubImage2DEXT( texture, target, level, xoffset, yoffset, width, height, format, imageSize, bits );
  LEAVE_GL();
}

static void WINAPI wine_glCompressedTextureSubImage3DEXT( GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, GLvoid* bits ) {
  void (*func_glCompressedTextureSubImage3DEXT)( GLuint, GLenum, GLint, GLint, GLint, GLint, GLsizei, GLsizei, GLsizei, GLenum, GLsizei, GLvoid* ) = extension_funcs[EXT_glCompressedTextureSubImage3DEXT];
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %p)\n", texture, target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, bits );
  ENTER_GL();
  func_glCompressedTextureSubImage3DEXT( texture, target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, bits );
  LEAVE_GL();
}

static void WINAPI wine_glConvolutionFilter1D( GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, GLvoid* image ) {
  void (*func_glConvolutionFilter1D)( GLenum, GLenum, GLsizei, GLenum, GLenum, GLvoid* ) = extension_funcs[EXT_glConvolutionFilter1D];
  TRACE("(%d, %d, %d, %d, %d, %p)\n", target, internalformat, width, format, type, image );
  ENTER_GL();
  func_glConvolutionFilter1D( target, internalformat, width, format, type, image );
  LEAVE_GL();
}

static void WINAPI wine_glConvolutionFilter1DEXT( GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, GLvoid* image ) {
  void (*func_glConvolutionFilter1DEXT)( GLenum, GLenum, GLsizei, GLenum, GLenum, GLvoid* ) = extension_funcs[EXT_glConvolutionFilter1DEXT];
  TRACE("(%d, %d, %d, %d, %d, %p)\n", target, internalformat, width, format, type, image );
  ENTER_GL();
  func_glConvolutionFilter1DEXT( target, internalformat, width, format, type, image );
  LEAVE_GL();
}

static void WINAPI wine_glConvolutionFilter2D( GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid* image ) {
  void (*func_glConvolutionFilter2D)( GLenum, GLenum, GLsizei, GLsizei, GLenum, GLenum, GLvoid* ) = extension_funcs[EXT_glConvolutionFilter2D];
  TRACE("(%d, %d, %d, %d, %d, %d, %p)\n", target, internalformat, width, height, format, type, image );
  ENTER_GL();
  func_glConvolutionFilter2D( target, internalformat, width, height, format, type, image );
  LEAVE_GL();
}

static void WINAPI wine_glConvolutionFilter2DEXT( GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid* image ) {
  void (*func_glConvolutionFilter2DEXT)( GLenum, GLenum, GLsizei, GLsizei, GLenum, GLenum, GLvoid* ) = extension_funcs[EXT_glConvolutionFilter2DEXT];
  TRACE("(%d, %d, %d, %d, %d, %d, %p)\n", target, internalformat, width, height, format, type, image );
  ENTER_GL();
  func_glConvolutionFilter2DEXT( target, internalformat, width, height, format, type, image );
  LEAVE_GL();
}

static void WINAPI wine_glConvolutionParameterf( GLenum target, GLenum pname, GLfloat params ) {
  void (*func_glConvolutionParameterf)( GLenum, GLenum, GLfloat ) = extension_funcs[EXT_glConvolutionParameterf];
  TRACE("(%d, %d, %f)\n", target, pname, params );
  ENTER_GL();
  func_glConvolutionParameterf( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glConvolutionParameterfEXT( GLenum target, GLenum pname, GLfloat params ) {
  void (*func_glConvolutionParameterfEXT)( GLenum, GLenum, GLfloat ) = extension_funcs[EXT_glConvolutionParameterfEXT];
  TRACE("(%d, %d, %f)\n", target, pname, params );
  ENTER_GL();
  func_glConvolutionParameterfEXT( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glConvolutionParameterfv( GLenum target, GLenum pname, GLfloat* params ) {
  void (*func_glConvolutionParameterfv)( GLenum, GLenum, GLfloat* ) = extension_funcs[EXT_glConvolutionParameterfv];
  TRACE("(%d, %d, %p)\n", target, pname, params );
  ENTER_GL();
  func_glConvolutionParameterfv( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glConvolutionParameterfvEXT( GLenum target, GLenum pname, GLfloat* params ) {
  void (*func_glConvolutionParameterfvEXT)( GLenum, GLenum, GLfloat* ) = extension_funcs[EXT_glConvolutionParameterfvEXT];
  TRACE("(%d, %d, %p)\n", target, pname, params );
  ENTER_GL();
  func_glConvolutionParameterfvEXT( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glConvolutionParameteri( GLenum target, GLenum pname, GLint params ) {
  void (*func_glConvolutionParameteri)( GLenum, GLenum, GLint ) = extension_funcs[EXT_glConvolutionParameteri];
  TRACE("(%d, %d, %d)\n", target, pname, params );
  ENTER_GL();
  func_glConvolutionParameteri( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glConvolutionParameteriEXT( GLenum target, GLenum pname, GLint params ) {
  void (*func_glConvolutionParameteriEXT)( GLenum, GLenum, GLint ) = extension_funcs[EXT_glConvolutionParameteriEXT];
  TRACE("(%d, %d, %d)\n", target, pname, params );
  ENTER_GL();
  func_glConvolutionParameteriEXT( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glConvolutionParameteriv( GLenum target, GLenum pname, GLint* params ) {
  void (*func_glConvolutionParameteriv)( GLenum, GLenum, GLint* ) = extension_funcs[EXT_glConvolutionParameteriv];
  TRACE("(%d, %d, %p)\n", target, pname, params );
  ENTER_GL();
  func_glConvolutionParameteriv( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glConvolutionParameterivEXT( GLenum target, GLenum pname, GLint* params ) {
  void (*func_glConvolutionParameterivEXT)( GLenum, GLenum, GLint* ) = extension_funcs[EXT_glConvolutionParameterivEXT];
  TRACE("(%d, %d, %p)\n", target, pname, params );
  ENTER_GL();
  func_glConvolutionParameterivEXT( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glCopyBufferSubData( GLenum readTarget, GLenum writeTarget, INT_PTR readOffset, INT_PTR writeOffset, INT_PTR size ) {
  void (*func_glCopyBufferSubData)( GLenum, GLenum, INT_PTR, INT_PTR, INT_PTR ) = extension_funcs[EXT_glCopyBufferSubData];
  TRACE("(%d, %d, %ld, %ld, %ld)\n", readTarget, writeTarget, readOffset, writeOffset, size );
  ENTER_GL();
  func_glCopyBufferSubData( readTarget, writeTarget, readOffset, writeOffset, size );
  LEAVE_GL();
}

static void WINAPI wine_glCopyColorSubTable( GLenum target, GLsizei start, GLint x, GLint y, GLsizei width ) {
  void (*func_glCopyColorSubTable)( GLenum, GLsizei, GLint, GLint, GLsizei ) = extension_funcs[EXT_glCopyColorSubTable];
  TRACE("(%d, %d, %d, %d, %d)\n", target, start, x, y, width );
  ENTER_GL();
  func_glCopyColorSubTable( target, start, x, y, width );
  LEAVE_GL();
}

static void WINAPI wine_glCopyColorSubTableEXT( GLenum target, GLsizei start, GLint x, GLint y, GLsizei width ) {
  void (*func_glCopyColorSubTableEXT)( GLenum, GLsizei, GLint, GLint, GLsizei ) = extension_funcs[EXT_glCopyColorSubTableEXT];
  TRACE("(%d, %d, %d, %d, %d)\n", target, start, x, y, width );
  ENTER_GL();
  func_glCopyColorSubTableEXT( target, start, x, y, width );
  LEAVE_GL();
}

static void WINAPI wine_glCopyColorTable( GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width ) {
  void (*func_glCopyColorTable)( GLenum, GLenum, GLint, GLint, GLsizei ) = extension_funcs[EXT_glCopyColorTable];
  TRACE("(%d, %d, %d, %d, %d)\n", target, internalformat, x, y, width );
  ENTER_GL();
  func_glCopyColorTable( target, internalformat, x, y, width );
  LEAVE_GL();
}

static void WINAPI wine_glCopyColorTableSGI( GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width ) {
  void (*func_glCopyColorTableSGI)( GLenum, GLenum, GLint, GLint, GLsizei ) = extension_funcs[EXT_glCopyColorTableSGI];
  TRACE("(%d, %d, %d, %d, %d)\n", target, internalformat, x, y, width );
  ENTER_GL();
  func_glCopyColorTableSGI( target, internalformat, x, y, width );
  LEAVE_GL();
}

static void WINAPI wine_glCopyConvolutionFilter1D( GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width ) {
  void (*func_glCopyConvolutionFilter1D)( GLenum, GLenum, GLint, GLint, GLsizei ) = extension_funcs[EXT_glCopyConvolutionFilter1D];
  TRACE("(%d, %d, %d, %d, %d)\n", target, internalformat, x, y, width );
  ENTER_GL();
  func_glCopyConvolutionFilter1D( target, internalformat, x, y, width );
  LEAVE_GL();
}

static void WINAPI wine_glCopyConvolutionFilter1DEXT( GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width ) {
  void (*func_glCopyConvolutionFilter1DEXT)( GLenum, GLenum, GLint, GLint, GLsizei ) = extension_funcs[EXT_glCopyConvolutionFilter1DEXT];
  TRACE("(%d, %d, %d, %d, %d)\n", target, internalformat, x, y, width );
  ENTER_GL();
  func_glCopyConvolutionFilter1DEXT( target, internalformat, x, y, width );
  LEAVE_GL();
}

static void WINAPI wine_glCopyConvolutionFilter2D( GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height ) {
  void (*func_glCopyConvolutionFilter2D)( GLenum, GLenum, GLint, GLint, GLsizei, GLsizei ) = extension_funcs[EXT_glCopyConvolutionFilter2D];
  TRACE("(%d, %d, %d, %d, %d, %d)\n", target, internalformat, x, y, width, height );
  ENTER_GL();
  func_glCopyConvolutionFilter2D( target, internalformat, x, y, width, height );
  LEAVE_GL();
}

static void WINAPI wine_glCopyConvolutionFilter2DEXT( GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height ) {
  void (*func_glCopyConvolutionFilter2DEXT)( GLenum, GLenum, GLint, GLint, GLsizei, GLsizei ) = extension_funcs[EXT_glCopyConvolutionFilter2DEXT];
  TRACE("(%d, %d, %d, %d, %d, %d)\n", target, internalformat, x, y, width, height );
  ENTER_GL();
  func_glCopyConvolutionFilter2DEXT( target, internalformat, x, y, width, height );
  LEAVE_GL();
}

static void WINAPI wine_glCopyImageSubDataNV( GLuint srcName, GLenum srcTarget, GLint srcLevel, GLint srcX, GLint srcY, GLint srcZ, GLuint dstName, GLenum dstTarget, GLint dstLevel, GLint dstX, GLint dstY, GLint dstZ, GLsizei width, GLsizei height, GLsizei depth ) {
  void (*func_glCopyImageSubDataNV)( GLuint, GLenum, GLint, GLint, GLint, GLint, GLuint, GLenum, GLint, GLint, GLint, GLint, GLsizei, GLsizei, GLsizei ) = extension_funcs[EXT_glCopyImageSubDataNV];
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d)\n", srcName, srcTarget, srcLevel, srcX, srcY, srcZ, dstName, dstTarget, dstLevel, dstX, dstY, dstZ, width, height, depth );
  ENTER_GL();
  func_glCopyImageSubDataNV( srcName, srcTarget, srcLevel, srcX, srcY, srcZ, dstName, dstTarget, dstLevel, dstX, dstY, dstZ, width, height, depth );
  LEAVE_GL();
}

static void WINAPI wine_glCopyMultiTexImage1DEXT( GLenum texunit, GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLint border ) {
  void (*func_glCopyMultiTexImage1DEXT)( GLenum, GLenum, GLint, GLenum, GLint, GLint, GLsizei, GLint ) = extension_funcs[EXT_glCopyMultiTexImage1DEXT];
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d)\n", texunit, target, level, internalformat, x, y, width, border );
  ENTER_GL();
  func_glCopyMultiTexImage1DEXT( texunit, target, level, internalformat, x, y, width, border );
  LEAVE_GL();
}

static void WINAPI wine_glCopyMultiTexImage2DEXT( GLenum texunit, GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border ) {
  void (*func_glCopyMultiTexImage2DEXT)( GLenum, GLenum, GLint, GLenum, GLint, GLint, GLsizei, GLsizei, GLint ) = extension_funcs[EXT_glCopyMultiTexImage2DEXT];
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %d)\n", texunit, target, level, internalformat, x, y, width, height, border );
  ENTER_GL();
  func_glCopyMultiTexImage2DEXT( texunit, target, level, internalformat, x, y, width, height, border );
  LEAVE_GL();
}

static void WINAPI wine_glCopyMultiTexSubImage1DEXT( GLenum texunit, GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width ) {
  void (*func_glCopyMultiTexSubImage1DEXT)( GLenum, GLenum, GLint, GLint, GLint, GLint, GLsizei ) = extension_funcs[EXT_glCopyMultiTexSubImage1DEXT];
  TRACE("(%d, %d, %d, %d, %d, %d, %d)\n", texunit, target, level, xoffset, x, y, width );
  ENTER_GL();
  func_glCopyMultiTexSubImage1DEXT( texunit, target, level, xoffset, x, y, width );
  LEAVE_GL();
}

static void WINAPI wine_glCopyMultiTexSubImage2DEXT( GLenum texunit, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height ) {
  void (*func_glCopyMultiTexSubImage2DEXT)( GLenum, GLenum, GLint, GLint, GLint, GLint, GLint, GLsizei, GLsizei ) = extension_funcs[EXT_glCopyMultiTexSubImage2DEXT];
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %d)\n", texunit, target, level, xoffset, yoffset, x, y, width, height );
  ENTER_GL();
  func_glCopyMultiTexSubImage2DEXT( texunit, target, level, xoffset, yoffset, x, y, width, height );
  LEAVE_GL();
}

static void WINAPI wine_glCopyMultiTexSubImage3DEXT( GLenum texunit, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height ) {
  void (*func_glCopyMultiTexSubImage3DEXT)( GLenum, GLenum, GLint, GLint, GLint, GLint, GLint, GLint, GLsizei, GLsizei ) = extension_funcs[EXT_glCopyMultiTexSubImage3DEXT];
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %d, %d)\n", texunit, target, level, xoffset, yoffset, zoffset, x, y, width, height );
  ENTER_GL();
  func_glCopyMultiTexSubImage3DEXT( texunit, target, level, xoffset, yoffset, zoffset, x, y, width, height );
  LEAVE_GL();
}

static void WINAPI wine_glCopyTexImage1DEXT( GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLint border ) {
  void (*func_glCopyTexImage1DEXT)( GLenum, GLint, GLenum, GLint, GLint, GLsizei, GLint ) = extension_funcs[EXT_glCopyTexImage1DEXT];
  TRACE("(%d, %d, %d, %d, %d, %d, %d)\n", target, level, internalformat, x, y, width, border );
  ENTER_GL();
  func_glCopyTexImage1DEXT( target, level, internalformat, x, y, width, border );
  LEAVE_GL();
}

static void WINAPI wine_glCopyTexImage2DEXT( GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border ) {
  void (*func_glCopyTexImage2DEXT)( GLenum, GLint, GLenum, GLint, GLint, GLsizei, GLsizei, GLint ) = extension_funcs[EXT_glCopyTexImage2DEXT];
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d)\n", target, level, internalformat, x, y, width, height, border );
  ENTER_GL();
  func_glCopyTexImage2DEXT( target, level, internalformat, x, y, width, height, border );
  LEAVE_GL();
}

static void WINAPI wine_glCopyTexSubImage1DEXT( GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width ) {
  void (*func_glCopyTexSubImage1DEXT)( GLenum, GLint, GLint, GLint, GLint, GLsizei ) = extension_funcs[EXT_glCopyTexSubImage1DEXT];
  TRACE("(%d, %d, %d, %d, %d, %d)\n", target, level, xoffset, x, y, width );
  ENTER_GL();
  func_glCopyTexSubImage1DEXT( target, level, xoffset, x, y, width );
  LEAVE_GL();
}

static void WINAPI wine_glCopyTexSubImage2DEXT( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height ) {
  void (*func_glCopyTexSubImage2DEXT)( GLenum, GLint, GLint, GLint, GLint, GLint, GLsizei, GLsizei ) = extension_funcs[EXT_glCopyTexSubImage2DEXT];
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d)\n", target, level, xoffset, yoffset, x, y, width, height );
  ENTER_GL();
  func_glCopyTexSubImage2DEXT( target, level, xoffset, yoffset, x, y, width, height );
  LEAVE_GL();
}

static void WINAPI wine_glCopyTexSubImage3D( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height ) {
  void (*func_glCopyTexSubImage3D)( GLenum, GLint, GLint, GLint, GLint, GLint, GLint, GLsizei, GLsizei ) = extension_funcs[EXT_glCopyTexSubImage3D];
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %d)\n", target, level, xoffset, yoffset, zoffset, x, y, width, height );
  ENTER_GL();
  func_glCopyTexSubImage3D( target, level, xoffset, yoffset, zoffset, x, y, width, height );
  LEAVE_GL();
}

static void WINAPI wine_glCopyTexSubImage3DEXT( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height ) {
  void (*func_glCopyTexSubImage3DEXT)( GLenum, GLint, GLint, GLint, GLint, GLint, GLint, GLsizei, GLsizei ) = extension_funcs[EXT_glCopyTexSubImage3DEXT];
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %d)\n", target, level, xoffset, yoffset, zoffset, x, y, width, height );
  ENTER_GL();
  func_glCopyTexSubImage3DEXT( target, level, xoffset, yoffset, zoffset, x, y, width, height );
  LEAVE_GL();
}

static void WINAPI wine_glCopyTextureImage1DEXT( GLuint texture, GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLint border ) {
  void (*func_glCopyTextureImage1DEXT)( GLuint, GLenum, GLint, GLenum, GLint, GLint, GLsizei, GLint ) = extension_funcs[EXT_glCopyTextureImage1DEXT];
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d)\n", texture, target, level, internalformat, x, y, width, border );
  ENTER_GL();
  func_glCopyTextureImage1DEXT( texture, target, level, internalformat, x, y, width, border );
  LEAVE_GL();
}

static void WINAPI wine_glCopyTextureImage2DEXT( GLuint texture, GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border ) {
  void (*func_glCopyTextureImage2DEXT)( GLuint, GLenum, GLint, GLenum, GLint, GLint, GLsizei, GLsizei, GLint ) = extension_funcs[EXT_glCopyTextureImage2DEXT];
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %d)\n", texture, target, level, internalformat, x, y, width, height, border );
  ENTER_GL();
  func_glCopyTextureImage2DEXT( texture, target, level, internalformat, x, y, width, height, border );
  LEAVE_GL();
}

static void WINAPI wine_glCopyTextureSubImage1DEXT( GLuint texture, GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width ) {
  void (*func_glCopyTextureSubImage1DEXT)( GLuint, GLenum, GLint, GLint, GLint, GLint, GLsizei ) = extension_funcs[EXT_glCopyTextureSubImage1DEXT];
  TRACE("(%d, %d, %d, %d, %d, %d, %d)\n", texture, target, level, xoffset, x, y, width );
  ENTER_GL();
  func_glCopyTextureSubImage1DEXT( texture, target, level, xoffset, x, y, width );
  LEAVE_GL();
}

static void WINAPI wine_glCopyTextureSubImage2DEXT( GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height ) {
  void (*func_glCopyTextureSubImage2DEXT)( GLuint, GLenum, GLint, GLint, GLint, GLint, GLint, GLsizei, GLsizei ) = extension_funcs[EXT_glCopyTextureSubImage2DEXT];
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %d)\n", texture, target, level, xoffset, yoffset, x, y, width, height );
  ENTER_GL();
  func_glCopyTextureSubImage2DEXT( texture, target, level, xoffset, yoffset, x, y, width, height );
  LEAVE_GL();
}

static void WINAPI wine_glCopyTextureSubImage3DEXT( GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height ) {
  void (*func_glCopyTextureSubImage3DEXT)( GLuint, GLenum, GLint, GLint, GLint, GLint, GLint, GLint, GLsizei, GLsizei ) = extension_funcs[EXT_glCopyTextureSubImage3DEXT];
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %d, %d)\n", texture, target, level, xoffset, yoffset, zoffset, x, y, width, height );
  ENTER_GL();
  func_glCopyTextureSubImage3DEXT( texture, target, level, xoffset, yoffset, zoffset, x, y, width, height );
  LEAVE_GL();
}

static GLuint WINAPI wine_glCreateProgram( void ) {
  GLuint ret_value;
  GLuint (*func_glCreateProgram)( void ) = extension_funcs[EXT_glCreateProgram];
  TRACE("()\n");
  ENTER_GL();
  ret_value = func_glCreateProgram( );
  LEAVE_GL();
  return ret_value;
}

static unsigned int WINAPI wine_glCreateProgramObjectARB( void ) {
  unsigned int ret_value;
  unsigned int (*func_glCreateProgramObjectARB)( void ) = extension_funcs[EXT_glCreateProgramObjectARB];
  TRACE("()\n");
  ENTER_GL();
  ret_value = func_glCreateProgramObjectARB( );
  LEAVE_GL();
  return ret_value;
}

static GLuint WINAPI wine_glCreateShader( GLenum type ) {
  GLuint ret_value;
  GLuint (*func_glCreateShader)( GLenum ) = extension_funcs[EXT_glCreateShader];
  TRACE("(%d)\n", type );
  ENTER_GL();
  ret_value = func_glCreateShader( type );
  LEAVE_GL();
  return ret_value;
}

static unsigned int WINAPI wine_glCreateShaderObjectARB( GLenum shaderType ) {
  unsigned int ret_value;
  unsigned int (*func_glCreateShaderObjectARB)( GLenum ) = extension_funcs[EXT_glCreateShaderObjectARB];
  TRACE("(%d)\n", shaderType );
  ENTER_GL();
  ret_value = func_glCreateShaderObjectARB( shaderType );
  LEAVE_GL();
  return ret_value;
}

static GLuint WINAPI wine_glCreateShaderProgramEXT( GLenum type, char* string ) {
  GLuint ret_value;
  GLuint (*func_glCreateShaderProgramEXT)( GLenum, char* ) = extension_funcs[EXT_glCreateShaderProgramEXT];
  TRACE("(%d, %p)\n", type, string );
  ENTER_GL();
  ret_value = func_glCreateShaderProgramEXT( type, string );
  LEAVE_GL();
  return ret_value;
}

static GLuint WINAPI wine_glCreateShaderProgramv( GLenum type, GLsizei count, char** strings ) {
  GLuint ret_value;
  GLuint (*func_glCreateShaderProgramv)( GLenum, GLsizei, char** ) = extension_funcs[EXT_glCreateShaderProgramv];
  TRACE("(%d, %d, %p)\n", type, count, strings );
  ENTER_GL();
  ret_value = func_glCreateShaderProgramv( type, count, strings );
  LEAVE_GL();
  return ret_value;
}

static GLvoid* WINAPI wine_glCreateSyncFromCLeventARB( void * context, void * event, GLbitfield flags ) {
  GLvoid* ret_value;
  GLvoid* (*func_glCreateSyncFromCLeventARB)( void *, void *, GLbitfield ) = extension_funcs[EXT_glCreateSyncFromCLeventARB];
  TRACE("(%p, %p, %d)\n", context, event, flags );
  ENTER_GL();
  ret_value = func_glCreateSyncFromCLeventARB( context, event, flags );
  LEAVE_GL();
  return ret_value;
}

static void WINAPI wine_glCullParameterdvEXT( GLenum pname, GLdouble* params ) {
  void (*func_glCullParameterdvEXT)( GLenum, GLdouble* ) = extension_funcs[EXT_glCullParameterdvEXT];
  TRACE("(%d, %p)\n", pname, params );
  ENTER_GL();
  func_glCullParameterdvEXT( pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glCullParameterfvEXT( GLenum pname, GLfloat* params ) {
  void (*func_glCullParameterfvEXT)( GLenum, GLfloat* ) = extension_funcs[EXT_glCullParameterfvEXT];
  TRACE("(%d, %p)\n", pname, params );
  ENTER_GL();
  func_glCullParameterfvEXT( pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glCurrentPaletteMatrixARB( GLint index ) {
  void (*func_glCurrentPaletteMatrixARB)( GLint ) = extension_funcs[EXT_glCurrentPaletteMatrixARB];
  TRACE("(%d)\n", index );
  ENTER_GL();
  func_glCurrentPaletteMatrixARB( index );
  LEAVE_GL();
}

static void WINAPI wine_glDebugMessageCallbackAMD( void * callback, GLvoid* userParam ) {
  void (*func_glDebugMessageCallbackAMD)( void *, GLvoid* ) = extension_funcs[EXT_glDebugMessageCallbackAMD];
  TRACE("(%p, %p)\n", callback, userParam );
  ENTER_GL();
  func_glDebugMessageCallbackAMD( callback, userParam );
  LEAVE_GL();
}

static void WINAPI wine_glDebugMessageCallbackARB( void * callback, GLvoid* userParam ) {
  void (*func_glDebugMessageCallbackARB)( void *, GLvoid* ) = extension_funcs[EXT_glDebugMessageCallbackARB];
  TRACE("(%p, %p)\n", callback, userParam );
  ENTER_GL();
  func_glDebugMessageCallbackARB( callback, userParam );
  LEAVE_GL();
}

static void WINAPI wine_glDebugMessageControlARB( GLenum source, GLenum type, GLenum severity, GLsizei count, GLuint* ids, GLboolean enabled ) {
  void (*func_glDebugMessageControlARB)( GLenum, GLenum, GLenum, GLsizei, GLuint*, GLboolean ) = extension_funcs[EXT_glDebugMessageControlARB];
  TRACE("(%d, %d, %d, %d, %p, %d)\n", source, type, severity, count, ids, enabled );
  ENTER_GL();
  func_glDebugMessageControlARB( source, type, severity, count, ids, enabled );
  LEAVE_GL();
}

static void WINAPI wine_glDebugMessageEnableAMD( GLenum category, GLenum severity, GLsizei count, GLuint* ids, GLboolean enabled ) {
  void (*func_glDebugMessageEnableAMD)( GLenum, GLenum, GLsizei, GLuint*, GLboolean ) = extension_funcs[EXT_glDebugMessageEnableAMD];
  TRACE("(%d, %d, %d, %p, %d)\n", category, severity, count, ids, enabled );
  ENTER_GL();
  func_glDebugMessageEnableAMD( category, severity, count, ids, enabled );
  LEAVE_GL();
}

static void WINAPI wine_glDebugMessageInsertAMD( GLenum category, GLenum severity, GLuint id, GLsizei length, char* buf ) {
  void (*func_glDebugMessageInsertAMD)( GLenum, GLenum, GLuint, GLsizei, char* ) = extension_funcs[EXT_glDebugMessageInsertAMD];
  TRACE("(%d, %d, %d, %d, %p)\n", category, severity, id, length, buf );
  ENTER_GL();
  func_glDebugMessageInsertAMD( category, severity, id, length, buf );
  LEAVE_GL();
}

static void WINAPI wine_glDebugMessageInsertARB( GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, char* buf ) {
  void (*func_glDebugMessageInsertARB)( GLenum, GLenum, GLuint, GLenum, GLsizei, char* ) = extension_funcs[EXT_glDebugMessageInsertARB];
  TRACE("(%d, %d, %d, %d, %d, %p)\n", source, type, id, severity, length, buf );
  ENTER_GL();
  func_glDebugMessageInsertARB( source, type, id, severity, length, buf );
  LEAVE_GL();
}

static void WINAPI wine_glDeformSGIX( GLbitfield mask ) {
  void (*func_glDeformSGIX)( GLbitfield ) = extension_funcs[EXT_glDeformSGIX];
  TRACE("(%d)\n", mask );
  ENTER_GL();
  func_glDeformSGIX( mask );
  LEAVE_GL();
}

static void WINAPI wine_glDeformationMap3dSGIX( GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, GLdouble w1, GLdouble w2, GLint wstride, GLint worder, GLdouble* points ) {
  void (*func_glDeformationMap3dSGIX)( GLenum, GLdouble, GLdouble, GLint, GLint, GLdouble, GLdouble, GLint, GLint, GLdouble, GLdouble, GLint, GLint, GLdouble* ) = extension_funcs[EXT_glDeformationMap3dSGIX];
  TRACE("(%d, %f, %f, %d, %d, %f, %f, %d, %d, %f, %f, %d, %d, %p)\n", target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, w1, w2, wstride, worder, points );
  ENTER_GL();
  func_glDeformationMap3dSGIX( target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, w1, w2, wstride, worder, points );
  LEAVE_GL();
}

static void WINAPI wine_glDeformationMap3fSGIX( GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, GLfloat w1, GLfloat w2, GLint wstride, GLint worder, GLfloat* points ) {
  void (*func_glDeformationMap3fSGIX)( GLenum, GLfloat, GLfloat, GLint, GLint, GLfloat, GLfloat, GLint, GLint, GLfloat, GLfloat, GLint, GLint, GLfloat* ) = extension_funcs[EXT_glDeformationMap3fSGIX];
  TRACE("(%d, %f, %f, %d, %d, %f, %f, %d, %d, %f, %f, %d, %d, %p)\n", target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, w1, w2, wstride, worder, points );
  ENTER_GL();
  func_glDeformationMap3fSGIX( target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, w1, w2, wstride, worder, points );
  LEAVE_GL();
}

static void WINAPI wine_glDeleteAsyncMarkersSGIX( GLuint marker, GLsizei range ) {
  void (*func_glDeleteAsyncMarkersSGIX)( GLuint, GLsizei ) = extension_funcs[EXT_glDeleteAsyncMarkersSGIX];
  TRACE("(%d, %d)\n", marker, range );
  ENTER_GL();
  func_glDeleteAsyncMarkersSGIX( marker, range );
  LEAVE_GL();
}

static void WINAPI wine_glDeleteBufferRegion( GLenum region ) {
  void (*func_glDeleteBufferRegion)( GLenum ) = extension_funcs[EXT_glDeleteBufferRegion];
  TRACE("(%d)\n", region );
  ENTER_GL();
  func_glDeleteBufferRegion( region );
  LEAVE_GL();
}

static void WINAPI wine_glDeleteBuffers( GLsizei n, GLuint* buffers ) {
  void (*func_glDeleteBuffers)( GLsizei, GLuint* ) = extension_funcs[EXT_glDeleteBuffers];
  TRACE("(%d, %p)\n", n, buffers );
  ENTER_GL();
  func_glDeleteBuffers( n, buffers );
  LEAVE_GL();
}

static void WINAPI wine_glDeleteBuffersARB( GLsizei n, GLuint* buffers ) {
  void (*func_glDeleteBuffersARB)( GLsizei, GLuint* ) = extension_funcs[EXT_glDeleteBuffersARB];
  TRACE("(%d, %p)\n", n, buffers );
  ENTER_GL();
  func_glDeleteBuffersARB( n, buffers );
  LEAVE_GL();
}

static void WINAPI wine_glDeleteFencesAPPLE( GLsizei n, GLuint* fences ) {
  void (*func_glDeleteFencesAPPLE)( GLsizei, GLuint* ) = extension_funcs[EXT_glDeleteFencesAPPLE];
  TRACE("(%d, %p)\n", n, fences );
  ENTER_GL();
  func_glDeleteFencesAPPLE( n, fences );
  LEAVE_GL();
}

static void WINAPI wine_glDeleteFencesNV( GLsizei n, GLuint* fences ) {
  void (*func_glDeleteFencesNV)( GLsizei, GLuint* ) = extension_funcs[EXT_glDeleteFencesNV];
  TRACE("(%d, %p)\n", n, fences );
  ENTER_GL();
  func_glDeleteFencesNV( n, fences );
  LEAVE_GL();
}

static void WINAPI wine_glDeleteFragmentShaderATI( GLuint id ) {
  void (*func_glDeleteFragmentShaderATI)( GLuint ) = extension_funcs[EXT_glDeleteFragmentShaderATI];
  TRACE("(%d)\n", id );
  ENTER_GL();
  func_glDeleteFragmentShaderATI( id );
  LEAVE_GL();
}

static void WINAPI wine_glDeleteFramebuffers( GLsizei n, GLuint* framebuffers ) {
  void (*func_glDeleteFramebuffers)( GLsizei, GLuint* ) = extension_funcs[EXT_glDeleteFramebuffers];
  TRACE("(%d, %p)\n", n, framebuffers );
  ENTER_GL();
  func_glDeleteFramebuffers( n, framebuffers );
  LEAVE_GL();
}

static void WINAPI wine_glDeleteFramebuffersEXT( GLsizei n, GLuint* framebuffers ) {
  void (*func_glDeleteFramebuffersEXT)( GLsizei, GLuint* ) = extension_funcs[EXT_glDeleteFramebuffersEXT];
  TRACE("(%d, %p)\n", n, framebuffers );
  ENTER_GL();
  func_glDeleteFramebuffersEXT( n, framebuffers );
  LEAVE_GL();
}

static void WINAPI wine_glDeleteNamedStringARB( GLint namelen, char* name ) {
  void (*func_glDeleteNamedStringARB)( GLint, char* ) = extension_funcs[EXT_glDeleteNamedStringARB];
  TRACE("(%d, %p)\n", namelen, name );
  ENTER_GL();
  func_glDeleteNamedStringARB( namelen, name );
  LEAVE_GL();
}

static void WINAPI wine_glDeleteNamesAMD( GLenum identifier, GLuint num, GLuint* names ) {
  void (*func_glDeleteNamesAMD)( GLenum, GLuint, GLuint* ) = extension_funcs[EXT_glDeleteNamesAMD];
  TRACE("(%d, %d, %p)\n", identifier, num, names );
  ENTER_GL();
  func_glDeleteNamesAMD( identifier, num, names );
  LEAVE_GL();
}

static void WINAPI wine_glDeleteObjectARB( unsigned int obj ) {
  void (*func_glDeleteObjectARB)( unsigned int ) = extension_funcs[EXT_glDeleteObjectARB];
  TRACE("(%d)\n", obj );
  ENTER_GL();
  func_glDeleteObjectARB( obj );
  LEAVE_GL();
}

static void WINAPI wine_glDeleteObjectBufferATI( GLuint buffer ) {
  void (*func_glDeleteObjectBufferATI)( GLuint ) = extension_funcs[EXT_glDeleteObjectBufferATI];
  TRACE("(%d)\n", buffer );
  ENTER_GL();
  func_glDeleteObjectBufferATI( buffer );
  LEAVE_GL();
}

static void WINAPI wine_glDeleteOcclusionQueriesNV( GLsizei n, GLuint* ids ) {
  void (*func_glDeleteOcclusionQueriesNV)( GLsizei, GLuint* ) = extension_funcs[EXT_glDeleteOcclusionQueriesNV];
  TRACE("(%d, %p)\n", n, ids );
  ENTER_GL();
  func_glDeleteOcclusionQueriesNV( n, ids );
  LEAVE_GL();
}

static void WINAPI wine_glDeletePerfMonitorsAMD( GLsizei n, GLuint* monitors ) {
  void (*func_glDeletePerfMonitorsAMD)( GLsizei, GLuint* ) = extension_funcs[EXT_glDeletePerfMonitorsAMD];
  TRACE("(%d, %p)\n", n, monitors );
  ENTER_GL();
  func_glDeletePerfMonitorsAMD( n, monitors );
  LEAVE_GL();
}

static void WINAPI wine_glDeleteProgram( GLuint program ) {
  void (*func_glDeleteProgram)( GLuint ) = extension_funcs[EXT_glDeleteProgram];
  TRACE("(%d)\n", program );
  ENTER_GL();
  func_glDeleteProgram( program );
  LEAVE_GL();
}

static void WINAPI wine_glDeleteProgramPipelines( GLsizei n, GLuint* pipelines ) {
  void (*func_glDeleteProgramPipelines)( GLsizei, GLuint* ) = extension_funcs[EXT_glDeleteProgramPipelines];
  TRACE("(%d, %p)\n", n, pipelines );
  ENTER_GL();
  func_glDeleteProgramPipelines( n, pipelines );
  LEAVE_GL();
}

static void WINAPI wine_glDeleteProgramsARB( GLsizei n, GLuint* programs ) {
  void (*func_glDeleteProgramsARB)( GLsizei, GLuint* ) = extension_funcs[EXT_glDeleteProgramsARB];
  TRACE("(%d, %p)\n", n, programs );
  ENTER_GL();
  func_glDeleteProgramsARB( n, programs );
  LEAVE_GL();
}

static void WINAPI wine_glDeleteProgramsNV( GLsizei n, GLuint* programs ) {
  void (*func_glDeleteProgramsNV)( GLsizei, GLuint* ) = extension_funcs[EXT_glDeleteProgramsNV];
  TRACE("(%d, %p)\n", n, programs );
  ENTER_GL();
  func_glDeleteProgramsNV( n, programs );
  LEAVE_GL();
}

static void WINAPI wine_glDeleteQueries( GLsizei n, GLuint* ids ) {
  void (*func_glDeleteQueries)( GLsizei, GLuint* ) = extension_funcs[EXT_glDeleteQueries];
  TRACE("(%d, %p)\n", n, ids );
  ENTER_GL();
  func_glDeleteQueries( n, ids );
  LEAVE_GL();
}

static void WINAPI wine_glDeleteQueriesARB( GLsizei n, GLuint* ids ) {
  void (*func_glDeleteQueriesARB)( GLsizei, GLuint* ) = extension_funcs[EXT_glDeleteQueriesARB];
  TRACE("(%d, %p)\n", n, ids );
  ENTER_GL();
  func_glDeleteQueriesARB( n, ids );
  LEAVE_GL();
}

static void WINAPI wine_glDeleteRenderbuffers( GLsizei n, GLuint* renderbuffers ) {
  void (*func_glDeleteRenderbuffers)( GLsizei, GLuint* ) = extension_funcs[EXT_glDeleteRenderbuffers];
  TRACE("(%d, %p)\n", n, renderbuffers );
  ENTER_GL();
  func_glDeleteRenderbuffers( n, renderbuffers );
  LEAVE_GL();
}

static void WINAPI wine_glDeleteRenderbuffersEXT( GLsizei n, GLuint* renderbuffers ) {
  void (*func_glDeleteRenderbuffersEXT)( GLsizei, GLuint* ) = extension_funcs[EXT_glDeleteRenderbuffersEXT];
  TRACE("(%d, %p)\n", n, renderbuffers );
  ENTER_GL();
  func_glDeleteRenderbuffersEXT( n, renderbuffers );
  LEAVE_GL();
}

static void WINAPI wine_glDeleteSamplers( GLsizei count, GLuint* samplers ) {
  void (*func_glDeleteSamplers)( GLsizei, GLuint* ) = extension_funcs[EXT_glDeleteSamplers];
  TRACE("(%d, %p)\n", count, samplers );
  ENTER_GL();
  func_glDeleteSamplers( count, samplers );
  LEAVE_GL();
}

static void WINAPI wine_glDeleteShader( GLuint shader ) {
  void (*func_glDeleteShader)( GLuint ) = extension_funcs[EXT_glDeleteShader];
  TRACE("(%d)\n", shader );
  ENTER_GL();
  func_glDeleteShader( shader );
  LEAVE_GL();
}

static void WINAPI wine_glDeleteSync( GLvoid* sync ) {
  void (*func_glDeleteSync)( GLvoid* ) = extension_funcs[EXT_glDeleteSync];
  TRACE("(%p)\n", sync );
  ENTER_GL();
  func_glDeleteSync( sync );
  LEAVE_GL();
}

static void WINAPI wine_glDeleteTexturesEXT( GLsizei n, GLuint* textures ) {
  void (*func_glDeleteTexturesEXT)( GLsizei, GLuint* ) = extension_funcs[EXT_glDeleteTexturesEXT];
  TRACE("(%d, %p)\n", n, textures );
  ENTER_GL();
  func_glDeleteTexturesEXT( n, textures );
  LEAVE_GL();
}

static void WINAPI wine_glDeleteTransformFeedbacks( GLsizei n, GLuint* ids ) {
  void (*func_glDeleteTransformFeedbacks)( GLsizei, GLuint* ) = extension_funcs[EXT_glDeleteTransformFeedbacks];
  TRACE("(%d, %p)\n", n, ids );
  ENTER_GL();
  func_glDeleteTransformFeedbacks( n, ids );
  LEAVE_GL();
}

static void WINAPI wine_glDeleteTransformFeedbacksNV( GLsizei n, GLuint* ids ) {
  void (*func_glDeleteTransformFeedbacksNV)( GLsizei, GLuint* ) = extension_funcs[EXT_glDeleteTransformFeedbacksNV];
  TRACE("(%d, %p)\n", n, ids );
  ENTER_GL();
  func_glDeleteTransformFeedbacksNV( n, ids );
  LEAVE_GL();
}

static void WINAPI wine_glDeleteVertexArrays( GLsizei n, GLuint* arrays ) {
  void (*func_glDeleteVertexArrays)( GLsizei, GLuint* ) = extension_funcs[EXT_glDeleteVertexArrays];
  TRACE("(%d, %p)\n", n, arrays );
  ENTER_GL();
  func_glDeleteVertexArrays( n, arrays );
  LEAVE_GL();
}

static void WINAPI wine_glDeleteVertexArraysAPPLE( GLsizei n, GLuint* arrays ) {
  void (*func_glDeleteVertexArraysAPPLE)( GLsizei, GLuint* ) = extension_funcs[EXT_glDeleteVertexArraysAPPLE];
  TRACE("(%d, %p)\n", n, arrays );
  ENTER_GL();
  func_glDeleteVertexArraysAPPLE( n, arrays );
  LEAVE_GL();
}

static void WINAPI wine_glDeleteVertexShaderEXT( GLuint id ) {
  void (*func_glDeleteVertexShaderEXT)( GLuint ) = extension_funcs[EXT_glDeleteVertexShaderEXT];
  TRACE("(%d)\n", id );
  ENTER_GL();
  func_glDeleteVertexShaderEXT( id );
  LEAVE_GL();
}

static void WINAPI wine_glDepthBoundsEXT( GLclampd zmin, GLclampd zmax ) {
  void (*func_glDepthBoundsEXT)( GLclampd, GLclampd ) = extension_funcs[EXT_glDepthBoundsEXT];
  TRACE("(%f, %f)\n", zmin, zmax );
  ENTER_GL();
  func_glDepthBoundsEXT( zmin, zmax );
  LEAVE_GL();
}

static void WINAPI wine_glDepthBoundsdNV( GLdouble zmin, GLdouble zmax ) {
  void (*func_glDepthBoundsdNV)( GLdouble, GLdouble ) = extension_funcs[EXT_glDepthBoundsdNV];
  TRACE("(%f, %f)\n", zmin, zmax );
  ENTER_GL();
  func_glDepthBoundsdNV( zmin, zmax );
  LEAVE_GL();
}

static void WINAPI wine_glDepthRangeArrayv( GLuint first, GLsizei count, GLclampd* v ) {
  void (*func_glDepthRangeArrayv)( GLuint, GLsizei, GLclampd* ) = extension_funcs[EXT_glDepthRangeArrayv];
  TRACE("(%d, %d, %p)\n", first, count, v );
  ENTER_GL();
  func_glDepthRangeArrayv( first, count, v );
  LEAVE_GL();
}

static void WINAPI wine_glDepthRangeIndexed( GLuint index, GLclampd n, GLclampd f ) {
  void (*func_glDepthRangeIndexed)( GLuint, GLclampd, GLclampd ) = extension_funcs[EXT_glDepthRangeIndexed];
  TRACE("(%d, %f, %f)\n", index, n, f );
  ENTER_GL();
  func_glDepthRangeIndexed( index, n, f );
  LEAVE_GL();
}

static void WINAPI wine_glDepthRangedNV( GLdouble zNear, GLdouble zFar ) {
  void (*func_glDepthRangedNV)( GLdouble, GLdouble ) = extension_funcs[EXT_glDepthRangedNV];
  TRACE("(%f, %f)\n", zNear, zFar );
  ENTER_GL();
  func_glDepthRangedNV( zNear, zFar );
  LEAVE_GL();
}

static void WINAPI wine_glDepthRangef( GLclampf n, GLclampf f ) {
  void (*func_glDepthRangef)( GLclampf, GLclampf ) = extension_funcs[EXT_glDepthRangef];
  TRACE("(%f, %f)\n", n, f );
  ENTER_GL();
  func_glDepthRangef( n, f );
  LEAVE_GL();
}

static void WINAPI wine_glDetachObjectARB( unsigned int containerObj, unsigned int attachedObj ) {
  void (*func_glDetachObjectARB)( unsigned int, unsigned int ) = extension_funcs[EXT_glDetachObjectARB];
  TRACE("(%d, %d)\n", containerObj, attachedObj );
  ENTER_GL();
  func_glDetachObjectARB( containerObj, attachedObj );
  LEAVE_GL();
}

static void WINAPI wine_glDetachShader( GLuint program, GLuint shader ) {
  void (*func_glDetachShader)( GLuint, GLuint ) = extension_funcs[EXT_glDetachShader];
  TRACE("(%d, %d)\n", program, shader );
  ENTER_GL();
  func_glDetachShader( program, shader );
  LEAVE_GL();
}

static void WINAPI wine_glDetailTexFuncSGIS( GLenum target, GLsizei n, GLfloat* points ) {
  void (*func_glDetailTexFuncSGIS)( GLenum, GLsizei, GLfloat* ) = extension_funcs[EXT_glDetailTexFuncSGIS];
  TRACE("(%d, %d, %p)\n", target, n, points );
  ENTER_GL();
  func_glDetailTexFuncSGIS( target, n, points );
  LEAVE_GL();
}

static void WINAPI wine_glDisableClientStateIndexedEXT( GLenum array, GLuint index ) {
  void (*func_glDisableClientStateIndexedEXT)( GLenum, GLuint ) = extension_funcs[EXT_glDisableClientStateIndexedEXT];
  TRACE("(%d, %d)\n", array, index );
  ENTER_GL();
  func_glDisableClientStateIndexedEXT( array, index );
  LEAVE_GL();
}

static void WINAPI wine_glDisableIndexedEXT( GLenum target, GLuint index ) {
  void (*func_glDisableIndexedEXT)( GLenum, GLuint ) = extension_funcs[EXT_glDisableIndexedEXT];
  TRACE("(%d, %d)\n", target, index );
  ENTER_GL();
  func_glDisableIndexedEXT( target, index );
  LEAVE_GL();
}

static void WINAPI wine_glDisableVariantClientStateEXT( GLuint id ) {
  void (*func_glDisableVariantClientStateEXT)( GLuint ) = extension_funcs[EXT_glDisableVariantClientStateEXT];
  TRACE("(%d)\n", id );
  ENTER_GL();
  func_glDisableVariantClientStateEXT( id );
  LEAVE_GL();
}

static void WINAPI wine_glDisableVertexAttribAPPLE( GLuint index, GLenum pname ) {
  void (*func_glDisableVertexAttribAPPLE)( GLuint, GLenum ) = extension_funcs[EXT_glDisableVertexAttribAPPLE];
  TRACE("(%d, %d)\n", index, pname );
  ENTER_GL();
  func_glDisableVertexAttribAPPLE( index, pname );
  LEAVE_GL();
}

static void WINAPI wine_glDisableVertexAttribArray( GLuint index ) {
  void (*func_glDisableVertexAttribArray)( GLuint ) = extension_funcs[EXT_glDisableVertexAttribArray];
  TRACE("(%d)\n", index );
  ENTER_GL();
  func_glDisableVertexAttribArray( index );
  LEAVE_GL();
}

static void WINAPI wine_glDisableVertexAttribArrayARB( GLuint index ) {
  void (*func_glDisableVertexAttribArrayARB)( GLuint ) = extension_funcs[EXT_glDisableVertexAttribArrayARB];
  TRACE("(%d)\n", index );
  ENTER_GL();
  func_glDisableVertexAttribArrayARB( index );
  LEAVE_GL();
}

static void WINAPI wine_glDisablei( GLenum target, GLuint index ) {
  void (*func_glDisablei)( GLenum, GLuint ) = extension_funcs[EXT_glDisablei];
  TRACE("(%d, %d)\n", target, index );
  ENTER_GL();
  func_glDisablei( target, index );
  LEAVE_GL();
}

static void WINAPI wine_glDrawArraysEXT( GLenum mode, GLint first, GLsizei count ) {
  void (*func_glDrawArraysEXT)( GLenum, GLint, GLsizei ) = extension_funcs[EXT_glDrawArraysEXT];
  TRACE("(%d, %d, %d)\n", mode, first, count );
  ENTER_GL();
  func_glDrawArraysEXT( mode, first, count );
  LEAVE_GL();
}

static void WINAPI wine_glDrawArraysIndirect( GLenum mode, GLvoid* indirect ) {
  void (*func_glDrawArraysIndirect)( GLenum, GLvoid* ) = extension_funcs[EXT_glDrawArraysIndirect];
  TRACE("(%d, %p)\n", mode, indirect );
  ENTER_GL();
  func_glDrawArraysIndirect( mode, indirect );
  LEAVE_GL();
}

static void WINAPI wine_glDrawArraysInstanced( GLenum mode, GLint first, GLsizei count, GLsizei primcount ) {
  void (*func_glDrawArraysInstanced)( GLenum, GLint, GLsizei, GLsizei ) = extension_funcs[EXT_glDrawArraysInstanced];
  TRACE("(%d, %d, %d, %d)\n", mode, first, count, primcount );
  ENTER_GL();
  func_glDrawArraysInstanced( mode, first, count, primcount );
  LEAVE_GL();
}

static void WINAPI wine_glDrawArraysInstancedARB( GLenum mode, GLint first, GLsizei count, GLsizei primcount ) {
  void (*func_glDrawArraysInstancedARB)( GLenum, GLint, GLsizei, GLsizei ) = extension_funcs[EXT_glDrawArraysInstancedARB];
  TRACE("(%d, %d, %d, %d)\n", mode, first, count, primcount );
  ENTER_GL();
  func_glDrawArraysInstancedARB( mode, first, count, primcount );
  LEAVE_GL();
}

static void WINAPI wine_glDrawArraysInstancedEXT( GLenum mode, GLint start, GLsizei count, GLsizei primcount ) {
  void (*func_glDrawArraysInstancedEXT)( GLenum, GLint, GLsizei, GLsizei ) = extension_funcs[EXT_glDrawArraysInstancedEXT];
  TRACE("(%d, %d, %d, %d)\n", mode, start, count, primcount );
  ENTER_GL();
  func_glDrawArraysInstancedEXT( mode, start, count, primcount );
  LEAVE_GL();
}

static void WINAPI wine_glDrawBufferRegion( GLenum region, GLint x, GLint y, GLsizei width, GLsizei height, GLint xDest, GLint yDest ) {
  void (*func_glDrawBufferRegion)( GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLint ) = extension_funcs[EXT_glDrawBufferRegion];
  TRACE("(%d, %d, %d, %d, %d, %d, %d)\n", region, x, y, width, height, xDest, yDest );
  ENTER_GL();
  func_glDrawBufferRegion( region, x, y, width, height, xDest, yDest );
  LEAVE_GL();
}

static void WINAPI wine_glDrawBuffers( GLsizei n, GLenum* bufs ) {
  void (*func_glDrawBuffers)( GLsizei, GLenum* ) = extension_funcs[EXT_glDrawBuffers];
  TRACE("(%d, %p)\n", n, bufs );
  ENTER_GL();
  func_glDrawBuffers( n, bufs );
  LEAVE_GL();
}

static void WINAPI wine_glDrawBuffersARB( GLsizei n, GLenum* bufs ) {
  void (*func_glDrawBuffersARB)( GLsizei, GLenum* ) = extension_funcs[EXT_glDrawBuffersARB];
  TRACE("(%d, %p)\n", n, bufs );
  ENTER_GL();
  func_glDrawBuffersARB( n, bufs );
  LEAVE_GL();
}

static void WINAPI wine_glDrawBuffersATI( GLsizei n, GLenum* bufs ) {
  void (*func_glDrawBuffersATI)( GLsizei, GLenum* ) = extension_funcs[EXT_glDrawBuffersATI];
  TRACE("(%d, %p)\n", n, bufs );
  ENTER_GL();
  func_glDrawBuffersATI( n, bufs );
  LEAVE_GL();
}

static void WINAPI wine_glDrawElementArrayAPPLE( GLenum mode, GLint first, GLsizei count ) {
  void (*func_glDrawElementArrayAPPLE)( GLenum, GLint, GLsizei ) = extension_funcs[EXT_glDrawElementArrayAPPLE];
  TRACE("(%d, %d, %d)\n", mode, first, count );
  ENTER_GL();
  func_glDrawElementArrayAPPLE( mode, first, count );
  LEAVE_GL();
}

static void WINAPI wine_glDrawElementArrayATI( GLenum mode, GLsizei count ) {
  void (*func_glDrawElementArrayATI)( GLenum, GLsizei ) = extension_funcs[EXT_glDrawElementArrayATI];
  TRACE("(%d, %d)\n", mode, count );
  ENTER_GL();
  func_glDrawElementArrayATI( mode, count );
  LEAVE_GL();
}

static void WINAPI wine_glDrawElementsBaseVertex( GLenum mode, GLsizei count, GLenum type, GLvoid* indices, GLint basevertex ) {
  void (*func_glDrawElementsBaseVertex)( GLenum, GLsizei, GLenum, GLvoid*, GLint ) = extension_funcs[EXT_glDrawElementsBaseVertex];
  TRACE("(%d, %d, %d, %p, %d)\n", mode, count, type, indices, basevertex );
  ENTER_GL();
  func_glDrawElementsBaseVertex( mode, count, type, indices, basevertex );
  LEAVE_GL();
}

static void WINAPI wine_glDrawElementsIndirect( GLenum mode, GLenum type, GLvoid* indirect ) {
  void (*func_glDrawElementsIndirect)( GLenum, GLenum, GLvoid* ) = extension_funcs[EXT_glDrawElementsIndirect];
  TRACE("(%d, %d, %p)\n", mode, type, indirect );
  ENTER_GL();
  func_glDrawElementsIndirect( mode, type, indirect );
  LEAVE_GL();
}

static void WINAPI wine_glDrawElementsInstanced( GLenum mode, GLsizei count, GLenum type, GLvoid* indices, GLsizei primcount ) {
  void (*func_glDrawElementsInstanced)( GLenum, GLsizei, GLenum, GLvoid*, GLsizei ) = extension_funcs[EXT_glDrawElementsInstanced];
  TRACE("(%d, %d, %d, %p, %d)\n", mode, count, type, indices, primcount );
  ENTER_GL();
  func_glDrawElementsInstanced( mode, count, type, indices, primcount );
  LEAVE_GL();
}

static void WINAPI wine_glDrawElementsInstancedARB( GLenum mode, GLsizei count, GLenum type, GLvoid* indices, GLsizei primcount ) {
  void (*func_glDrawElementsInstancedARB)( GLenum, GLsizei, GLenum, GLvoid*, GLsizei ) = extension_funcs[EXT_glDrawElementsInstancedARB];
  TRACE("(%d, %d, %d, %p, %d)\n", mode, count, type, indices, primcount );
  ENTER_GL();
  func_glDrawElementsInstancedARB( mode, count, type, indices, primcount );
  LEAVE_GL();
}

static void WINAPI wine_glDrawElementsInstancedBaseVertex( GLenum mode, GLsizei count, GLenum type, GLvoid* indices, GLsizei primcount, GLint basevertex ) {
  void (*func_glDrawElementsInstancedBaseVertex)( GLenum, GLsizei, GLenum, GLvoid*, GLsizei, GLint ) = extension_funcs[EXT_glDrawElementsInstancedBaseVertex];
  TRACE("(%d, %d, %d, %p, %d, %d)\n", mode, count, type, indices, primcount, basevertex );
  ENTER_GL();
  func_glDrawElementsInstancedBaseVertex( mode, count, type, indices, primcount, basevertex );
  LEAVE_GL();
}

static void WINAPI wine_glDrawElementsInstancedEXT( GLenum mode, GLsizei count, GLenum type, GLvoid* indices, GLsizei primcount ) {
  void (*func_glDrawElementsInstancedEXT)( GLenum, GLsizei, GLenum, GLvoid*, GLsizei ) = extension_funcs[EXT_glDrawElementsInstancedEXT];
  TRACE("(%d, %d, %d, %p, %d)\n", mode, count, type, indices, primcount );
  ENTER_GL();
  func_glDrawElementsInstancedEXT( mode, count, type, indices, primcount );
  LEAVE_GL();
}

static void WINAPI wine_glDrawMeshArraysSUN( GLenum mode, GLint first, GLsizei count, GLsizei width ) {
  void (*func_glDrawMeshArraysSUN)( GLenum, GLint, GLsizei, GLsizei ) = extension_funcs[EXT_glDrawMeshArraysSUN];
  TRACE("(%d, %d, %d, %d)\n", mode, first, count, width );
  ENTER_GL();
  func_glDrawMeshArraysSUN( mode, first, count, width );
  LEAVE_GL();
}

static void WINAPI wine_glDrawRangeElementArrayAPPLE( GLenum mode, GLuint start, GLuint end, GLint first, GLsizei count ) {
  void (*func_glDrawRangeElementArrayAPPLE)( GLenum, GLuint, GLuint, GLint, GLsizei ) = extension_funcs[EXT_glDrawRangeElementArrayAPPLE];
  TRACE("(%d, %d, %d, %d, %d)\n", mode, start, end, first, count );
  ENTER_GL();
  func_glDrawRangeElementArrayAPPLE( mode, start, end, first, count );
  LEAVE_GL();
}

static void WINAPI wine_glDrawRangeElementArrayATI( GLenum mode, GLuint start, GLuint end, GLsizei count ) {
  void (*func_glDrawRangeElementArrayATI)( GLenum, GLuint, GLuint, GLsizei ) = extension_funcs[EXT_glDrawRangeElementArrayATI];
  TRACE("(%d, %d, %d, %d)\n", mode, start, end, count );
  ENTER_GL();
  func_glDrawRangeElementArrayATI( mode, start, end, count );
  LEAVE_GL();
}

static void WINAPI wine_glDrawRangeElements( GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, GLvoid* indices ) {
  void (*func_glDrawRangeElements)( GLenum, GLuint, GLuint, GLsizei, GLenum, GLvoid* ) = extension_funcs[EXT_glDrawRangeElements];
  TRACE("(%d, %d, %d, %d, %d, %p)\n", mode, start, end, count, type, indices );
  ENTER_GL();
  func_glDrawRangeElements( mode, start, end, count, type, indices );
  LEAVE_GL();
}

static void WINAPI wine_glDrawRangeElementsBaseVertex( GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, GLvoid* indices, GLint basevertex ) {
  void (*func_glDrawRangeElementsBaseVertex)( GLenum, GLuint, GLuint, GLsizei, GLenum, GLvoid*, GLint ) = extension_funcs[EXT_glDrawRangeElementsBaseVertex];
  TRACE("(%d, %d, %d, %d, %d, %p, %d)\n", mode, start, end, count, type, indices, basevertex );
  ENTER_GL();
  func_glDrawRangeElementsBaseVertex( mode, start, end, count, type, indices, basevertex );
  LEAVE_GL();
}

static void WINAPI wine_glDrawRangeElementsEXT( GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, GLvoid* indices ) {
  void (*func_glDrawRangeElementsEXT)( GLenum, GLuint, GLuint, GLsizei, GLenum, GLvoid* ) = extension_funcs[EXT_glDrawRangeElementsEXT];
  TRACE("(%d, %d, %d, %d, %d, %p)\n", mode, start, end, count, type, indices );
  ENTER_GL();
  func_glDrawRangeElementsEXT( mode, start, end, count, type, indices );
  LEAVE_GL();
}

static void WINAPI wine_glDrawTransformFeedback( GLenum mode, GLuint id ) {
  void (*func_glDrawTransformFeedback)( GLenum, GLuint ) = extension_funcs[EXT_glDrawTransformFeedback];
  TRACE("(%d, %d)\n", mode, id );
  ENTER_GL();
  func_glDrawTransformFeedback( mode, id );
  LEAVE_GL();
}

static void WINAPI wine_glDrawTransformFeedbackNV( GLenum mode, GLuint id ) {
  void (*func_glDrawTransformFeedbackNV)( GLenum, GLuint ) = extension_funcs[EXT_glDrawTransformFeedbackNV];
  TRACE("(%d, %d)\n", mode, id );
  ENTER_GL();
  func_glDrawTransformFeedbackNV( mode, id );
  LEAVE_GL();
}

static void WINAPI wine_glDrawTransformFeedbackStream( GLenum mode, GLuint id, GLuint stream ) {
  void (*func_glDrawTransformFeedbackStream)( GLenum, GLuint, GLuint ) = extension_funcs[EXT_glDrawTransformFeedbackStream];
  TRACE("(%d, %d, %d)\n", mode, id, stream );
  ENTER_GL();
  func_glDrawTransformFeedbackStream( mode, id, stream );
  LEAVE_GL();
}

static void WINAPI wine_glEdgeFlagFormatNV( GLsizei stride ) {
  void (*func_glEdgeFlagFormatNV)( GLsizei ) = extension_funcs[EXT_glEdgeFlagFormatNV];
  TRACE("(%d)\n", stride );
  ENTER_GL();
  func_glEdgeFlagFormatNV( stride );
  LEAVE_GL();
}

static void WINAPI wine_glEdgeFlagPointerEXT( GLsizei stride, GLsizei count, GLboolean* pointer ) {
  void (*func_glEdgeFlagPointerEXT)( GLsizei, GLsizei, GLboolean* ) = extension_funcs[EXT_glEdgeFlagPointerEXT];
  TRACE("(%d, %d, %p)\n", stride, count, pointer );
  ENTER_GL();
  func_glEdgeFlagPointerEXT( stride, count, pointer );
  LEAVE_GL();
}

static void WINAPI wine_glEdgeFlagPointerListIBM( GLint stride, GLboolean** pointer, GLint ptrstride ) {
  void (*func_glEdgeFlagPointerListIBM)( GLint, GLboolean**, GLint ) = extension_funcs[EXT_glEdgeFlagPointerListIBM];
  TRACE("(%d, %p, %d)\n", stride, pointer, ptrstride );
  ENTER_GL();
  func_glEdgeFlagPointerListIBM( stride, pointer, ptrstride );
  LEAVE_GL();
}

static void WINAPI wine_glElementPointerAPPLE( GLenum type, GLvoid* pointer ) {
  void (*func_glElementPointerAPPLE)( GLenum, GLvoid* ) = extension_funcs[EXT_glElementPointerAPPLE];
  TRACE("(%d, %p)\n", type, pointer );
  ENTER_GL();
  func_glElementPointerAPPLE( type, pointer );
  LEAVE_GL();
}

static void WINAPI wine_glElementPointerATI( GLenum type, GLvoid* pointer ) {
  void (*func_glElementPointerATI)( GLenum, GLvoid* ) = extension_funcs[EXT_glElementPointerATI];
  TRACE("(%d, %p)\n", type, pointer );
  ENTER_GL();
  func_glElementPointerATI( type, pointer );
  LEAVE_GL();
}

static void WINAPI wine_glEnableClientStateIndexedEXT( GLenum array, GLuint index ) {
  void (*func_glEnableClientStateIndexedEXT)( GLenum, GLuint ) = extension_funcs[EXT_glEnableClientStateIndexedEXT];
  TRACE("(%d, %d)\n", array, index );
  ENTER_GL();
  func_glEnableClientStateIndexedEXT( array, index );
  LEAVE_GL();
}

static void WINAPI wine_glEnableIndexedEXT( GLenum target, GLuint index ) {
  void (*func_glEnableIndexedEXT)( GLenum, GLuint ) = extension_funcs[EXT_glEnableIndexedEXT];
  TRACE("(%d, %d)\n", target, index );
  ENTER_GL();
  func_glEnableIndexedEXT( target, index );
  LEAVE_GL();
}

static void WINAPI wine_glEnableVariantClientStateEXT( GLuint id ) {
  void (*func_glEnableVariantClientStateEXT)( GLuint ) = extension_funcs[EXT_glEnableVariantClientStateEXT];
  TRACE("(%d)\n", id );
  ENTER_GL();
  func_glEnableVariantClientStateEXT( id );
  LEAVE_GL();
}

static void WINAPI wine_glEnableVertexAttribAPPLE( GLuint index, GLenum pname ) {
  void (*func_glEnableVertexAttribAPPLE)( GLuint, GLenum ) = extension_funcs[EXT_glEnableVertexAttribAPPLE];
  TRACE("(%d, %d)\n", index, pname );
  ENTER_GL();
  func_glEnableVertexAttribAPPLE( index, pname );
  LEAVE_GL();
}

static void WINAPI wine_glEnableVertexAttribArray( GLuint index ) {
  void (*func_glEnableVertexAttribArray)( GLuint ) = extension_funcs[EXT_glEnableVertexAttribArray];
  TRACE("(%d)\n", index );
  ENTER_GL();
  func_glEnableVertexAttribArray( index );
  LEAVE_GL();
}

static void WINAPI wine_glEnableVertexAttribArrayARB( GLuint index ) {
  void (*func_glEnableVertexAttribArrayARB)( GLuint ) = extension_funcs[EXT_glEnableVertexAttribArrayARB];
  TRACE("(%d)\n", index );
  ENTER_GL();
  func_glEnableVertexAttribArrayARB( index );
  LEAVE_GL();
}

static void WINAPI wine_glEnablei( GLenum target, GLuint index ) {
  void (*func_glEnablei)( GLenum, GLuint ) = extension_funcs[EXT_glEnablei];
  TRACE("(%d, %d)\n", target, index );
  ENTER_GL();
  func_glEnablei( target, index );
  LEAVE_GL();
}

static void WINAPI wine_glEndConditionalRender( void ) {
  void (*func_glEndConditionalRender)( void ) = extension_funcs[EXT_glEndConditionalRender];
  TRACE("()\n");
  ENTER_GL();
  func_glEndConditionalRender( );
  LEAVE_GL();
}

static void WINAPI wine_glEndConditionalRenderNV( void ) {
  void (*func_glEndConditionalRenderNV)( void ) = extension_funcs[EXT_glEndConditionalRenderNV];
  TRACE("()\n");
  ENTER_GL();
  func_glEndConditionalRenderNV( );
  LEAVE_GL();
}

static void WINAPI wine_glEndFragmentShaderATI( void ) {
  void (*func_glEndFragmentShaderATI)( void ) = extension_funcs[EXT_glEndFragmentShaderATI];
  TRACE("()\n");
  ENTER_GL();
  func_glEndFragmentShaderATI( );
  LEAVE_GL();
}

static void WINAPI wine_glEndOcclusionQueryNV( void ) {
  void (*func_glEndOcclusionQueryNV)( void ) = extension_funcs[EXT_glEndOcclusionQueryNV];
  TRACE("()\n");
  ENTER_GL();
  func_glEndOcclusionQueryNV( );
  LEAVE_GL();
}

static void WINAPI wine_glEndPerfMonitorAMD( GLuint monitor ) {
  void (*func_glEndPerfMonitorAMD)( GLuint ) = extension_funcs[EXT_glEndPerfMonitorAMD];
  TRACE("(%d)\n", monitor );
  ENTER_GL();
  func_glEndPerfMonitorAMD( monitor );
  LEAVE_GL();
}

static void WINAPI wine_glEndQuery( GLenum target ) {
  void (*func_glEndQuery)( GLenum ) = extension_funcs[EXT_glEndQuery];
  TRACE("(%d)\n", target );
  ENTER_GL();
  func_glEndQuery( target );
  LEAVE_GL();
}

static void WINAPI wine_glEndQueryARB( GLenum target ) {
  void (*func_glEndQueryARB)( GLenum ) = extension_funcs[EXT_glEndQueryARB];
  TRACE("(%d)\n", target );
  ENTER_GL();
  func_glEndQueryARB( target );
  LEAVE_GL();
}

static void WINAPI wine_glEndQueryIndexed( GLenum target, GLuint index ) {
  void (*func_glEndQueryIndexed)( GLenum, GLuint ) = extension_funcs[EXT_glEndQueryIndexed];
  TRACE("(%d, %d)\n", target, index );
  ENTER_GL();
  func_glEndQueryIndexed( target, index );
  LEAVE_GL();
}

static void WINAPI wine_glEndTransformFeedback( void ) {
  void (*func_glEndTransformFeedback)( void ) = extension_funcs[EXT_glEndTransformFeedback];
  TRACE("()\n");
  ENTER_GL();
  func_glEndTransformFeedback( );
  LEAVE_GL();
}

static void WINAPI wine_glEndTransformFeedbackEXT( void ) {
  void (*func_glEndTransformFeedbackEXT)( void ) = extension_funcs[EXT_glEndTransformFeedbackEXT];
  TRACE("()\n");
  ENTER_GL();
  func_glEndTransformFeedbackEXT( );
  LEAVE_GL();
}

static void WINAPI wine_glEndTransformFeedbackNV( void ) {
  void (*func_glEndTransformFeedbackNV)( void ) = extension_funcs[EXT_glEndTransformFeedbackNV];
  TRACE("()\n");
  ENTER_GL();
  func_glEndTransformFeedbackNV( );
  LEAVE_GL();
}

static void WINAPI wine_glEndVertexShaderEXT( void ) {
  void (*func_glEndVertexShaderEXT)( void ) = extension_funcs[EXT_glEndVertexShaderEXT];
  TRACE("()\n");
  ENTER_GL();
  func_glEndVertexShaderEXT( );
  LEAVE_GL();
}

static void WINAPI wine_glEndVideoCaptureNV( GLuint video_capture_slot ) {
  void (*func_glEndVideoCaptureNV)( GLuint ) = extension_funcs[EXT_glEndVideoCaptureNV];
  TRACE("(%d)\n", video_capture_slot );
  ENTER_GL();
  func_glEndVideoCaptureNV( video_capture_slot );
  LEAVE_GL();
}

static void WINAPI wine_glEvalMapsNV( GLenum target, GLenum mode ) {
  void (*func_glEvalMapsNV)( GLenum, GLenum ) = extension_funcs[EXT_glEvalMapsNV];
  TRACE("(%d, %d)\n", target, mode );
  ENTER_GL();
  func_glEvalMapsNV( target, mode );
  LEAVE_GL();
}

static void WINAPI wine_glExecuteProgramNV( GLenum target, GLuint id, GLfloat* params ) {
  void (*func_glExecuteProgramNV)( GLenum, GLuint, GLfloat* ) = extension_funcs[EXT_glExecuteProgramNV];
  TRACE("(%d, %d, %p)\n", target, id, params );
  ENTER_GL();
  func_glExecuteProgramNV( target, id, params );
  LEAVE_GL();
}

static void WINAPI wine_glExtractComponentEXT( GLuint res, GLuint src, GLuint num ) {
  void (*func_glExtractComponentEXT)( GLuint, GLuint, GLuint ) = extension_funcs[EXT_glExtractComponentEXT];
  TRACE("(%d, %d, %d)\n", res, src, num );
  ENTER_GL();
  func_glExtractComponentEXT( res, src, num );
  LEAVE_GL();
}

static GLvoid* WINAPI wine_glFenceSync( GLenum condition, GLbitfield flags ) {
  GLvoid* ret_value;
  GLvoid* (*func_glFenceSync)( GLenum, GLbitfield ) = extension_funcs[EXT_glFenceSync];
  TRACE("(%d, %d)\n", condition, flags );
  ENTER_GL();
  ret_value = func_glFenceSync( condition, flags );
  LEAVE_GL();
  return ret_value;
}

static void WINAPI wine_glFinalCombinerInputNV( GLenum variable, GLenum input, GLenum mapping, GLenum componentUsage ) {
  void (*func_glFinalCombinerInputNV)( GLenum, GLenum, GLenum, GLenum ) = extension_funcs[EXT_glFinalCombinerInputNV];
  TRACE("(%d, %d, %d, %d)\n", variable, input, mapping, componentUsage );
  ENTER_GL();
  func_glFinalCombinerInputNV( variable, input, mapping, componentUsage );
  LEAVE_GL();
}

static GLint WINAPI wine_glFinishAsyncSGIX( GLuint* markerp ) {
  GLint ret_value;
  GLint (*func_glFinishAsyncSGIX)( GLuint* ) = extension_funcs[EXT_glFinishAsyncSGIX];
  TRACE("(%p)\n", markerp );
  ENTER_GL();
  ret_value = func_glFinishAsyncSGIX( markerp );
  LEAVE_GL();
  return ret_value;
}

static void WINAPI wine_glFinishFenceAPPLE( GLuint fence ) {
  void (*func_glFinishFenceAPPLE)( GLuint ) = extension_funcs[EXT_glFinishFenceAPPLE];
  TRACE("(%d)\n", fence );
  ENTER_GL();
  func_glFinishFenceAPPLE( fence );
  LEAVE_GL();
}

static void WINAPI wine_glFinishFenceNV( GLuint fence ) {
  void (*func_glFinishFenceNV)( GLuint ) = extension_funcs[EXT_glFinishFenceNV];
  TRACE("(%d)\n", fence );
  ENTER_GL();
  func_glFinishFenceNV( fence );
  LEAVE_GL();
}

static void WINAPI wine_glFinishObjectAPPLE( GLenum object, GLint name ) {
  void (*func_glFinishObjectAPPLE)( GLenum, GLint ) = extension_funcs[EXT_glFinishObjectAPPLE];
  TRACE("(%d, %d)\n", object, name );
  ENTER_GL();
  func_glFinishObjectAPPLE( object, name );
  LEAVE_GL();
}

static void WINAPI wine_glFinishTextureSUNX( void ) {
  void (*func_glFinishTextureSUNX)( void ) = extension_funcs[EXT_glFinishTextureSUNX];
  TRACE("()\n");
  ENTER_GL();
  func_glFinishTextureSUNX( );
  LEAVE_GL();
}

static void WINAPI wine_glFlushMappedBufferRange( GLenum target, INT_PTR offset, INT_PTR length ) {
  void (*func_glFlushMappedBufferRange)( GLenum, INT_PTR, INT_PTR ) = extension_funcs[EXT_glFlushMappedBufferRange];
  TRACE("(%d, %ld, %ld)\n", target, offset, length );
  ENTER_GL();
  func_glFlushMappedBufferRange( target, offset, length );
  LEAVE_GL();
}

static void WINAPI wine_glFlushMappedBufferRangeAPPLE( GLenum target, INT_PTR offset, INT_PTR size ) {
  void (*func_glFlushMappedBufferRangeAPPLE)( GLenum, INT_PTR, INT_PTR ) = extension_funcs[EXT_glFlushMappedBufferRangeAPPLE];
  TRACE("(%d, %ld, %ld)\n", target, offset, size );
  ENTER_GL();
  func_glFlushMappedBufferRangeAPPLE( target, offset, size );
  LEAVE_GL();
}

static void WINAPI wine_glFlushMappedNamedBufferRangeEXT( GLuint buffer, INT_PTR offset, INT_PTR length ) {
  void (*func_glFlushMappedNamedBufferRangeEXT)( GLuint, INT_PTR, INT_PTR ) = extension_funcs[EXT_glFlushMappedNamedBufferRangeEXT];
  TRACE("(%d, %ld, %ld)\n", buffer, offset, length );
  ENTER_GL();
  func_glFlushMappedNamedBufferRangeEXT( buffer, offset, length );
  LEAVE_GL();
}

static void WINAPI wine_glFlushPixelDataRangeNV( GLenum target ) {
  void (*func_glFlushPixelDataRangeNV)( GLenum ) = extension_funcs[EXT_glFlushPixelDataRangeNV];
  TRACE("(%d)\n", target );
  ENTER_GL();
  func_glFlushPixelDataRangeNV( target );
  LEAVE_GL();
}

static void WINAPI wine_glFlushRasterSGIX( void ) {
  void (*func_glFlushRasterSGIX)( void ) = extension_funcs[EXT_glFlushRasterSGIX];
  TRACE("()\n");
  ENTER_GL();
  func_glFlushRasterSGIX( );
  LEAVE_GL();
}

static void WINAPI wine_glFlushVertexArrayRangeAPPLE( GLsizei length, GLvoid* pointer ) {
  void (*func_glFlushVertexArrayRangeAPPLE)( GLsizei, GLvoid* ) = extension_funcs[EXT_glFlushVertexArrayRangeAPPLE];
  TRACE("(%d, %p)\n", length, pointer );
  ENTER_GL();
  func_glFlushVertexArrayRangeAPPLE( length, pointer );
  LEAVE_GL();
}

static void WINAPI wine_glFlushVertexArrayRangeNV( void ) {
  void (*func_glFlushVertexArrayRangeNV)( void ) = extension_funcs[EXT_glFlushVertexArrayRangeNV];
  TRACE("()\n");
  ENTER_GL();
  func_glFlushVertexArrayRangeNV( );
  LEAVE_GL();
}

static void WINAPI wine_glFogCoordFormatNV( GLenum type, GLsizei stride ) {
  void (*func_glFogCoordFormatNV)( GLenum, GLsizei ) = extension_funcs[EXT_glFogCoordFormatNV];
  TRACE("(%d, %d)\n", type, stride );
  ENTER_GL();
  func_glFogCoordFormatNV( type, stride );
  LEAVE_GL();
}

static void WINAPI wine_glFogCoordPointer( GLenum type, GLsizei stride, GLvoid* pointer ) {
  void (*func_glFogCoordPointer)( GLenum, GLsizei, GLvoid* ) = extension_funcs[EXT_glFogCoordPointer];
  TRACE("(%d, %d, %p)\n", type, stride, pointer );
  ENTER_GL();
  func_glFogCoordPointer( type, stride, pointer );
  LEAVE_GL();
}

static void WINAPI wine_glFogCoordPointerEXT( GLenum type, GLsizei stride, GLvoid* pointer ) {
  void (*func_glFogCoordPointerEXT)( GLenum, GLsizei, GLvoid* ) = extension_funcs[EXT_glFogCoordPointerEXT];
  TRACE("(%d, %d, %p)\n", type, stride, pointer );
  ENTER_GL();
  func_glFogCoordPointerEXT( type, stride, pointer );
  LEAVE_GL();
}

static void WINAPI wine_glFogCoordPointerListIBM( GLenum type, GLint stride, GLvoid** pointer, GLint ptrstride ) {
  void (*func_glFogCoordPointerListIBM)( GLenum, GLint, GLvoid**, GLint ) = extension_funcs[EXT_glFogCoordPointerListIBM];
  TRACE("(%d, %d, %p, %d)\n", type, stride, pointer, ptrstride );
  ENTER_GL();
  func_glFogCoordPointerListIBM( type, stride, pointer, ptrstride );
  LEAVE_GL();
}

static void WINAPI wine_glFogCoordd( GLdouble coord ) {
  void (*func_glFogCoordd)( GLdouble ) = extension_funcs[EXT_glFogCoordd];
  TRACE("(%f)\n", coord );
  ENTER_GL();
  func_glFogCoordd( coord );
  LEAVE_GL();
}

static void WINAPI wine_glFogCoorddEXT( GLdouble coord ) {
  void (*func_glFogCoorddEXT)( GLdouble ) = extension_funcs[EXT_glFogCoorddEXT];
  TRACE("(%f)\n", coord );
  ENTER_GL();
  func_glFogCoorddEXT( coord );
  LEAVE_GL();
}

static void WINAPI wine_glFogCoorddv( GLdouble* coord ) {
  void (*func_glFogCoorddv)( GLdouble* ) = extension_funcs[EXT_glFogCoorddv];
  TRACE("(%p)\n", coord );
  ENTER_GL();
  func_glFogCoorddv( coord );
  LEAVE_GL();
}

static void WINAPI wine_glFogCoorddvEXT( GLdouble* coord ) {
  void (*func_glFogCoorddvEXT)( GLdouble* ) = extension_funcs[EXT_glFogCoorddvEXT];
  TRACE("(%p)\n", coord );
  ENTER_GL();
  func_glFogCoorddvEXT( coord );
  LEAVE_GL();
}

static void WINAPI wine_glFogCoordf( GLfloat coord ) {
  void (*func_glFogCoordf)( GLfloat ) = extension_funcs[EXT_glFogCoordf];
  TRACE("(%f)\n", coord );
  ENTER_GL();
  func_glFogCoordf( coord );
  LEAVE_GL();
}

static void WINAPI wine_glFogCoordfEXT( GLfloat coord ) {
  void (*func_glFogCoordfEXT)( GLfloat ) = extension_funcs[EXT_glFogCoordfEXT];
  TRACE("(%f)\n", coord );
  ENTER_GL();
  func_glFogCoordfEXT( coord );
  LEAVE_GL();
}

static void WINAPI wine_glFogCoordfv( GLfloat* coord ) {
  void (*func_glFogCoordfv)( GLfloat* ) = extension_funcs[EXT_glFogCoordfv];
  TRACE("(%p)\n", coord );
  ENTER_GL();
  func_glFogCoordfv( coord );
  LEAVE_GL();
}

static void WINAPI wine_glFogCoordfvEXT( GLfloat* coord ) {
  void (*func_glFogCoordfvEXT)( GLfloat* ) = extension_funcs[EXT_glFogCoordfvEXT];
  TRACE("(%p)\n", coord );
  ENTER_GL();
  func_glFogCoordfvEXT( coord );
  LEAVE_GL();
}

static void WINAPI wine_glFogCoordhNV( unsigned short fog ) {
  void (*func_glFogCoordhNV)( unsigned short ) = extension_funcs[EXT_glFogCoordhNV];
  TRACE("(%d)\n", fog );
  ENTER_GL();
  func_glFogCoordhNV( fog );
  LEAVE_GL();
}

static void WINAPI wine_glFogCoordhvNV( unsigned short* fog ) {
  void (*func_glFogCoordhvNV)( unsigned short* ) = extension_funcs[EXT_glFogCoordhvNV];
  TRACE("(%p)\n", fog );
  ENTER_GL();
  func_glFogCoordhvNV( fog );
  LEAVE_GL();
}

static void WINAPI wine_glFogFuncSGIS( GLsizei n, GLfloat* points ) {
  void (*func_glFogFuncSGIS)( GLsizei, GLfloat* ) = extension_funcs[EXT_glFogFuncSGIS];
  TRACE("(%d, %p)\n", n, points );
  ENTER_GL();
  func_glFogFuncSGIS( n, points );
  LEAVE_GL();
}

static void WINAPI wine_glFragmentColorMaterialSGIX( GLenum face, GLenum mode ) {
  void (*func_glFragmentColorMaterialSGIX)( GLenum, GLenum ) = extension_funcs[EXT_glFragmentColorMaterialSGIX];
  TRACE("(%d, %d)\n", face, mode );
  ENTER_GL();
  func_glFragmentColorMaterialSGIX( face, mode );
  LEAVE_GL();
}

static void WINAPI wine_glFragmentLightModelfSGIX( GLenum pname, GLfloat param ) {
  void (*func_glFragmentLightModelfSGIX)( GLenum, GLfloat ) = extension_funcs[EXT_glFragmentLightModelfSGIX];
  TRACE("(%d, %f)\n", pname, param );
  ENTER_GL();
  func_glFragmentLightModelfSGIX( pname, param );
  LEAVE_GL();
}

static void WINAPI wine_glFragmentLightModelfvSGIX( GLenum pname, GLfloat* params ) {
  void (*func_glFragmentLightModelfvSGIX)( GLenum, GLfloat* ) = extension_funcs[EXT_glFragmentLightModelfvSGIX];
  TRACE("(%d, %p)\n", pname, params );
  ENTER_GL();
  func_glFragmentLightModelfvSGIX( pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glFragmentLightModeliSGIX( GLenum pname, GLint param ) {
  void (*func_glFragmentLightModeliSGIX)( GLenum, GLint ) = extension_funcs[EXT_glFragmentLightModeliSGIX];
  TRACE("(%d, %d)\n", pname, param );
  ENTER_GL();
  func_glFragmentLightModeliSGIX( pname, param );
  LEAVE_GL();
}

static void WINAPI wine_glFragmentLightModelivSGIX( GLenum pname, GLint* params ) {
  void (*func_glFragmentLightModelivSGIX)( GLenum, GLint* ) = extension_funcs[EXT_glFragmentLightModelivSGIX];
  TRACE("(%d, %p)\n", pname, params );
  ENTER_GL();
  func_glFragmentLightModelivSGIX( pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glFragmentLightfSGIX( GLenum light, GLenum pname, GLfloat param ) {
  void (*func_glFragmentLightfSGIX)( GLenum, GLenum, GLfloat ) = extension_funcs[EXT_glFragmentLightfSGIX];
  TRACE("(%d, %d, %f)\n", light, pname, param );
  ENTER_GL();
  func_glFragmentLightfSGIX( light, pname, param );
  LEAVE_GL();
}

static void WINAPI wine_glFragmentLightfvSGIX( GLenum light, GLenum pname, GLfloat* params ) {
  void (*func_glFragmentLightfvSGIX)( GLenum, GLenum, GLfloat* ) = extension_funcs[EXT_glFragmentLightfvSGIX];
  TRACE("(%d, %d, %p)\n", light, pname, params );
  ENTER_GL();
  func_glFragmentLightfvSGIX( light, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glFragmentLightiSGIX( GLenum light, GLenum pname, GLint param ) {
  void (*func_glFragmentLightiSGIX)( GLenum, GLenum, GLint ) = extension_funcs[EXT_glFragmentLightiSGIX];
  TRACE("(%d, %d, %d)\n", light, pname, param );
  ENTER_GL();
  func_glFragmentLightiSGIX( light, pname, param );
  LEAVE_GL();
}

static void WINAPI wine_glFragmentLightivSGIX( GLenum light, GLenum pname, GLint* params ) {
  void (*func_glFragmentLightivSGIX)( GLenum, GLenum, GLint* ) = extension_funcs[EXT_glFragmentLightivSGIX];
  TRACE("(%d, %d, %p)\n", light, pname, params );
  ENTER_GL();
  func_glFragmentLightivSGIX( light, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glFragmentMaterialfSGIX( GLenum face, GLenum pname, GLfloat param ) {
  void (*func_glFragmentMaterialfSGIX)( GLenum, GLenum, GLfloat ) = extension_funcs[EXT_glFragmentMaterialfSGIX];
  TRACE("(%d, %d, %f)\n", face, pname, param );
  ENTER_GL();
  func_glFragmentMaterialfSGIX( face, pname, param );
  LEAVE_GL();
}

static void WINAPI wine_glFragmentMaterialfvSGIX( GLenum face, GLenum pname, GLfloat* params ) {
  void (*func_glFragmentMaterialfvSGIX)( GLenum, GLenum, GLfloat* ) = extension_funcs[EXT_glFragmentMaterialfvSGIX];
  TRACE("(%d, %d, %p)\n", face, pname, params );
  ENTER_GL();
  func_glFragmentMaterialfvSGIX( face, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glFragmentMaterialiSGIX( GLenum face, GLenum pname, GLint param ) {
  void (*func_glFragmentMaterialiSGIX)( GLenum, GLenum, GLint ) = extension_funcs[EXT_glFragmentMaterialiSGIX];
  TRACE("(%d, %d, %d)\n", face, pname, param );
  ENTER_GL();
  func_glFragmentMaterialiSGIX( face, pname, param );
  LEAVE_GL();
}

static void WINAPI wine_glFragmentMaterialivSGIX( GLenum face, GLenum pname, GLint* params ) {
  void (*func_glFragmentMaterialivSGIX)( GLenum, GLenum, GLint* ) = extension_funcs[EXT_glFragmentMaterialivSGIX];
  TRACE("(%d, %d, %p)\n", face, pname, params );
  ENTER_GL();
  func_glFragmentMaterialivSGIX( face, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glFrameTerminatorGREMEDY( void ) {
  void (*func_glFrameTerminatorGREMEDY)( void ) = extension_funcs[EXT_glFrameTerminatorGREMEDY];
  TRACE("()\n");
  ENTER_GL();
  func_glFrameTerminatorGREMEDY( );
  LEAVE_GL();
}

static void WINAPI wine_glFrameZoomSGIX( GLint factor ) {
  void (*func_glFrameZoomSGIX)( GLint ) = extension_funcs[EXT_glFrameZoomSGIX];
  TRACE("(%d)\n", factor );
  ENTER_GL();
  func_glFrameZoomSGIX( factor );
  LEAVE_GL();
}

static void WINAPI wine_glFramebufferDrawBufferEXT( GLuint framebuffer, GLenum mode ) {
  void (*func_glFramebufferDrawBufferEXT)( GLuint, GLenum ) = extension_funcs[EXT_glFramebufferDrawBufferEXT];
  TRACE("(%d, %d)\n", framebuffer, mode );
  ENTER_GL();
  func_glFramebufferDrawBufferEXT( framebuffer, mode );
  LEAVE_GL();
}

static void WINAPI wine_glFramebufferDrawBuffersEXT( GLuint framebuffer, GLsizei n, GLenum* bufs ) {
  void (*func_glFramebufferDrawBuffersEXT)( GLuint, GLsizei, GLenum* ) = extension_funcs[EXT_glFramebufferDrawBuffersEXT];
  TRACE("(%d, %d, %p)\n", framebuffer, n, bufs );
  ENTER_GL();
  func_glFramebufferDrawBuffersEXT( framebuffer, n, bufs );
  LEAVE_GL();
}

static void WINAPI wine_glFramebufferReadBufferEXT( GLuint framebuffer, GLenum mode ) {
  void (*func_glFramebufferReadBufferEXT)( GLuint, GLenum ) = extension_funcs[EXT_glFramebufferReadBufferEXT];
  TRACE("(%d, %d)\n", framebuffer, mode );
  ENTER_GL();
  func_glFramebufferReadBufferEXT( framebuffer, mode );
  LEAVE_GL();
}

static void WINAPI wine_glFramebufferRenderbuffer( GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer ) {
  void (*func_glFramebufferRenderbuffer)( GLenum, GLenum, GLenum, GLuint ) = extension_funcs[EXT_glFramebufferRenderbuffer];
  TRACE("(%d, %d, %d, %d)\n", target, attachment, renderbuffertarget, renderbuffer );
  ENTER_GL();
  func_glFramebufferRenderbuffer( target, attachment, renderbuffertarget, renderbuffer );
  LEAVE_GL();
}

static void WINAPI wine_glFramebufferRenderbufferEXT( GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer ) {
  void (*func_glFramebufferRenderbufferEXT)( GLenum, GLenum, GLenum, GLuint ) = extension_funcs[EXT_glFramebufferRenderbufferEXT];
  TRACE("(%d, %d, %d, %d)\n", target, attachment, renderbuffertarget, renderbuffer );
  ENTER_GL();
  func_glFramebufferRenderbufferEXT( target, attachment, renderbuffertarget, renderbuffer );
  LEAVE_GL();
}

static void WINAPI wine_glFramebufferTexture( GLenum target, GLenum attachment, GLuint texture, GLint level ) {
  void (*func_glFramebufferTexture)( GLenum, GLenum, GLuint, GLint ) = extension_funcs[EXT_glFramebufferTexture];
  TRACE("(%d, %d, %d, %d)\n", target, attachment, texture, level );
  ENTER_GL();
  func_glFramebufferTexture( target, attachment, texture, level );
  LEAVE_GL();
}

static void WINAPI wine_glFramebufferTexture1D( GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level ) {
  void (*func_glFramebufferTexture1D)( GLenum, GLenum, GLenum, GLuint, GLint ) = extension_funcs[EXT_glFramebufferTexture1D];
  TRACE("(%d, %d, %d, %d, %d)\n", target, attachment, textarget, texture, level );
  ENTER_GL();
  func_glFramebufferTexture1D( target, attachment, textarget, texture, level );
  LEAVE_GL();
}

static void WINAPI wine_glFramebufferTexture1DEXT( GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level ) {
  void (*func_glFramebufferTexture1DEXT)( GLenum, GLenum, GLenum, GLuint, GLint ) = extension_funcs[EXT_glFramebufferTexture1DEXT];
  TRACE("(%d, %d, %d, %d, %d)\n", target, attachment, textarget, texture, level );
  ENTER_GL();
  func_glFramebufferTexture1DEXT( target, attachment, textarget, texture, level );
  LEAVE_GL();
}

static void WINAPI wine_glFramebufferTexture2D( GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level ) {
  void (*func_glFramebufferTexture2D)( GLenum, GLenum, GLenum, GLuint, GLint ) = extension_funcs[EXT_glFramebufferTexture2D];
  TRACE("(%d, %d, %d, %d, %d)\n", target, attachment, textarget, texture, level );
  ENTER_GL();
  func_glFramebufferTexture2D( target, attachment, textarget, texture, level );
  LEAVE_GL();
}

static void WINAPI wine_glFramebufferTexture2DEXT( GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level ) {
  void (*func_glFramebufferTexture2DEXT)( GLenum, GLenum, GLenum, GLuint, GLint ) = extension_funcs[EXT_glFramebufferTexture2DEXT];
  TRACE("(%d, %d, %d, %d, %d)\n", target, attachment, textarget, texture, level );
  ENTER_GL();
  func_glFramebufferTexture2DEXT( target, attachment, textarget, texture, level );
  LEAVE_GL();
}

static void WINAPI wine_glFramebufferTexture3D( GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint zoffset ) {
  void (*func_glFramebufferTexture3D)( GLenum, GLenum, GLenum, GLuint, GLint, GLint ) = extension_funcs[EXT_glFramebufferTexture3D];
  TRACE("(%d, %d, %d, %d, %d, %d)\n", target, attachment, textarget, texture, level, zoffset );
  ENTER_GL();
  func_glFramebufferTexture3D( target, attachment, textarget, texture, level, zoffset );
  LEAVE_GL();
}

static void WINAPI wine_glFramebufferTexture3DEXT( GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint zoffset ) {
  void (*func_glFramebufferTexture3DEXT)( GLenum, GLenum, GLenum, GLuint, GLint, GLint ) = extension_funcs[EXT_glFramebufferTexture3DEXT];
  TRACE("(%d, %d, %d, %d, %d, %d)\n", target, attachment, textarget, texture, level, zoffset );
  ENTER_GL();
  func_glFramebufferTexture3DEXT( target, attachment, textarget, texture, level, zoffset );
  LEAVE_GL();
}

static void WINAPI wine_glFramebufferTextureARB( GLenum target, GLenum attachment, GLuint texture, GLint level ) {
  void (*func_glFramebufferTextureARB)( GLenum, GLenum, GLuint, GLint ) = extension_funcs[EXT_glFramebufferTextureARB];
  TRACE("(%d, %d, %d, %d)\n", target, attachment, texture, level );
  ENTER_GL();
  func_glFramebufferTextureARB( target, attachment, texture, level );
  LEAVE_GL();
}

static void WINAPI wine_glFramebufferTextureEXT( GLenum target, GLenum attachment, GLuint texture, GLint level ) {
  void (*func_glFramebufferTextureEXT)( GLenum, GLenum, GLuint, GLint ) = extension_funcs[EXT_glFramebufferTextureEXT];
  TRACE("(%d, %d, %d, %d)\n", target, attachment, texture, level );
  ENTER_GL();
  func_glFramebufferTextureEXT( target, attachment, texture, level );
  LEAVE_GL();
}

static void WINAPI wine_glFramebufferTextureFaceARB( GLenum target, GLenum attachment, GLuint texture, GLint level, GLenum face ) {
  void (*func_glFramebufferTextureFaceARB)( GLenum, GLenum, GLuint, GLint, GLenum ) = extension_funcs[EXT_glFramebufferTextureFaceARB];
  TRACE("(%d, %d, %d, %d, %d)\n", target, attachment, texture, level, face );
  ENTER_GL();
  func_glFramebufferTextureFaceARB( target, attachment, texture, level, face );
  LEAVE_GL();
}

static void WINAPI wine_glFramebufferTextureFaceEXT( GLenum target, GLenum attachment, GLuint texture, GLint level, GLenum face ) {
  void (*func_glFramebufferTextureFaceEXT)( GLenum, GLenum, GLuint, GLint, GLenum ) = extension_funcs[EXT_glFramebufferTextureFaceEXT];
  TRACE("(%d, %d, %d, %d, %d)\n", target, attachment, texture, level, face );
  ENTER_GL();
  func_glFramebufferTextureFaceEXT( target, attachment, texture, level, face );
  LEAVE_GL();
}

static void WINAPI wine_glFramebufferTextureLayer( GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer ) {
  void (*func_glFramebufferTextureLayer)( GLenum, GLenum, GLuint, GLint, GLint ) = extension_funcs[EXT_glFramebufferTextureLayer];
  TRACE("(%d, %d, %d, %d, %d)\n", target, attachment, texture, level, layer );
  ENTER_GL();
  func_glFramebufferTextureLayer( target, attachment, texture, level, layer );
  LEAVE_GL();
}

static void WINAPI wine_glFramebufferTextureLayerARB( GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer ) {
  void (*func_glFramebufferTextureLayerARB)( GLenum, GLenum, GLuint, GLint, GLint ) = extension_funcs[EXT_glFramebufferTextureLayerARB];
  TRACE("(%d, %d, %d, %d, %d)\n", target, attachment, texture, level, layer );
  ENTER_GL();
  func_glFramebufferTextureLayerARB( target, attachment, texture, level, layer );
  LEAVE_GL();
}

static void WINAPI wine_glFramebufferTextureLayerEXT( GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer ) {
  void (*func_glFramebufferTextureLayerEXT)( GLenum, GLenum, GLuint, GLint, GLint ) = extension_funcs[EXT_glFramebufferTextureLayerEXT];
  TRACE("(%d, %d, %d, %d, %d)\n", target, attachment, texture, level, layer );
  ENTER_GL();
  func_glFramebufferTextureLayerEXT( target, attachment, texture, level, layer );
  LEAVE_GL();
}

static void WINAPI wine_glFreeObjectBufferATI( GLuint buffer ) {
  void (*func_glFreeObjectBufferATI)( GLuint ) = extension_funcs[EXT_glFreeObjectBufferATI];
  TRACE("(%d)\n", buffer );
  ENTER_GL();
  func_glFreeObjectBufferATI( buffer );
  LEAVE_GL();
}

static GLuint WINAPI wine_glGenAsyncMarkersSGIX( GLsizei range ) {
  GLuint ret_value;
  GLuint (*func_glGenAsyncMarkersSGIX)( GLsizei ) = extension_funcs[EXT_glGenAsyncMarkersSGIX];
  TRACE("(%d)\n", range );
  ENTER_GL();
  ret_value = func_glGenAsyncMarkersSGIX( range );
  LEAVE_GL();
  return ret_value;
}

static void WINAPI wine_glGenBuffers( GLsizei n, GLuint* buffers ) {
  void (*func_glGenBuffers)( GLsizei, GLuint* ) = extension_funcs[EXT_glGenBuffers];
  TRACE("(%d, %p)\n", n, buffers );
  ENTER_GL();
  func_glGenBuffers( n, buffers );
  LEAVE_GL();
}

static void WINAPI wine_glGenBuffersARB( GLsizei n, GLuint* buffers ) {
  void (*func_glGenBuffersARB)( GLsizei, GLuint* ) = extension_funcs[EXT_glGenBuffersARB];
  TRACE("(%d, %p)\n", n, buffers );
  ENTER_GL();
  func_glGenBuffersARB( n, buffers );
  LEAVE_GL();
}

static void WINAPI wine_glGenFencesAPPLE( GLsizei n, GLuint* fences ) {
  void (*func_glGenFencesAPPLE)( GLsizei, GLuint* ) = extension_funcs[EXT_glGenFencesAPPLE];
  TRACE("(%d, %p)\n", n, fences );
  ENTER_GL();
  func_glGenFencesAPPLE( n, fences );
  LEAVE_GL();
}

static void WINAPI wine_glGenFencesNV( GLsizei n, GLuint* fences ) {
  void (*func_glGenFencesNV)( GLsizei, GLuint* ) = extension_funcs[EXT_glGenFencesNV];
  TRACE("(%d, %p)\n", n, fences );
  ENTER_GL();
  func_glGenFencesNV( n, fences );
  LEAVE_GL();
}

static GLuint WINAPI wine_glGenFragmentShadersATI( GLuint range ) {
  GLuint ret_value;
  GLuint (*func_glGenFragmentShadersATI)( GLuint ) = extension_funcs[EXT_glGenFragmentShadersATI];
  TRACE("(%d)\n", range );
  ENTER_GL();
  ret_value = func_glGenFragmentShadersATI( range );
  LEAVE_GL();
  return ret_value;
}

static void WINAPI wine_glGenFramebuffers( GLsizei n, GLuint* framebuffers ) {
  void (*func_glGenFramebuffers)( GLsizei, GLuint* ) = extension_funcs[EXT_glGenFramebuffers];
  TRACE("(%d, %p)\n", n, framebuffers );
  ENTER_GL();
  func_glGenFramebuffers( n, framebuffers );
  LEAVE_GL();
}

static void WINAPI wine_glGenFramebuffersEXT( GLsizei n, GLuint* framebuffers ) {
  void (*func_glGenFramebuffersEXT)( GLsizei, GLuint* ) = extension_funcs[EXT_glGenFramebuffersEXT];
  TRACE("(%d, %p)\n", n, framebuffers );
  ENTER_GL();
  func_glGenFramebuffersEXT( n, framebuffers );
  LEAVE_GL();
}

static void WINAPI wine_glGenNamesAMD( GLenum identifier, GLuint num, GLuint* names ) {
  void (*func_glGenNamesAMD)( GLenum, GLuint, GLuint* ) = extension_funcs[EXT_glGenNamesAMD];
  TRACE("(%d, %d, %p)\n", identifier, num, names );
  ENTER_GL();
  func_glGenNamesAMD( identifier, num, names );
  LEAVE_GL();
}

static void WINAPI wine_glGenOcclusionQueriesNV( GLsizei n, GLuint* ids ) {
  void (*func_glGenOcclusionQueriesNV)( GLsizei, GLuint* ) = extension_funcs[EXT_glGenOcclusionQueriesNV];
  TRACE("(%d, %p)\n", n, ids );
  ENTER_GL();
  func_glGenOcclusionQueriesNV( n, ids );
  LEAVE_GL();
}

static void WINAPI wine_glGenPerfMonitorsAMD( GLsizei n, GLuint* monitors ) {
  void (*func_glGenPerfMonitorsAMD)( GLsizei, GLuint* ) = extension_funcs[EXT_glGenPerfMonitorsAMD];
  TRACE("(%d, %p)\n", n, monitors );
  ENTER_GL();
  func_glGenPerfMonitorsAMD( n, monitors );
  LEAVE_GL();
}

static void WINAPI wine_glGenProgramPipelines( GLsizei n, GLuint* pipelines ) {
  void (*func_glGenProgramPipelines)( GLsizei, GLuint* ) = extension_funcs[EXT_glGenProgramPipelines];
  TRACE("(%d, %p)\n", n, pipelines );
  ENTER_GL();
  func_glGenProgramPipelines( n, pipelines );
  LEAVE_GL();
}

static void WINAPI wine_glGenProgramsARB( GLsizei n, GLuint* programs ) {
  void (*func_glGenProgramsARB)( GLsizei, GLuint* ) = extension_funcs[EXT_glGenProgramsARB];
  TRACE("(%d, %p)\n", n, programs );
  ENTER_GL();
  func_glGenProgramsARB( n, programs );
  LEAVE_GL();
}

static void WINAPI wine_glGenProgramsNV( GLsizei n, GLuint* programs ) {
  void (*func_glGenProgramsNV)( GLsizei, GLuint* ) = extension_funcs[EXT_glGenProgramsNV];
  TRACE("(%d, %p)\n", n, programs );
  ENTER_GL();
  func_glGenProgramsNV( n, programs );
  LEAVE_GL();
}

static void WINAPI wine_glGenQueries( GLsizei n, GLuint* ids ) {
  void (*func_glGenQueries)( GLsizei, GLuint* ) = extension_funcs[EXT_glGenQueries];
  TRACE("(%d, %p)\n", n, ids );
  ENTER_GL();
  func_glGenQueries( n, ids );
  LEAVE_GL();
}

static void WINAPI wine_glGenQueriesARB( GLsizei n, GLuint* ids ) {
  void (*func_glGenQueriesARB)( GLsizei, GLuint* ) = extension_funcs[EXT_glGenQueriesARB];
  TRACE("(%d, %p)\n", n, ids );
  ENTER_GL();
  func_glGenQueriesARB( n, ids );
  LEAVE_GL();
}

static void WINAPI wine_glGenRenderbuffers( GLsizei n, GLuint* renderbuffers ) {
  void (*func_glGenRenderbuffers)( GLsizei, GLuint* ) = extension_funcs[EXT_glGenRenderbuffers];
  TRACE("(%d, %p)\n", n, renderbuffers );
  ENTER_GL();
  func_glGenRenderbuffers( n, renderbuffers );
  LEAVE_GL();
}

static void WINAPI wine_glGenRenderbuffersEXT( GLsizei n, GLuint* renderbuffers ) {
  void (*func_glGenRenderbuffersEXT)( GLsizei, GLuint* ) = extension_funcs[EXT_glGenRenderbuffersEXT];
  TRACE("(%d, %p)\n", n, renderbuffers );
  ENTER_GL();
  func_glGenRenderbuffersEXT( n, renderbuffers );
  LEAVE_GL();
}

static void WINAPI wine_glGenSamplers( GLsizei count, GLuint* samplers ) {
  void (*func_glGenSamplers)( GLsizei, GLuint* ) = extension_funcs[EXT_glGenSamplers];
  TRACE("(%d, %p)\n", count, samplers );
  ENTER_GL();
  func_glGenSamplers( count, samplers );
  LEAVE_GL();
}

static GLuint WINAPI wine_glGenSymbolsEXT( GLenum datatype, GLenum storagetype, GLenum range, GLuint components ) {
  GLuint ret_value;
  GLuint (*func_glGenSymbolsEXT)( GLenum, GLenum, GLenum, GLuint ) = extension_funcs[EXT_glGenSymbolsEXT];
  TRACE("(%d, %d, %d, %d)\n", datatype, storagetype, range, components );
  ENTER_GL();
  ret_value = func_glGenSymbolsEXT( datatype, storagetype, range, components );
  LEAVE_GL();
  return ret_value;
}

static void WINAPI wine_glGenTexturesEXT( GLsizei n, GLuint* textures ) {
  void (*func_glGenTexturesEXT)( GLsizei, GLuint* ) = extension_funcs[EXT_glGenTexturesEXT];
  TRACE("(%d, %p)\n", n, textures );
  ENTER_GL();
  func_glGenTexturesEXT( n, textures );
  LEAVE_GL();
}

static void WINAPI wine_glGenTransformFeedbacks( GLsizei n, GLuint* ids ) {
  void (*func_glGenTransformFeedbacks)( GLsizei, GLuint* ) = extension_funcs[EXT_glGenTransformFeedbacks];
  TRACE("(%d, %p)\n", n, ids );
  ENTER_GL();
  func_glGenTransformFeedbacks( n, ids );
  LEAVE_GL();
}

static void WINAPI wine_glGenTransformFeedbacksNV( GLsizei n, GLuint* ids ) {
  void (*func_glGenTransformFeedbacksNV)( GLsizei, GLuint* ) = extension_funcs[EXT_glGenTransformFeedbacksNV];
  TRACE("(%d, %p)\n", n, ids );
  ENTER_GL();
  func_glGenTransformFeedbacksNV( n, ids );
  LEAVE_GL();
}

static void WINAPI wine_glGenVertexArrays( GLsizei n, GLuint* arrays ) {
  void (*func_glGenVertexArrays)( GLsizei, GLuint* ) = extension_funcs[EXT_glGenVertexArrays];
  TRACE("(%d, %p)\n", n, arrays );
  ENTER_GL();
  func_glGenVertexArrays( n, arrays );
  LEAVE_GL();
}

static void WINAPI wine_glGenVertexArraysAPPLE( GLsizei n, GLuint* arrays ) {
  void (*func_glGenVertexArraysAPPLE)( GLsizei, GLuint* ) = extension_funcs[EXT_glGenVertexArraysAPPLE];
  TRACE("(%d, %p)\n", n, arrays );
  ENTER_GL();
  func_glGenVertexArraysAPPLE( n, arrays );
  LEAVE_GL();
}

static GLuint WINAPI wine_glGenVertexShadersEXT( GLuint range ) {
  GLuint ret_value;
  GLuint (*func_glGenVertexShadersEXT)( GLuint ) = extension_funcs[EXT_glGenVertexShadersEXT];
  TRACE("(%d)\n", range );
  ENTER_GL();
  ret_value = func_glGenVertexShadersEXT( range );
  LEAVE_GL();
  return ret_value;
}

static void WINAPI wine_glGenerateMipmap( GLenum target ) {
  void (*func_glGenerateMipmap)( GLenum ) = extension_funcs[EXT_glGenerateMipmap];
  TRACE("(%d)\n", target );
  ENTER_GL();
  func_glGenerateMipmap( target );
  LEAVE_GL();
}

static void WINAPI wine_glGenerateMipmapEXT( GLenum target ) {
  void (*func_glGenerateMipmapEXT)( GLenum ) = extension_funcs[EXT_glGenerateMipmapEXT];
  TRACE("(%d)\n", target );
  ENTER_GL();
  func_glGenerateMipmapEXT( target );
  LEAVE_GL();
}

static void WINAPI wine_glGenerateMultiTexMipmapEXT( GLenum texunit, GLenum target ) {
  void (*func_glGenerateMultiTexMipmapEXT)( GLenum, GLenum ) = extension_funcs[EXT_glGenerateMultiTexMipmapEXT];
  TRACE("(%d, %d)\n", texunit, target );
  ENTER_GL();
  func_glGenerateMultiTexMipmapEXT( texunit, target );
  LEAVE_GL();
}

static void WINAPI wine_glGenerateTextureMipmapEXT( GLuint texture, GLenum target ) {
  void (*func_glGenerateTextureMipmapEXT)( GLuint, GLenum ) = extension_funcs[EXT_glGenerateTextureMipmapEXT];
  TRACE("(%d, %d)\n", texture, target );
  ENTER_GL();
  func_glGenerateTextureMipmapEXT( texture, target );
  LEAVE_GL();
}

static void WINAPI wine_glGetActiveAttrib( GLuint program, GLuint index, GLsizei bufSize, GLsizei* length, GLint* size, GLenum* type, char* name ) {
  void (*func_glGetActiveAttrib)( GLuint, GLuint, GLsizei, GLsizei*, GLint*, GLenum*, char* ) = extension_funcs[EXT_glGetActiveAttrib];
  TRACE("(%d, %d, %d, %p, %p, %p, %p)\n", program, index, bufSize, length, size, type, name );
  ENTER_GL();
  func_glGetActiveAttrib( program, index, bufSize, length, size, type, name );
  LEAVE_GL();
}

static void WINAPI wine_glGetActiveAttribARB( unsigned int programObj, GLuint index, GLsizei maxLength, GLsizei* length, GLint* size, GLenum* type, char* name ) {
  void (*func_glGetActiveAttribARB)( unsigned int, GLuint, GLsizei, GLsizei*, GLint*, GLenum*, char* ) = extension_funcs[EXT_glGetActiveAttribARB];
  TRACE("(%d, %d, %d, %p, %p, %p, %p)\n", programObj, index, maxLength, length, size, type, name );
  ENTER_GL();
  func_glGetActiveAttribARB( programObj, index, maxLength, length, size, type, name );
  LEAVE_GL();
}

static void WINAPI wine_glGetActiveSubroutineName( GLuint program, GLenum shadertype, GLuint index, GLsizei bufsize, GLsizei* length, char* name ) {
  void (*func_glGetActiveSubroutineName)( GLuint, GLenum, GLuint, GLsizei, GLsizei*, char* ) = extension_funcs[EXT_glGetActiveSubroutineName];
  TRACE("(%d, %d, %d, %d, %p, %p)\n", program, shadertype, index, bufsize, length, name );
  ENTER_GL();
  func_glGetActiveSubroutineName( program, shadertype, index, bufsize, length, name );
  LEAVE_GL();
}

static void WINAPI wine_glGetActiveSubroutineUniformName( GLuint program, GLenum shadertype, GLuint index, GLsizei bufsize, GLsizei* length, char* name ) {
  void (*func_glGetActiveSubroutineUniformName)( GLuint, GLenum, GLuint, GLsizei, GLsizei*, char* ) = extension_funcs[EXT_glGetActiveSubroutineUniformName];
  TRACE("(%d, %d, %d, %d, %p, %p)\n", program, shadertype, index, bufsize, length, name );
  ENTER_GL();
  func_glGetActiveSubroutineUniformName( program, shadertype, index, bufsize, length, name );
  LEAVE_GL();
}

static void WINAPI wine_glGetActiveSubroutineUniformiv( GLuint program, GLenum shadertype, GLuint index, GLenum pname, GLint* values ) {
  void (*func_glGetActiveSubroutineUniformiv)( GLuint, GLenum, GLuint, GLenum, GLint* ) = extension_funcs[EXT_glGetActiveSubroutineUniformiv];
  TRACE("(%d, %d, %d, %d, %p)\n", program, shadertype, index, pname, values );
  ENTER_GL();
  func_glGetActiveSubroutineUniformiv( program, shadertype, index, pname, values );
  LEAVE_GL();
}

static void WINAPI wine_glGetActiveUniform( GLuint program, GLuint index, GLsizei bufSize, GLsizei* length, GLint* size, GLenum* type, char* name ) {
  void (*func_glGetActiveUniform)( GLuint, GLuint, GLsizei, GLsizei*, GLint*, GLenum*, char* ) = extension_funcs[EXT_glGetActiveUniform];
  TRACE("(%d, %d, %d, %p, %p, %p, %p)\n", program, index, bufSize, length, size, type, name );
  ENTER_GL();
  func_glGetActiveUniform( program, index, bufSize, length, size, type, name );
  LEAVE_GL();
}

static void WINAPI wine_glGetActiveUniformARB( unsigned int programObj, GLuint index, GLsizei maxLength, GLsizei* length, GLint* size, GLenum* type, char* name ) {
  void (*func_glGetActiveUniformARB)( unsigned int, GLuint, GLsizei, GLsizei*, GLint*, GLenum*, char* ) = extension_funcs[EXT_glGetActiveUniformARB];
  TRACE("(%d, %d, %d, %p, %p, %p, %p)\n", programObj, index, maxLength, length, size, type, name );
  ENTER_GL();
  func_glGetActiveUniformARB( programObj, index, maxLength, length, size, type, name );
  LEAVE_GL();
}

static void WINAPI wine_glGetActiveUniformBlockName( GLuint program, GLuint uniformBlockIndex, GLsizei bufSize, GLsizei* length, char* uniformBlockName ) {
  void (*func_glGetActiveUniformBlockName)( GLuint, GLuint, GLsizei, GLsizei*, char* ) = extension_funcs[EXT_glGetActiveUniformBlockName];
  TRACE("(%d, %d, %d, %p, %p)\n", program, uniformBlockIndex, bufSize, length, uniformBlockName );
  ENTER_GL();
  func_glGetActiveUniformBlockName( program, uniformBlockIndex, bufSize, length, uniformBlockName );
  LEAVE_GL();
}

static void WINAPI wine_glGetActiveUniformBlockiv( GLuint program, GLuint uniformBlockIndex, GLenum pname, GLint* params ) {
  void (*func_glGetActiveUniformBlockiv)( GLuint, GLuint, GLenum, GLint* ) = extension_funcs[EXT_glGetActiveUniformBlockiv];
  TRACE("(%d, %d, %d, %p)\n", program, uniformBlockIndex, pname, params );
  ENTER_GL();
  func_glGetActiveUniformBlockiv( program, uniformBlockIndex, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetActiveUniformName( GLuint program, GLuint uniformIndex, GLsizei bufSize, GLsizei* length, char* uniformName ) {
  void (*func_glGetActiveUniformName)( GLuint, GLuint, GLsizei, GLsizei*, char* ) = extension_funcs[EXT_glGetActiveUniformName];
  TRACE("(%d, %d, %d, %p, %p)\n", program, uniformIndex, bufSize, length, uniformName );
  ENTER_GL();
  func_glGetActiveUniformName( program, uniformIndex, bufSize, length, uniformName );
  LEAVE_GL();
}

static void WINAPI wine_glGetActiveUniformsiv( GLuint program, GLsizei uniformCount, GLuint* uniformIndices, GLenum pname, GLint* params ) {
  void (*func_glGetActiveUniformsiv)( GLuint, GLsizei, GLuint*, GLenum, GLint* ) = extension_funcs[EXT_glGetActiveUniformsiv];
  TRACE("(%d, %d, %p, %d, %p)\n", program, uniformCount, uniformIndices, pname, params );
  ENTER_GL();
  func_glGetActiveUniformsiv( program, uniformCount, uniformIndices, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetActiveVaryingNV( GLuint program, GLuint index, GLsizei bufSize, GLsizei* length, GLsizei* size, GLenum* type, char* name ) {
  void (*func_glGetActiveVaryingNV)( GLuint, GLuint, GLsizei, GLsizei*, GLsizei*, GLenum*, char* ) = extension_funcs[EXT_glGetActiveVaryingNV];
  TRACE("(%d, %d, %d, %p, %p, %p, %p)\n", program, index, bufSize, length, size, type, name );
  ENTER_GL();
  func_glGetActiveVaryingNV( program, index, bufSize, length, size, type, name );
  LEAVE_GL();
}

static void WINAPI wine_glGetArrayObjectfvATI( GLenum array, GLenum pname, GLfloat* params ) {
  void (*func_glGetArrayObjectfvATI)( GLenum, GLenum, GLfloat* ) = extension_funcs[EXT_glGetArrayObjectfvATI];
  TRACE("(%d, %d, %p)\n", array, pname, params );
  ENTER_GL();
  func_glGetArrayObjectfvATI( array, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetArrayObjectivATI( GLenum array, GLenum pname, GLint* params ) {
  void (*func_glGetArrayObjectivATI)( GLenum, GLenum, GLint* ) = extension_funcs[EXT_glGetArrayObjectivATI];
  TRACE("(%d, %d, %p)\n", array, pname, params );
  ENTER_GL();
  func_glGetArrayObjectivATI( array, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetAttachedObjectsARB( unsigned int containerObj, GLsizei maxCount, GLsizei* count, unsigned int* obj ) {
  void (*func_glGetAttachedObjectsARB)( unsigned int, GLsizei, GLsizei*, unsigned int* ) = extension_funcs[EXT_glGetAttachedObjectsARB];
  TRACE("(%d, %d, %p, %p)\n", containerObj, maxCount, count, obj );
  ENTER_GL();
  func_glGetAttachedObjectsARB( containerObj, maxCount, count, obj );
  LEAVE_GL();
}

static void WINAPI wine_glGetAttachedShaders( GLuint program, GLsizei maxCount, GLsizei* count, GLuint* obj ) {
  void (*func_glGetAttachedShaders)( GLuint, GLsizei, GLsizei*, GLuint* ) = extension_funcs[EXT_glGetAttachedShaders];
  TRACE("(%d, %d, %p, %p)\n", program, maxCount, count, obj );
  ENTER_GL();
  func_glGetAttachedShaders( program, maxCount, count, obj );
  LEAVE_GL();
}

static GLint WINAPI wine_glGetAttribLocation( GLuint program, char* name ) {
  GLint ret_value;
  GLint (*func_glGetAttribLocation)( GLuint, char* ) = extension_funcs[EXT_glGetAttribLocation];
  TRACE("(%d, %p)\n", program, name );
  ENTER_GL();
  ret_value = func_glGetAttribLocation( program, name );
  LEAVE_GL();
  return ret_value;
}

static GLint WINAPI wine_glGetAttribLocationARB( unsigned int programObj, char* name ) {
  GLint ret_value;
  GLint (*func_glGetAttribLocationARB)( unsigned int, char* ) = extension_funcs[EXT_glGetAttribLocationARB];
  TRACE("(%d, %p)\n", programObj, name );
  ENTER_GL();
  ret_value = func_glGetAttribLocationARB( programObj, name );
  LEAVE_GL();
  return ret_value;
}

static void WINAPI wine_glGetBooleanIndexedvEXT( GLenum target, GLuint index, GLboolean* data ) {
  void (*func_glGetBooleanIndexedvEXT)( GLenum, GLuint, GLboolean* ) = extension_funcs[EXT_glGetBooleanIndexedvEXT];
  TRACE("(%d, %d, %p)\n", target, index, data );
  ENTER_GL();
  func_glGetBooleanIndexedvEXT( target, index, data );
  LEAVE_GL();
}

static void WINAPI wine_glGetBooleani_v( GLenum target, GLuint index, GLboolean* data ) {
  void (*func_glGetBooleani_v)( GLenum, GLuint, GLboolean* ) = extension_funcs[EXT_glGetBooleani_v];
  TRACE("(%d, %d, %p)\n", target, index, data );
  ENTER_GL();
  func_glGetBooleani_v( target, index, data );
  LEAVE_GL();
}

static void WINAPI wine_glGetBufferParameteri64v( GLenum target, GLenum pname, INT64* params ) {
  void (*func_glGetBufferParameteri64v)( GLenum, GLenum, INT64* ) = extension_funcs[EXT_glGetBufferParameteri64v];
  TRACE("(%d, %d, %p)\n", target, pname, params );
  ENTER_GL();
  func_glGetBufferParameteri64v( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetBufferParameteriv( GLenum target, GLenum pname, GLint* params ) {
  void (*func_glGetBufferParameteriv)( GLenum, GLenum, GLint* ) = extension_funcs[EXT_glGetBufferParameteriv];
  TRACE("(%d, %d, %p)\n", target, pname, params );
  ENTER_GL();
  func_glGetBufferParameteriv( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetBufferParameterivARB( GLenum target, GLenum pname, GLint* params ) {
  void (*func_glGetBufferParameterivARB)( GLenum, GLenum, GLint* ) = extension_funcs[EXT_glGetBufferParameterivARB];
  TRACE("(%d, %d, %p)\n", target, pname, params );
  ENTER_GL();
  func_glGetBufferParameterivARB( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetBufferParameterui64vNV( GLenum target, GLenum pname, UINT64* params ) {
  void (*func_glGetBufferParameterui64vNV)( GLenum, GLenum, UINT64* ) = extension_funcs[EXT_glGetBufferParameterui64vNV];
  TRACE("(%d, %d, %p)\n", target, pname, params );
  ENTER_GL();
  func_glGetBufferParameterui64vNV( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetBufferPointerv( GLenum target, GLenum pname, GLvoid** params ) {
  void (*func_glGetBufferPointerv)( GLenum, GLenum, GLvoid** ) = extension_funcs[EXT_glGetBufferPointerv];
  TRACE("(%d, %d, %p)\n", target, pname, params );
  ENTER_GL();
  func_glGetBufferPointerv( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetBufferPointervARB( GLenum target, GLenum pname, GLvoid** params ) {
  void (*func_glGetBufferPointervARB)( GLenum, GLenum, GLvoid** ) = extension_funcs[EXT_glGetBufferPointervARB];
  TRACE("(%d, %d, %p)\n", target, pname, params );
  ENTER_GL();
  func_glGetBufferPointervARB( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetBufferSubData( GLenum target, INT_PTR offset, INT_PTR size, GLvoid* data ) {
  void (*func_glGetBufferSubData)( GLenum, INT_PTR, INT_PTR, GLvoid* ) = extension_funcs[EXT_glGetBufferSubData];
  TRACE("(%d, %ld, %ld, %p)\n", target, offset, size, data );
  ENTER_GL();
  func_glGetBufferSubData( target, offset, size, data );
  LEAVE_GL();
}

static void WINAPI wine_glGetBufferSubDataARB( GLenum target, INT_PTR offset, INT_PTR size, GLvoid* data ) {
  void (*func_glGetBufferSubDataARB)( GLenum, INT_PTR, INT_PTR, GLvoid* ) = extension_funcs[EXT_glGetBufferSubDataARB];
  TRACE("(%d, %ld, %ld, %p)\n", target, offset, size, data );
  ENTER_GL();
  func_glGetBufferSubDataARB( target, offset, size, data );
  LEAVE_GL();
}

static void WINAPI wine_glGetColorTable( GLenum target, GLenum format, GLenum type, GLvoid* table ) {
  void (*func_glGetColorTable)( GLenum, GLenum, GLenum, GLvoid* ) = extension_funcs[EXT_glGetColorTable];
  TRACE("(%d, %d, %d, %p)\n", target, format, type, table );
  ENTER_GL();
  func_glGetColorTable( target, format, type, table );
  LEAVE_GL();
}

static void WINAPI wine_glGetColorTableEXT( GLenum target, GLenum format, GLenum type, GLvoid* data ) {
  void (*func_glGetColorTableEXT)( GLenum, GLenum, GLenum, GLvoid* ) = extension_funcs[EXT_glGetColorTableEXT];
  TRACE("(%d, %d, %d, %p)\n", target, format, type, data );
  ENTER_GL();
  func_glGetColorTableEXT( target, format, type, data );
  LEAVE_GL();
}

static void WINAPI wine_glGetColorTableParameterfv( GLenum target, GLenum pname, GLfloat* params ) {
  void (*func_glGetColorTableParameterfv)( GLenum, GLenum, GLfloat* ) = extension_funcs[EXT_glGetColorTableParameterfv];
  TRACE("(%d, %d, %p)\n", target, pname, params );
  ENTER_GL();
  func_glGetColorTableParameterfv( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetColorTableParameterfvEXT( GLenum target, GLenum pname, GLfloat* params ) {
  void (*func_glGetColorTableParameterfvEXT)( GLenum, GLenum, GLfloat* ) = extension_funcs[EXT_glGetColorTableParameterfvEXT];
  TRACE("(%d, %d, %p)\n", target, pname, params );
  ENTER_GL();
  func_glGetColorTableParameterfvEXT( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetColorTableParameterfvSGI( GLenum target, GLenum pname, GLfloat* params ) {
  void (*func_glGetColorTableParameterfvSGI)( GLenum, GLenum, GLfloat* ) = extension_funcs[EXT_glGetColorTableParameterfvSGI];
  TRACE("(%d, %d, %p)\n", target, pname, params );
  ENTER_GL();
  func_glGetColorTableParameterfvSGI( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetColorTableParameteriv( GLenum target, GLenum pname, GLint* params ) {
  void (*func_glGetColorTableParameteriv)( GLenum, GLenum, GLint* ) = extension_funcs[EXT_glGetColorTableParameteriv];
  TRACE("(%d, %d, %p)\n", target, pname, params );
  ENTER_GL();
  func_glGetColorTableParameteriv( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetColorTableParameterivEXT( GLenum target, GLenum pname, GLint* params ) {
  void (*func_glGetColorTableParameterivEXT)( GLenum, GLenum, GLint* ) = extension_funcs[EXT_glGetColorTableParameterivEXT];
  TRACE("(%d, %d, %p)\n", target, pname, params );
  ENTER_GL();
  func_glGetColorTableParameterivEXT( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetColorTableParameterivSGI( GLenum target, GLenum pname, GLint* params ) {
  void (*func_glGetColorTableParameterivSGI)( GLenum, GLenum, GLint* ) = extension_funcs[EXT_glGetColorTableParameterivSGI];
  TRACE("(%d, %d, %p)\n", target, pname, params );
  ENTER_GL();
  func_glGetColorTableParameterivSGI( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetColorTableSGI( GLenum target, GLenum format, GLenum type, GLvoid* table ) {
  void (*func_glGetColorTableSGI)( GLenum, GLenum, GLenum, GLvoid* ) = extension_funcs[EXT_glGetColorTableSGI];
  TRACE("(%d, %d, %d, %p)\n", target, format, type, table );
  ENTER_GL();
  func_glGetColorTableSGI( target, format, type, table );
  LEAVE_GL();
}

static void WINAPI wine_glGetCombinerInputParameterfvNV( GLenum stage, GLenum portion, GLenum variable, GLenum pname, GLfloat* params ) {
  void (*func_glGetCombinerInputParameterfvNV)( GLenum, GLenum, GLenum, GLenum, GLfloat* ) = extension_funcs[EXT_glGetCombinerInputParameterfvNV];
  TRACE("(%d, %d, %d, %d, %p)\n", stage, portion, variable, pname, params );
  ENTER_GL();
  func_glGetCombinerInputParameterfvNV( stage, portion, variable, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetCombinerInputParameterivNV( GLenum stage, GLenum portion, GLenum variable, GLenum pname, GLint* params ) {
  void (*func_glGetCombinerInputParameterivNV)( GLenum, GLenum, GLenum, GLenum, GLint* ) = extension_funcs[EXT_glGetCombinerInputParameterivNV];
  TRACE("(%d, %d, %d, %d, %p)\n", stage, portion, variable, pname, params );
  ENTER_GL();
  func_glGetCombinerInputParameterivNV( stage, portion, variable, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetCombinerOutputParameterfvNV( GLenum stage, GLenum portion, GLenum pname, GLfloat* params ) {
  void (*func_glGetCombinerOutputParameterfvNV)( GLenum, GLenum, GLenum, GLfloat* ) = extension_funcs[EXT_glGetCombinerOutputParameterfvNV];
  TRACE("(%d, %d, %d, %p)\n", stage, portion, pname, params );
  ENTER_GL();
  func_glGetCombinerOutputParameterfvNV( stage, portion, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetCombinerOutputParameterivNV( GLenum stage, GLenum portion, GLenum pname, GLint* params ) {
  void (*func_glGetCombinerOutputParameterivNV)( GLenum, GLenum, GLenum, GLint* ) = extension_funcs[EXT_glGetCombinerOutputParameterivNV];
  TRACE("(%d, %d, %d, %p)\n", stage, portion, pname, params );
  ENTER_GL();
  func_glGetCombinerOutputParameterivNV( stage, portion, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetCombinerStageParameterfvNV( GLenum stage, GLenum pname, GLfloat* params ) {
  void (*func_glGetCombinerStageParameterfvNV)( GLenum, GLenum, GLfloat* ) = extension_funcs[EXT_glGetCombinerStageParameterfvNV];
  TRACE("(%d, %d, %p)\n", stage, pname, params );
  ENTER_GL();
  func_glGetCombinerStageParameterfvNV( stage, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetCompressedMultiTexImageEXT( GLenum texunit, GLenum target, GLint lod, GLvoid* img ) {
  void (*func_glGetCompressedMultiTexImageEXT)( GLenum, GLenum, GLint, GLvoid* ) = extension_funcs[EXT_glGetCompressedMultiTexImageEXT];
  TRACE("(%d, %d, %d, %p)\n", texunit, target, lod, img );
  ENTER_GL();
  func_glGetCompressedMultiTexImageEXT( texunit, target, lod, img );
  LEAVE_GL();
}

static void WINAPI wine_glGetCompressedTexImage( GLenum target, GLint level, GLvoid* img ) {
  void (*func_glGetCompressedTexImage)( GLenum, GLint, GLvoid* ) = extension_funcs[EXT_glGetCompressedTexImage];
  TRACE("(%d, %d, %p)\n", target, level, img );
  ENTER_GL();
  func_glGetCompressedTexImage( target, level, img );
  LEAVE_GL();
}

static void WINAPI wine_glGetCompressedTexImageARB( GLenum target, GLint level, GLvoid* img ) {
  void (*func_glGetCompressedTexImageARB)( GLenum, GLint, GLvoid* ) = extension_funcs[EXT_glGetCompressedTexImageARB];
  TRACE("(%d, %d, %p)\n", target, level, img );
  ENTER_GL();
  func_glGetCompressedTexImageARB( target, level, img );
  LEAVE_GL();
}

static void WINAPI wine_glGetCompressedTextureImageEXT( GLuint texture, GLenum target, GLint lod, GLvoid* img ) {
  void (*func_glGetCompressedTextureImageEXT)( GLuint, GLenum, GLint, GLvoid* ) = extension_funcs[EXT_glGetCompressedTextureImageEXT];
  TRACE("(%d, %d, %d, %p)\n", texture, target, lod, img );
  ENTER_GL();
  func_glGetCompressedTextureImageEXT( texture, target, lod, img );
  LEAVE_GL();
}

static void WINAPI wine_glGetConvolutionFilter( GLenum target, GLenum format, GLenum type, GLvoid* image ) {
  void (*func_glGetConvolutionFilter)( GLenum, GLenum, GLenum, GLvoid* ) = extension_funcs[EXT_glGetConvolutionFilter];
  TRACE("(%d, %d, %d, %p)\n", target, format, type, image );
  ENTER_GL();
  func_glGetConvolutionFilter( target, format, type, image );
  LEAVE_GL();
}

static void WINAPI wine_glGetConvolutionFilterEXT( GLenum target, GLenum format, GLenum type, GLvoid* image ) {
  void (*func_glGetConvolutionFilterEXT)( GLenum, GLenum, GLenum, GLvoid* ) = extension_funcs[EXT_glGetConvolutionFilterEXT];
  TRACE("(%d, %d, %d, %p)\n", target, format, type, image );
  ENTER_GL();
  func_glGetConvolutionFilterEXT( target, format, type, image );
  LEAVE_GL();
}

static void WINAPI wine_glGetConvolutionParameterfv( GLenum target, GLenum pname, GLfloat* params ) {
  void (*func_glGetConvolutionParameterfv)( GLenum, GLenum, GLfloat* ) = extension_funcs[EXT_glGetConvolutionParameterfv];
  TRACE("(%d, %d, %p)\n", target, pname, params );
  ENTER_GL();
  func_glGetConvolutionParameterfv( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetConvolutionParameterfvEXT( GLenum target, GLenum pname, GLfloat* params ) {
  void (*func_glGetConvolutionParameterfvEXT)( GLenum, GLenum, GLfloat* ) = extension_funcs[EXT_glGetConvolutionParameterfvEXT];
  TRACE("(%d, %d, %p)\n", target, pname, params );
  ENTER_GL();
  func_glGetConvolutionParameterfvEXT( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetConvolutionParameteriv( GLenum target, GLenum pname, GLint* params ) {
  void (*func_glGetConvolutionParameteriv)( GLenum, GLenum, GLint* ) = extension_funcs[EXT_glGetConvolutionParameteriv];
  TRACE("(%d, %d, %p)\n", target, pname, params );
  ENTER_GL();
  func_glGetConvolutionParameteriv( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetConvolutionParameterivEXT( GLenum target, GLenum pname, GLint* params ) {
  void (*func_glGetConvolutionParameterivEXT)( GLenum, GLenum, GLint* ) = extension_funcs[EXT_glGetConvolutionParameterivEXT];
  TRACE("(%d, %d, %p)\n", target, pname, params );
  ENTER_GL();
  func_glGetConvolutionParameterivEXT( target, pname, params );
  LEAVE_GL();
}

static GLuint WINAPI wine_glGetDebugMessageLogAMD( GLuint count, GLsizei bufsize, GLenum* categories, GLuint* severities, GLuint* ids, GLsizei* lengths, char* message ) {
  GLuint ret_value;
  GLuint (*func_glGetDebugMessageLogAMD)( GLuint, GLsizei, GLenum*, GLuint*, GLuint*, GLsizei*, char* ) = extension_funcs[EXT_glGetDebugMessageLogAMD];
  TRACE("(%d, %d, %p, %p, %p, %p, %p)\n", count, bufsize, categories, severities, ids, lengths, message );
  ENTER_GL();
  ret_value = func_glGetDebugMessageLogAMD( count, bufsize, categories, severities, ids, lengths, message );
  LEAVE_GL();
  return ret_value;
}

static GLuint WINAPI wine_glGetDebugMessageLogARB( GLuint count, GLsizei bufsize, GLenum* sources, GLenum* types, GLuint* ids, GLenum* severities, GLsizei* lengths, char* messageLog ) {
  GLuint ret_value;
  GLuint (*func_glGetDebugMessageLogARB)( GLuint, GLsizei, GLenum*, GLenum*, GLuint*, GLenum*, GLsizei*, char* ) = extension_funcs[EXT_glGetDebugMessageLogARB];
  TRACE("(%d, %d, %p, %p, %p, %p, %p, %p)\n", count, bufsize, sources, types, ids, severities, lengths, messageLog );
  ENTER_GL();
  ret_value = func_glGetDebugMessageLogARB( count, bufsize, sources, types, ids, severities, lengths, messageLog );
  LEAVE_GL();
  return ret_value;
}

static void WINAPI wine_glGetDetailTexFuncSGIS( GLenum target, GLfloat* points ) {
  void (*func_glGetDetailTexFuncSGIS)( GLenum, GLfloat* ) = extension_funcs[EXT_glGetDetailTexFuncSGIS];
  TRACE("(%d, %p)\n", target, points );
  ENTER_GL();
  func_glGetDetailTexFuncSGIS( target, points );
  LEAVE_GL();
}

static void WINAPI wine_glGetDoubleIndexedvEXT( GLenum target, GLuint index, GLdouble* data ) {
  void (*func_glGetDoubleIndexedvEXT)( GLenum, GLuint, GLdouble* ) = extension_funcs[EXT_glGetDoubleIndexedvEXT];
  TRACE("(%d, %d, %p)\n", target, index, data );
  ENTER_GL();
  func_glGetDoubleIndexedvEXT( target, index, data );
  LEAVE_GL();
}

static void WINAPI wine_glGetDoublei_v( GLenum target, GLuint index, GLdouble* data ) {
  void (*func_glGetDoublei_v)( GLenum, GLuint, GLdouble* ) = extension_funcs[EXT_glGetDoublei_v];
  TRACE("(%d, %d, %p)\n", target, index, data );
  ENTER_GL();
  func_glGetDoublei_v( target, index, data );
  LEAVE_GL();
}

static void WINAPI wine_glGetFenceivNV( GLuint fence, GLenum pname, GLint* params ) {
  void (*func_glGetFenceivNV)( GLuint, GLenum, GLint* ) = extension_funcs[EXT_glGetFenceivNV];
  TRACE("(%d, %d, %p)\n", fence, pname, params );
  ENTER_GL();
  func_glGetFenceivNV( fence, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetFinalCombinerInputParameterfvNV( GLenum variable, GLenum pname, GLfloat* params ) {
  void (*func_glGetFinalCombinerInputParameterfvNV)( GLenum, GLenum, GLfloat* ) = extension_funcs[EXT_glGetFinalCombinerInputParameterfvNV];
  TRACE("(%d, %d, %p)\n", variable, pname, params );
  ENTER_GL();
  func_glGetFinalCombinerInputParameterfvNV( variable, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetFinalCombinerInputParameterivNV( GLenum variable, GLenum pname, GLint* params ) {
  void (*func_glGetFinalCombinerInputParameterivNV)( GLenum, GLenum, GLint* ) = extension_funcs[EXT_glGetFinalCombinerInputParameterivNV];
  TRACE("(%d, %d, %p)\n", variable, pname, params );
  ENTER_GL();
  func_glGetFinalCombinerInputParameterivNV( variable, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetFloatIndexedvEXT( GLenum target, GLuint index, GLfloat* data ) {
  void (*func_glGetFloatIndexedvEXT)( GLenum, GLuint, GLfloat* ) = extension_funcs[EXT_glGetFloatIndexedvEXT];
  TRACE("(%d, %d, %p)\n", target, index, data );
  ENTER_GL();
  func_glGetFloatIndexedvEXT( target, index, data );
  LEAVE_GL();
}

static void WINAPI wine_glGetFloati_v( GLenum target, GLuint index, GLfloat* data ) {
  void (*func_glGetFloati_v)( GLenum, GLuint, GLfloat* ) = extension_funcs[EXT_glGetFloati_v];
  TRACE("(%d, %d, %p)\n", target, index, data );
  ENTER_GL();
  func_glGetFloati_v( target, index, data );
  LEAVE_GL();
}

static void WINAPI wine_glGetFogFuncSGIS( GLfloat* points ) {
  void (*func_glGetFogFuncSGIS)( GLfloat* ) = extension_funcs[EXT_glGetFogFuncSGIS];
  TRACE("(%p)\n", points );
  ENTER_GL();
  func_glGetFogFuncSGIS( points );
  LEAVE_GL();
}

static GLint WINAPI wine_glGetFragDataIndex( GLuint program, char* name ) {
  GLint ret_value;
  GLint (*func_glGetFragDataIndex)( GLuint, char* ) = extension_funcs[EXT_glGetFragDataIndex];
  TRACE("(%d, %p)\n", program, name );
  ENTER_GL();
  ret_value = func_glGetFragDataIndex( program, name );
  LEAVE_GL();
  return ret_value;
}

static GLint WINAPI wine_glGetFragDataLocation( GLuint program, char* name ) {
  GLint ret_value;
  GLint (*func_glGetFragDataLocation)( GLuint, char* ) = extension_funcs[EXT_glGetFragDataLocation];
  TRACE("(%d, %p)\n", program, name );
  ENTER_GL();
  ret_value = func_glGetFragDataLocation( program, name );
  LEAVE_GL();
  return ret_value;
}

static GLint WINAPI wine_glGetFragDataLocationEXT( GLuint program, char* name ) {
  GLint ret_value;
  GLint (*func_glGetFragDataLocationEXT)( GLuint, char* ) = extension_funcs[EXT_glGetFragDataLocationEXT];
  TRACE("(%d, %p)\n", program, name );
  ENTER_GL();
  ret_value = func_glGetFragDataLocationEXT( program, name );
  LEAVE_GL();
  return ret_value;
}

static void WINAPI wine_glGetFragmentLightfvSGIX( GLenum light, GLenum pname, GLfloat* params ) {
  void (*func_glGetFragmentLightfvSGIX)( GLenum, GLenum, GLfloat* ) = extension_funcs[EXT_glGetFragmentLightfvSGIX];
  TRACE("(%d, %d, %p)\n", light, pname, params );
  ENTER_GL();
  func_glGetFragmentLightfvSGIX( light, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetFragmentLightivSGIX( GLenum light, GLenum pname, GLint* params ) {
  void (*func_glGetFragmentLightivSGIX)( GLenum, GLenum, GLint* ) = extension_funcs[EXT_glGetFragmentLightivSGIX];
  TRACE("(%d, %d, %p)\n", light, pname, params );
  ENTER_GL();
  func_glGetFragmentLightivSGIX( light, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetFragmentMaterialfvSGIX( GLenum face, GLenum pname, GLfloat* params ) {
  void (*func_glGetFragmentMaterialfvSGIX)( GLenum, GLenum, GLfloat* ) = extension_funcs[EXT_glGetFragmentMaterialfvSGIX];
  TRACE("(%d, %d, %p)\n", face, pname, params );
  ENTER_GL();
  func_glGetFragmentMaterialfvSGIX( face, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetFragmentMaterialivSGIX( GLenum face, GLenum pname, GLint* params ) {
  void (*func_glGetFragmentMaterialivSGIX)( GLenum, GLenum, GLint* ) = extension_funcs[EXT_glGetFragmentMaterialivSGIX];
  TRACE("(%d, %d, %p)\n", face, pname, params );
  ENTER_GL();
  func_glGetFragmentMaterialivSGIX( face, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetFramebufferAttachmentParameteriv( GLenum target, GLenum attachment, GLenum pname, GLint* params ) {
  void (*func_glGetFramebufferAttachmentParameteriv)( GLenum, GLenum, GLenum, GLint* ) = extension_funcs[EXT_glGetFramebufferAttachmentParameteriv];
  TRACE("(%d, %d, %d, %p)\n", target, attachment, pname, params );
  ENTER_GL();
  func_glGetFramebufferAttachmentParameteriv( target, attachment, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetFramebufferAttachmentParameterivEXT( GLenum target, GLenum attachment, GLenum pname, GLint* params ) {
  void (*func_glGetFramebufferAttachmentParameterivEXT)( GLenum, GLenum, GLenum, GLint* ) = extension_funcs[EXT_glGetFramebufferAttachmentParameterivEXT];
  TRACE("(%d, %d, %d, %p)\n", target, attachment, pname, params );
  ENTER_GL();
  func_glGetFramebufferAttachmentParameterivEXT( target, attachment, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetFramebufferParameterivEXT( GLuint framebuffer, GLenum pname, GLint* params ) {
  void (*func_glGetFramebufferParameterivEXT)( GLuint, GLenum, GLint* ) = extension_funcs[EXT_glGetFramebufferParameterivEXT];
  TRACE("(%d, %d, %p)\n", framebuffer, pname, params );
  ENTER_GL();
  func_glGetFramebufferParameterivEXT( framebuffer, pname, params );
  LEAVE_GL();
}

static GLenum WINAPI wine_glGetGraphicsResetStatusARB( void ) {
  GLenum ret_value;
  GLenum (*func_glGetGraphicsResetStatusARB)( void ) = extension_funcs[EXT_glGetGraphicsResetStatusARB];
  TRACE("()\n");
  ENTER_GL();
  ret_value = func_glGetGraphicsResetStatusARB( );
  LEAVE_GL();
  return ret_value;
}

static unsigned int WINAPI wine_glGetHandleARB( GLenum pname ) {
  unsigned int ret_value;
  unsigned int (*func_glGetHandleARB)( GLenum ) = extension_funcs[EXT_glGetHandleARB];
  TRACE("(%d)\n", pname );
  ENTER_GL();
  ret_value = func_glGetHandleARB( pname );
  LEAVE_GL();
  return ret_value;
}

static void WINAPI wine_glGetHistogram( GLenum target, GLboolean reset, GLenum format, GLenum type, GLvoid* values ) {
  void (*func_glGetHistogram)( GLenum, GLboolean, GLenum, GLenum, GLvoid* ) = extension_funcs[EXT_glGetHistogram];
  TRACE("(%d, %d, %d, %d, %p)\n", target, reset, format, type, values );
  ENTER_GL();
  func_glGetHistogram( target, reset, format, type, values );
  LEAVE_GL();
}

static void WINAPI wine_glGetHistogramEXT( GLenum target, GLboolean reset, GLenum format, GLenum type, GLvoid* values ) {
  void (*func_glGetHistogramEXT)( GLenum, GLboolean, GLenum, GLenum, GLvoid* ) = extension_funcs[EXT_glGetHistogramEXT];
  TRACE("(%d, %d, %d, %d, %p)\n", target, reset, format, type, values );
  ENTER_GL();
  func_glGetHistogramEXT( target, reset, format, type, values );
  LEAVE_GL();
}

static void WINAPI wine_glGetHistogramParameterfv( GLenum target, GLenum pname, GLfloat* params ) {
  void (*func_glGetHistogramParameterfv)( GLenum, GLenum, GLfloat* ) = extension_funcs[EXT_glGetHistogramParameterfv];
  TRACE("(%d, %d, %p)\n", target, pname, params );
  ENTER_GL();
  func_glGetHistogramParameterfv( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetHistogramParameterfvEXT( GLenum target, GLenum pname, GLfloat* params ) {
  void (*func_glGetHistogramParameterfvEXT)( GLenum, GLenum, GLfloat* ) = extension_funcs[EXT_glGetHistogramParameterfvEXT];
  TRACE("(%d, %d, %p)\n", target, pname, params );
  ENTER_GL();
  func_glGetHistogramParameterfvEXT( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetHistogramParameteriv( GLenum target, GLenum pname, GLint* params ) {
  void (*func_glGetHistogramParameteriv)( GLenum, GLenum, GLint* ) = extension_funcs[EXT_glGetHistogramParameteriv];
  TRACE("(%d, %d, %p)\n", target, pname, params );
  ENTER_GL();
  func_glGetHistogramParameteriv( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetHistogramParameterivEXT( GLenum target, GLenum pname, GLint* params ) {
  void (*func_glGetHistogramParameterivEXT)( GLenum, GLenum, GLint* ) = extension_funcs[EXT_glGetHistogramParameterivEXT];
  TRACE("(%d, %d, %p)\n", target, pname, params );
  ENTER_GL();
  func_glGetHistogramParameterivEXT( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetImageTransformParameterfvHP( GLenum target, GLenum pname, GLfloat* params ) {
  void (*func_glGetImageTransformParameterfvHP)( GLenum, GLenum, GLfloat* ) = extension_funcs[EXT_glGetImageTransformParameterfvHP];
  TRACE("(%d, %d, %p)\n", target, pname, params );
  ENTER_GL();
  func_glGetImageTransformParameterfvHP( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetImageTransformParameterivHP( GLenum target, GLenum pname, GLint* params ) {
  void (*func_glGetImageTransformParameterivHP)( GLenum, GLenum, GLint* ) = extension_funcs[EXT_glGetImageTransformParameterivHP];
  TRACE("(%d, %d, %p)\n", target, pname, params );
  ENTER_GL();
  func_glGetImageTransformParameterivHP( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetInfoLogARB( unsigned int obj, GLsizei maxLength, GLsizei* length, char* infoLog ) {
  void (*func_glGetInfoLogARB)( unsigned int, GLsizei, GLsizei*, char* ) = extension_funcs[EXT_glGetInfoLogARB];
  TRACE("(%d, %d, %p, %p)\n", obj, maxLength, length, infoLog );
  ENTER_GL();
  func_glGetInfoLogARB( obj, maxLength, length, infoLog );
  LEAVE_GL();
}

static GLint WINAPI wine_glGetInstrumentsSGIX( void ) {
  GLint ret_value;
  GLint (*func_glGetInstrumentsSGIX)( void ) = extension_funcs[EXT_glGetInstrumentsSGIX];
  TRACE("()\n");
  ENTER_GL();
  ret_value = func_glGetInstrumentsSGIX( );
  LEAVE_GL();
  return ret_value;
}

static void WINAPI wine_glGetInteger64i_v( GLenum target, GLuint index, INT64* data ) {
  void (*func_glGetInteger64i_v)( GLenum, GLuint, INT64* ) = extension_funcs[EXT_glGetInteger64i_v];
  TRACE("(%d, %d, %p)\n", target, index, data );
  ENTER_GL();
  func_glGetInteger64i_v( target, index, data );
  LEAVE_GL();
}

static void WINAPI wine_glGetInteger64v( GLenum pname, INT64* params ) {
  void (*func_glGetInteger64v)( GLenum, INT64* ) = extension_funcs[EXT_glGetInteger64v];
  TRACE("(%d, %p)\n", pname, params );
  ENTER_GL();
  func_glGetInteger64v( pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetIntegerIndexedvEXT( GLenum target, GLuint index, GLint* data ) {
  void (*func_glGetIntegerIndexedvEXT)( GLenum, GLuint, GLint* ) = extension_funcs[EXT_glGetIntegerIndexedvEXT];
  TRACE("(%d, %d, %p)\n", target, index, data );
  ENTER_GL();
  func_glGetIntegerIndexedvEXT( target, index, data );
  LEAVE_GL();
}

static void WINAPI wine_glGetIntegeri_v( GLenum target, GLuint index, GLint* data ) {
  void (*func_glGetIntegeri_v)( GLenum, GLuint, GLint* ) = extension_funcs[EXT_glGetIntegeri_v];
  TRACE("(%d, %d, %p)\n", target, index, data );
  ENTER_GL();
  func_glGetIntegeri_v( target, index, data );
  LEAVE_GL();
}

static void WINAPI wine_glGetIntegerui64i_vNV( GLenum value, GLuint index, UINT64* result ) {
  void (*func_glGetIntegerui64i_vNV)( GLenum, GLuint, UINT64* ) = extension_funcs[EXT_glGetIntegerui64i_vNV];
  TRACE("(%d, %d, %p)\n", value, index, result );
  ENTER_GL();
  func_glGetIntegerui64i_vNV( value, index, result );
  LEAVE_GL();
}

static void WINAPI wine_glGetIntegerui64vNV( GLenum value, UINT64* result ) {
  void (*func_glGetIntegerui64vNV)( GLenum, UINT64* ) = extension_funcs[EXT_glGetIntegerui64vNV];
  TRACE("(%d, %p)\n", value, result );
  ENTER_GL();
  func_glGetIntegerui64vNV( value, result );
  LEAVE_GL();
}

static void WINAPI wine_glGetInvariantBooleanvEXT( GLuint id, GLenum value, GLboolean* data ) {
  void (*func_glGetInvariantBooleanvEXT)( GLuint, GLenum, GLboolean* ) = extension_funcs[EXT_glGetInvariantBooleanvEXT];
  TRACE("(%d, %d, %p)\n", id, value, data );
  ENTER_GL();
  func_glGetInvariantBooleanvEXT( id, value, data );
  LEAVE_GL();
}

static void WINAPI wine_glGetInvariantFloatvEXT( GLuint id, GLenum value, GLfloat* data ) {
  void (*func_glGetInvariantFloatvEXT)( GLuint, GLenum, GLfloat* ) = extension_funcs[EXT_glGetInvariantFloatvEXT];
  TRACE("(%d, %d, %p)\n", id, value, data );
  ENTER_GL();
  func_glGetInvariantFloatvEXT( id, value, data );
  LEAVE_GL();
}

static void WINAPI wine_glGetInvariantIntegervEXT( GLuint id, GLenum value, GLint* data ) {
  void (*func_glGetInvariantIntegervEXT)( GLuint, GLenum, GLint* ) = extension_funcs[EXT_glGetInvariantIntegervEXT];
  TRACE("(%d, %d, %p)\n", id, value, data );
  ENTER_GL();
  func_glGetInvariantIntegervEXT( id, value, data );
  LEAVE_GL();
}

static void WINAPI wine_glGetListParameterfvSGIX( GLuint list, GLenum pname, GLfloat* params ) {
  void (*func_glGetListParameterfvSGIX)( GLuint, GLenum, GLfloat* ) = extension_funcs[EXT_glGetListParameterfvSGIX];
  TRACE("(%d, %d, %p)\n", list, pname, params );
  ENTER_GL();
  func_glGetListParameterfvSGIX( list, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetListParameterivSGIX( GLuint list, GLenum pname, GLint* params ) {
  void (*func_glGetListParameterivSGIX)( GLuint, GLenum, GLint* ) = extension_funcs[EXT_glGetListParameterivSGIX];
  TRACE("(%d, %d, %p)\n", list, pname, params );
  ENTER_GL();
  func_glGetListParameterivSGIX( list, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetLocalConstantBooleanvEXT( GLuint id, GLenum value, GLboolean* data ) {
  void (*func_glGetLocalConstantBooleanvEXT)( GLuint, GLenum, GLboolean* ) = extension_funcs[EXT_glGetLocalConstantBooleanvEXT];
  TRACE("(%d, %d, %p)\n", id, value, data );
  ENTER_GL();
  func_glGetLocalConstantBooleanvEXT( id, value, data );
  LEAVE_GL();
}

static void WINAPI wine_glGetLocalConstantFloatvEXT( GLuint id, GLenum value, GLfloat* data ) {
  void (*func_glGetLocalConstantFloatvEXT)( GLuint, GLenum, GLfloat* ) = extension_funcs[EXT_glGetLocalConstantFloatvEXT];
  TRACE("(%d, %d, %p)\n", id, value, data );
  ENTER_GL();
  func_glGetLocalConstantFloatvEXT( id, value, data );
  LEAVE_GL();
}

static void WINAPI wine_glGetLocalConstantIntegervEXT( GLuint id, GLenum value, GLint* data ) {
  void (*func_glGetLocalConstantIntegervEXT)( GLuint, GLenum, GLint* ) = extension_funcs[EXT_glGetLocalConstantIntegervEXT];
  TRACE("(%d, %d, %p)\n", id, value, data );
  ENTER_GL();
  func_glGetLocalConstantIntegervEXT( id, value, data );
  LEAVE_GL();
}

static void WINAPI wine_glGetMapAttribParameterfvNV( GLenum target, GLuint index, GLenum pname, GLfloat* params ) {
  void (*func_glGetMapAttribParameterfvNV)( GLenum, GLuint, GLenum, GLfloat* ) = extension_funcs[EXT_glGetMapAttribParameterfvNV];
  TRACE("(%d, %d, %d, %p)\n", target, index, pname, params );
  ENTER_GL();
  func_glGetMapAttribParameterfvNV( target, index, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetMapAttribParameterivNV( GLenum target, GLuint index, GLenum pname, GLint* params ) {
  void (*func_glGetMapAttribParameterivNV)( GLenum, GLuint, GLenum, GLint* ) = extension_funcs[EXT_glGetMapAttribParameterivNV];
  TRACE("(%d, %d, %d, %p)\n", target, index, pname, params );
  ENTER_GL();
  func_glGetMapAttribParameterivNV( target, index, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetMapControlPointsNV( GLenum target, GLuint index, GLenum type, GLsizei ustride, GLsizei vstride, GLboolean packed, GLvoid* points ) {
  void (*func_glGetMapControlPointsNV)( GLenum, GLuint, GLenum, GLsizei, GLsizei, GLboolean, GLvoid* ) = extension_funcs[EXT_glGetMapControlPointsNV];
  TRACE("(%d, %d, %d, %d, %d, %d, %p)\n", target, index, type, ustride, vstride, packed, points );
  ENTER_GL();
  func_glGetMapControlPointsNV( target, index, type, ustride, vstride, packed, points );
  LEAVE_GL();
}

static void WINAPI wine_glGetMapParameterfvNV( GLenum target, GLenum pname, GLfloat* params ) {
  void (*func_glGetMapParameterfvNV)( GLenum, GLenum, GLfloat* ) = extension_funcs[EXT_glGetMapParameterfvNV];
  TRACE("(%d, %d, %p)\n", target, pname, params );
  ENTER_GL();
  func_glGetMapParameterfvNV( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetMapParameterivNV( GLenum target, GLenum pname, GLint* params ) {
  void (*func_glGetMapParameterivNV)( GLenum, GLenum, GLint* ) = extension_funcs[EXT_glGetMapParameterivNV];
  TRACE("(%d, %d, %p)\n", target, pname, params );
  ENTER_GL();
  func_glGetMapParameterivNV( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetMinmax( GLenum target, GLboolean reset, GLenum format, GLenum type, GLvoid* values ) {
  void (*func_glGetMinmax)( GLenum, GLboolean, GLenum, GLenum, GLvoid* ) = extension_funcs[EXT_glGetMinmax];
  TRACE("(%d, %d, %d, %d, %p)\n", target, reset, format, type, values );
  ENTER_GL();
  func_glGetMinmax( target, reset, format, type, values );
  LEAVE_GL();
}

static void WINAPI wine_glGetMinmaxEXT( GLenum target, GLboolean reset, GLenum format, GLenum type, GLvoid* values ) {
  void (*func_glGetMinmaxEXT)( GLenum, GLboolean, GLenum, GLenum, GLvoid* ) = extension_funcs[EXT_glGetMinmaxEXT];
  TRACE("(%d, %d, %d, %d, %p)\n", target, reset, format, type, values );
  ENTER_GL();
  func_glGetMinmaxEXT( target, reset, format, type, values );
  LEAVE_GL();
}

static void WINAPI wine_glGetMinmaxParameterfv( GLenum target, GLenum pname, GLfloat* params ) {
  void (*func_glGetMinmaxParameterfv)( GLenum, GLenum, GLfloat* ) = extension_funcs[EXT_glGetMinmaxParameterfv];
  TRACE("(%d, %d, %p)\n", target, pname, params );
  ENTER_GL();
  func_glGetMinmaxParameterfv( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetMinmaxParameterfvEXT( GLenum target, GLenum pname, GLfloat* params ) {
  void (*func_glGetMinmaxParameterfvEXT)( GLenum, GLenum, GLfloat* ) = extension_funcs[EXT_glGetMinmaxParameterfvEXT];
  TRACE("(%d, %d, %p)\n", target, pname, params );
  ENTER_GL();
  func_glGetMinmaxParameterfvEXT( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetMinmaxParameteriv( GLenum target, GLenum pname, GLint* params ) {
  void (*func_glGetMinmaxParameteriv)( GLenum, GLenum, GLint* ) = extension_funcs[EXT_glGetMinmaxParameteriv];
  TRACE("(%d, %d, %p)\n", target, pname, params );
  ENTER_GL();
  func_glGetMinmaxParameteriv( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetMinmaxParameterivEXT( GLenum target, GLenum pname, GLint* params ) {
  void (*func_glGetMinmaxParameterivEXT)( GLenum, GLenum, GLint* ) = extension_funcs[EXT_glGetMinmaxParameterivEXT];
  TRACE("(%d, %d, %p)\n", target, pname, params );
  ENTER_GL();
  func_glGetMinmaxParameterivEXT( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetMultiTexEnvfvEXT( GLenum texunit, GLenum target, GLenum pname, GLfloat* params ) {
  void (*func_glGetMultiTexEnvfvEXT)( GLenum, GLenum, GLenum, GLfloat* ) = extension_funcs[EXT_glGetMultiTexEnvfvEXT];
  TRACE("(%d, %d, %d, %p)\n", texunit, target, pname, params );
  ENTER_GL();
  func_glGetMultiTexEnvfvEXT( texunit, target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetMultiTexEnvivEXT( GLenum texunit, GLenum target, GLenum pname, GLint* params ) {
  void (*func_glGetMultiTexEnvivEXT)( GLenum, GLenum, GLenum, GLint* ) = extension_funcs[EXT_glGetMultiTexEnvivEXT];
  TRACE("(%d, %d, %d, %p)\n", texunit, target, pname, params );
  ENTER_GL();
  func_glGetMultiTexEnvivEXT( texunit, target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetMultiTexGendvEXT( GLenum texunit, GLenum coord, GLenum pname, GLdouble* params ) {
  void (*func_glGetMultiTexGendvEXT)( GLenum, GLenum, GLenum, GLdouble* ) = extension_funcs[EXT_glGetMultiTexGendvEXT];
  TRACE("(%d, %d, %d, %p)\n", texunit, coord, pname, params );
  ENTER_GL();
  func_glGetMultiTexGendvEXT( texunit, coord, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetMultiTexGenfvEXT( GLenum texunit, GLenum coord, GLenum pname, GLfloat* params ) {
  void (*func_glGetMultiTexGenfvEXT)( GLenum, GLenum, GLenum, GLfloat* ) = extension_funcs[EXT_glGetMultiTexGenfvEXT];
  TRACE("(%d, %d, %d, %p)\n", texunit, coord, pname, params );
  ENTER_GL();
  func_glGetMultiTexGenfvEXT( texunit, coord, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetMultiTexGenivEXT( GLenum texunit, GLenum coord, GLenum pname, GLint* params ) {
  void (*func_glGetMultiTexGenivEXT)( GLenum, GLenum, GLenum, GLint* ) = extension_funcs[EXT_glGetMultiTexGenivEXT];
  TRACE("(%d, %d, %d, %p)\n", texunit, coord, pname, params );
  ENTER_GL();
  func_glGetMultiTexGenivEXT( texunit, coord, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetMultiTexImageEXT( GLenum texunit, GLenum target, GLint level, GLenum format, GLenum type, GLvoid* pixels ) {
  void (*func_glGetMultiTexImageEXT)( GLenum, GLenum, GLint, GLenum, GLenum, GLvoid* ) = extension_funcs[EXT_glGetMultiTexImageEXT];
  TRACE("(%d, %d, %d, %d, %d, %p)\n", texunit, target, level, format, type, pixels );
  ENTER_GL();
  func_glGetMultiTexImageEXT( texunit, target, level, format, type, pixels );
  LEAVE_GL();
}

static void WINAPI wine_glGetMultiTexLevelParameterfvEXT( GLenum texunit, GLenum target, GLint level, GLenum pname, GLfloat* params ) {
  void (*func_glGetMultiTexLevelParameterfvEXT)( GLenum, GLenum, GLint, GLenum, GLfloat* ) = extension_funcs[EXT_glGetMultiTexLevelParameterfvEXT];
  TRACE("(%d, %d, %d, %d, %p)\n", texunit, target, level, pname, params );
  ENTER_GL();
  func_glGetMultiTexLevelParameterfvEXT( texunit, target, level, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetMultiTexLevelParameterivEXT( GLenum texunit, GLenum target, GLint level, GLenum pname, GLint* params ) {
  void (*func_glGetMultiTexLevelParameterivEXT)( GLenum, GLenum, GLint, GLenum, GLint* ) = extension_funcs[EXT_glGetMultiTexLevelParameterivEXT];
  TRACE("(%d, %d, %d, %d, %p)\n", texunit, target, level, pname, params );
  ENTER_GL();
  func_glGetMultiTexLevelParameterivEXT( texunit, target, level, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetMultiTexParameterIivEXT( GLenum texunit, GLenum target, GLenum pname, GLint* params ) {
  void (*func_glGetMultiTexParameterIivEXT)( GLenum, GLenum, GLenum, GLint* ) = extension_funcs[EXT_glGetMultiTexParameterIivEXT];
  TRACE("(%d, %d, %d, %p)\n", texunit, target, pname, params );
  ENTER_GL();
  func_glGetMultiTexParameterIivEXT( texunit, target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetMultiTexParameterIuivEXT( GLenum texunit, GLenum target, GLenum pname, GLuint* params ) {
  void (*func_glGetMultiTexParameterIuivEXT)( GLenum, GLenum, GLenum, GLuint* ) = extension_funcs[EXT_glGetMultiTexParameterIuivEXT];
  TRACE("(%d, %d, %d, %p)\n", texunit, target, pname, params );
  ENTER_GL();
  func_glGetMultiTexParameterIuivEXT( texunit, target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetMultiTexParameterfvEXT( GLenum texunit, GLenum target, GLenum pname, GLfloat* params ) {
  void (*func_glGetMultiTexParameterfvEXT)( GLenum, GLenum, GLenum, GLfloat* ) = extension_funcs[EXT_glGetMultiTexParameterfvEXT];
  TRACE("(%d, %d, %d, %p)\n", texunit, target, pname, params );
  ENTER_GL();
  func_glGetMultiTexParameterfvEXT( texunit, target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetMultiTexParameterivEXT( GLenum texunit, GLenum target, GLenum pname, GLint* params ) {
  void (*func_glGetMultiTexParameterivEXT)( GLenum, GLenum, GLenum, GLint* ) = extension_funcs[EXT_glGetMultiTexParameterivEXT];
  TRACE("(%d, %d, %d, %p)\n", texunit, target, pname, params );
  ENTER_GL();
  func_glGetMultiTexParameterivEXT( texunit, target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetMultisamplefv( GLenum pname, GLuint index, GLfloat* val ) {
  void (*func_glGetMultisamplefv)( GLenum, GLuint, GLfloat* ) = extension_funcs[EXT_glGetMultisamplefv];
  TRACE("(%d, %d, %p)\n", pname, index, val );
  ENTER_GL();
  func_glGetMultisamplefv( pname, index, val );
  LEAVE_GL();
}

static void WINAPI wine_glGetMultisamplefvNV( GLenum pname, GLuint index, GLfloat* val ) {
  void (*func_glGetMultisamplefvNV)( GLenum, GLuint, GLfloat* ) = extension_funcs[EXT_glGetMultisamplefvNV];
  TRACE("(%d, %d, %p)\n", pname, index, val );
  ENTER_GL();
  func_glGetMultisamplefvNV( pname, index, val );
  LEAVE_GL();
}

static void WINAPI wine_glGetNamedBufferParameterivEXT( GLuint buffer, GLenum pname, GLint* params ) {
  void (*func_glGetNamedBufferParameterivEXT)( GLuint, GLenum, GLint* ) = extension_funcs[EXT_glGetNamedBufferParameterivEXT];
  TRACE("(%d, %d, %p)\n", buffer, pname, params );
  ENTER_GL();
  func_glGetNamedBufferParameterivEXT( buffer, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetNamedBufferParameterui64vNV( GLuint buffer, GLenum pname, UINT64* params ) {
  void (*func_glGetNamedBufferParameterui64vNV)( GLuint, GLenum, UINT64* ) = extension_funcs[EXT_glGetNamedBufferParameterui64vNV];
  TRACE("(%d, %d, %p)\n", buffer, pname, params );
  ENTER_GL();
  func_glGetNamedBufferParameterui64vNV( buffer, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetNamedBufferPointervEXT( GLuint buffer, GLenum pname, GLvoid** params ) {
  void (*func_glGetNamedBufferPointervEXT)( GLuint, GLenum, GLvoid** ) = extension_funcs[EXT_glGetNamedBufferPointervEXT];
  TRACE("(%d, %d, %p)\n", buffer, pname, params );
  ENTER_GL();
  func_glGetNamedBufferPointervEXT( buffer, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetNamedBufferSubDataEXT( GLuint buffer, INT_PTR offset, INT_PTR size, GLvoid* data ) {
  void (*func_glGetNamedBufferSubDataEXT)( GLuint, INT_PTR, INT_PTR, GLvoid* ) = extension_funcs[EXT_glGetNamedBufferSubDataEXT];
  TRACE("(%d, %ld, %ld, %p)\n", buffer, offset, size, data );
  ENTER_GL();
  func_glGetNamedBufferSubDataEXT( buffer, offset, size, data );
  LEAVE_GL();
}

static void WINAPI wine_glGetNamedFramebufferAttachmentParameterivEXT( GLuint framebuffer, GLenum attachment, GLenum pname, GLint* params ) {
  void (*func_glGetNamedFramebufferAttachmentParameterivEXT)( GLuint, GLenum, GLenum, GLint* ) = extension_funcs[EXT_glGetNamedFramebufferAttachmentParameterivEXT];
  TRACE("(%d, %d, %d, %p)\n", framebuffer, attachment, pname, params );
  ENTER_GL();
  func_glGetNamedFramebufferAttachmentParameterivEXT( framebuffer, attachment, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetNamedProgramLocalParameterIivEXT( GLuint program, GLenum target, GLuint index, GLint* params ) {
  void (*func_glGetNamedProgramLocalParameterIivEXT)( GLuint, GLenum, GLuint, GLint* ) = extension_funcs[EXT_glGetNamedProgramLocalParameterIivEXT];
  TRACE("(%d, %d, %d, %p)\n", program, target, index, params );
  ENTER_GL();
  func_glGetNamedProgramLocalParameterIivEXT( program, target, index, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetNamedProgramLocalParameterIuivEXT( GLuint program, GLenum target, GLuint index, GLuint* params ) {
  void (*func_glGetNamedProgramLocalParameterIuivEXT)( GLuint, GLenum, GLuint, GLuint* ) = extension_funcs[EXT_glGetNamedProgramLocalParameterIuivEXT];
  TRACE("(%d, %d, %d, %p)\n", program, target, index, params );
  ENTER_GL();
  func_glGetNamedProgramLocalParameterIuivEXT( program, target, index, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetNamedProgramLocalParameterdvEXT( GLuint program, GLenum target, GLuint index, GLdouble* params ) {
  void (*func_glGetNamedProgramLocalParameterdvEXT)( GLuint, GLenum, GLuint, GLdouble* ) = extension_funcs[EXT_glGetNamedProgramLocalParameterdvEXT];
  TRACE("(%d, %d, %d, %p)\n", program, target, index, params );
  ENTER_GL();
  func_glGetNamedProgramLocalParameterdvEXT( program, target, index, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetNamedProgramLocalParameterfvEXT( GLuint program, GLenum target, GLuint index, GLfloat* params ) {
  void (*func_glGetNamedProgramLocalParameterfvEXT)( GLuint, GLenum, GLuint, GLfloat* ) = extension_funcs[EXT_glGetNamedProgramLocalParameterfvEXT];
  TRACE("(%d, %d, %d, %p)\n", program, target, index, params );
  ENTER_GL();
  func_glGetNamedProgramLocalParameterfvEXT( program, target, index, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetNamedProgramStringEXT( GLuint program, GLenum target, GLenum pname, GLvoid* string ) {
  void (*func_glGetNamedProgramStringEXT)( GLuint, GLenum, GLenum, GLvoid* ) = extension_funcs[EXT_glGetNamedProgramStringEXT];
  TRACE("(%d, %d, %d, %p)\n", program, target, pname, string );
  ENTER_GL();
  func_glGetNamedProgramStringEXT( program, target, pname, string );
  LEAVE_GL();
}

static void WINAPI wine_glGetNamedProgramivEXT( GLuint program, GLenum target, GLenum pname, GLint* params ) {
  void (*func_glGetNamedProgramivEXT)( GLuint, GLenum, GLenum, GLint* ) = extension_funcs[EXT_glGetNamedProgramivEXT];
  TRACE("(%d, %d, %d, %p)\n", program, target, pname, params );
  ENTER_GL();
  func_glGetNamedProgramivEXT( program, target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetNamedRenderbufferParameterivEXT( GLuint renderbuffer, GLenum pname, GLint* params ) {
  void (*func_glGetNamedRenderbufferParameterivEXT)( GLuint, GLenum, GLint* ) = extension_funcs[EXT_glGetNamedRenderbufferParameterivEXT];
  TRACE("(%d, %d, %p)\n", renderbuffer, pname, params );
  ENTER_GL();
  func_glGetNamedRenderbufferParameterivEXT( renderbuffer, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetNamedStringARB( GLint namelen, char* name, GLsizei bufSize, GLint* stringlen, char* string ) {
  void (*func_glGetNamedStringARB)( GLint, char*, GLsizei, GLint*, char* ) = extension_funcs[EXT_glGetNamedStringARB];
  TRACE("(%d, %p, %d, %p, %p)\n", namelen, name, bufSize, stringlen, string );
  ENTER_GL();
  func_glGetNamedStringARB( namelen, name, bufSize, stringlen, string );
  LEAVE_GL();
}

static void WINAPI wine_glGetNamedStringivARB( GLint namelen, char* name, GLenum pname, GLint* params ) {
  void (*func_glGetNamedStringivARB)( GLint, char*, GLenum, GLint* ) = extension_funcs[EXT_glGetNamedStringivARB];
  TRACE("(%d, %p, %d, %p)\n", namelen, name, pname, params );
  ENTER_GL();
  func_glGetNamedStringivARB( namelen, name, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetObjectBufferfvATI( GLuint buffer, GLenum pname, GLfloat* params ) {
  void (*func_glGetObjectBufferfvATI)( GLuint, GLenum, GLfloat* ) = extension_funcs[EXT_glGetObjectBufferfvATI];
  TRACE("(%d, %d, %p)\n", buffer, pname, params );
  ENTER_GL();
  func_glGetObjectBufferfvATI( buffer, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetObjectBufferivATI( GLuint buffer, GLenum pname, GLint* params ) {
  void (*func_glGetObjectBufferivATI)( GLuint, GLenum, GLint* ) = extension_funcs[EXT_glGetObjectBufferivATI];
  TRACE("(%d, %d, %p)\n", buffer, pname, params );
  ENTER_GL();
  func_glGetObjectBufferivATI( buffer, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetObjectParameterfvARB( unsigned int obj, GLenum pname, GLfloat* params ) {
  void (*func_glGetObjectParameterfvARB)( unsigned int, GLenum, GLfloat* ) = extension_funcs[EXT_glGetObjectParameterfvARB];
  TRACE("(%d, %d, %p)\n", obj, pname, params );
  ENTER_GL();
  func_glGetObjectParameterfvARB( obj, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetObjectParameterivAPPLE( GLenum objectType, GLuint name, GLenum pname, GLint* params ) {
  void (*func_glGetObjectParameterivAPPLE)( GLenum, GLuint, GLenum, GLint* ) = extension_funcs[EXT_glGetObjectParameterivAPPLE];
  TRACE("(%d, %d, %d, %p)\n", objectType, name, pname, params );
  ENTER_GL();
  func_glGetObjectParameterivAPPLE( objectType, name, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetObjectParameterivARB( unsigned int obj, GLenum pname, GLint* params ) {
  void (*func_glGetObjectParameterivARB)( unsigned int, GLenum, GLint* ) = extension_funcs[EXT_glGetObjectParameterivARB];
  TRACE("(%d, %d, %p)\n", obj, pname, params );
  ENTER_GL();
  func_glGetObjectParameterivARB( obj, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetOcclusionQueryivNV( GLuint id, GLenum pname, GLint* params ) {
  void (*func_glGetOcclusionQueryivNV)( GLuint, GLenum, GLint* ) = extension_funcs[EXT_glGetOcclusionQueryivNV];
  TRACE("(%d, %d, %p)\n", id, pname, params );
  ENTER_GL();
  func_glGetOcclusionQueryivNV( id, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetOcclusionQueryuivNV( GLuint id, GLenum pname, GLuint* params ) {
  void (*func_glGetOcclusionQueryuivNV)( GLuint, GLenum, GLuint* ) = extension_funcs[EXT_glGetOcclusionQueryuivNV];
  TRACE("(%d, %d, %p)\n", id, pname, params );
  ENTER_GL();
  func_glGetOcclusionQueryuivNV( id, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetPerfMonitorCounterDataAMD( GLuint monitor, GLenum pname, GLsizei dataSize, GLuint* data, GLint* bytesWritten ) {
  void (*func_glGetPerfMonitorCounterDataAMD)( GLuint, GLenum, GLsizei, GLuint*, GLint* ) = extension_funcs[EXT_glGetPerfMonitorCounterDataAMD];
  TRACE("(%d, %d, %d, %p, %p)\n", monitor, pname, dataSize, data, bytesWritten );
  ENTER_GL();
  func_glGetPerfMonitorCounterDataAMD( monitor, pname, dataSize, data, bytesWritten );
  LEAVE_GL();
}

static void WINAPI wine_glGetPerfMonitorCounterInfoAMD( GLuint group, GLuint counter, GLenum pname, GLvoid* data ) {
  void (*func_glGetPerfMonitorCounterInfoAMD)( GLuint, GLuint, GLenum, GLvoid* ) = extension_funcs[EXT_glGetPerfMonitorCounterInfoAMD];
  TRACE("(%d, %d, %d, %p)\n", group, counter, pname, data );
  ENTER_GL();
  func_glGetPerfMonitorCounterInfoAMD( group, counter, pname, data );
  LEAVE_GL();
}

static void WINAPI wine_glGetPerfMonitorCounterStringAMD( GLuint group, GLuint counter, GLsizei bufSize, GLsizei* length, char* counterString ) {
  void (*func_glGetPerfMonitorCounterStringAMD)( GLuint, GLuint, GLsizei, GLsizei*, char* ) = extension_funcs[EXT_glGetPerfMonitorCounterStringAMD];
  TRACE("(%d, %d, %d, %p, %p)\n", group, counter, bufSize, length, counterString );
  ENTER_GL();
  func_glGetPerfMonitorCounterStringAMD( group, counter, bufSize, length, counterString );
  LEAVE_GL();
}

static void WINAPI wine_glGetPerfMonitorCountersAMD( GLuint group, GLint* numCounters, GLint* maxActiveCounters, GLsizei counterSize, GLuint* counters ) {
  void (*func_glGetPerfMonitorCountersAMD)( GLuint, GLint*, GLint*, GLsizei, GLuint* ) = extension_funcs[EXT_glGetPerfMonitorCountersAMD];
  TRACE("(%d, %p, %p, %d, %p)\n", group, numCounters, maxActiveCounters, counterSize, counters );
  ENTER_GL();
  func_glGetPerfMonitorCountersAMD( group, numCounters, maxActiveCounters, counterSize, counters );
  LEAVE_GL();
}

static void WINAPI wine_glGetPerfMonitorGroupStringAMD( GLuint group, GLsizei bufSize, GLsizei* length, char* groupString ) {
  void (*func_glGetPerfMonitorGroupStringAMD)( GLuint, GLsizei, GLsizei*, char* ) = extension_funcs[EXT_glGetPerfMonitorGroupStringAMD];
  TRACE("(%d, %d, %p, %p)\n", group, bufSize, length, groupString );
  ENTER_GL();
  func_glGetPerfMonitorGroupStringAMD( group, bufSize, length, groupString );
  LEAVE_GL();
}

static void WINAPI wine_glGetPerfMonitorGroupsAMD( GLint* numGroups, GLsizei groupsSize, GLuint* groups ) {
  void (*func_glGetPerfMonitorGroupsAMD)( GLint*, GLsizei, GLuint* ) = extension_funcs[EXT_glGetPerfMonitorGroupsAMD];
  TRACE("(%p, %d, %p)\n", numGroups, groupsSize, groups );
  ENTER_GL();
  func_glGetPerfMonitorGroupsAMD( numGroups, groupsSize, groups );
  LEAVE_GL();
}

static void WINAPI wine_glGetPixelTexGenParameterfvSGIS( GLenum pname, GLfloat* params ) {
  void (*func_glGetPixelTexGenParameterfvSGIS)( GLenum, GLfloat* ) = extension_funcs[EXT_glGetPixelTexGenParameterfvSGIS];
  TRACE("(%d, %p)\n", pname, params );
  ENTER_GL();
  func_glGetPixelTexGenParameterfvSGIS( pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetPixelTexGenParameterivSGIS( GLenum pname, GLint* params ) {
  void (*func_glGetPixelTexGenParameterivSGIS)( GLenum, GLint* ) = extension_funcs[EXT_glGetPixelTexGenParameterivSGIS];
  TRACE("(%d, %p)\n", pname, params );
  ENTER_GL();
  func_glGetPixelTexGenParameterivSGIS( pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetPointerIndexedvEXT( GLenum target, GLuint index, GLvoid** data ) {
  void (*func_glGetPointerIndexedvEXT)( GLenum, GLuint, GLvoid** ) = extension_funcs[EXT_glGetPointerIndexedvEXT];
  TRACE("(%d, %d, %p)\n", target, index, data );
  ENTER_GL();
  func_glGetPointerIndexedvEXT( target, index, data );
  LEAVE_GL();
}

static void WINAPI wine_glGetPointervEXT( GLenum pname, GLvoid** params ) {
  void (*func_glGetPointervEXT)( GLenum, GLvoid** ) = extension_funcs[EXT_glGetPointervEXT];
  TRACE("(%d, %p)\n", pname, params );
  ENTER_GL();
  func_glGetPointervEXT( pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetProgramBinary( GLuint program, GLsizei bufSize, GLsizei* length, GLenum* binaryFormat, GLvoid* binary ) {
  void (*func_glGetProgramBinary)( GLuint, GLsizei, GLsizei*, GLenum*, GLvoid* ) = extension_funcs[EXT_glGetProgramBinary];
  TRACE("(%d, %d, %p, %p, %p)\n", program, bufSize, length, binaryFormat, binary );
  ENTER_GL();
  func_glGetProgramBinary( program, bufSize, length, binaryFormat, binary );
  LEAVE_GL();
}

static void WINAPI wine_glGetProgramEnvParameterIivNV( GLenum target, GLuint index, GLint* params ) {
  void (*func_glGetProgramEnvParameterIivNV)( GLenum, GLuint, GLint* ) = extension_funcs[EXT_glGetProgramEnvParameterIivNV];
  TRACE("(%d, %d, %p)\n", target, index, params );
  ENTER_GL();
  func_glGetProgramEnvParameterIivNV( target, index, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetProgramEnvParameterIuivNV( GLenum target, GLuint index, GLuint* params ) {
  void (*func_glGetProgramEnvParameterIuivNV)( GLenum, GLuint, GLuint* ) = extension_funcs[EXT_glGetProgramEnvParameterIuivNV];
  TRACE("(%d, %d, %p)\n", target, index, params );
  ENTER_GL();
  func_glGetProgramEnvParameterIuivNV( target, index, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetProgramEnvParameterdvARB( GLenum target, GLuint index, GLdouble* params ) {
  void (*func_glGetProgramEnvParameterdvARB)( GLenum, GLuint, GLdouble* ) = extension_funcs[EXT_glGetProgramEnvParameterdvARB];
  TRACE("(%d, %d, %p)\n", target, index, params );
  ENTER_GL();
  func_glGetProgramEnvParameterdvARB( target, index, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetProgramEnvParameterfvARB( GLenum target, GLuint index, GLfloat* params ) {
  void (*func_glGetProgramEnvParameterfvARB)( GLenum, GLuint, GLfloat* ) = extension_funcs[EXT_glGetProgramEnvParameterfvARB];
  TRACE("(%d, %d, %p)\n", target, index, params );
  ENTER_GL();
  func_glGetProgramEnvParameterfvARB( target, index, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetProgramInfoLog( GLuint program, GLsizei bufSize, GLsizei* length, char* infoLog ) {
  void (*func_glGetProgramInfoLog)( GLuint, GLsizei, GLsizei*, char* ) = extension_funcs[EXT_glGetProgramInfoLog];
  TRACE("(%d, %d, %p, %p)\n", program, bufSize, length, infoLog );
  ENTER_GL();
  func_glGetProgramInfoLog( program, bufSize, length, infoLog );
  LEAVE_GL();
}

static void WINAPI wine_glGetProgramLocalParameterIivNV( GLenum target, GLuint index, GLint* params ) {
  void (*func_glGetProgramLocalParameterIivNV)( GLenum, GLuint, GLint* ) = extension_funcs[EXT_glGetProgramLocalParameterIivNV];
  TRACE("(%d, %d, %p)\n", target, index, params );
  ENTER_GL();
  func_glGetProgramLocalParameterIivNV( target, index, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetProgramLocalParameterIuivNV( GLenum target, GLuint index, GLuint* params ) {
  void (*func_glGetProgramLocalParameterIuivNV)( GLenum, GLuint, GLuint* ) = extension_funcs[EXT_glGetProgramLocalParameterIuivNV];
  TRACE("(%d, %d, %p)\n", target, index, params );
  ENTER_GL();
  func_glGetProgramLocalParameterIuivNV( target, index, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetProgramLocalParameterdvARB( GLenum target, GLuint index, GLdouble* params ) {
  void (*func_glGetProgramLocalParameterdvARB)( GLenum, GLuint, GLdouble* ) = extension_funcs[EXT_glGetProgramLocalParameterdvARB];
  TRACE("(%d, %d, %p)\n", target, index, params );
  ENTER_GL();
  func_glGetProgramLocalParameterdvARB( target, index, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetProgramLocalParameterfvARB( GLenum target, GLuint index, GLfloat* params ) {
  void (*func_glGetProgramLocalParameterfvARB)( GLenum, GLuint, GLfloat* ) = extension_funcs[EXT_glGetProgramLocalParameterfvARB];
  TRACE("(%d, %d, %p)\n", target, index, params );
  ENTER_GL();
  func_glGetProgramLocalParameterfvARB( target, index, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetProgramNamedParameterdvNV( GLuint id, GLsizei len, GLubyte* name, GLdouble* params ) {
  void (*func_glGetProgramNamedParameterdvNV)( GLuint, GLsizei, GLubyte*, GLdouble* ) = extension_funcs[EXT_glGetProgramNamedParameterdvNV];
  TRACE("(%d, %d, %p, %p)\n", id, len, name, params );
  ENTER_GL();
  func_glGetProgramNamedParameterdvNV( id, len, name, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetProgramNamedParameterfvNV( GLuint id, GLsizei len, GLubyte* name, GLfloat* params ) {
  void (*func_glGetProgramNamedParameterfvNV)( GLuint, GLsizei, GLubyte*, GLfloat* ) = extension_funcs[EXT_glGetProgramNamedParameterfvNV];
  TRACE("(%d, %d, %p, %p)\n", id, len, name, params );
  ENTER_GL();
  func_glGetProgramNamedParameterfvNV( id, len, name, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetProgramParameterdvNV( GLenum target, GLuint index, GLenum pname, GLdouble* params ) {
  void (*func_glGetProgramParameterdvNV)( GLenum, GLuint, GLenum, GLdouble* ) = extension_funcs[EXT_glGetProgramParameterdvNV];
  TRACE("(%d, %d, %d, %p)\n", target, index, pname, params );
  ENTER_GL();
  func_glGetProgramParameterdvNV( target, index, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetProgramParameterfvNV( GLenum target, GLuint index, GLenum pname, GLfloat* params ) {
  void (*func_glGetProgramParameterfvNV)( GLenum, GLuint, GLenum, GLfloat* ) = extension_funcs[EXT_glGetProgramParameterfvNV];
  TRACE("(%d, %d, %d, %p)\n", target, index, pname, params );
  ENTER_GL();
  func_glGetProgramParameterfvNV( target, index, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetProgramPipelineInfoLog( GLuint pipeline, GLsizei bufSize, GLsizei* length, char* infoLog ) {
  void (*func_glGetProgramPipelineInfoLog)( GLuint, GLsizei, GLsizei*, char* ) = extension_funcs[EXT_glGetProgramPipelineInfoLog];
  TRACE("(%d, %d, %p, %p)\n", pipeline, bufSize, length, infoLog );
  ENTER_GL();
  func_glGetProgramPipelineInfoLog( pipeline, bufSize, length, infoLog );
  LEAVE_GL();
}

static void WINAPI wine_glGetProgramPipelineiv( GLuint pipeline, GLenum pname, GLint* params ) {
  void (*func_glGetProgramPipelineiv)( GLuint, GLenum, GLint* ) = extension_funcs[EXT_glGetProgramPipelineiv];
  TRACE("(%d, %d, %p)\n", pipeline, pname, params );
  ENTER_GL();
  func_glGetProgramPipelineiv( pipeline, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetProgramStageiv( GLuint program, GLenum shadertype, GLenum pname, GLint* values ) {
  void (*func_glGetProgramStageiv)( GLuint, GLenum, GLenum, GLint* ) = extension_funcs[EXT_glGetProgramStageiv];
  TRACE("(%d, %d, %d, %p)\n", program, shadertype, pname, values );
  ENTER_GL();
  func_glGetProgramStageiv( program, shadertype, pname, values );
  LEAVE_GL();
}

static void WINAPI wine_glGetProgramStringARB( GLenum target, GLenum pname, GLvoid* string ) {
  void (*func_glGetProgramStringARB)( GLenum, GLenum, GLvoid* ) = extension_funcs[EXT_glGetProgramStringARB];
  TRACE("(%d, %d, %p)\n", target, pname, string );
  ENTER_GL();
  func_glGetProgramStringARB( target, pname, string );
  LEAVE_GL();
}

static void WINAPI wine_glGetProgramStringNV( GLuint id, GLenum pname, GLubyte* program ) {
  void (*func_glGetProgramStringNV)( GLuint, GLenum, GLubyte* ) = extension_funcs[EXT_glGetProgramStringNV];
  TRACE("(%d, %d, %p)\n", id, pname, program );
  ENTER_GL();
  func_glGetProgramStringNV( id, pname, program );
  LEAVE_GL();
}

static void WINAPI wine_glGetProgramSubroutineParameteruivNV( GLenum target, GLuint index, GLuint* param ) {
  void (*func_glGetProgramSubroutineParameteruivNV)( GLenum, GLuint, GLuint* ) = extension_funcs[EXT_glGetProgramSubroutineParameteruivNV];
  TRACE("(%d, %d, %p)\n", target, index, param );
  ENTER_GL();
  func_glGetProgramSubroutineParameteruivNV( target, index, param );
  LEAVE_GL();
}

static void WINAPI wine_glGetProgramiv( GLuint program, GLenum pname, GLint* params ) {
  void (*func_glGetProgramiv)( GLuint, GLenum, GLint* ) = extension_funcs[EXT_glGetProgramiv];
  TRACE("(%d, %d, %p)\n", program, pname, params );
  ENTER_GL();
  func_glGetProgramiv( program, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetProgramivARB( GLenum target, GLenum pname, GLint* params ) {
  void (*func_glGetProgramivARB)( GLenum, GLenum, GLint* ) = extension_funcs[EXT_glGetProgramivARB];
  TRACE("(%d, %d, %p)\n", target, pname, params );
  ENTER_GL();
  func_glGetProgramivARB( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetProgramivNV( GLuint id, GLenum pname, GLint* params ) {
  void (*func_glGetProgramivNV)( GLuint, GLenum, GLint* ) = extension_funcs[EXT_glGetProgramivNV];
  TRACE("(%d, %d, %p)\n", id, pname, params );
  ENTER_GL();
  func_glGetProgramivNV( id, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetQueryIndexediv( GLenum target, GLuint index, GLenum pname, GLint* params ) {
  void (*func_glGetQueryIndexediv)( GLenum, GLuint, GLenum, GLint* ) = extension_funcs[EXT_glGetQueryIndexediv];
  TRACE("(%d, %d, %d, %p)\n", target, index, pname, params );
  ENTER_GL();
  func_glGetQueryIndexediv( target, index, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetQueryObjecti64v( GLuint id, GLenum pname, INT64* params ) {
  void (*func_glGetQueryObjecti64v)( GLuint, GLenum, INT64* ) = extension_funcs[EXT_glGetQueryObjecti64v];
  TRACE("(%d, %d, %p)\n", id, pname, params );
  ENTER_GL();
  func_glGetQueryObjecti64v( id, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetQueryObjecti64vEXT( GLuint id, GLenum pname, INT64* params ) {
  void (*func_glGetQueryObjecti64vEXT)( GLuint, GLenum, INT64* ) = extension_funcs[EXT_glGetQueryObjecti64vEXT];
  TRACE("(%d, %d, %p)\n", id, pname, params );
  ENTER_GL();
  func_glGetQueryObjecti64vEXT( id, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetQueryObjectiv( GLuint id, GLenum pname, GLint* params ) {
  void (*func_glGetQueryObjectiv)( GLuint, GLenum, GLint* ) = extension_funcs[EXT_glGetQueryObjectiv];
  TRACE("(%d, %d, %p)\n", id, pname, params );
  ENTER_GL();
  func_glGetQueryObjectiv( id, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetQueryObjectivARB( GLuint id, GLenum pname, GLint* params ) {
  void (*func_glGetQueryObjectivARB)( GLuint, GLenum, GLint* ) = extension_funcs[EXT_glGetQueryObjectivARB];
  TRACE("(%d, %d, %p)\n", id, pname, params );
  ENTER_GL();
  func_glGetQueryObjectivARB( id, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetQueryObjectui64v( GLuint id, GLenum pname, UINT64* params ) {
  void (*func_glGetQueryObjectui64v)( GLuint, GLenum, UINT64* ) = extension_funcs[EXT_glGetQueryObjectui64v];
  TRACE("(%d, %d, %p)\n", id, pname, params );
  ENTER_GL();
  func_glGetQueryObjectui64v( id, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetQueryObjectui64vEXT( GLuint id, GLenum pname, UINT64* params ) {
  void (*func_glGetQueryObjectui64vEXT)( GLuint, GLenum, UINT64* ) = extension_funcs[EXT_glGetQueryObjectui64vEXT];
  TRACE("(%d, %d, %p)\n", id, pname, params );
  ENTER_GL();
  func_glGetQueryObjectui64vEXT( id, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetQueryObjectuiv( GLuint id, GLenum pname, GLuint* params ) {
  void (*func_glGetQueryObjectuiv)( GLuint, GLenum, GLuint* ) = extension_funcs[EXT_glGetQueryObjectuiv];
  TRACE("(%d, %d, %p)\n", id, pname, params );
  ENTER_GL();
  func_glGetQueryObjectuiv( id, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetQueryObjectuivARB( GLuint id, GLenum pname, GLuint* params ) {
  void (*func_glGetQueryObjectuivARB)( GLuint, GLenum, GLuint* ) = extension_funcs[EXT_glGetQueryObjectuivARB];
  TRACE("(%d, %d, %p)\n", id, pname, params );
  ENTER_GL();
  func_glGetQueryObjectuivARB( id, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetQueryiv( GLenum target, GLenum pname, GLint* params ) {
  void (*func_glGetQueryiv)( GLenum, GLenum, GLint* ) = extension_funcs[EXT_glGetQueryiv];
  TRACE("(%d, %d, %p)\n", target, pname, params );
  ENTER_GL();
  func_glGetQueryiv( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetQueryivARB( GLenum target, GLenum pname, GLint* params ) {
  void (*func_glGetQueryivARB)( GLenum, GLenum, GLint* ) = extension_funcs[EXT_glGetQueryivARB];
  TRACE("(%d, %d, %p)\n", target, pname, params );
  ENTER_GL();
  func_glGetQueryivARB( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetRenderbufferParameteriv( GLenum target, GLenum pname, GLint* params ) {
  void (*func_glGetRenderbufferParameteriv)( GLenum, GLenum, GLint* ) = extension_funcs[EXT_glGetRenderbufferParameteriv];
  TRACE("(%d, %d, %p)\n", target, pname, params );
  ENTER_GL();
  func_glGetRenderbufferParameteriv( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetRenderbufferParameterivEXT( GLenum target, GLenum pname, GLint* params ) {
  void (*func_glGetRenderbufferParameterivEXT)( GLenum, GLenum, GLint* ) = extension_funcs[EXT_glGetRenderbufferParameterivEXT];
  TRACE("(%d, %d, %p)\n", target, pname, params );
  ENTER_GL();
  func_glGetRenderbufferParameterivEXT( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetSamplerParameterIiv( GLuint sampler, GLenum pname, GLint* params ) {
  void (*func_glGetSamplerParameterIiv)( GLuint, GLenum, GLint* ) = extension_funcs[EXT_glGetSamplerParameterIiv];
  TRACE("(%d, %d, %p)\n", sampler, pname, params );
  ENTER_GL();
  func_glGetSamplerParameterIiv( sampler, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetSamplerParameterIuiv( GLuint sampler, GLenum pname, GLuint* params ) {
  void (*func_glGetSamplerParameterIuiv)( GLuint, GLenum, GLuint* ) = extension_funcs[EXT_glGetSamplerParameterIuiv];
  TRACE("(%d, %d, %p)\n", sampler, pname, params );
  ENTER_GL();
  func_glGetSamplerParameterIuiv( sampler, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetSamplerParameterfv( GLuint sampler, GLenum pname, GLfloat* params ) {
  void (*func_glGetSamplerParameterfv)( GLuint, GLenum, GLfloat* ) = extension_funcs[EXT_glGetSamplerParameterfv];
  TRACE("(%d, %d, %p)\n", sampler, pname, params );
  ENTER_GL();
  func_glGetSamplerParameterfv( sampler, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetSamplerParameteriv( GLuint sampler, GLenum pname, GLint* params ) {
  void (*func_glGetSamplerParameteriv)( GLuint, GLenum, GLint* ) = extension_funcs[EXT_glGetSamplerParameteriv];
  TRACE("(%d, %d, %p)\n", sampler, pname, params );
  ENTER_GL();
  func_glGetSamplerParameteriv( sampler, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetSeparableFilter( GLenum target, GLenum format, GLenum type, GLvoid* row, GLvoid* column, GLvoid* span ) {
  void (*func_glGetSeparableFilter)( GLenum, GLenum, GLenum, GLvoid*, GLvoid*, GLvoid* ) = extension_funcs[EXT_glGetSeparableFilter];
  TRACE("(%d, %d, %d, %p, %p, %p)\n", target, format, type, row, column, span );
  ENTER_GL();
  func_glGetSeparableFilter( target, format, type, row, column, span );
  LEAVE_GL();
}

static void WINAPI wine_glGetSeparableFilterEXT( GLenum target, GLenum format, GLenum type, GLvoid* row, GLvoid* column, GLvoid* span ) {
  void (*func_glGetSeparableFilterEXT)( GLenum, GLenum, GLenum, GLvoid*, GLvoid*, GLvoid* ) = extension_funcs[EXT_glGetSeparableFilterEXT];
  TRACE("(%d, %d, %d, %p, %p, %p)\n", target, format, type, row, column, span );
  ENTER_GL();
  func_glGetSeparableFilterEXT( target, format, type, row, column, span );
  LEAVE_GL();
}

static void WINAPI wine_glGetShaderInfoLog( GLuint shader, GLsizei bufSize, GLsizei* length, char* infoLog ) {
  void (*func_glGetShaderInfoLog)( GLuint, GLsizei, GLsizei*, char* ) = extension_funcs[EXT_glGetShaderInfoLog];
  TRACE("(%d, %d, %p, %p)\n", shader, bufSize, length, infoLog );
  ENTER_GL();
  func_glGetShaderInfoLog( shader, bufSize, length, infoLog );
  LEAVE_GL();
}

static void WINAPI wine_glGetShaderPrecisionFormat( GLenum shadertype, GLenum precisiontype, GLint* range, GLint* precision ) {
  void (*func_glGetShaderPrecisionFormat)( GLenum, GLenum, GLint*, GLint* ) = extension_funcs[EXT_glGetShaderPrecisionFormat];
  TRACE("(%d, %d, %p, %p)\n", shadertype, precisiontype, range, precision );
  ENTER_GL();
  func_glGetShaderPrecisionFormat( shadertype, precisiontype, range, precision );
  LEAVE_GL();
}

static void WINAPI wine_glGetShaderSource( GLuint shader, GLsizei bufSize, GLsizei* length, char* source ) {
  void (*func_glGetShaderSource)( GLuint, GLsizei, GLsizei*, char* ) = extension_funcs[EXT_glGetShaderSource];
  TRACE("(%d, %d, %p, %p)\n", shader, bufSize, length, source );
  ENTER_GL();
  func_glGetShaderSource( shader, bufSize, length, source );
  LEAVE_GL();
}

static void WINAPI wine_glGetShaderSourceARB( unsigned int obj, GLsizei maxLength, GLsizei* length, char* source ) {
  void (*func_glGetShaderSourceARB)( unsigned int, GLsizei, GLsizei*, char* ) = extension_funcs[EXT_glGetShaderSourceARB];
  TRACE("(%d, %d, %p, %p)\n", obj, maxLength, length, source );
  ENTER_GL();
  func_glGetShaderSourceARB( obj, maxLength, length, source );
  LEAVE_GL();
}

static void WINAPI wine_glGetShaderiv( GLuint shader, GLenum pname, GLint* params ) {
  void (*func_glGetShaderiv)( GLuint, GLenum, GLint* ) = extension_funcs[EXT_glGetShaderiv];
  TRACE("(%d, %d, %p)\n", shader, pname, params );
  ENTER_GL();
  func_glGetShaderiv( shader, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetSharpenTexFuncSGIS( GLenum target, GLfloat* points ) {
  void (*func_glGetSharpenTexFuncSGIS)( GLenum, GLfloat* ) = extension_funcs[EXT_glGetSharpenTexFuncSGIS];
  TRACE("(%d, %p)\n", target, points );
  ENTER_GL();
  func_glGetSharpenTexFuncSGIS( target, points );
  LEAVE_GL();
}

static const GLubyte * WINAPI wine_glGetStringi( GLenum name, GLuint index ) {
  const GLubyte * ret_value;
  const GLubyte * (*func_glGetStringi)( GLenum, GLuint ) = extension_funcs[EXT_glGetStringi];
  TRACE("(%d, %d)\n", name, index );
  ENTER_GL();
  ret_value = func_glGetStringi( name, index );
  LEAVE_GL();
  return ret_value;
}

static GLuint WINAPI wine_glGetSubroutineIndex( GLuint program, GLenum shadertype, char* name ) {
  GLuint ret_value;
  GLuint (*func_glGetSubroutineIndex)( GLuint, GLenum, char* ) = extension_funcs[EXT_glGetSubroutineIndex];
  TRACE("(%d, %d, %p)\n", program, shadertype, name );
  ENTER_GL();
  ret_value = func_glGetSubroutineIndex( program, shadertype, name );
  LEAVE_GL();
  return ret_value;
}

static GLint WINAPI wine_glGetSubroutineUniformLocation( GLuint program, GLenum shadertype, char* name ) {
  GLint ret_value;
  GLint (*func_glGetSubroutineUniformLocation)( GLuint, GLenum, char* ) = extension_funcs[EXT_glGetSubroutineUniformLocation];
  TRACE("(%d, %d, %p)\n", program, shadertype, name );
  ENTER_GL();
  ret_value = func_glGetSubroutineUniformLocation( program, shadertype, name );
  LEAVE_GL();
  return ret_value;
}

static void WINAPI wine_glGetSynciv( GLvoid* sync, GLenum pname, GLsizei bufSize, GLsizei* length, GLint* values ) {
  void (*func_glGetSynciv)( GLvoid*, GLenum, GLsizei, GLsizei*, GLint* ) = extension_funcs[EXT_glGetSynciv];
  TRACE("(%p, %d, %d, %p, %p)\n", sync, pname, bufSize, length, values );
  ENTER_GL();
  func_glGetSynciv( sync, pname, bufSize, length, values );
  LEAVE_GL();
}

static void WINAPI wine_glGetTexBumpParameterfvATI( GLenum pname, GLfloat* param ) {
  void (*func_glGetTexBumpParameterfvATI)( GLenum, GLfloat* ) = extension_funcs[EXT_glGetTexBumpParameterfvATI];
  TRACE("(%d, %p)\n", pname, param );
  ENTER_GL();
  func_glGetTexBumpParameterfvATI( pname, param );
  LEAVE_GL();
}

static void WINAPI wine_glGetTexBumpParameterivATI( GLenum pname, GLint* param ) {
  void (*func_glGetTexBumpParameterivATI)( GLenum, GLint* ) = extension_funcs[EXT_glGetTexBumpParameterivATI];
  TRACE("(%d, %p)\n", pname, param );
  ENTER_GL();
  func_glGetTexBumpParameterivATI( pname, param );
  LEAVE_GL();
}

static void WINAPI wine_glGetTexFilterFuncSGIS( GLenum target, GLenum filter, GLfloat* weights ) {
  void (*func_glGetTexFilterFuncSGIS)( GLenum, GLenum, GLfloat* ) = extension_funcs[EXT_glGetTexFilterFuncSGIS];
  TRACE("(%d, %d, %p)\n", target, filter, weights );
  ENTER_GL();
  func_glGetTexFilterFuncSGIS( target, filter, weights );
  LEAVE_GL();
}

static void WINAPI wine_glGetTexParameterIiv( GLenum target, GLenum pname, GLint* params ) {
  void (*func_glGetTexParameterIiv)( GLenum, GLenum, GLint* ) = extension_funcs[EXT_glGetTexParameterIiv];
  TRACE("(%d, %d, %p)\n", target, pname, params );
  ENTER_GL();
  func_glGetTexParameterIiv( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetTexParameterIivEXT( GLenum target, GLenum pname, GLint* params ) {
  void (*func_glGetTexParameterIivEXT)( GLenum, GLenum, GLint* ) = extension_funcs[EXT_glGetTexParameterIivEXT];
  TRACE("(%d, %d, %p)\n", target, pname, params );
  ENTER_GL();
  func_glGetTexParameterIivEXT( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetTexParameterIuiv( GLenum target, GLenum pname, GLuint* params ) {
  void (*func_glGetTexParameterIuiv)( GLenum, GLenum, GLuint* ) = extension_funcs[EXT_glGetTexParameterIuiv];
  TRACE("(%d, %d, %p)\n", target, pname, params );
  ENTER_GL();
  func_glGetTexParameterIuiv( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetTexParameterIuivEXT( GLenum target, GLenum pname, GLuint* params ) {
  void (*func_glGetTexParameterIuivEXT)( GLenum, GLenum, GLuint* ) = extension_funcs[EXT_glGetTexParameterIuivEXT];
  TRACE("(%d, %d, %p)\n", target, pname, params );
  ENTER_GL();
  func_glGetTexParameterIuivEXT( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetTexParameterPointervAPPLE( GLenum target, GLenum pname, GLvoid** params ) {
  void (*func_glGetTexParameterPointervAPPLE)( GLenum, GLenum, GLvoid** ) = extension_funcs[EXT_glGetTexParameterPointervAPPLE];
  TRACE("(%d, %d, %p)\n", target, pname, params );
  ENTER_GL();
  func_glGetTexParameterPointervAPPLE( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetTextureImageEXT( GLuint texture, GLenum target, GLint level, GLenum format, GLenum type, GLvoid* pixels ) {
  void (*func_glGetTextureImageEXT)( GLuint, GLenum, GLint, GLenum, GLenum, GLvoid* ) = extension_funcs[EXT_glGetTextureImageEXT];
  TRACE("(%d, %d, %d, %d, %d, %p)\n", texture, target, level, format, type, pixels );
  ENTER_GL();
  func_glGetTextureImageEXT( texture, target, level, format, type, pixels );
  LEAVE_GL();
}

static void WINAPI wine_glGetTextureLevelParameterfvEXT( GLuint texture, GLenum target, GLint level, GLenum pname, GLfloat* params ) {
  void (*func_glGetTextureLevelParameterfvEXT)( GLuint, GLenum, GLint, GLenum, GLfloat* ) = extension_funcs[EXT_glGetTextureLevelParameterfvEXT];
  TRACE("(%d, %d, %d, %d, %p)\n", texture, target, level, pname, params );
  ENTER_GL();
  func_glGetTextureLevelParameterfvEXT( texture, target, level, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetTextureLevelParameterivEXT( GLuint texture, GLenum target, GLint level, GLenum pname, GLint* params ) {
  void (*func_glGetTextureLevelParameterivEXT)( GLuint, GLenum, GLint, GLenum, GLint* ) = extension_funcs[EXT_glGetTextureLevelParameterivEXT];
  TRACE("(%d, %d, %d, %d, %p)\n", texture, target, level, pname, params );
  ENTER_GL();
  func_glGetTextureLevelParameterivEXT( texture, target, level, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetTextureParameterIivEXT( GLuint texture, GLenum target, GLenum pname, GLint* params ) {
  void (*func_glGetTextureParameterIivEXT)( GLuint, GLenum, GLenum, GLint* ) = extension_funcs[EXT_glGetTextureParameterIivEXT];
  TRACE("(%d, %d, %d, %p)\n", texture, target, pname, params );
  ENTER_GL();
  func_glGetTextureParameterIivEXT( texture, target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetTextureParameterIuivEXT( GLuint texture, GLenum target, GLenum pname, GLuint* params ) {
  void (*func_glGetTextureParameterIuivEXT)( GLuint, GLenum, GLenum, GLuint* ) = extension_funcs[EXT_glGetTextureParameterIuivEXT];
  TRACE("(%d, %d, %d, %p)\n", texture, target, pname, params );
  ENTER_GL();
  func_glGetTextureParameterIuivEXT( texture, target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetTextureParameterfvEXT( GLuint texture, GLenum target, GLenum pname, GLfloat* params ) {
  void (*func_glGetTextureParameterfvEXT)( GLuint, GLenum, GLenum, GLfloat* ) = extension_funcs[EXT_glGetTextureParameterfvEXT];
  TRACE("(%d, %d, %d, %p)\n", texture, target, pname, params );
  ENTER_GL();
  func_glGetTextureParameterfvEXT( texture, target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetTextureParameterivEXT( GLuint texture, GLenum target, GLenum pname, GLint* params ) {
  void (*func_glGetTextureParameterivEXT)( GLuint, GLenum, GLenum, GLint* ) = extension_funcs[EXT_glGetTextureParameterivEXT];
  TRACE("(%d, %d, %d, %p)\n", texture, target, pname, params );
  ENTER_GL();
  func_glGetTextureParameterivEXT( texture, target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetTrackMatrixivNV( GLenum target, GLuint address, GLenum pname, GLint* params ) {
  void (*func_glGetTrackMatrixivNV)( GLenum, GLuint, GLenum, GLint* ) = extension_funcs[EXT_glGetTrackMatrixivNV];
  TRACE("(%d, %d, %d, %p)\n", target, address, pname, params );
  ENTER_GL();
  func_glGetTrackMatrixivNV( target, address, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetTransformFeedbackVarying( GLuint program, GLuint index, GLsizei bufSize, GLsizei* length, GLsizei* size, GLenum* type, char* name ) {
  void (*func_glGetTransformFeedbackVarying)( GLuint, GLuint, GLsizei, GLsizei*, GLsizei*, GLenum*, char* ) = extension_funcs[EXT_glGetTransformFeedbackVarying];
  TRACE("(%d, %d, %d, %p, %p, %p, %p)\n", program, index, bufSize, length, size, type, name );
  ENTER_GL();
  func_glGetTransformFeedbackVarying( program, index, bufSize, length, size, type, name );
  LEAVE_GL();
}

static void WINAPI wine_glGetTransformFeedbackVaryingEXT( GLuint program, GLuint index, GLsizei bufSize, GLsizei* length, GLsizei* size, GLenum* type, char* name ) {
  void (*func_glGetTransformFeedbackVaryingEXT)( GLuint, GLuint, GLsizei, GLsizei*, GLsizei*, GLenum*, char* ) = extension_funcs[EXT_glGetTransformFeedbackVaryingEXT];
  TRACE("(%d, %d, %d, %p, %p, %p, %p)\n", program, index, bufSize, length, size, type, name );
  ENTER_GL();
  func_glGetTransformFeedbackVaryingEXT( program, index, bufSize, length, size, type, name );
  LEAVE_GL();
}

static void WINAPI wine_glGetTransformFeedbackVaryingNV( GLuint program, GLuint index, GLint* location ) {
  void (*func_glGetTransformFeedbackVaryingNV)( GLuint, GLuint, GLint* ) = extension_funcs[EXT_glGetTransformFeedbackVaryingNV];
  TRACE("(%d, %d, %p)\n", program, index, location );
  ENTER_GL();
  func_glGetTransformFeedbackVaryingNV( program, index, location );
  LEAVE_GL();
}

static GLuint WINAPI wine_glGetUniformBlockIndex( GLuint program, char* uniformBlockName ) {
  GLuint ret_value;
  GLuint (*func_glGetUniformBlockIndex)( GLuint, char* ) = extension_funcs[EXT_glGetUniformBlockIndex];
  TRACE("(%d, %p)\n", program, uniformBlockName );
  ENTER_GL();
  ret_value = func_glGetUniformBlockIndex( program, uniformBlockName );
  LEAVE_GL();
  return ret_value;
}

static GLint WINAPI wine_glGetUniformBufferSizeEXT( GLuint program, GLint location ) {
  GLint ret_value;
  GLint (*func_glGetUniformBufferSizeEXT)( GLuint, GLint ) = extension_funcs[EXT_glGetUniformBufferSizeEXT];
  TRACE("(%d, %d)\n", program, location );
  ENTER_GL();
  ret_value = func_glGetUniformBufferSizeEXT( program, location );
  LEAVE_GL();
  return ret_value;
}

static void WINAPI wine_glGetUniformIndices( GLuint program, GLsizei uniformCount, char** uniformNames, GLuint* uniformIndices ) {
  void (*func_glGetUniformIndices)( GLuint, GLsizei, char**, GLuint* ) = extension_funcs[EXT_glGetUniformIndices];
  TRACE("(%d, %d, %p, %p)\n", program, uniformCount, uniformNames, uniformIndices );
  ENTER_GL();
  func_glGetUniformIndices( program, uniformCount, uniformNames, uniformIndices );
  LEAVE_GL();
}

static GLint WINAPI wine_glGetUniformLocation( GLuint program, char* name ) {
  GLint ret_value;
  GLint (*func_glGetUniformLocation)( GLuint, char* ) = extension_funcs[EXT_glGetUniformLocation];
  TRACE("(%d, %p)\n", program, name );
  ENTER_GL();
  ret_value = func_glGetUniformLocation( program, name );
  LEAVE_GL();
  return ret_value;
}

static GLint WINAPI wine_glGetUniformLocationARB( unsigned int programObj, char* name ) {
  GLint ret_value;
  GLint (*func_glGetUniformLocationARB)( unsigned int, char* ) = extension_funcs[EXT_glGetUniformLocationARB];
  TRACE("(%d, %p)\n", programObj, name );
  ENTER_GL();
  ret_value = func_glGetUniformLocationARB( programObj, name );
  LEAVE_GL();
  return ret_value;
}

static INT_PTR WINAPI wine_glGetUniformOffsetEXT( GLuint program, GLint location ) {
  INT_PTR ret_value;
  INT_PTR (*func_glGetUniformOffsetEXT)( GLuint, GLint ) = extension_funcs[EXT_glGetUniformOffsetEXT];
  TRACE("(%d, %d)\n", program, location );
  ENTER_GL();
  ret_value = func_glGetUniformOffsetEXT( program, location );
  LEAVE_GL();
  return ret_value;
}

static void WINAPI wine_glGetUniformSubroutineuiv( GLenum shadertype, GLint location, GLuint* params ) {
  void (*func_glGetUniformSubroutineuiv)( GLenum, GLint, GLuint* ) = extension_funcs[EXT_glGetUniformSubroutineuiv];
  TRACE("(%d, %d, %p)\n", shadertype, location, params );
  ENTER_GL();
  func_glGetUniformSubroutineuiv( shadertype, location, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetUniformdv( GLuint program, GLint location, GLdouble* params ) {
  void (*func_glGetUniformdv)( GLuint, GLint, GLdouble* ) = extension_funcs[EXT_glGetUniformdv];
  TRACE("(%d, %d, %p)\n", program, location, params );
  ENTER_GL();
  func_glGetUniformdv( program, location, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetUniformfv( GLuint program, GLint location, GLfloat* params ) {
  void (*func_glGetUniformfv)( GLuint, GLint, GLfloat* ) = extension_funcs[EXT_glGetUniformfv];
  TRACE("(%d, %d, %p)\n", program, location, params );
  ENTER_GL();
  func_glGetUniformfv( program, location, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetUniformfvARB( unsigned int programObj, GLint location, GLfloat* params ) {
  void (*func_glGetUniformfvARB)( unsigned int, GLint, GLfloat* ) = extension_funcs[EXT_glGetUniformfvARB];
  TRACE("(%d, %d, %p)\n", programObj, location, params );
  ENTER_GL();
  func_glGetUniformfvARB( programObj, location, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetUniformi64vNV( GLuint program, GLint location, INT64* params ) {
  void (*func_glGetUniformi64vNV)( GLuint, GLint, INT64* ) = extension_funcs[EXT_glGetUniformi64vNV];
  TRACE("(%d, %d, %p)\n", program, location, params );
  ENTER_GL();
  func_glGetUniformi64vNV( program, location, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetUniformiv( GLuint program, GLint location, GLint* params ) {
  void (*func_glGetUniformiv)( GLuint, GLint, GLint* ) = extension_funcs[EXT_glGetUniformiv];
  TRACE("(%d, %d, %p)\n", program, location, params );
  ENTER_GL();
  func_glGetUniformiv( program, location, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetUniformivARB( unsigned int programObj, GLint location, GLint* params ) {
  void (*func_glGetUniformivARB)( unsigned int, GLint, GLint* ) = extension_funcs[EXT_glGetUniformivARB];
  TRACE("(%d, %d, %p)\n", programObj, location, params );
  ENTER_GL();
  func_glGetUniformivARB( programObj, location, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetUniformui64vNV( GLuint program, GLint location, UINT64* params ) {
  void (*func_glGetUniformui64vNV)( GLuint, GLint, UINT64* ) = extension_funcs[EXT_glGetUniformui64vNV];
  TRACE("(%d, %d, %p)\n", program, location, params );
  ENTER_GL();
  func_glGetUniformui64vNV( program, location, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetUniformuiv( GLuint program, GLint location, GLuint* params ) {
  void (*func_glGetUniformuiv)( GLuint, GLint, GLuint* ) = extension_funcs[EXT_glGetUniformuiv];
  TRACE("(%d, %d, %p)\n", program, location, params );
  ENTER_GL();
  func_glGetUniformuiv( program, location, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetUniformuivEXT( GLuint program, GLint location, GLuint* params ) {
  void (*func_glGetUniformuivEXT)( GLuint, GLint, GLuint* ) = extension_funcs[EXT_glGetUniformuivEXT];
  TRACE("(%d, %d, %p)\n", program, location, params );
  ENTER_GL();
  func_glGetUniformuivEXT( program, location, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetVariantArrayObjectfvATI( GLuint id, GLenum pname, GLfloat* params ) {
  void (*func_glGetVariantArrayObjectfvATI)( GLuint, GLenum, GLfloat* ) = extension_funcs[EXT_glGetVariantArrayObjectfvATI];
  TRACE("(%d, %d, %p)\n", id, pname, params );
  ENTER_GL();
  func_glGetVariantArrayObjectfvATI( id, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetVariantArrayObjectivATI( GLuint id, GLenum pname, GLint* params ) {
  void (*func_glGetVariantArrayObjectivATI)( GLuint, GLenum, GLint* ) = extension_funcs[EXT_glGetVariantArrayObjectivATI];
  TRACE("(%d, %d, %p)\n", id, pname, params );
  ENTER_GL();
  func_glGetVariantArrayObjectivATI( id, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetVariantBooleanvEXT( GLuint id, GLenum value, GLboolean* data ) {
  void (*func_glGetVariantBooleanvEXT)( GLuint, GLenum, GLboolean* ) = extension_funcs[EXT_glGetVariantBooleanvEXT];
  TRACE("(%d, %d, %p)\n", id, value, data );
  ENTER_GL();
  func_glGetVariantBooleanvEXT( id, value, data );
  LEAVE_GL();
}

static void WINAPI wine_glGetVariantFloatvEXT( GLuint id, GLenum value, GLfloat* data ) {
  void (*func_glGetVariantFloatvEXT)( GLuint, GLenum, GLfloat* ) = extension_funcs[EXT_glGetVariantFloatvEXT];
  TRACE("(%d, %d, %p)\n", id, value, data );
  ENTER_GL();
  func_glGetVariantFloatvEXT( id, value, data );
  LEAVE_GL();
}

static void WINAPI wine_glGetVariantIntegervEXT( GLuint id, GLenum value, GLint* data ) {
  void (*func_glGetVariantIntegervEXT)( GLuint, GLenum, GLint* ) = extension_funcs[EXT_glGetVariantIntegervEXT];
  TRACE("(%d, %d, %p)\n", id, value, data );
  ENTER_GL();
  func_glGetVariantIntegervEXT( id, value, data );
  LEAVE_GL();
}

static void WINAPI wine_glGetVariantPointervEXT( GLuint id, GLenum value, GLvoid** data ) {
  void (*func_glGetVariantPointervEXT)( GLuint, GLenum, GLvoid** ) = extension_funcs[EXT_glGetVariantPointervEXT];
  TRACE("(%d, %d, %p)\n", id, value, data );
  ENTER_GL();
  func_glGetVariantPointervEXT( id, value, data );
  LEAVE_GL();
}

static GLint WINAPI wine_glGetVaryingLocationNV( GLuint program, char* name ) {
  GLint ret_value;
  GLint (*func_glGetVaryingLocationNV)( GLuint, char* ) = extension_funcs[EXT_glGetVaryingLocationNV];
  TRACE("(%d, %p)\n", program, name );
  ENTER_GL();
  ret_value = func_glGetVaryingLocationNV( program, name );
  LEAVE_GL();
  return ret_value;
}

static void WINAPI wine_glGetVertexAttribArrayObjectfvATI( GLuint index, GLenum pname, GLfloat* params ) {
  void (*func_glGetVertexAttribArrayObjectfvATI)( GLuint, GLenum, GLfloat* ) = extension_funcs[EXT_glGetVertexAttribArrayObjectfvATI];
  TRACE("(%d, %d, %p)\n", index, pname, params );
  ENTER_GL();
  func_glGetVertexAttribArrayObjectfvATI( index, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetVertexAttribArrayObjectivATI( GLuint index, GLenum pname, GLint* params ) {
  void (*func_glGetVertexAttribArrayObjectivATI)( GLuint, GLenum, GLint* ) = extension_funcs[EXT_glGetVertexAttribArrayObjectivATI];
  TRACE("(%d, %d, %p)\n", index, pname, params );
  ENTER_GL();
  func_glGetVertexAttribArrayObjectivATI( index, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetVertexAttribIiv( GLuint index, GLenum pname, GLint* params ) {
  void (*func_glGetVertexAttribIiv)( GLuint, GLenum, GLint* ) = extension_funcs[EXT_glGetVertexAttribIiv];
  TRACE("(%d, %d, %p)\n", index, pname, params );
  ENTER_GL();
  func_glGetVertexAttribIiv( index, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetVertexAttribIivEXT( GLuint index, GLenum pname, GLint* params ) {
  void (*func_glGetVertexAttribIivEXT)( GLuint, GLenum, GLint* ) = extension_funcs[EXT_glGetVertexAttribIivEXT];
  TRACE("(%d, %d, %p)\n", index, pname, params );
  ENTER_GL();
  func_glGetVertexAttribIivEXT( index, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetVertexAttribIuiv( GLuint index, GLenum pname, GLuint* params ) {
  void (*func_glGetVertexAttribIuiv)( GLuint, GLenum, GLuint* ) = extension_funcs[EXT_glGetVertexAttribIuiv];
  TRACE("(%d, %d, %p)\n", index, pname, params );
  ENTER_GL();
  func_glGetVertexAttribIuiv( index, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetVertexAttribIuivEXT( GLuint index, GLenum pname, GLuint* params ) {
  void (*func_glGetVertexAttribIuivEXT)( GLuint, GLenum, GLuint* ) = extension_funcs[EXT_glGetVertexAttribIuivEXT];
  TRACE("(%d, %d, %p)\n", index, pname, params );
  ENTER_GL();
  func_glGetVertexAttribIuivEXT( index, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetVertexAttribLdv( GLuint index, GLenum pname, GLdouble* params ) {
  void (*func_glGetVertexAttribLdv)( GLuint, GLenum, GLdouble* ) = extension_funcs[EXT_glGetVertexAttribLdv];
  TRACE("(%d, %d, %p)\n", index, pname, params );
  ENTER_GL();
  func_glGetVertexAttribLdv( index, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetVertexAttribLdvEXT( GLuint index, GLenum pname, GLdouble* params ) {
  void (*func_glGetVertexAttribLdvEXT)( GLuint, GLenum, GLdouble* ) = extension_funcs[EXT_glGetVertexAttribLdvEXT];
  TRACE("(%d, %d, %p)\n", index, pname, params );
  ENTER_GL();
  func_glGetVertexAttribLdvEXT( index, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetVertexAttribLi64vNV( GLuint index, GLenum pname, INT64* params ) {
  void (*func_glGetVertexAttribLi64vNV)( GLuint, GLenum, INT64* ) = extension_funcs[EXT_glGetVertexAttribLi64vNV];
  TRACE("(%d, %d, %p)\n", index, pname, params );
  ENTER_GL();
  func_glGetVertexAttribLi64vNV( index, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetVertexAttribLui64vNV( GLuint index, GLenum pname, UINT64* params ) {
  void (*func_glGetVertexAttribLui64vNV)( GLuint, GLenum, UINT64* ) = extension_funcs[EXT_glGetVertexAttribLui64vNV];
  TRACE("(%d, %d, %p)\n", index, pname, params );
  ENTER_GL();
  func_glGetVertexAttribLui64vNV( index, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetVertexAttribPointerv( GLuint index, GLenum pname, GLvoid** pointer ) {
  void (*func_glGetVertexAttribPointerv)( GLuint, GLenum, GLvoid** ) = extension_funcs[EXT_glGetVertexAttribPointerv];
  TRACE("(%d, %d, %p)\n", index, pname, pointer );
  ENTER_GL();
  func_glGetVertexAttribPointerv( index, pname, pointer );
  LEAVE_GL();
}

static void WINAPI wine_glGetVertexAttribPointervARB( GLuint index, GLenum pname, GLvoid** pointer ) {
  void (*func_glGetVertexAttribPointervARB)( GLuint, GLenum, GLvoid** ) = extension_funcs[EXT_glGetVertexAttribPointervARB];
  TRACE("(%d, %d, %p)\n", index, pname, pointer );
  ENTER_GL();
  func_glGetVertexAttribPointervARB( index, pname, pointer );
  LEAVE_GL();
}

static void WINAPI wine_glGetVertexAttribPointervNV( GLuint index, GLenum pname, GLvoid** pointer ) {
  void (*func_glGetVertexAttribPointervNV)( GLuint, GLenum, GLvoid** ) = extension_funcs[EXT_glGetVertexAttribPointervNV];
  TRACE("(%d, %d, %p)\n", index, pname, pointer );
  ENTER_GL();
  func_glGetVertexAttribPointervNV( index, pname, pointer );
  LEAVE_GL();
}

static void WINAPI wine_glGetVertexAttribdv( GLuint index, GLenum pname, GLdouble* params ) {
  void (*func_glGetVertexAttribdv)( GLuint, GLenum, GLdouble* ) = extension_funcs[EXT_glGetVertexAttribdv];
  TRACE("(%d, %d, %p)\n", index, pname, params );
  ENTER_GL();
  func_glGetVertexAttribdv( index, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetVertexAttribdvARB( GLuint index, GLenum pname, GLdouble* params ) {
  void (*func_glGetVertexAttribdvARB)( GLuint, GLenum, GLdouble* ) = extension_funcs[EXT_glGetVertexAttribdvARB];
  TRACE("(%d, %d, %p)\n", index, pname, params );
  ENTER_GL();
  func_glGetVertexAttribdvARB( index, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetVertexAttribdvNV( GLuint index, GLenum pname, GLdouble* params ) {
  void (*func_glGetVertexAttribdvNV)( GLuint, GLenum, GLdouble* ) = extension_funcs[EXT_glGetVertexAttribdvNV];
  TRACE("(%d, %d, %p)\n", index, pname, params );
  ENTER_GL();
  func_glGetVertexAttribdvNV( index, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetVertexAttribfv( GLuint index, GLenum pname, GLfloat* params ) {
  void (*func_glGetVertexAttribfv)( GLuint, GLenum, GLfloat* ) = extension_funcs[EXT_glGetVertexAttribfv];
  TRACE("(%d, %d, %p)\n", index, pname, params );
  ENTER_GL();
  func_glGetVertexAttribfv( index, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetVertexAttribfvARB( GLuint index, GLenum pname, GLfloat* params ) {
  void (*func_glGetVertexAttribfvARB)( GLuint, GLenum, GLfloat* ) = extension_funcs[EXT_glGetVertexAttribfvARB];
  TRACE("(%d, %d, %p)\n", index, pname, params );
  ENTER_GL();
  func_glGetVertexAttribfvARB( index, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetVertexAttribfvNV( GLuint index, GLenum pname, GLfloat* params ) {
  void (*func_glGetVertexAttribfvNV)( GLuint, GLenum, GLfloat* ) = extension_funcs[EXT_glGetVertexAttribfvNV];
  TRACE("(%d, %d, %p)\n", index, pname, params );
  ENTER_GL();
  func_glGetVertexAttribfvNV( index, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetVertexAttribiv( GLuint index, GLenum pname, GLint* params ) {
  void (*func_glGetVertexAttribiv)( GLuint, GLenum, GLint* ) = extension_funcs[EXT_glGetVertexAttribiv];
  TRACE("(%d, %d, %p)\n", index, pname, params );
  ENTER_GL();
  func_glGetVertexAttribiv( index, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetVertexAttribivARB( GLuint index, GLenum pname, GLint* params ) {
  void (*func_glGetVertexAttribivARB)( GLuint, GLenum, GLint* ) = extension_funcs[EXT_glGetVertexAttribivARB];
  TRACE("(%d, %d, %p)\n", index, pname, params );
  ENTER_GL();
  func_glGetVertexAttribivARB( index, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetVertexAttribivNV( GLuint index, GLenum pname, GLint* params ) {
  void (*func_glGetVertexAttribivNV)( GLuint, GLenum, GLint* ) = extension_funcs[EXT_glGetVertexAttribivNV];
  TRACE("(%d, %d, %p)\n", index, pname, params );
  ENTER_GL();
  func_glGetVertexAttribivNV( index, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetVideoCaptureStreamdvNV( GLuint video_capture_slot, GLuint stream, GLenum pname, GLdouble* params ) {
  void (*func_glGetVideoCaptureStreamdvNV)( GLuint, GLuint, GLenum, GLdouble* ) = extension_funcs[EXT_glGetVideoCaptureStreamdvNV];
  TRACE("(%d, %d, %d, %p)\n", video_capture_slot, stream, pname, params );
  ENTER_GL();
  func_glGetVideoCaptureStreamdvNV( video_capture_slot, stream, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetVideoCaptureStreamfvNV( GLuint video_capture_slot, GLuint stream, GLenum pname, GLfloat* params ) {
  void (*func_glGetVideoCaptureStreamfvNV)( GLuint, GLuint, GLenum, GLfloat* ) = extension_funcs[EXT_glGetVideoCaptureStreamfvNV];
  TRACE("(%d, %d, %d, %p)\n", video_capture_slot, stream, pname, params );
  ENTER_GL();
  func_glGetVideoCaptureStreamfvNV( video_capture_slot, stream, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetVideoCaptureStreamivNV( GLuint video_capture_slot, GLuint stream, GLenum pname, GLint* params ) {
  void (*func_glGetVideoCaptureStreamivNV)( GLuint, GLuint, GLenum, GLint* ) = extension_funcs[EXT_glGetVideoCaptureStreamivNV];
  TRACE("(%d, %d, %d, %p)\n", video_capture_slot, stream, pname, params );
  ENTER_GL();
  func_glGetVideoCaptureStreamivNV( video_capture_slot, stream, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetVideoCaptureivNV( GLuint video_capture_slot, GLenum pname, GLint* params ) {
  void (*func_glGetVideoCaptureivNV)( GLuint, GLenum, GLint* ) = extension_funcs[EXT_glGetVideoCaptureivNV];
  TRACE("(%d, %d, %p)\n", video_capture_slot, pname, params );
  ENTER_GL();
  func_glGetVideoCaptureivNV( video_capture_slot, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetVideoi64vNV( GLuint video_slot, GLenum pname, INT64* params ) {
  void (*func_glGetVideoi64vNV)( GLuint, GLenum, INT64* ) = extension_funcs[EXT_glGetVideoi64vNV];
  TRACE("(%d, %d, %p)\n", video_slot, pname, params );
  ENTER_GL();
  func_glGetVideoi64vNV( video_slot, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetVideoivNV( GLuint video_slot, GLenum pname, GLint* params ) {
  void (*func_glGetVideoivNV)( GLuint, GLenum, GLint* ) = extension_funcs[EXT_glGetVideoivNV];
  TRACE("(%d, %d, %p)\n", video_slot, pname, params );
  ENTER_GL();
  func_glGetVideoivNV( video_slot, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetVideoui64vNV( GLuint video_slot, GLenum pname, UINT64* params ) {
  void (*func_glGetVideoui64vNV)( GLuint, GLenum, UINT64* ) = extension_funcs[EXT_glGetVideoui64vNV];
  TRACE("(%d, %d, %p)\n", video_slot, pname, params );
  ENTER_GL();
  func_glGetVideoui64vNV( video_slot, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetVideouivNV( GLuint video_slot, GLenum pname, GLuint* params ) {
  void (*func_glGetVideouivNV)( GLuint, GLenum, GLuint* ) = extension_funcs[EXT_glGetVideouivNV];
  TRACE("(%d, %d, %p)\n", video_slot, pname, params );
  ENTER_GL();
  func_glGetVideouivNV( video_slot, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetnColorTableARB( GLenum target, GLenum format, GLenum type, GLsizei bufSize, GLvoid* table ) {
  void (*func_glGetnColorTableARB)( GLenum, GLenum, GLenum, GLsizei, GLvoid* ) = extension_funcs[EXT_glGetnColorTableARB];
  TRACE("(%d, %d, %d, %d, %p)\n", target, format, type, bufSize, table );
  ENTER_GL();
  func_glGetnColorTableARB( target, format, type, bufSize, table );
  LEAVE_GL();
}

static void WINAPI wine_glGetnCompressedTexImageARB( GLenum target, GLint lod, GLsizei bufSize, GLvoid* img ) {
  void (*func_glGetnCompressedTexImageARB)( GLenum, GLint, GLsizei, GLvoid* ) = extension_funcs[EXT_glGetnCompressedTexImageARB];
  TRACE("(%d, %d, %d, %p)\n", target, lod, bufSize, img );
  ENTER_GL();
  func_glGetnCompressedTexImageARB( target, lod, bufSize, img );
  LEAVE_GL();
}

static void WINAPI wine_glGetnConvolutionFilterARB( GLenum target, GLenum format, GLenum type, GLsizei bufSize, GLvoid* image ) {
  void (*func_glGetnConvolutionFilterARB)( GLenum, GLenum, GLenum, GLsizei, GLvoid* ) = extension_funcs[EXT_glGetnConvolutionFilterARB];
  TRACE("(%d, %d, %d, %d, %p)\n", target, format, type, bufSize, image );
  ENTER_GL();
  func_glGetnConvolutionFilterARB( target, format, type, bufSize, image );
  LEAVE_GL();
}

static void WINAPI wine_glGetnHistogramARB( GLenum target, GLboolean reset, GLenum format, GLenum type, GLsizei bufSize, GLvoid* values ) {
  void (*func_glGetnHistogramARB)( GLenum, GLboolean, GLenum, GLenum, GLsizei, GLvoid* ) = extension_funcs[EXT_glGetnHistogramARB];
  TRACE("(%d, %d, %d, %d, %d, %p)\n", target, reset, format, type, bufSize, values );
  ENTER_GL();
  func_glGetnHistogramARB( target, reset, format, type, bufSize, values );
  LEAVE_GL();
}

static void WINAPI wine_glGetnMapdvARB( GLenum target, GLenum query, GLsizei bufSize, GLdouble* v ) {
  void (*func_glGetnMapdvARB)( GLenum, GLenum, GLsizei, GLdouble* ) = extension_funcs[EXT_glGetnMapdvARB];
  TRACE("(%d, %d, %d, %p)\n", target, query, bufSize, v );
  ENTER_GL();
  func_glGetnMapdvARB( target, query, bufSize, v );
  LEAVE_GL();
}

static void WINAPI wine_glGetnMapfvARB( GLenum target, GLenum query, GLsizei bufSize, GLfloat* v ) {
  void (*func_glGetnMapfvARB)( GLenum, GLenum, GLsizei, GLfloat* ) = extension_funcs[EXT_glGetnMapfvARB];
  TRACE("(%d, %d, %d, %p)\n", target, query, bufSize, v );
  ENTER_GL();
  func_glGetnMapfvARB( target, query, bufSize, v );
  LEAVE_GL();
}

static void WINAPI wine_glGetnMapivARB( GLenum target, GLenum query, GLsizei bufSize, GLint* v ) {
  void (*func_glGetnMapivARB)( GLenum, GLenum, GLsizei, GLint* ) = extension_funcs[EXT_glGetnMapivARB];
  TRACE("(%d, %d, %d, %p)\n", target, query, bufSize, v );
  ENTER_GL();
  func_glGetnMapivARB( target, query, bufSize, v );
  LEAVE_GL();
}

static void WINAPI wine_glGetnMinmaxARB( GLenum target, GLboolean reset, GLenum format, GLenum type, GLsizei bufSize, GLvoid* values ) {
  void (*func_glGetnMinmaxARB)( GLenum, GLboolean, GLenum, GLenum, GLsizei, GLvoid* ) = extension_funcs[EXT_glGetnMinmaxARB];
  TRACE("(%d, %d, %d, %d, %d, %p)\n", target, reset, format, type, bufSize, values );
  ENTER_GL();
  func_glGetnMinmaxARB( target, reset, format, type, bufSize, values );
  LEAVE_GL();
}

static void WINAPI wine_glGetnPixelMapfvARB( GLenum map, GLsizei bufSize, GLfloat* values ) {
  void (*func_glGetnPixelMapfvARB)( GLenum, GLsizei, GLfloat* ) = extension_funcs[EXT_glGetnPixelMapfvARB];
  TRACE("(%d, %d, %p)\n", map, bufSize, values );
  ENTER_GL();
  func_glGetnPixelMapfvARB( map, bufSize, values );
  LEAVE_GL();
}

static void WINAPI wine_glGetnPixelMapuivARB( GLenum map, GLsizei bufSize, GLuint* values ) {
  void (*func_glGetnPixelMapuivARB)( GLenum, GLsizei, GLuint* ) = extension_funcs[EXT_glGetnPixelMapuivARB];
  TRACE("(%d, %d, %p)\n", map, bufSize, values );
  ENTER_GL();
  func_glGetnPixelMapuivARB( map, bufSize, values );
  LEAVE_GL();
}

static void WINAPI wine_glGetnPixelMapusvARB( GLenum map, GLsizei bufSize, GLushort* values ) {
  void (*func_glGetnPixelMapusvARB)( GLenum, GLsizei, GLushort* ) = extension_funcs[EXT_glGetnPixelMapusvARB];
  TRACE("(%d, %d, %p)\n", map, bufSize, values );
  ENTER_GL();
  func_glGetnPixelMapusvARB( map, bufSize, values );
  LEAVE_GL();
}

static void WINAPI wine_glGetnPolygonStippleARB( GLsizei bufSize, GLubyte* pattern ) {
  void (*func_glGetnPolygonStippleARB)( GLsizei, GLubyte* ) = extension_funcs[EXT_glGetnPolygonStippleARB];
  TRACE("(%d, %p)\n", bufSize, pattern );
  ENTER_GL();
  func_glGetnPolygonStippleARB( bufSize, pattern );
  LEAVE_GL();
}

static void WINAPI wine_glGetnSeparableFilterARB( GLenum target, GLenum format, GLenum type, GLsizei rowBufSize, GLvoid* row, GLsizei columnBufSize, GLvoid* column, GLvoid* span ) {
  void (*func_glGetnSeparableFilterARB)( GLenum, GLenum, GLenum, GLsizei, GLvoid*, GLsizei, GLvoid*, GLvoid* ) = extension_funcs[EXT_glGetnSeparableFilterARB];
  TRACE("(%d, %d, %d, %d, %p, %d, %p, %p)\n", target, format, type, rowBufSize, row, columnBufSize, column, span );
  ENTER_GL();
  func_glGetnSeparableFilterARB( target, format, type, rowBufSize, row, columnBufSize, column, span );
  LEAVE_GL();
}

static void WINAPI wine_glGetnTexImageARB( GLenum target, GLint level, GLenum format, GLenum type, GLsizei bufSize, GLvoid* img ) {
  void (*func_glGetnTexImageARB)( GLenum, GLint, GLenum, GLenum, GLsizei, GLvoid* ) = extension_funcs[EXT_glGetnTexImageARB];
  TRACE("(%d, %d, %d, %d, %d, %p)\n", target, level, format, type, bufSize, img );
  ENTER_GL();
  func_glGetnTexImageARB( target, level, format, type, bufSize, img );
  LEAVE_GL();
}

static void WINAPI wine_glGetnUniformdvARB( GLuint program, GLint location, GLsizei bufSize, GLdouble* params ) {
  void (*func_glGetnUniformdvARB)( GLuint, GLint, GLsizei, GLdouble* ) = extension_funcs[EXT_glGetnUniformdvARB];
  TRACE("(%d, %d, %d, %p)\n", program, location, bufSize, params );
  ENTER_GL();
  func_glGetnUniformdvARB( program, location, bufSize, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetnUniformfvARB( GLuint program, GLint location, GLsizei bufSize, GLfloat* params ) {
  void (*func_glGetnUniformfvARB)( GLuint, GLint, GLsizei, GLfloat* ) = extension_funcs[EXT_glGetnUniformfvARB];
  TRACE("(%d, %d, %d, %p)\n", program, location, bufSize, params );
  ENTER_GL();
  func_glGetnUniformfvARB( program, location, bufSize, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetnUniformivARB( GLuint program, GLint location, GLsizei bufSize, GLint* params ) {
  void (*func_glGetnUniformivARB)( GLuint, GLint, GLsizei, GLint* ) = extension_funcs[EXT_glGetnUniformivARB];
  TRACE("(%d, %d, %d, %p)\n", program, location, bufSize, params );
  ENTER_GL();
  func_glGetnUniformivARB( program, location, bufSize, params );
  LEAVE_GL();
}

static void WINAPI wine_glGetnUniformuivARB( GLuint program, GLint location, GLsizei bufSize, GLuint* params ) {
  void (*func_glGetnUniformuivARB)( GLuint, GLint, GLsizei, GLuint* ) = extension_funcs[EXT_glGetnUniformuivARB];
  TRACE("(%d, %d, %d, %p)\n", program, location, bufSize, params );
  ENTER_GL();
  func_glGetnUniformuivARB( program, location, bufSize, params );
  LEAVE_GL();
}

static void WINAPI wine_glGlobalAlphaFactorbSUN( GLbyte factor ) {
  void (*func_glGlobalAlphaFactorbSUN)( GLbyte ) = extension_funcs[EXT_glGlobalAlphaFactorbSUN];
  TRACE("(%d)\n", factor );
  ENTER_GL();
  func_glGlobalAlphaFactorbSUN( factor );
  LEAVE_GL();
}

static void WINAPI wine_glGlobalAlphaFactordSUN( GLdouble factor ) {
  void (*func_glGlobalAlphaFactordSUN)( GLdouble ) = extension_funcs[EXT_glGlobalAlphaFactordSUN];
  TRACE("(%f)\n", factor );
  ENTER_GL();
  func_glGlobalAlphaFactordSUN( factor );
  LEAVE_GL();
}

static void WINAPI wine_glGlobalAlphaFactorfSUN( GLfloat factor ) {
  void (*func_glGlobalAlphaFactorfSUN)( GLfloat ) = extension_funcs[EXT_glGlobalAlphaFactorfSUN];
  TRACE("(%f)\n", factor );
  ENTER_GL();
  func_glGlobalAlphaFactorfSUN( factor );
  LEAVE_GL();
}

static void WINAPI wine_glGlobalAlphaFactoriSUN( GLint factor ) {
  void (*func_glGlobalAlphaFactoriSUN)( GLint ) = extension_funcs[EXT_glGlobalAlphaFactoriSUN];
  TRACE("(%d)\n", factor );
  ENTER_GL();
  func_glGlobalAlphaFactoriSUN( factor );
  LEAVE_GL();
}

static void WINAPI wine_glGlobalAlphaFactorsSUN( GLshort factor ) {
  void (*func_glGlobalAlphaFactorsSUN)( GLshort ) = extension_funcs[EXT_glGlobalAlphaFactorsSUN];
  TRACE("(%d)\n", factor );
  ENTER_GL();
  func_glGlobalAlphaFactorsSUN( factor );
  LEAVE_GL();
}

static void WINAPI wine_glGlobalAlphaFactorubSUN( GLubyte factor ) {
  void (*func_glGlobalAlphaFactorubSUN)( GLubyte ) = extension_funcs[EXT_glGlobalAlphaFactorubSUN];
  TRACE("(%d)\n", factor );
  ENTER_GL();
  func_glGlobalAlphaFactorubSUN( factor );
  LEAVE_GL();
}

static void WINAPI wine_glGlobalAlphaFactoruiSUN( GLuint factor ) {
  void (*func_glGlobalAlphaFactoruiSUN)( GLuint ) = extension_funcs[EXT_glGlobalAlphaFactoruiSUN];
  TRACE("(%d)\n", factor );
  ENTER_GL();
  func_glGlobalAlphaFactoruiSUN( factor );
  LEAVE_GL();
}

static void WINAPI wine_glGlobalAlphaFactorusSUN( GLushort factor ) {
  void (*func_glGlobalAlphaFactorusSUN)( GLushort ) = extension_funcs[EXT_glGlobalAlphaFactorusSUN];
  TRACE("(%d)\n", factor );
  ENTER_GL();
  func_glGlobalAlphaFactorusSUN( factor );
  LEAVE_GL();
}

static void WINAPI wine_glHintPGI( GLenum target, GLint mode ) {
  void (*func_glHintPGI)( GLenum, GLint ) = extension_funcs[EXT_glHintPGI];
  TRACE("(%d, %d)\n", target, mode );
  ENTER_GL();
  func_glHintPGI( target, mode );
  LEAVE_GL();
}

static void WINAPI wine_glHistogram( GLenum target, GLsizei width, GLenum internalformat, GLboolean sink ) {
  void (*func_glHistogram)( GLenum, GLsizei, GLenum, GLboolean ) = extension_funcs[EXT_glHistogram];
  TRACE("(%d, %d, %d, %d)\n", target, width, internalformat, sink );
  ENTER_GL();
  func_glHistogram( target, width, internalformat, sink );
  LEAVE_GL();
}

static void WINAPI wine_glHistogramEXT( GLenum target, GLsizei width, GLenum internalformat, GLboolean sink ) {
  void (*func_glHistogramEXT)( GLenum, GLsizei, GLenum, GLboolean ) = extension_funcs[EXT_glHistogramEXT];
  TRACE("(%d, %d, %d, %d)\n", target, width, internalformat, sink );
  ENTER_GL();
  func_glHistogramEXT( target, width, internalformat, sink );
  LEAVE_GL();
}

static void WINAPI wine_glIglooInterfaceSGIX( GLenum pname, GLvoid* params ) {
  void (*func_glIglooInterfaceSGIX)( GLenum, GLvoid* ) = extension_funcs[EXT_glIglooInterfaceSGIX];
  TRACE("(%d, %p)\n", pname, params );
  ENTER_GL();
  func_glIglooInterfaceSGIX( pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glImageTransformParameterfHP( GLenum target, GLenum pname, GLfloat param ) {
  void (*func_glImageTransformParameterfHP)( GLenum, GLenum, GLfloat ) = extension_funcs[EXT_glImageTransformParameterfHP];
  TRACE("(%d, %d, %f)\n", target, pname, param );
  ENTER_GL();
  func_glImageTransformParameterfHP( target, pname, param );
  LEAVE_GL();
}

static void WINAPI wine_glImageTransformParameterfvHP( GLenum target, GLenum pname, GLfloat* params ) {
  void (*func_glImageTransformParameterfvHP)( GLenum, GLenum, GLfloat* ) = extension_funcs[EXT_glImageTransformParameterfvHP];
  TRACE("(%d, %d, %p)\n", target, pname, params );
  ENTER_GL();
  func_glImageTransformParameterfvHP( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glImageTransformParameteriHP( GLenum target, GLenum pname, GLint param ) {
  void (*func_glImageTransformParameteriHP)( GLenum, GLenum, GLint ) = extension_funcs[EXT_glImageTransformParameteriHP];
  TRACE("(%d, %d, %d)\n", target, pname, param );
  ENTER_GL();
  func_glImageTransformParameteriHP( target, pname, param );
  LEAVE_GL();
}

static void WINAPI wine_glImageTransformParameterivHP( GLenum target, GLenum pname, GLint* params ) {
  void (*func_glImageTransformParameterivHP)( GLenum, GLenum, GLint* ) = extension_funcs[EXT_glImageTransformParameterivHP];
  TRACE("(%d, %d, %p)\n", target, pname, params );
  ENTER_GL();
  func_glImageTransformParameterivHP( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glIndexFormatNV( GLenum type, GLsizei stride ) {
  void (*func_glIndexFormatNV)( GLenum, GLsizei ) = extension_funcs[EXT_glIndexFormatNV];
  TRACE("(%d, %d)\n", type, stride );
  ENTER_GL();
  func_glIndexFormatNV( type, stride );
  LEAVE_GL();
}

static void WINAPI wine_glIndexFuncEXT( GLenum func, GLclampf ref ) {
  void (*func_glIndexFuncEXT)( GLenum, GLclampf ) = extension_funcs[EXT_glIndexFuncEXT];
  TRACE("(%d, %f)\n", func, ref );
  ENTER_GL();
  func_glIndexFuncEXT( func, ref );
  LEAVE_GL();
}

static void WINAPI wine_glIndexMaterialEXT( GLenum face, GLenum mode ) {
  void (*func_glIndexMaterialEXT)( GLenum, GLenum ) = extension_funcs[EXT_glIndexMaterialEXT];
  TRACE("(%d, %d)\n", face, mode );
  ENTER_GL();
  func_glIndexMaterialEXT( face, mode );
  LEAVE_GL();
}

static void WINAPI wine_glIndexPointerEXT( GLenum type, GLsizei stride, GLsizei count, GLvoid* pointer ) {
  void (*func_glIndexPointerEXT)( GLenum, GLsizei, GLsizei, GLvoid* ) = extension_funcs[EXT_glIndexPointerEXT];
  TRACE("(%d, %d, %d, %p)\n", type, stride, count, pointer );
  ENTER_GL();
  func_glIndexPointerEXT( type, stride, count, pointer );
  LEAVE_GL();
}

static void WINAPI wine_glIndexPointerListIBM( GLenum type, GLint stride, GLvoid** pointer, GLint ptrstride ) {
  void (*func_glIndexPointerListIBM)( GLenum, GLint, GLvoid**, GLint ) = extension_funcs[EXT_glIndexPointerListIBM];
  TRACE("(%d, %d, %p, %d)\n", type, stride, pointer, ptrstride );
  ENTER_GL();
  func_glIndexPointerListIBM( type, stride, pointer, ptrstride );
  LEAVE_GL();
}

static void WINAPI wine_glInsertComponentEXT( GLuint res, GLuint src, GLuint num ) {
  void (*func_glInsertComponentEXT)( GLuint, GLuint, GLuint ) = extension_funcs[EXT_glInsertComponentEXT];
  TRACE("(%d, %d, %d)\n", res, src, num );
  ENTER_GL();
  func_glInsertComponentEXT( res, src, num );
  LEAVE_GL();
}

static void WINAPI wine_glInstrumentsBufferSGIX( GLsizei size, GLint* buffer ) {
  void (*func_glInstrumentsBufferSGIX)( GLsizei, GLint* ) = extension_funcs[EXT_glInstrumentsBufferSGIX];
  TRACE("(%d, %p)\n", size, buffer );
  ENTER_GL();
  func_glInstrumentsBufferSGIX( size, buffer );
  LEAVE_GL();
}

static GLboolean WINAPI wine_glIsAsyncMarkerSGIX( GLuint marker ) {
  GLboolean ret_value;
  GLboolean (*func_glIsAsyncMarkerSGIX)( GLuint ) = extension_funcs[EXT_glIsAsyncMarkerSGIX];
  TRACE("(%d)\n", marker );
  ENTER_GL();
  ret_value = func_glIsAsyncMarkerSGIX( marker );
  LEAVE_GL();
  return ret_value;
}

static GLboolean WINAPI wine_glIsBuffer( GLuint buffer ) {
  GLboolean ret_value;
  GLboolean (*func_glIsBuffer)( GLuint ) = extension_funcs[EXT_glIsBuffer];
  TRACE("(%d)\n", buffer );
  ENTER_GL();
  ret_value = func_glIsBuffer( buffer );
  LEAVE_GL();
  return ret_value;
}

static GLboolean WINAPI wine_glIsBufferARB( GLuint buffer ) {
  GLboolean ret_value;
  GLboolean (*func_glIsBufferARB)( GLuint ) = extension_funcs[EXT_glIsBufferARB];
  TRACE("(%d)\n", buffer );
  ENTER_GL();
  ret_value = func_glIsBufferARB( buffer );
  LEAVE_GL();
  return ret_value;
}

static GLboolean WINAPI wine_glIsBufferResidentNV( GLenum target ) {
  GLboolean ret_value;
  GLboolean (*func_glIsBufferResidentNV)( GLenum ) = extension_funcs[EXT_glIsBufferResidentNV];
  TRACE("(%d)\n", target );
  ENTER_GL();
  ret_value = func_glIsBufferResidentNV( target );
  LEAVE_GL();
  return ret_value;
}

static GLboolean WINAPI wine_glIsEnabledIndexedEXT( GLenum target, GLuint index ) {
  GLboolean ret_value;
  GLboolean (*func_glIsEnabledIndexedEXT)( GLenum, GLuint ) = extension_funcs[EXT_glIsEnabledIndexedEXT];
  TRACE("(%d, %d)\n", target, index );
  ENTER_GL();
  ret_value = func_glIsEnabledIndexedEXT( target, index );
  LEAVE_GL();
  return ret_value;
}

static GLboolean WINAPI wine_glIsEnabledi( GLenum target, GLuint index ) {
  GLboolean ret_value;
  GLboolean (*func_glIsEnabledi)( GLenum, GLuint ) = extension_funcs[EXT_glIsEnabledi];
  TRACE("(%d, %d)\n", target, index );
  ENTER_GL();
  ret_value = func_glIsEnabledi( target, index );
  LEAVE_GL();
  return ret_value;
}

static GLboolean WINAPI wine_glIsFenceAPPLE( GLuint fence ) {
  GLboolean ret_value;
  GLboolean (*func_glIsFenceAPPLE)( GLuint ) = extension_funcs[EXT_glIsFenceAPPLE];
  TRACE("(%d)\n", fence );
  ENTER_GL();
  ret_value = func_glIsFenceAPPLE( fence );
  LEAVE_GL();
  return ret_value;
}

static GLboolean WINAPI wine_glIsFenceNV( GLuint fence ) {
  GLboolean ret_value;
  GLboolean (*func_glIsFenceNV)( GLuint ) = extension_funcs[EXT_glIsFenceNV];
  TRACE("(%d)\n", fence );
  ENTER_GL();
  ret_value = func_glIsFenceNV( fence );
  LEAVE_GL();
  return ret_value;
}

static GLboolean WINAPI wine_glIsFramebuffer( GLuint framebuffer ) {
  GLboolean ret_value;
  GLboolean (*func_glIsFramebuffer)( GLuint ) = extension_funcs[EXT_glIsFramebuffer];
  TRACE("(%d)\n", framebuffer );
  ENTER_GL();
  ret_value = func_glIsFramebuffer( framebuffer );
  LEAVE_GL();
  return ret_value;
}

static GLboolean WINAPI wine_glIsFramebufferEXT( GLuint framebuffer ) {
  GLboolean ret_value;
  GLboolean (*func_glIsFramebufferEXT)( GLuint ) = extension_funcs[EXT_glIsFramebufferEXT];
  TRACE("(%d)\n", framebuffer );
  ENTER_GL();
  ret_value = func_glIsFramebufferEXT( framebuffer );
  LEAVE_GL();
  return ret_value;
}

static GLboolean WINAPI wine_glIsNameAMD( GLenum identifier, GLuint name ) {
  GLboolean ret_value;
  GLboolean (*func_glIsNameAMD)( GLenum, GLuint ) = extension_funcs[EXT_glIsNameAMD];
  TRACE("(%d, %d)\n", identifier, name );
  ENTER_GL();
  ret_value = func_glIsNameAMD( identifier, name );
  LEAVE_GL();
  return ret_value;
}

static GLboolean WINAPI wine_glIsNamedBufferResidentNV( GLuint buffer ) {
  GLboolean ret_value;
  GLboolean (*func_glIsNamedBufferResidentNV)( GLuint ) = extension_funcs[EXT_glIsNamedBufferResidentNV];
  TRACE("(%d)\n", buffer );
  ENTER_GL();
  ret_value = func_glIsNamedBufferResidentNV( buffer );
  LEAVE_GL();
  return ret_value;
}

static GLboolean WINAPI wine_glIsNamedStringARB( GLint namelen, char* name ) {
  GLboolean ret_value;
  GLboolean (*func_glIsNamedStringARB)( GLint, char* ) = extension_funcs[EXT_glIsNamedStringARB];
  TRACE("(%d, %p)\n", namelen, name );
  ENTER_GL();
  ret_value = func_glIsNamedStringARB( namelen, name );
  LEAVE_GL();
  return ret_value;
}

static GLboolean WINAPI wine_glIsObjectBufferATI( GLuint buffer ) {
  GLboolean ret_value;
  GLboolean (*func_glIsObjectBufferATI)( GLuint ) = extension_funcs[EXT_glIsObjectBufferATI];
  TRACE("(%d)\n", buffer );
  ENTER_GL();
  ret_value = func_glIsObjectBufferATI( buffer );
  LEAVE_GL();
  return ret_value;
}

static GLboolean WINAPI wine_glIsOcclusionQueryNV( GLuint id ) {
  GLboolean ret_value;
  GLboolean (*func_glIsOcclusionQueryNV)( GLuint ) = extension_funcs[EXT_glIsOcclusionQueryNV];
  TRACE("(%d)\n", id );
  ENTER_GL();
  ret_value = func_glIsOcclusionQueryNV( id );
  LEAVE_GL();
  return ret_value;
}

static GLboolean WINAPI wine_glIsProgram( GLuint program ) {
  GLboolean ret_value;
  GLboolean (*func_glIsProgram)( GLuint ) = extension_funcs[EXT_glIsProgram];
  TRACE("(%d)\n", program );
  ENTER_GL();
  ret_value = func_glIsProgram( program );
  LEAVE_GL();
  return ret_value;
}

static GLboolean WINAPI wine_glIsProgramARB( GLuint program ) {
  GLboolean ret_value;
  GLboolean (*func_glIsProgramARB)( GLuint ) = extension_funcs[EXT_glIsProgramARB];
  TRACE("(%d)\n", program );
  ENTER_GL();
  ret_value = func_glIsProgramARB( program );
  LEAVE_GL();
  return ret_value;
}

static GLboolean WINAPI wine_glIsProgramNV( GLuint id ) {
  GLboolean ret_value;
  GLboolean (*func_glIsProgramNV)( GLuint ) = extension_funcs[EXT_glIsProgramNV];
  TRACE("(%d)\n", id );
  ENTER_GL();
  ret_value = func_glIsProgramNV( id );
  LEAVE_GL();
  return ret_value;
}

static GLboolean WINAPI wine_glIsProgramPipeline( GLuint pipeline ) {
  GLboolean ret_value;
  GLboolean (*func_glIsProgramPipeline)( GLuint ) = extension_funcs[EXT_glIsProgramPipeline];
  TRACE("(%d)\n", pipeline );
  ENTER_GL();
  ret_value = func_glIsProgramPipeline( pipeline );
  LEAVE_GL();
  return ret_value;
}

static GLboolean WINAPI wine_glIsQuery( GLuint id ) {
  GLboolean ret_value;
  GLboolean (*func_glIsQuery)( GLuint ) = extension_funcs[EXT_glIsQuery];
  TRACE("(%d)\n", id );
  ENTER_GL();
  ret_value = func_glIsQuery( id );
  LEAVE_GL();
  return ret_value;
}

static GLboolean WINAPI wine_glIsQueryARB( GLuint id ) {
  GLboolean ret_value;
  GLboolean (*func_glIsQueryARB)( GLuint ) = extension_funcs[EXT_glIsQueryARB];
  TRACE("(%d)\n", id );
  ENTER_GL();
  ret_value = func_glIsQueryARB( id );
  LEAVE_GL();
  return ret_value;
}

static GLboolean WINAPI wine_glIsRenderbuffer( GLuint renderbuffer ) {
  GLboolean ret_value;
  GLboolean (*func_glIsRenderbuffer)( GLuint ) = extension_funcs[EXT_glIsRenderbuffer];
  TRACE("(%d)\n", renderbuffer );
  ENTER_GL();
  ret_value = func_glIsRenderbuffer( renderbuffer );
  LEAVE_GL();
  return ret_value;
}

static GLboolean WINAPI wine_glIsRenderbufferEXT( GLuint renderbuffer ) {
  GLboolean ret_value;
  GLboolean (*func_glIsRenderbufferEXT)( GLuint ) = extension_funcs[EXT_glIsRenderbufferEXT];
  TRACE("(%d)\n", renderbuffer );
  ENTER_GL();
  ret_value = func_glIsRenderbufferEXT( renderbuffer );
  LEAVE_GL();
  return ret_value;
}

static GLboolean WINAPI wine_glIsSampler( GLuint sampler ) {
  GLboolean ret_value;
  GLboolean (*func_glIsSampler)( GLuint ) = extension_funcs[EXT_glIsSampler];
  TRACE("(%d)\n", sampler );
  ENTER_GL();
  ret_value = func_glIsSampler( sampler );
  LEAVE_GL();
  return ret_value;
}

static GLboolean WINAPI wine_glIsShader( GLuint shader ) {
  GLboolean ret_value;
  GLboolean (*func_glIsShader)( GLuint ) = extension_funcs[EXT_glIsShader];
  TRACE("(%d)\n", shader );
  ENTER_GL();
  ret_value = func_glIsShader( shader );
  LEAVE_GL();
  return ret_value;
}

static GLboolean WINAPI wine_glIsSync( GLvoid* sync ) {
  GLboolean ret_value;
  GLboolean (*func_glIsSync)( GLvoid* ) = extension_funcs[EXT_glIsSync];
  TRACE("(%p)\n", sync );
  ENTER_GL();
  ret_value = func_glIsSync( sync );
  LEAVE_GL();
  return ret_value;
}

static GLboolean WINAPI wine_glIsTextureEXT( GLuint texture ) {
  GLboolean ret_value;
  GLboolean (*func_glIsTextureEXT)( GLuint ) = extension_funcs[EXT_glIsTextureEXT];
  TRACE("(%d)\n", texture );
  ENTER_GL();
  ret_value = func_glIsTextureEXT( texture );
  LEAVE_GL();
  return ret_value;
}

static GLboolean WINAPI wine_glIsTransformFeedback( GLuint id ) {
  GLboolean ret_value;
  GLboolean (*func_glIsTransformFeedback)( GLuint ) = extension_funcs[EXT_glIsTransformFeedback];
  TRACE("(%d)\n", id );
  ENTER_GL();
  ret_value = func_glIsTransformFeedback( id );
  LEAVE_GL();
  return ret_value;
}

static GLboolean WINAPI wine_glIsTransformFeedbackNV( GLuint id ) {
  GLboolean ret_value;
  GLboolean (*func_glIsTransformFeedbackNV)( GLuint ) = extension_funcs[EXT_glIsTransformFeedbackNV];
  TRACE("(%d)\n", id );
  ENTER_GL();
  ret_value = func_glIsTransformFeedbackNV( id );
  LEAVE_GL();
  return ret_value;
}

static GLboolean WINAPI wine_glIsVariantEnabledEXT( GLuint id, GLenum cap ) {
  GLboolean ret_value;
  GLboolean (*func_glIsVariantEnabledEXT)( GLuint, GLenum ) = extension_funcs[EXT_glIsVariantEnabledEXT];
  TRACE("(%d, %d)\n", id, cap );
  ENTER_GL();
  ret_value = func_glIsVariantEnabledEXT( id, cap );
  LEAVE_GL();
  return ret_value;
}

static GLboolean WINAPI wine_glIsVertexArray( GLuint array ) {
  GLboolean ret_value;
  GLboolean (*func_glIsVertexArray)( GLuint ) = extension_funcs[EXT_glIsVertexArray];
  TRACE("(%d)\n", array );
  ENTER_GL();
  ret_value = func_glIsVertexArray( array );
  LEAVE_GL();
  return ret_value;
}

static GLboolean WINAPI wine_glIsVertexArrayAPPLE( GLuint array ) {
  GLboolean ret_value;
  GLboolean (*func_glIsVertexArrayAPPLE)( GLuint ) = extension_funcs[EXT_glIsVertexArrayAPPLE];
  TRACE("(%d)\n", array );
  ENTER_GL();
  ret_value = func_glIsVertexArrayAPPLE( array );
  LEAVE_GL();
  return ret_value;
}

static GLboolean WINAPI wine_glIsVertexAttribEnabledAPPLE( GLuint index, GLenum pname ) {
  GLboolean ret_value;
  GLboolean (*func_glIsVertexAttribEnabledAPPLE)( GLuint, GLenum ) = extension_funcs[EXT_glIsVertexAttribEnabledAPPLE];
  TRACE("(%d, %d)\n", index, pname );
  ENTER_GL();
  ret_value = func_glIsVertexAttribEnabledAPPLE( index, pname );
  LEAVE_GL();
  return ret_value;
}

static void WINAPI wine_glLightEnviSGIX( GLenum pname, GLint param ) {
  void (*func_glLightEnviSGIX)( GLenum, GLint ) = extension_funcs[EXT_glLightEnviSGIX];
  TRACE("(%d, %d)\n", pname, param );
  ENTER_GL();
  func_glLightEnviSGIX( pname, param );
  LEAVE_GL();
}

static void WINAPI wine_glLinkProgram( GLuint program ) {
  void (*func_glLinkProgram)( GLuint ) = extension_funcs[EXT_glLinkProgram];
  TRACE("(%d)\n", program );
  ENTER_GL();
  func_glLinkProgram( program );
  LEAVE_GL();
}

static void WINAPI wine_glLinkProgramARB( unsigned int programObj ) {
  void (*func_glLinkProgramARB)( unsigned int ) = extension_funcs[EXT_glLinkProgramARB];
  TRACE("(%d)\n", programObj );
  ENTER_GL();
  func_glLinkProgramARB( programObj );
  LEAVE_GL();
}

static void WINAPI wine_glListParameterfSGIX( GLuint list, GLenum pname, GLfloat param ) {
  void (*func_glListParameterfSGIX)( GLuint, GLenum, GLfloat ) = extension_funcs[EXT_glListParameterfSGIX];
  TRACE("(%d, %d, %f)\n", list, pname, param );
  ENTER_GL();
  func_glListParameterfSGIX( list, pname, param );
  LEAVE_GL();
}

static void WINAPI wine_glListParameterfvSGIX( GLuint list, GLenum pname, GLfloat* params ) {
  void (*func_glListParameterfvSGIX)( GLuint, GLenum, GLfloat* ) = extension_funcs[EXT_glListParameterfvSGIX];
  TRACE("(%d, %d, %p)\n", list, pname, params );
  ENTER_GL();
  func_glListParameterfvSGIX( list, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glListParameteriSGIX( GLuint list, GLenum pname, GLint param ) {
  void (*func_glListParameteriSGIX)( GLuint, GLenum, GLint ) = extension_funcs[EXT_glListParameteriSGIX];
  TRACE("(%d, %d, %d)\n", list, pname, param );
  ENTER_GL();
  func_glListParameteriSGIX( list, pname, param );
  LEAVE_GL();
}

static void WINAPI wine_glListParameterivSGIX( GLuint list, GLenum pname, GLint* params ) {
  void (*func_glListParameterivSGIX)( GLuint, GLenum, GLint* ) = extension_funcs[EXT_glListParameterivSGIX];
  TRACE("(%d, %d, %p)\n", list, pname, params );
  ENTER_GL();
  func_glListParameterivSGIX( list, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glLoadIdentityDeformationMapSGIX( GLbitfield mask ) {
  void (*func_glLoadIdentityDeformationMapSGIX)( GLbitfield ) = extension_funcs[EXT_glLoadIdentityDeformationMapSGIX];
  TRACE("(%d)\n", mask );
  ENTER_GL();
  func_glLoadIdentityDeformationMapSGIX( mask );
  LEAVE_GL();
}

static void WINAPI wine_glLoadProgramNV( GLenum target, GLuint id, GLsizei len, GLubyte* program ) {
  void (*func_glLoadProgramNV)( GLenum, GLuint, GLsizei, GLubyte* ) = extension_funcs[EXT_glLoadProgramNV];
  TRACE("(%d, %d, %d, %p)\n", target, id, len, program );
  ENTER_GL();
  func_glLoadProgramNV( target, id, len, program );
  LEAVE_GL();
}

static void WINAPI wine_glLoadTransposeMatrixd( GLdouble* m ) {
  void (*func_glLoadTransposeMatrixd)( GLdouble* ) = extension_funcs[EXT_glLoadTransposeMatrixd];
  TRACE("(%p)\n", m );
  ENTER_GL();
  func_glLoadTransposeMatrixd( m );
  LEAVE_GL();
}

static void WINAPI wine_glLoadTransposeMatrixdARB( GLdouble* m ) {
  void (*func_glLoadTransposeMatrixdARB)( GLdouble* ) = extension_funcs[EXT_glLoadTransposeMatrixdARB];
  TRACE("(%p)\n", m );
  ENTER_GL();
  func_glLoadTransposeMatrixdARB( m );
  LEAVE_GL();
}

static void WINAPI wine_glLoadTransposeMatrixf( GLfloat* m ) {
  void (*func_glLoadTransposeMatrixf)( GLfloat* ) = extension_funcs[EXT_glLoadTransposeMatrixf];
  TRACE("(%p)\n", m );
  ENTER_GL();
  func_glLoadTransposeMatrixf( m );
  LEAVE_GL();
}

static void WINAPI wine_glLoadTransposeMatrixfARB( GLfloat* m ) {
  void (*func_glLoadTransposeMatrixfARB)( GLfloat* ) = extension_funcs[EXT_glLoadTransposeMatrixfARB];
  TRACE("(%p)\n", m );
  ENTER_GL();
  func_glLoadTransposeMatrixfARB( m );
  LEAVE_GL();
}

static void WINAPI wine_glLockArraysEXT( GLint first, GLsizei count ) {
  void (*func_glLockArraysEXT)( GLint, GLsizei ) = extension_funcs[EXT_glLockArraysEXT];
  TRACE("(%d, %d)\n", first, count );
  ENTER_GL();
  func_glLockArraysEXT( first, count );
  LEAVE_GL();
}

static void WINAPI wine_glMTexCoord2fSGIS( GLenum target, GLfloat s, GLfloat t ) {
  void (*func_glMTexCoord2fSGIS)( GLenum, GLfloat, GLfloat ) = extension_funcs[EXT_glMTexCoord2fSGIS];
  TRACE("(%d, %f, %f)\n", target, s, t );
  ENTER_GL();
  func_glMTexCoord2fSGIS( target, s, t );
  LEAVE_GL();
}

static void WINAPI wine_glMTexCoord2fvSGIS( GLenum target, GLfloat * v ) {
  void (*func_glMTexCoord2fvSGIS)( GLenum, GLfloat * ) = extension_funcs[EXT_glMTexCoord2fvSGIS];
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMTexCoord2fvSGIS( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMakeBufferNonResidentNV( GLenum target ) {
  void (*func_glMakeBufferNonResidentNV)( GLenum ) = extension_funcs[EXT_glMakeBufferNonResidentNV];
  TRACE("(%d)\n", target );
  ENTER_GL();
  func_glMakeBufferNonResidentNV( target );
  LEAVE_GL();
}

static void WINAPI wine_glMakeBufferResidentNV( GLenum target, GLenum access ) {
  void (*func_glMakeBufferResidentNV)( GLenum, GLenum ) = extension_funcs[EXT_glMakeBufferResidentNV];
  TRACE("(%d, %d)\n", target, access );
  ENTER_GL();
  func_glMakeBufferResidentNV( target, access );
  LEAVE_GL();
}

static void WINAPI wine_glMakeNamedBufferNonResidentNV( GLuint buffer ) {
  void (*func_glMakeNamedBufferNonResidentNV)( GLuint ) = extension_funcs[EXT_glMakeNamedBufferNonResidentNV];
  TRACE("(%d)\n", buffer );
  ENTER_GL();
  func_glMakeNamedBufferNonResidentNV( buffer );
  LEAVE_GL();
}

static void WINAPI wine_glMakeNamedBufferResidentNV( GLuint buffer, GLenum access ) {
  void (*func_glMakeNamedBufferResidentNV)( GLuint, GLenum ) = extension_funcs[EXT_glMakeNamedBufferResidentNV];
  TRACE("(%d, %d)\n", buffer, access );
  ENTER_GL();
  func_glMakeNamedBufferResidentNV( buffer, access );
  LEAVE_GL();
}

static GLvoid* WINAPI wine_glMapBuffer( GLenum target, GLenum access ) {
  GLvoid* ret_value;
  GLvoid* (*func_glMapBuffer)( GLenum, GLenum ) = extension_funcs[EXT_glMapBuffer];
  TRACE("(%d, %d)\n", target, access );
  ENTER_GL();
  ret_value = func_glMapBuffer( target, access );
  LEAVE_GL();
  return ret_value;
}

static GLvoid* WINAPI wine_glMapBufferARB( GLenum target, GLenum access ) {
  GLvoid* ret_value;
  GLvoid* (*func_glMapBufferARB)( GLenum, GLenum ) = extension_funcs[EXT_glMapBufferARB];
  TRACE("(%d, %d)\n", target, access );
  ENTER_GL();
  ret_value = func_glMapBufferARB( target, access );
  LEAVE_GL();
  return ret_value;
}

static GLvoid* WINAPI wine_glMapBufferRange( GLenum target, INT_PTR offset, INT_PTR length, GLbitfield access ) {
  GLvoid* ret_value;
  GLvoid* (*func_glMapBufferRange)( GLenum, INT_PTR, INT_PTR, GLbitfield ) = extension_funcs[EXT_glMapBufferRange];
  TRACE("(%d, %ld, %ld, %d)\n", target, offset, length, access );
  ENTER_GL();
  ret_value = func_glMapBufferRange( target, offset, length, access );
  LEAVE_GL();
  return ret_value;
}

static void WINAPI wine_glMapControlPointsNV( GLenum target, GLuint index, GLenum type, GLsizei ustride, GLsizei vstride, GLint uorder, GLint vorder, GLboolean packed, GLvoid* points ) {
  void (*func_glMapControlPointsNV)( GLenum, GLuint, GLenum, GLsizei, GLsizei, GLint, GLint, GLboolean, GLvoid* ) = extension_funcs[EXT_glMapControlPointsNV];
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %p)\n", target, index, type, ustride, vstride, uorder, vorder, packed, points );
  ENTER_GL();
  func_glMapControlPointsNV( target, index, type, ustride, vstride, uorder, vorder, packed, points );
  LEAVE_GL();
}

static GLvoid* WINAPI wine_glMapNamedBufferEXT( GLuint buffer, GLenum access ) {
  GLvoid* ret_value;
  GLvoid* (*func_glMapNamedBufferEXT)( GLuint, GLenum ) = extension_funcs[EXT_glMapNamedBufferEXT];
  TRACE("(%d, %d)\n", buffer, access );
  ENTER_GL();
  ret_value = func_glMapNamedBufferEXT( buffer, access );
  LEAVE_GL();
  return ret_value;
}

static GLvoid* WINAPI wine_glMapNamedBufferRangeEXT( GLuint buffer, INT_PTR offset, INT_PTR length, GLbitfield access ) {
  GLvoid* ret_value;
  GLvoid* (*func_glMapNamedBufferRangeEXT)( GLuint, INT_PTR, INT_PTR, GLbitfield ) = extension_funcs[EXT_glMapNamedBufferRangeEXT];
  TRACE("(%d, %ld, %ld, %d)\n", buffer, offset, length, access );
  ENTER_GL();
  ret_value = func_glMapNamedBufferRangeEXT( buffer, offset, length, access );
  LEAVE_GL();
  return ret_value;
}

static GLvoid* WINAPI wine_glMapObjectBufferATI( GLuint buffer ) {
  GLvoid* ret_value;
  GLvoid* (*func_glMapObjectBufferATI)( GLuint ) = extension_funcs[EXT_glMapObjectBufferATI];
  TRACE("(%d)\n", buffer );
  ENTER_GL();
  ret_value = func_glMapObjectBufferATI( buffer );
  LEAVE_GL();
  return ret_value;
}

static void WINAPI wine_glMapParameterfvNV( GLenum target, GLenum pname, GLfloat* params ) {
  void (*func_glMapParameterfvNV)( GLenum, GLenum, GLfloat* ) = extension_funcs[EXT_glMapParameterfvNV];
  TRACE("(%d, %d, %p)\n", target, pname, params );
  ENTER_GL();
  func_glMapParameterfvNV( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glMapParameterivNV( GLenum target, GLenum pname, GLint* params ) {
  void (*func_glMapParameterivNV)( GLenum, GLenum, GLint* ) = extension_funcs[EXT_glMapParameterivNV];
  TRACE("(%d, %d, %p)\n", target, pname, params );
  ENTER_GL();
  func_glMapParameterivNV( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glMapVertexAttrib1dAPPLE( GLuint index, GLuint size, GLdouble u1, GLdouble u2, GLint stride, GLint order, GLdouble* points ) {
  void (*func_glMapVertexAttrib1dAPPLE)( GLuint, GLuint, GLdouble, GLdouble, GLint, GLint, GLdouble* ) = extension_funcs[EXT_glMapVertexAttrib1dAPPLE];
  TRACE("(%d, %d, %f, %f, %d, %d, %p)\n", index, size, u1, u2, stride, order, points );
  ENTER_GL();
  func_glMapVertexAttrib1dAPPLE( index, size, u1, u2, stride, order, points );
  LEAVE_GL();
}

static void WINAPI wine_glMapVertexAttrib1fAPPLE( GLuint index, GLuint size, GLfloat u1, GLfloat u2, GLint stride, GLint order, GLfloat* points ) {
  void (*func_glMapVertexAttrib1fAPPLE)( GLuint, GLuint, GLfloat, GLfloat, GLint, GLint, GLfloat* ) = extension_funcs[EXT_glMapVertexAttrib1fAPPLE];
  TRACE("(%d, %d, %f, %f, %d, %d, %p)\n", index, size, u1, u2, stride, order, points );
  ENTER_GL();
  func_glMapVertexAttrib1fAPPLE( index, size, u1, u2, stride, order, points );
  LEAVE_GL();
}

static void WINAPI wine_glMapVertexAttrib2dAPPLE( GLuint index, GLuint size, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, GLdouble* points ) {
  void (*func_glMapVertexAttrib2dAPPLE)( GLuint, GLuint, GLdouble, GLdouble, GLint, GLint, GLdouble, GLdouble, GLint, GLint, GLdouble* ) = extension_funcs[EXT_glMapVertexAttrib2dAPPLE];
  TRACE("(%d, %d, %f, %f, %d, %d, %f, %f, %d, %d, %p)\n", index, size, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points );
  ENTER_GL();
  func_glMapVertexAttrib2dAPPLE( index, size, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points );
  LEAVE_GL();
}

static void WINAPI wine_glMapVertexAttrib2fAPPLE( GLuint index, GLuint size, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, GLfloat* points ) {
  void (*func_glMapVertexAttrib2fAPPLE)( GLuint, GLuint, GLfloat, GLfloat, GLint, GLint, GLfloat, GLfloat, GLint, GLint, GLfloat* ) = extension_funcs[EXT_glMapVertexAttrib2fAPPLE];
  TRACE("(%d, %d, %f, %f, %d, %d, %f, %f, %d, %d, %p)\n", index, size, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points );
  ENTER_GL();
  func_glMapVertexAttrib2fAPPLE( index, size, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points );
  LEAVE_GL();
}

static void WINAPI wine_glMatrixFrustumEXT( GLenum mode, GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar ) {
  void (*func_glMatrixFrustumEXT)( GLenum, GLdouble, GLdouble, GLdouble, GLdouble, GLdouble, GLdouble ) = extension_funcs[EXT_glMatrixFrustumEXT];
  TRACE("(%d, %f, %f, %f, %f, %f, %f)\n", mode, left, right, bottom, top, zNear, zFar );
  ENTER_GL();
  func_glMatrixFrustumEXT( mode, left, right, bottom, top, zNear, zFar );
  LEAVE_GL();
}

static void WINAPI wine_glMatrixIndexPointerARB( GLint size, GLenum type, GLsizei stride, GLvoid* pointer ) {
  void (*func_glMatrixIndexPointerARB)( GLint, GLenum, GLsizei, GLvoid* ) = extension_funcs[EXT_glMatrixIndexPointerARB];
  TRACE("(%d, %d, %d, %p)\n", size, type, stride, pointer );
  ENTER_GL();
  func_glMatrixIndexPointerARB( size, type, stride, pointer );
  LEAVE_GL();
}

static void WINAPI wine_glMatrixIndexubvARB( GLint size, GLubyte* indices ) {
  void (*func_glMatrixIndexubvARB)( GLint, GLubyte* ) = extension_funcs[EXT_glMatrixIndexubvARB];
  TRACE("(%d, %p)\n", size, indices );
  ENTER_GL();
  func_glMatrixIndexubvARB( size, indices );
  LEAVE_GL();
}

static void WINAPI wine_glMatrixIndexuivARB( GLint size, GLuint* indices ) {
  void (*func_glMatrixIndexuivARB)( GLint, GLuint* ) = extension_funcs[EXT_glMatrixIndexuivARB];
  TRACE("(%d, %p)\n", size, indices );
  ENTER_GL();
  func_glMatrixIndexuivARB( size, indices );
  LEAVE_GL();
}

static void WINAPI wine_glMatrixIndexusvARB( GLint size, GLushort* indices ) {
  void (*func_glMatrixIndexusvARB)( GLint, GLushort* ) = extension_funcs[EXT_glMatrixIndexusvARB];
  TRACE("(%d, %p)\n", size, indices );
  ENTER_GL();
  func_glMatrixIndexusvARB( size, indices );
  LEAVE_GL();
}

static void WINAPI wine_glMatrixLoadIdentityEXT( GLenum mode ) {
  void (*func_glMatrixLoadIdentityEXT)( GLenum ) = extension_funcs[EXT_glMatrixLoadIdentityEXT];
  TRACE("(%d)\n", mode );
  ENTER_GL();
  func_glMatrixLoadIdentityEXT( mode );
  LEAVE_GL();
}

static void WINAPI wine_glMatrixLoadTransposedEXT( GLenum mode, GLdouble* m ) {
  void (*func_glMatrixLoadTransposedEXT)( GLenum, GLdouble* ) = extension_funcs[EXT_glMatrixLoadTransposedEXT];
  TRACE("(%d, %p)\n", mode, m );
  ENTER_GL();
  func_glMatrixLoadTransposedEXT( mode, m );
  LEAVE_GL();
}

static void WINAPI wine_glMatrixLoadTransposefEXT( GLenum mode, GLfloat* m ) {
  void (*func_glMatrixLoadTransposefEXT)( GLenum, GLfloat* ) = extension_funcs[EXT_glMatrixLoadTransposefEXT];
  TRACE("(%d, %p)\n", mode, m );
  ENTER_GL();
  func_glMatrixLoadTransposefEXT( mode, m );
  LEAVE_GL();
}

static void WINAPI wine_glMatrixLoaddEXT( GLenum mode, GLdouble* m ) {
  void (*func_glMatrixLoaddEXT)( GLenum, GLdouble* ) = extension_funcs[EXT_glMatrixLoaddEXT];
  TRACE("(%d, %p)\n", mode, m );
  ENTER_GL();
  func_glMatrixLoaddEXT( mode, m );
  LEAVE_GL();
}

static void WINAPI wine_glMatrixLoadfEXT( GLenum mode, GLfloat* m ) {
  void (*func_glMatrixLoadfEXT)( GLenum, GLfloat* ) = extension_funcs[EXT_glMatrixLoadfEXT];
  TRACE("(%d, %p)\n", mode, m );
  ENTER_GL();
  func_glMatrixLoadfEXT( mode, m );
  LEAVE_GL();
}

static void WINAPI wine_glMatrixMultTransposedEXT( GLenum mode, GLdouble* m ) {
  void (*func_glMatrixMultTransposedEXT)( GLenum, GLdouble* ) = extension_funcs[EXT_glMatrixMultTransposedEXT];
  TRACE("(%d, %p)\n", mode, m );
  ENTER_GL();
  func_glMatrixMultTransposedEXT( mode, m );
  LEAVE_GL();
}

static void WINAPI wine_glMatrixMultTransposefEXT( GLenum mode, GLfloat* m ) {
  void (*func_glMatrixMultTransposefEXT)( GLenum, GLfloat* ) = extension_funcs[EXT_glMatrixMultTransposefEXT];
  TRACE("(%d, %p)\n", mode, m );
  ENTER_GL();
  func_glMatrixMultTransposefEXT( mode, m );
  LEAVE_GL();
}

static void WINAPI wine_glMatrixMultdEXT( GLenum mode, GLdouble* m ) {
  void (*func_glMatrixMultdEXT)( GLenum, GLdouble* ) = extension_funcs[EXT_glMatrixMultdEXT];
  TRACE("(%d, %p)\n", mode, m );
  ENTER_GL();
  func_glMatrixMultdEXT( mode, m );
  LEAVE_GL();
}

static void WINAPI wine_glMatrixMultfEXT( GLenum mode, GLfloat* m ) {
  void (*func_glMatrixMultfEXT)( GLenum, GLfloat* ) = extension_funcs[EXT_glMatrixMultfEXT];
  TRACE("(%d, %p)\n", mode, m );
  ENTER_GL();
  func_glMatrixMultfEXT( mode, m );
  LEAVE_GL();
}

static void WINAPI wine_glMatrixOrthoEXT( GLenum mode, GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar ) {
  void (*func_glMatrixOrthoEXT)( GLenum, GLdouble, GLdouble, GLdouble, GLdouble, GLdouble, GLdouble ) = extension_funcs[EXT_glMatrixOrthoEXT];
  TRACE("(%d, %f, %f, %f, %f, %f, %f)\n", mode, left, right, bottom, top, zNear, zFar );
  ENTER_GL();
  func_glMatrixOrthoEXT( mode, left, right, bottom, top, zNear, zFar );
  LEAVE_GL();
}

static void WINAPI wine_glMatrixPopEXT( GLenum mode ) {
  void (*func_glMatrixPopEXT)( GLenum ) = extension_funcs[EXT_glMatrixPopEXT];
  TRACE("(%d)\n", mode );
  ENTER_GL();
  func_glMatrixPopEXT( mode );
  LEAVE_GL();
}

static void WINAPI wine_glMatrixPushEXT( GLenum mode ) {
  void (*func_glMatrixPushEXT)( GLenum ) = extension_funcs[EXT_glMatrixPushEXT];
  TRACE("(%d)\n", mode );
  ENTER_GL();
  func_glMatrixPushEXT( mode );
  LEAVE_GL();
}

static void WINAPI wine_glMatrixRotatedEXT( GLenum mode, GLdouble angle, GLdouble x, GLdouble y, GLdouble z ) {
  void (*func_glMatrixRotatedEXT)( GLenum, GLdouble, GLdouble, GLdouble, GLdouble ) = extension_funcs[EXT_glMatrixRotatedEXT];
  TRACE("(%d, %f, %f, %f, %f)\n", mode, angle, x, y, z );
  ENTER_GL();
  func_glMatrixRotatedEXT( mode, angle, x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glMatrixRotatefEXT( GLenum mode, GLfloat angle, GLfloat x, GLfloat y, GLfloat z ) {
  void (*func_glMatrixRotatefEXT)( GLenum, GLfloat, GLfloat, GLfloat, GLfloat ) = extension_funcs[EXT_glMatrixRotatefEXT];
  TRACE("(%d, %f, %f, %f, %f)\n", mode, angle, x, y, z );
  ENTER_GL();
  func_glMatrixRotatefEXT( mode, angle, x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glMatrixScaledEXT( GLenum mode, GLdouble x, GLdouble y, GLdouble z ) {
  void (*func_glMatrixScaledEXT)( GLenum, GLdouble, GLdouble, GLdouble ) = extension_funcs[EXT_glMatrixScaledEXT];
  TRACE("(%d, %f, %f, %f)\n", mode, x, y, z );
  ENTER_GL();
  func_glMatrixScaledEXT( mode, x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glMatrixScalefEXT( GLenum mode, GLfloat x, GLfloat y, GLfloat z ) {
  void (*func_glMatrixScalefEXT)( GLenum, GLfloat, GLfloat, GLfloat ) = extension_funcs[EXT_glMatrixScalefEXT];
  TRACE("(%d, %f, %f, %f)\n", mode, x, y, z );
  ENTER_GL();
  func_glMatrixScalefEXT( mode, x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glMatrixTranslatedEXT( GLenum mode, GLdouble x, GLdouble y, GLdouble z ) {
  void (*func_glMatrixTranslatedEXT)( GLenum, GLdouble, GLdouble, GLdouble ) = extension_funcs[EXT_glMatrixTranslatedEXT];
  TRACE("(%d, %f, %f, %f)\n", mode, x, y, z );
  ENTER_GL();
  func_glMatrixTranslatedEXT( mode, x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glMatrixTranslatefEXT( GLenum mode, GLfloat x, GLfloat y, GLfloat z ) {
  void (*func_glMatrixTranslatefEXT)( GLenum, GLfloat, GLfloat, GLfloat ) = extension_funcs[EXT_glMatrixTranslatefEXT];
  TRACE("(%d, %f, %f, %f)\n", mode, x, y, z );
  ENTER_GL();
  func_glMatrixTranslatefEXT( mode, x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glMemoryBarrierEXT( GLbitfield barriers ) {
  void (*func_glMemoryBarrierEXT)( GLbitfield ) = extension_funcs[EXT_glMemoryBarrierEXT];
  TRACE("(%d)\n", barriers );
  ENTER_GL();
  func_glMemoryBarrierEXT( barriers );
  LEAVE_GL();
}

static void WINAPI wine_glMinSampleShading( GLclampf value ) {
  void (*func_glMinSampleShading)( GLclampf ) = extension_funcs[EXT_glMinSampleShading];
  TRACE("(%f)\n", value );
  ENTER_GL();
  func_glMinSampleShading( value );
  LEAVE_GL();
}

static void WINAPI wine_glMinSampleShadingARB( GLclampf value ) {
  void (*func_glMinSampleShadingARB)( GLclampf ) = extension_funcs[EXT_glMinSampleShadingARB];
  TRACE("(%f)\n", value );
  ENTER_GL();
  func_glMinSampleShadingARB( value );
  LEAVE_GL();
}

static void WINAPI wine_glMinmax( GLenum target, GLenum internalformat, GLboolean sink ) {
  void (*func_glMinmax)( GLenum, GLenum, GLboolean ) = extension_funcs[EXT_glMinmax];
  TRACE("(%d, %d, %d)\n", target, internalformat, sink );
  ENTER_GL();
  func_glMinmax( target, internalformat, sink );
  LEAVE_GL();
}

static void WINAPI wine_glMinmaxEXT( GLenum target, GLenum internalformat, GLboolean sink ) {
  void (*func_glMinmaxEXT)( GLenum, GLenum, GLboolean ) = extension_funcs[EXT_glMinmaxEXT];
  TRACE("(%d, %d, %d)\n", target, internalformat, sink );
  ENTER_GL();
  func_glMinmaxEXT( target, internalformat, sink );
  LEAVE_GL();
}

static void WINAPI wine_glMultTransposeMatrixd( GLdouble* m ) {
  void (*func_glMultTransposeMatrixd)( GLdouble* ) = extension_funcs[EXT_glMultTransposeMatrixd];
  TRACE("(%p)\n", m );
  ENTER_GL();
  func_glMultTransposeMatrixd( m );
  LEAVE_GL();
}

static void WINAPI wine_glMultTransposeMatrixdARB( GLdouble* m ) {
  void (*func_glMultTransposeMatrixdARB)( GLdouble* ) = extension_funcs[EXT_glMultTransposeMatrixdARB];
  TRACE("(%p)\n", m );
  ENTER_GL();
  func_glMultTransposeMatrixdARB( m );
  LEAVE_GL();
}

static void WINAPI wine_glMultTransposeMatrixf( GLfloat* m ) {
  void (*func_glMultTransposeMatrixf)( GLfloat* ) = extension_funcs[EXT_glMultTransposeMatrixf];
  TRACE("(%p)\n", m );
  ENTER_GL();
  func_glMultTransposeMatrixf( m );
  LEAVE_GL();
}

static void WINAPI wine_glMultTransposeMatrixfARB( GLfloat* m ) {
  void (*func_glMultTransposeMatrixfARB)( GLfloat* ) = extension_funcs[EXT_glMultTransposeMatrixfARB];
  TRACE("(%p)\n", m );
  ENTER_GL();
  func_glMultTransposeMatrixfARB( m );
  LEAVE_GL();
}

static void WINAPI wine_glMultiDrawArrays( GLenum mode, GLint* first, GLsizei* count, GLsizei primcount ) {
  void (*func_glMultiDrawArrays)( GLenum, GLint*, GLsizei*, GLsizei ) = extension_funcs[EXT_glMultiDrawArrays];
  TRACE("(%d, %p, %p, %d)\n", mode, first, count, primcount );
  ENTER_GL();
  func_glMultiDrawArrays( mode, first, count, primcount );
  LEAVE_GL();
}

static void WINAPI wine_glMultiDrawArraysEXT( GLenum mode, GLint* first, GLsizei* count, GLsizei primcount ) {
  void (*func_glMultiDrawArraysEXT)( GLenum, GLint*, GLsizei*, GLsizei ) = extension_funcs[EXT_glMultiDrawArraysEXT];
  TRACE("(%d, %p, %p, %d)\n", mode, first, count, primcount );
  ENTER_GL();
  func_glMultiDrawArraysEXT( mode, first, count, primcount );
  LEAVE_GL();
}

static void WINAPI wine_glMultiDrawElementArrayAPPLE( GLenum mode, GLint* first, GLsizei* count, GLsizei primcount ) {
  void (*func_glMultiDrawElementArrayAPPLE)( GLenum, GLint*, GLsizei*, GLsizei ) = extension_funcs[EXT_glMultiDrawElementArrayAPPLE];
  TRACE("(%d, %p, %p, %d)\n", mode, first, count, primcount );
  ENTER_GL();
  func_glMultiDrawElementArrayAPPLE( mode, first, count, primcount );
  LEAVE_GL();
}

static void WINAPI wine_glMultiDrawElements( GLenum mode, GLsizei* count, GLenum type, GLvoid** indices, GLsizei primcount ) {
  void (*func_glMultiDrawElements)( GLenum, GLsizei*, GLenum, GLvoid**, GLsizei ) = extension_funcs[EXT_glMultiDrawElements];
  TRACE("(%d, %p, %d, %p, %d)\n", mode, count, type, indices, primcount );
  ENTER_GL();
  func_glMultiDrawElements( mode, count, type, indices, primcount );
  LEAVE_GL();
}

static void WINAPI wine_glMultiDrawElementsBaseVertex( GLenum mode, GLsizei* count, GLenum type, GLvoid** indices, GLsizei primcount, GLint* basevertex ) {
  void (*func_glMultiDrawElementsBaseVertex)( GLenum, GLsizei*, GLenum, GLvoid**, GLsizei, GLint* ) = extension_funcs[EXT_glMultiDrawElementsBaseVertex];
  TRACE("(%d, %p, %d, %p, %d, %p)\n", mode, count, type, indices, primcount, basevertex );
  ENTER_GL();
  func_glMultiDrawElementsBaseVertex( mode, count, type, indices, primcount, basevertex );
  LEAVE_GL();
}

static void WINAPI wine_glMultiDrawElementsEXT( GLenum mode, GLsizei* count, GLenum type, GLvoid** indices, GLsizei primcount ) {
  void (*func_glMultiDrawElementsEXT)( GLenum, GLsizei*, GLenum, GLvoid**, GLsizei ) = extension_funcs[EXT_glMultiDrawElementsEXT];
  TRACE("(%d, %p, %d, %p, %d)\n", mode, count, type, indices, primcount );
  ENTER_GL();
  func_glMultiDrawElementsEXT( mode, count, type, indices, primcount );
  LEAVE_GL();
}

static void WINAPI wine_glMultiDrawRangeElementArrayAPPLE( GLenum mode, GLuint start, GLuint end, GLint* first, GLsizei* count, GLsizei primcount ) {
  void (*func_glMultiDrawRangeElementArrayAPPLE)( GLenum, GLuint, GLuint, GLint*, GLsizei*, GLsizei ) = extension_funcs[EXT_glMultiDrawRangeElementArrayAPPLE];
  TRACE("(%d, %d, %d, %p, %p, %d)\n", mode, start, end, first, count, primcount );
  ENTER_GL();
  func_glMultiDrawRangeElementArrayAPPLE( mode, start, end, first, count, primcount );
  LEAVE_GL();
}

static void WINAPI wine_glMultiModeDrawArraysIBM( GLenum* mode, GLint* first, GLsizei* count, GLsizei primcount, GLint modestride ) {
  void (*func_glMultiModeDrawArraysIBM)( GLenum*, GLint*, GLsizei*, GLsizei, GLint ) = extension_funcs[EXT_glMultiModeDrawArraysIBM];
  TRACE("(%p, %p, %p, %d, %d)\n", mode, first, count, primcount, modestride );
  ENTER_GL();
  func_glMultiModeDrawArraysIBM( mode, first, count, primcount, modestride );
  LEAVE_GL();
}

static void WINAPI wine_glMultiModeDrawElementsIBM( GLenum* mode, GLsizei* count, GLenum type, GLvoid* const* indices, GLsizei primcount, GLint modestride ) {
  void (*func_glMultiModeDrawElementsIBM)( GLenum*, GLsizei*, GLenum, GLvoid* const*, GLsizei, GLint ) = extension_funcs[EXT_glMultiModeDrawElementsIBM];
  TRACE("(%p, %p, %d, %p, %d, %d)\n", mode, count, type, indices, primcount, modestride );
  ENTER_GL();
  func_glMultiModeDrawElementsIBM( mode, count, type, indices, primcount, modestride );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexBufferEXT( GLenum texunit, GLenum target, GLenum internalformat, GLuint buffer ) {
  void (*func_glMultiTexBufferEXT)( GLenum, GLenum, GLenum, GLuint ) = extension_funcs[EXT_glMultiTexBufferEXT];
  TRACE("(%d, %d, %d, %d)\n", texunit, target, internalformat, buffer );
  ENTER_GL();
  func_glMultiTexBufferEXT( texunit, target, internalformat, buffer );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord1d( GLenum target, GLdouble s ) {
  void (*func_glMultiTexCoord1d)( GLenum, GLdouble ) = extension_funcs[EXT_glMultiTexCoord1d];
  TRACE("(%d, %f)\n", target, s );
  ENTER_GL();
  func_glMultiTexCoord1d( target, s );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord1dARB( GLenum target, GLdouble s ) {
  void (*func_glMultiTexCoord1dARB)( GLenum, GLdouble ) = extension_funcs[EXT_glMultiTexCoord1dARB];
  TRACE("(%d, %f)\n", target, s );
  ENTER_GL();
  func_glMultiTexCoord1dARB( target, s );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord1dSGIS( GLenum target, GLdouble s ) {
  void (*func_glMultiTexCoord1dSGIS)( GLenum, GLdouble ) = extension_funcs[EXT_glMultiTexCoord1dSGIS];
  TRACE("(%d, %f)\n", target, s );
  ENTER_GL();
  func_glMultiTexCoord1dSGIS( target, s );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord1dv( GLenum target, GLdouble* v ) {
  void (*func_glMultiTexCoord1dv)( GLenum, GLdouble* ) = extension_funcs[EXT_glMultiTexCoord1dv];
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord1dv( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord1dvARB( GLenum target, GLdouble* v ) {
  void (*func_glMultiTexCoord1dvARB)( GLenum, GLdouble* ) = extension_funcs[EXT_glMultiTexCoord1dvARB];
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord1dvARB( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord1dvSGIS( GLenum target, GLdouble * v ) {
  void (*func_glMultiTexCoord1dvSGIS)( GLenum, GLdouble * ) = extension_funcs[EXT_glMultiTexCoord1dvSGIS];
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord1dvSGIS( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord1f( GLenum target, GLfloat s ) {
  void (*func_glMultiTexCoord1f)( GLenum, GLfloat ) = extension_funcs[EXT_glMultiTexCoord1f];
  TRACE("(%d, %f)\n", target, s );
  ENTER_GL();
  func_glMultiTexCoord1f( target, s );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord1fARB( GLenum target, GLfloat s ) {
  void (*func_glMultiTexCoord1fARB)( GLenum, GLfloat ) = extension_funcs[EXT_glMultiTexCoord1fARB];
  TRACE("(%d, %f)\n", target, s );
  ENTER_GL();
  func_glMultiTexCoord1fARB( target, s );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord1fSGIS( GLenum target, GLfloat s ) {
  void (*func_glMultiTexCoord1fSGIS)( GLenum, GLfloat ) = extension_funcs[EXT_glMultiTexCoord1fSGIS];
  TRACE("(%d, %f)\n", target, s );
  ENTER_GL();
  func_glMultiTexCoord1fSGIS( target, s );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord1fv( GLenum target, GLfloat* v ) {
  void (*func_glMultiTexCoord1fv)( GLenum, GLfloat* ) = extension_funcs[EXT_glMultiTexCoord1fv];
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord1fv( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord1fvARB( GLenum target, GLfloat* v ) {
  void (*func_glMultiTexCoord1fvARB)( GLenum, GLfloat* ) = extension_funcs[EXT_glMultiTexCoord1fvARB];
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord1fvARB( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord1fvSGIS( GLenum target, const GLfloat * v ) {
  void (*func_glMultiTexCoord1fvSGIS)( GLenum, const GLfloat * ) = extension_funcs[EXT_glMultiTexCoord1fvSGIS];
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord1fvSGIS( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord1hNV( GLenum target, unsigned short s ) {
  void (*func_glMultiTexCoord1hNV)( GLenum, unsigned short ) = extension_funcs[EXT_glMultiTexCoord1hNV];
  TRACE("(%d, %d)\n", target, s );
  ENTER_GL();
  func_glMultiTexCoord1hNV( target, s );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord1hvNV( GLenum target, unsigned short* v ) {
  void (*func_glMultiTexCoord1hvNV)( GLenum, unsigned short* ) = extension_funcs[EXT_glMultiTexCoord1hvNV];
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord1hvNV( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord1i( GLenum target, GLint s ) {
  void (*func_glMultiTexCoord1i)( GLenum, GLint ) = extension_funcs[EXT_glMultiTexCoord1i];
  TRACE("(%d, %d)\n", target, s );
  ENTER_GL();
  func_glMultiTexCoord1i( target, s );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord1iARB( GLenum target, GLint s ) {
  void (*func_glMultiTexCoord1iARB)( GLenum, GLint ) = extension_funcs[EXT_glMultiTexCoord1iARB];
  TRACE("(%d, %d)\n", target, s );
  ENTER_GL();
  func_glMultiTexCoord1iARB( target, s );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord1iSGIS( GLenum target, GLint s ) {
  void (*func_glMultiTexCoord1iSGIS)( GLenum, GLint ) = extension_funcs[EXT_glMultiTexCoord1iSGIS];
  TRACE("(%d, %d)\n", target, s );
  ENTER_GL();
  func_glMultiTexCoord1iSGIS( target, s );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord1iv( GLenum target, GLint* v ) {
  void (*func_glMultiTexCoord1iv)( GLenum, GLint* ) = extension_funcs[EXT_glMultiTexCoord1iv];
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord1iv( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord1ivARB( GLenum target, GLint* v ) {
  void (*func_glMultiTexCoord1ivARB)( GLenum, GLint* ) = extension_funcs[EXT_glMultiTexCoord1ivARB];
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord1ivARB( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord1ivSGIS( GLenum target, GLint * v ) {
  void (*func_glMultiTexCoord1ivSGIS)( GLenum, GLint * ) = extension_funcs[EXT_glMultiTexCoord1ivSGIS];
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord1ivSGIS( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord1s( GLenum target, GLshort s ) {
  void (*func_glMultiTexCoord1s)( GLenum, GLshort ) = extension_funcs[EXT_glMultiTexCoord1s];
  TRACE("(%d, %d)\n", target, s );
  ENTER_GL();
  func_glMultiTexCoord1s( target, s );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord1sARB( GLenum target, GLshort s ) {
  void (*func_glMultiTexCoord1sARB)( GLenum, GLshort ) = extension_funcs[EXT_glMultiTexCoord1sARB];
  TRACE("(%d, %d)\n", target, s );
  ENTER_GL();
  func_glMultiTexCoord1sARB( target, s );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord1sSGIS( GLenum target, GLshort s ) {
  void (*func_glMultiTexCoord1sSGIS)( GLenum, GLshort ) = extension_funcs[EXT_glMultiTexCoord1sSGIS];
  TRACE("(%d, %d)\n", target, s );
  ENTER_GL();
  func_glMultiTexCoord1sSGIS( target, s );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord1sv( GLenum target, GLshort* v ) {
  void (*func_glMultiTexCoord1sv)( GLenum, GLshort* ) = extension_funcs[EXT_glMultiTexCoord1sv];
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord1sv( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord1svARB( GLenum target, GLshort* v ) {
  void (*func_glMultiTexCoord1svARB)( GLenum, GLshort* ) = extension_funcs[EXT_glMultiTexCoord1svARB];
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord1svARB( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord1svSGIS( GLenum target, GLshort * v ) {
  void (*func_glMultiTexCoord1svSGIS)( GLenum, GLshort * ) = extension_funcs[EXT_glMultiTexCoord1svSGIS];
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord1svSGIS( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord2d( GLenum target, GLdouble s, GLdouble t ) {
  void (*func_glMultiTexCoord2d)( GLenum, GLdouble, GLdouble ) = extension_funcs[EXT_glMultiTexCoord2d];
  TRACE("(%d, %f, %f)\n", target, s, t );
  ENTER_GL();
  func_glMultiTexCoord2d( target, s, t );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord2dARB( GLenum target, GLdouble s, GLdouble t ) {
  void (*func_glMultiTexCoord2dARB)( GLenum, GLdouble, GLdouble ) = extension_funcs[EXT_glMultiTexCoord2dARB];
  TRACE("(%d, %f, %f)\n", target, s, t );
  ENTER_GL();
  func_glMultiTexCoord2dARB( target, s, t );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord2dSGIS( GLenum target, GLdouble s, GLdouble t ) {
  void (*func_glMultiTexCoord2dSGIS)( GLenum, GLdouble, GLdouble ) = extension_funcs[EXT_glMultiTexCoord2dSGIS];
  TRACE("(%d, %f, %f)\n", target, s, t );
  ENTER_GL();
  func_glMultiTexCoord2dSGIS( target, s, t );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord2dv( GLenum target, GLdouble* v ) {
  void (*func_glMultiTexCoord2dv)( GLenum, GLdouble* ) = extension_funcs[EXT_glMultiTexCoord2dv];
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord2dv( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord2dvARB( GLenum target, GLdouble* v ) {
  void (*func_glMultiTexCoord2dvARB)( GLenum, GLdouble* ) = extension_funcs[EXT_glMultiTexCoord2dvARB];
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord2dvARB( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord2dvSGIS( GLenum target, GLdouble * v ) {
  void (*func_glMultiTexCoord2dvSGIS)( GLenum, GLdouble * ) = extension_funcs[EXT_glMultiTexCoord2dvSGIS];
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord2dvSGIS( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord2f( GLenum target, GLfloat s, GLfloat t ) {
  void (*func_glMultiTexCoord2f)( GLenum, GLfloat, GLfloat ) = extension_funcs[EXT_glMultiTexCoord2f];
  TRACE("(%d, %f, %f)\n", target, s, t );
  ENTER_GL();
  func_glMultiTexCoord2f( target, s, t );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord2fARB( GLenum target, GLfloat s, GLfloat t ) {
  void (*func_glMultiTexCoord2fARB)( GLenum, GLfloat, GLfloat ) = extension_funcs[EXT_glMultiTexCoord2fARB];
  TRACE("(%d, %f, %f)\n", target, s, t );
  ENTER_GL();
  func_glMultiTexCoord2fARB( target, s, t );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord2fSGIS( GLenum target, GLfloat s, GLfloat t ) {
  void (*func_glMultiTexCoord2fSGIS)( GLenum, GLfloat, GLfloat ) = extension_funcs[EXT_glMultiTexCoord2fSGIS];
  TRACE("(%d, %f, %f)\n", target, s, t );
  ENTER_GL();
  func_glMultiTexCoord2fSGIS( target, s, t );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord2fv( GLenum target, GLfloat* v ) {
  void (*func_glMultiTexCoord2fv)( GLenum, GLfloat* ) = extension_funcs[EXT_glMultiTexCoord2fv];
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord2fv( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord2fvARB( GLenum target, GLfloat* v ) {
  void (*func_glMultiTexCoord2fvARB)( GLenum, GLfloat* ) = extension_funcs[EXT_glMultiTexCoord2fvARB];
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord2fvARB( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord2fvSGIS( GLenum target, GLfloat * v ) {
  void (*func_glMultiTexCoord2fvSGIS)( GLenum, GLfloat * ) = extension_funcs[EXT_glMultiTexCoord2fvSGIS];
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord2fvSGIS( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord2hNV( GLenum target, unsigned short s, unsigned short t ) {
  void (*func_glMultiTexCoord2hNV)( GLenum, unsigned short, unsigned short ) = extension_funcs[EXT_glMultiTexCoord2hNV];
  TRACE("(%d, %d, %d)\n", target, s, t );
  ENTER_GL();
  func_glMultiTexCoord2hNV( target, s, t );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord2hvNV( GLenum target, unsigned short* v ) {
  void (*func_glMultiTexCoord2hvNV)( GLenum, unsigned short* ) = extension_funcs[EXT_glMultiTexCoord2hvNV];
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord2hvNV( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord2i( GLenum target, GLint s, GLint t ) {
  void (*func_glMultiTexCoord2i)( GLenum, GLint, GLint ) = extension_funcs[EXT_glMultiTexCoord2i];
  TRACE("(%d, %d, %d)\n", target, s, t );
  ENTER_GL();
  func_glMultiTexCoord2i( target, s, t );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord2iARB( GLenum target, GLint s, GLint t ) {
  void (*func_glMultiTexCoord2iARB)( GLenum, GLint, GLint ) = extension_funcs[EXT_glMultiTexCoord2iARB];
  TRACE("(%d, %d, %d)\n", target, s, t );
  ENTER_GL();
  func_glMultiTexCoord2iARB( target, s, t );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord2iSGIS( GLenum target, GLint s, GLint t ) {
  void (*func_glMultiTexCoord2iSGIS)( GLenum, GLint, GLint ) = extension_funcs[EXT_glMultiTexCoord2iSGIS];
  TRACE("(%d, %d, %d)\n", target, s, t );
  ENTER_GL();
  func_glMultiTexCoord2iSGIS( target, s, t );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord2iv( GLenum target, GLint* v ) {
  void (*func_glMultiTexCoord2iv)( GLenum, GLint* ) = extension_funcs[EXT_glMultiTexCoord2iv];
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord2iv( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord2ivARB( GLenum target, GLint* v ) {
  void (*func_glMultiTexCoord2ivARB)( GLenum, GLint* ) = extension_funcs[EXT_glMultiTexCoord2ivARB];
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord2ivARB( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord2ivSGIS( GLenum target, GLint * v ) {
  void (*func_glMultiTexCoord2ivSGIS)( GLenum, GLint * ) = extension_funcs[EXT_glMultiTexCoord2ivSGIS];
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord2ivSGIS( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord2s( GLenum target, GLshort s, GLshort t ) {
  void (*func_glMultiTexCoord2s)( GLenum, GLshort, GLshort ) = extension_funcs[EXT_glMultiTexCoord2s];
  TRACE("(%d, %d, %d)\n", target, s, t );
  ENTER_GL();
  func_glMultiTexCoord2s( target, s, t );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord2sARB( GLenum target, GLshort s, GLshort t ) {
  void (*func_glMultiTexCoord2sARB)( GLenum, GLshort, GLshort ) = extension_funcs[EXT_glMultiTexCoord2sARB];
  TRACE("(%d, %d, %d)\n", target, s, t );
  ENTER_GL();
  func_glMultiTexCoord2sARB( target, s, t );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord2sSGIS( GLenum target, GLshort s, GLshort t ) {
  void (*func_glMultiTexCoord2sSGIS)( GLenum, GLshort, GLshort ) = extension_funcs[EXT_glMultiTexCoord2sSGIS];
  TRACE("(%d, %d, %d)\n", target, s, t );
  ENTER_GL();
  func_glMultiTexCoord2sSGIS( target, s, t );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord2sv( GLenum target, GLshort* v ) {
  void (*func_glMultiTexCoord2sv)( GLenum, GLshort* ) = extension_funcs[EXT_glMultiTexCoord2sv];
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord2sv( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord2svARB( GLenum target, GLshort* v ) {
  void (*func_glMultiTexCoord2svARB)( GLenum, GLshort* ) = extension_funcs[EXT_glMultiTexCoord2svARB];
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord2svARB( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord2svSGIS( GLenum target, GLshort * v ) {
  void (*func_glMultiTexCoord2svSGIS)( GLenum, GLshort * ) = extension_funcs[EXT_glMultiTexCoord2svSGIS];
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord2svSGIS( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord3d( GLenum target, GLdouble s, GLdouble t, GLdouble r ) {
  void (*func_glMultiTexCoord3d)( GLenum, GLdouble, GLdouble, GLdouble ) = extension_funcs[EXT_glMultiTexCoord3d];
  TRACE("(%d, %f, %f, %f)\n", target, s, t, r );
  ENTER_GL();
  func_glMultiTexCoord3d( target, s, t, r );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord3dARB( GLenum target, GLdouble s, GLdouble t, GLdouble r ) {
  void (*func_glMultiTexCoord3dARB)( GLenum, GLdouble, GLdouble, GLdouble ) = extension_funcs[EXT_glMultiTexCoord3dARB];
  TRACE("(%d, %f, %f, %f)\n", target, s, t, r );
  ENTER_GL();
  func_glMultiTexCoord3dARB( target, s, t, r );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord3dSGIS( GLenum target, GLdouble s, GLdouble t, GLdouble r ) {
  void (*func_glMultiTexCoord3dSGIS)( GLenum, GLdouble, GLdouble, GLdouble ) = extension_funcs[EXT_glMultiTexCoord3dSGIS];
  TRACE("(%d, %f, %f, %f)\n", target, s, t, r );
  ENTER_GL();
  func_glMultiTexCoord3dSGIS( target, s, t, r );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord3dv( GLenum target, GLdouble* v ) {
  void (*func_glMultiTexCoord3dv)( GLenum, GLdouble* ) = extension_funcs[EXT_glMultiTexCoord3dv];
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord3dv( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord3dvARB( GLenum target, GLdouble* v ) {
  void (*func_glMultiTexCoord3dvARB)( GLenum, GLdouble* ) = extension_funcs[EXT_glMultiTexCoord3dvARB];
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord3dvARB( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord3dvSGIS( GLenum target, GLdouble * v ) {
  void (*func_glMultiTexCoord3dvSGIS)( GLenum, GLdouble * ) = extension_funcs[EXT_glMultiTexCoord3dvSGIS];
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord3dvSGIS( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord3f( GLenum target, GLfloat s, GLfloat t, GLfloat r ) {
  void (*func_glMultiTexCoord3f)( GLenum, GLfloat, GLfloat, GLfloat ) = extension_funcs[EXT_glMultiTexCoord3f];
  TRACE("(%d, %f, %f, %f)\n", target, s, t, r );
  ENTER_GL();
  func_glMultiTexCoord3f( target, s, t, r );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord3fARB( GLenum target, GLfloat s, GLfloat t, GLfloat r ) {
  void (*func_glMultiTexCoord3fARB)( GLenum, GLfloat, GLfloat, GLfloat ) = extension_funcs[EXT_glMultiTexCoord3fARB];
  TRACE("(%d, %f, %f, %f)\n", target, s, t, r );
  ENTER_GL();
  func_glMultiTexCoord3fARB( target, s, t, r );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord3fSGIS( GLenum target, GLfloat s, GLfloat t, GLfloat r ) {
  void (*func_glMultiTexCoord3fSGIS)( GLenum, GLfloat, GLfloat, GLfloat ) = extension_funcs[EXT_glMultiTexCoord3fSGIS];
  TRACE("(%d, %f, %f, %f)\n", target, s, t, r );
  ENTER_GL();
  func_glMultiTexCoord3fSGIS( target, s, t, r );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord3fv( GLenum target, GLfloat* v ) {
  void (*func_glMultiTexCoord3fv)( GLenum, GLfloat* ) = extension_funcs[EXT_glMultiTexCoord3fv];
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord3fv( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord3fvARB( GLenum target, GLfloat* v ) {
  void (*func_glMultiTexCoord3fvARB)( GLenum, GLfloat* ) = extension_funcs[EXT_glMultiTexCoord3fvARB];
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord3fvARB( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord3fvSGIS( GLenum target, GLfloat * v ) {
  void (*func_glMultiTexCoord3fvSGIS)( GLenum, GLfloat * ) = extension_funcs[EXT_glMultiTexCoord3fvSGIS];
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord3fvSGIS( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord3hNV( GLenum target, unsigned short s, unsigned short t, unsigned short r ) {
  void (*func_glMultiTexCoord3hNV)( GLenum, unsigned short, unsigned short, unsigned short ) = extension_funcs[EXT_glMultiTexCoord3hNV];
  TRACE("(%d, %d, %d, %d)\n", target, s, t, r );
  ENTER_GL();
  func_glMultiTexCoord3hNV( target, s, t, r );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord3hvNV( GLenum target, unsigned short* v ) {
  void (*func_glMultiTexCoord3hvNV)( GLenum, unsigned short* ) = extension_funcs[EXT_glMultiTexCoord3hvNV];
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord3hvNV( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord3i( GLenum target, GLint s, GLint t, GLint r ) {
  void (*func_glMultiTexCoord3i)( GLenum, GLint, GLint, GLint ) = extension_funcs[EXT_glMultiTexCoord3i];
  TRACE("(%d, %d, %d, %d)\n", target, s, t, r );
  ENTER_GL();
  func_glMultiTexCoord3i( target, s, t, r );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord3iARB( GLenum target, GLint s, GLint t, GLint r ) {
  void (*func_glMultiTexCoord3iARB)( GLenum, GLint, GLint, GLint ) = extension_funcs[EXT_glMultiTexCoord3iARB];
  TRACE("(%d, %d, %d, %d)\n", target, s, t, r );
  ENTER_GL();
  func_glMultiTexCoord3iARB( target, s, t, r );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord3iSGIS( GLenum target, GLint s, GLint t, GLint r ) {
  void (*func_glMultiTexCoord3iSGIS)( GLenum, GLint, GLint, GLint ) = extension_funcs[EXT_glMultiTexCoord3iSGIS];
  TRACE("(%d, %d, %d, %d)\n", target, s, t, r );
  ENTER_GL();
  func_glMultiTexCoord3iSGIS( target, s, t, r );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord3iv( GLenum target, GLint* v ) {
  void (*func_glMultiTexCoord3iv)( GLenum, GLint* ) = extension_funcs[EXT_glMultiTexCoord3iv];
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord3iv( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord3ivARB( GLenum target, GLint* v ) {
  void (*func_glMultiTexCoord3ivARB)( GLenum, GLint* ) = extension_funcs[EXT_glMultiTexCoord3ivARB];
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord3ivARB( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord3ivSGIS( GLenum target, GLint * v ) {
  void (*func_glMultiTexCoord3ivSGIS)( GLenum, GLint * ) = extension_funcs[EXT_glMultiTexCoord3ivSGIS];
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord3ivSGIS( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord3s( GLenum target, GLshort s, GLshort t, GLshort r ) {
  void (*func_glMultiTexCoord3s)( GLenum, GLshort, GLshort, GLshort ) = extension_funcs[EXT_glMultiTexCoord3s];
  TRACE("(%d, %d, %d, %d)\n", target, s, t, r );
  ENTER_GL();
  func_glMultiTexCoord3s( target, s, t, r );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord3sARB( GLenum target, GLshort s, GLshort t, GLshort r ) {
  void (*func_glMultiTexCoord3sARB)( GLenum, GLshort, GLshort, GLshort ) = extension_funcs[EXT_glMultiTexCoord3sARB];
  TRACE("(%d, %d, %d, %d)\n", target, s, t, r );
  ENTER_GL();
  func_glMultiTexCoord3sARB( target, s, t, r );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord3sSGIS( GLenum target, GLshort s, GLshort t, GLshort r ) {
  void (*func_glMultiTexCoord3sSGIS)( GLenum, GLshort, GLshort, GLshort ) = extension_funcs[EXT_glMultiTexCoord3sSGIS];
  TRACE("(%d, %d, %d, %d)\n", target, s, t, r );
  ENTER_GL();
  func_glMultiTexCoord3sSGIS( target, s, t, r );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord3sv( GLenum target, GLshort* v ) {
  void (*func_glMultiTexCoord3sv)( GLenum, GLshort* ) = extension_funcs[EXT_glMultiTexCoord3sv];
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord3sv( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord3svARB( GLenum target, GLshort* v ) {
  void (*func_glMultiTexCoord3svARB)( GLenum, GLshort* ) = extension_funcs[EXT_glMultiTexCoord3svARB];
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord3svARB( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord3svSGIS( GLenum target, GLshort * v ) {
  void (*func_glMultiTexCoord3svSGIS)( GLenum, GLshort * ) = extension_funcs[EXT_glMultiTexCoord3svSGIS];
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord3svSGIS( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord4d( GLenum target, GLdouble s, GLdouble t, GLdouble r, GLdouble q ) {
  void (*func_glMultiTexCoord4d)( GLenum, GLdouble, GLdouble, GLdouble, GLdouble ) = extension_funcs[EXT_glMultiTexCoord4d];
  TRACE("(%d, %f, %f, %f, %f)\n", target, s, t, r, q );
  ENTER_GL();
  func_glMultiTexCoord4d( target, s, t, r, q );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord4dARB( GLenum target, GLdouble s, GLdouble t, GLdouble r, GLdouble q ) {
  void (*func_glMultiTexCoord4dARB)( GLenum, GLdouble, GLdouble, GLdouble, GLdouble ) = extension_funcs[EXT_glMultiTexCoord4dARB];
  TRACE("(%d, %f, %f, %f, %f)\n", target, s, t, r, q );
  ENTER_GL();
  func_glMultiTexCoord4dARB( target, s, t, r, q );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord4dSGIS( GLenum target, GLdouble s, GLdouble t, GLdouble r, GLdouble q ) {
  void (*func_glMultiTexCoord4dSGIS)( GLenum, GLdouble, GLdouble, GLdouble, GLdouble ) = extension_funcs[EXT_glMultiTexCoord4dSGIS];
  TRACE("(%d, %f, %f, %f, %f)\n", target, s, t, r, q );
  ENTER_GL();
  func_glMultiTexCoord4dSGIS( target, s, t, r, q );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord4dv( GLenum target, GLdouble* v ) {
  void (*func_glMultiTexCoord4dv)( GLenum, GLdouble* ) = extension_funcs[EXT_glMultiTexCoord4dv];
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord4dv( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord4dvARB( GLenum target, GLdouble* v ) {
  void (*func_glMultiTexCoord4dvARB)( GLenum, GLdouble* ) = extension_funcs[EXT_glMultiTexCoord4dvARB];
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord4dvARB( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord4dvSGIS( GLenum target, GLdouble * v ) {
  void (*func_glMultiTexCoord4dvSGIS)( GLenum, GLdouble * ) = extension_funcs[EXT_glMultiTexCoord4dvSGIS];
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord4dvSGIS( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord4f( GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q ) {
  void (*func_glMultiTexCoord4f)( GLenum, GLfloat, GLfloat, GLfloat, GLfloat ) = extension_funcs[EXT_glMultiTexCoord4f];
  TRACE("(%d, %f, %f, %f, %f)\n", target, s, t, r, q );
  ENTER_GL();
  func_glMultiTexCoord4f( target, s, t, r, q );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord4fARB( GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q ) {
  void (*func_glMultiTexCoord4fARB)( GLenum, GLfloat, GLfloat, GLfloat, GLfloat ) = extension_funcs[EXT_glMultiTexCoord4fARB];
  TRACE("(%d, %f, %f, %f, %f)\n", target, s, t, r, q );
  ENTER_GL();
  func_glMultiTexCoord4fARB( target, s, t, r, q );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord4fSGIS( GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q ) {
  void (*func_glMultiTexCoord4fSGIS)( GLenum, GLfloat, GLfloat, GLfloat, GLfloat ) = extension_funcs[EXT_glMultiTexCoord4fSGIS];
  TRACE("(%d, %f, %f, %f, %f)\n", target, s, t, r, q );
  ENTER_GL();
  func_glMultiTexCoord4fSGIS( target, s, t, r, q );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord4fv( GLenum target, GLfloat* v ) {
  void (*func_glMultiTexCoord4fv)( GLenum, GLfloat* ) = extension_funcs[EXT_glMultiTexCoord4fv];
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord4fv( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord4fvARB( GLenum target, GLfloat* v ) {
  void (*func_glMultiTexCoord4fvARB)( GLenum, GLfloat* ) = extension_funcs[EXT_glMultiTexCoord4fvARB];
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord4fvARB( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord4fvSGIS( GLenum target, GLfloat * v ) {
  void (*func_glMultiTexCoord4fvSGIS)( GLenum, GLfloat * ) = extension_funcs[EXT_glMultiTexCoord4fvSGIS];
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord4fvSGIS( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord4hNV( GLenum target, unsigned short s, unsigned short t, unsigned short r, unsigned short q ) {
  void (*func_glMultiTexCoord4hNV)( GLenum, unsigned short, unsigned short, unsigned short, unsigned short ) = extension_funcs[EXT_glMultiTexCoord4hNV];
  TRACE("(%d, %d, %d, %d, %d)\n", target, s, t, r, q );
  ENTER_GL();
  func_glMultiTexCoord4hNV( target, s, t, r, q );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord4hvNV( GLenum target, unsigned short* v ) {
  void (*func_glMultiTexCoord4hvNV)( GLenum, unsigned short* ) = extension_funcs[EXT_glMultiTexCoord4hvNV];
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord4hvNV( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord4i( GLenum target, GLint s, GLint t, GLint r, GLint q ) {
  void (*func_glMultiTexCoord4i)( GLenum, GLint, GLint, GLint, GLint ) = extension_funcs[EXT_glMultiTexCoord4i];
  TRACE("(%d, %d, %d, %d, %d)\n", target, s, t, r, q );
  ENTER_GL();
  func_glMultiTexCoord4i( target, s, t, r, q );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord4iARB( GLenum target, GLint s, GLint t, GLint r, GLint q ) {
  void (*func_glMultiTexCoord4iARB)( GLenum, GLint, GLint, GLint, GLint ) = extension_funcs[EXT_glMultiTexCoord4iARB];
  TRACE("(%d, %d, %d, %d, %d)\n", target, s, t, r, q );
  ENTER_GL();
  func_glMultiTexCoord4iARB( target, s, t, r, q );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord4iSGIS( GLenum target, GLint s, GLint t, GLint r, GLint q ) {
  void (*func_glMultiTexCoord4iSGIS)( GLenum, GLint, GLint, GLint, GLint ) = extension_funcs[EXT_glMultiTexCoord4iSGIS];
  TRACE("(%d, %d, %d, %d, %d)\n", target, s, t, r, q );
  ENTER_GL();
  func_glMultiTexCoord4iSGIS( target, s, t, r, q );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord4iv( GLenum target, GLint* v ) {
  void (*func_glMultiTexCoord4iv)( GLenum, GLint* ) = extension_funcs[EXT_glMultiTexCoord4iv];
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord4iv( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord4ivARB( GLenum target, GLint* v ) {
  void (*func_glMultiTexCoord4ivARB)( GLenum, GLint* ) = extension_funcs[EXT_glMultiTexCoord4ivARB];
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord4ivARB( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord4ivSGIS( GLenum target, GLint * v ) {
  void (*func_glMultiTexCoord4ivSGIS)( GLenum, GLint * ) = extension_funcs[EXT_glMultiTexCoord4ivSGIS];
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord4ivSGIS( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord4s( GLenum target, GLshort s, GLshort t, GLshort r, GLshort q ) {
  void (*func_glMultiTexCoord4s)( GLenum, GLshort, GLshort, GLshort, GLshort ) = extension_funcs[EXT_glMultiTexCoord4s];
  TRACE("(%d, %d, %d, %d, %d)\n", target, s, t, r, q );
  ENTER_GL();
  func_glMultiTexCoord4s( target, s, t, r, q );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord4sARB( GLenum target, GLshort s, GLshort t, GLshort r, GLshort q ) {
  void (*func_glMultiTexCoord4sARB)( GLenum, GLshort, GLshort, GLshort, GLshort ) = extension_funcs[EXT_glMultiTexCoord4sARB];
  TRACE("(%d, %d, %d, %d, %d)\n", target, s, t, r, q );
  ENTER_GL();
  func_glMultiTexCoord4sARB( target, s, t, r, q );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord4sSGIS( GLenum target, GLshort s, GLshort t, GLshort r, GLshort q ) {
  void (*func_glMultiTexCoord4sSGIS)( GLenum, GLshort, GLshort, GLshort, GLshort ) = extension_funcs[EXT_glMultiTexCoord4sSGIS];
  TRACE("(%d, %d, %d, %d, %d)\n", target, s, t, r, q );
  ENTER_GL();
  func_glMultiTexCoord4sSGIS( target, s, t, r, q );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord4sv( GLenum target, GLshort* v ) {
  void (*func_glMultiTexCoord4sv)( GLenum, GLshort* ) = extension_funcs[EXT_glMultiTexCoord4sv];
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord4sv( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord4svARB( GLenum target, GLshort* v ) {
  void (*func_glMultiTexCoord4svARB)( GLenum, GLshort* ) = extension_funcs[EXT_glMultiTexCoord4svARB];
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord4svARB( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoord4svSGIS( GLenum target, GLshort * v ) {
  void (*func_glMultiTexCoord4svSGIS)( GLenum, GLshort * ) = extension_funcs[EXT_glMultiTexCoord4svSGIS];
  TRACE("(%d, %p)\n", target, v );
  ENTER_GL();
  func_glMultiTexCoord4svSGIS( target, v );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoordP1ui( GLenum texture, GLenum type, GLuint coords ) {
  void (*func_glMultiTexCoordP1ui)( GLenum, GLenum, GLuint ) = extension_funcs[EXT_glMultiTexCoordP1ui];
  TRACE("(%d, %d, %d)\n", texture, type, coords );
  ENTER_GL();
  func_glMultiTexCoordP1ui( texture, type, coords );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoordP1uiv( GLenum texture, GLenum type, GLuint* coords ) {
  void (*func_glMultiTexCoordP1uiv)( GLenum, GLenum, GLuint* ) = extension_funcs[EXT_glMultiTexCoordP1uiv];
  TRACE("(%d, %d, %p)\n", texture, type, coords );
  ENTER_GL();
  func_glMultiTexCoordP1uiv( texture, type, coords );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoordP2ui( GLenum texture, GLenum type, GLuint coords ) {
  void (*func_glMultiTexCoordP2ui)( GLenum, GLenum, GLuint ) = extension_funcs[EXT_glMultiTexCoordP2ui];
  TRACE("(%d, %d, %d)\n", texture, type, coords );
  ENTER_GL();
  func_glMultiTexCoordP2ui( texture, type, coords );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoordP2uiv( GLenum texture, GLenum type, GLuint* coords ) {
  void (*func_glMultiTexCoordP2uiv)( GLenum, GLenum, GLuint* ) = extension_funcs[EXT_glMultiTexCoordP2uiv];
  TRACE("(%d, %d, %p)\n", texture, type, coords );
  ENTER_GL();
  func_glMultiTexCoordP2uiv( texture, type, coords );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoordP3ui( GLenum texture, GLenum type, GLuint coords ) {
  void (*func_glMultiTexCoordP3ui)( GLenum, GLenum, GLuint ) = extension_funcs[EXT_glMultiTexCoordP3ui];
  TRACE("(%d, %d, %d)\n", texture, type, coords );
  ENTER_GL();
  func_glMultiTexCoordP3ui( texture, type, coords );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoordP3uiv( GLenum texture, GLenum type, GLuint* coords ) {
  void (*func_glMultiTexCoordP3uiv)( GLenum, GLenum, GLuint* ) = extension_funcs[EXT_glMultiTexCoordP3uiv];
  TRACE("(%d, %d, %p)\n", texture, type, coords );
  ENTER_GL();
  func_glMultiTexCoordP3uiv( texture, type, coords );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoordP4ui( GLenum texture, GLenum type, GLuint coords ) {
  void (*func_glMultiTexCoordP4ui)( GLenum, GLenum, GLuint ) = extension_funcs[EXT_glMultiTexCoordP4ui];
  TRACE("(%d, %d, %d)\n", texture, type, coords );
  ENTER_GL();
  func_glMultiTexCoordP4ui( texture, type, coords );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoordP4uiv( GLenum texture, GLenum type, GLuint* coords ) {
  void (*func_glMultiTexCoordP4uiv)( GLenum, GLenum, GLuint* ) = extension_funcs[EXT_glMultiTexCoordP4uiv];
  TRACE("(%d, %d, %p)\n", texture, type, coords );
  ENTER_GL();
  func_glMultiTexCoordP4uiv( texture, type, coords );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoordPointerEXT( GLenum texunit, GLint size, GLenum type, GLsizei stride, GLvoid* pointer ) {
  void (*func_glMultiTexCoordPointerEXT)( GLenum, GLint, GLenum, GLsizei, GLvoid* ) = extension_funcs[EXT_glMultiTexCoordPointerEXT];
  TRACE("(%d, %d, %d, %d, %p)\n", texunit, size, type, stride, pointer );
  ENTER_GL();
  func_glMultiTexCoordPointerEXT( texunit, size, type, stride, pointer );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexCoordPointerSGIS( GLenum target, GLint size, GLenum type, GLsizei stride, GLvoid * pointer ) {
  void (*func_glMultiTexCoordPointerSGIS)( GLenum, GLint, GLenum, GLsizei, GLvoid * ) = extension_funcs[EXT_glMultiTexCoordPointerSGIS];
  TRACE("(%d, %d, %d, %d, %p)\n", target, size, type, stride, pointer );
  ENTER_GL();
  func_glMultiTexCoordPointerSGIS( target, size, type, stride, pointer );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexEnvfEXT( GLenum texunit, GLenum target, GLenum pname, GLfloat param ) {
  void (*func_glMultiTexEnvfEXT)( GLenum, GLenum, GLenum, GLfloat ) = extension_funcs[EXT_glMultiTexEnvfEXT];
  TRACE("(%d, %d, %d, %f)\n", texunit, target, pname, param );
  ENTER_GL();
  func_glMultiTexEnvfEXT( texunit, target, pname, param );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexEnvfvEXT( GLenum texunit, GLenum target, GLenum pname, GLfloat* params ) {
  void (*func_glMultiTexEnvfvEXT)( GLenum, GLenum, GLenum, GLfloat* ) = extension_funcs[EXT_glMultiTexEnvfvEXT];
  TRACE("(%d, %d, %d, %p)\n", texunit, target, pname, params );
  ENTER_GL();
  func_glMultiTexEnvfvEXT( texunit, target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexEnviEXT( GLenum texunit, GLenum target, GLenum pname, GLint param ) {
  void (*func_glMultiTexEnviEXT)( GLenum, GLenum, GLenum, GLint ) = extension_funcs[EXT_glMultiTexEnviEXT];
  TRACE("(%d, %d, %d, %d)\n", texunit, target, pname, param );
  ENTER_GL();
  func_glMultiTexEnviEXT( texunit, target, pname, param );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexEnvivEXT( GLenum texunit, GLenum target, GLenum pname, GLint* params ) {
  void (*func_glMultiTexEnvivEXT)( GLenum, GLenum, GLenum, GLint* ) = extension_funcs[EXT_glMultiTexEnvivEXT];
  TRACE("(%d, %d, %d, %p)\n", texunit, target, pname, params );
  ENTER_GL();
  func_glMultiTexEnvivEXT( texunit, target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexGendEXT( GLenum texunit, GLenum coord, GLenum pname, GLdouble param ) {
  void (*func_glMultiTexGendEXT)( GLenum, GLenum, GLenum, GLdouble ) = extension_funcs[EXT_glMultiTexGendEXT];
  TRACE("(%d, %d, %d, %f)\n", texunit, coord, pname, param );
  ENTER_GL();
  func_glMultiTexGendEXT( texunit, coord, pname, param );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexGendvEXT( GLenum texunit, GLenum coord, GLenum pname, GLdouble* params ) {
  void (*func_glMultiTexGendvEXT)( GLenum, GLenum, GLenum, GLdouble* ) = extension_funcs[EXT_glMultiTexGendvEXT];
  TRACE("(%d, %d, %d, %p)\n", texunit, coord, pname, params );
  ENTER_GL();
  func_glMultiTexGendvEXT( texunit, coord, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexGenfEXT( GLenum texunit, GLenum coord, GLenum pname, GLfloat param ) {
  void (*func_glMultiTexGenfEXT)( GLenum, GLenum, GLenum, GLfloat ) = extension_funcs[EXT_glMultiTexGenfEXT];
  TRACE("(%d, %d, %d, %f)\n", texunit, coord, pname, param );
  ENTER_GL();
  func_glMultiTexGenfEXT( texunit, coord, pname, param );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexGenfvEXT( GLenum texunit, GLenum coord, GLenum pname, GLfloat* params ) {
  void (*func_glMultiTexGenfvEXT)( GLenum, GLenum, GLenum, GLfloat* ) = extension_funcs[EXT_glMultiTexGenfvEXT];
  TRACE("(%d, %d, %d, %p)\n", texunit, coord, pname, params );
  ENTER_GL();
  func_glMultiTexGenfvEXT( texunit, coord, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexGeniEXT( GLenum texunit, GLenum coord, GLenum pname, GLint param ) {
  void (*func_glMultiTexGeniEXT)( GLenum, GLenum, GLenum, GLint ) = extension_funcs[EXT_glMultiTexGeniEXT];
  TRACE("(%d, %d, %d, %d)\n", texunit, coord, pname, param );
  ENTER_GL();
  func_glMultiTexGeniEXT( texunit, coord, pname, param );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexGenivEXT( GLenum texunit, GLenum coord, GLenum pname, GLint* params ) {
  void (*func_glMultiTexGenivEXT)( GLenum, GLenum, GLenum, GLint* ) = extension_funcs[EXT_glMultiTexGenivEXT];
  TRACE("(%d, %d, %d, %p)\n", texunit, coord, pname, params );
  ENTER_GL();
  func_glMultiTexGenivEXT( texunit, coord, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexImage1DEXT( GLenum texunit, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLenum format, GLenum type, GLvoid* pixels ) {
  void (*func_glMultiTexImage1DEXT)( GLenum, GLenum, GLint, GLenum, GLsizei, GLint, GLenum, GLenum, GLvoid* ) = extension_funcs[EXT_glMultiTexImage1DEXT];
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %p)\n", texunit, target, level, internalformat, width, border, format, type, pixels );
  ENTER_GL();
  func_glMultiTexImage1DEXT( texunit, target, level, internalformat, width, border, format, type, pixels );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexImage2DEXT( GLenum texunit, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, GLvoid* pixels ) {
  void (*func_glMultiTexImage2DEXT)( GLenum, GLenum, GLint, GLenum, GLsizei, GLsizei, GLint, GLenum, GLenum, GLvoid* ) = extension_funcs[EXT_glMultiTexImage2DEXT];
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %d, %p)\n", texunit, target, level, internalformat, width, height, border, format, type, pixels );
  ENTER_GL();
  func_glMultiTexImage2DEXT( texunit, target, level, internalformat, width, height, border, format, type, pixels );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexImage3DEXT( GLenum texunit, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, GLvoid* pixels ) {
  void (*func_glMultiTexImage3DEXT)( GLenum, GLenum, GLint, GLenum, GLsizei, GLsizei, GLsizei, GLint, GLenum, GLenum, GLvoid* ) = extension_funcs[EXT_glMultiTexImage3DEXT];
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %p)\n", texunit, target, level, internalformat, width, height, depth, border, format, type, pixels );
  ENTER_GL();
  func_glMultiTexImage3DEXT( texunit, target, level, internalformat, width, height, depth, border, format, type, pixels );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexParameterIivEXT( GLenum texunit, GLenum target, GLenum pname, GLint* params ) {
  void (*func_glMultiTexParameterIivEXT)( GLenum, GLenum, GLenum, GLint* ) = extension_funcs[EXT_glMultiTexParameterIivEXT];
  TRACE("(%d, %d, %d, %p)\n", texunit, target, pname, params );
  ENTER_GL();
  func_glMultiTexParameterIivEXT( texunit, target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexParameterIuivEXT( GLenum texunit, GLenum target, GLenum pname, GLuint* params ) {
  void (*func_glMultiTexParameterIuivEXT)( GLenum, GLenum, GLenum, GLuint* ) = extension_funcs[EXT_glMultiTexParameterIuivEXT];
  TRACE("(%d, %d, %d, %p)\n", texunit, target, pname, params );
  ENTER_GL();
  func_glMultiTexParameterIuivEXT( texunit, target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexParameterfEXT( GLenum texunit, GLenum target, GLenum pname, GLfloat param ) {
  void (*func_glMultiTexParameterfEXT)( GLenum, GLenum, GLenum, GLfloat ) = extension_funcs[EXT_glMultiTexParameterfEXT];
  TRACE("(%d, %d, %d, %f)\n", texunit, target, pname, param );
  ENTER_GL();
  func_glMultiTexParameterfEXT( texunit, target, pname, param );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexParameterfvEXT( GLenum texunit, GLenum target, GLenum pname, GLfloat* params ) {
  void (*func_glMultiTexParameterfvEXT)( GLenum, GLenum, GLenum, GLfloat* ) = extension_funcs[EXT_glMultiTexParameterfvEXT];
  TRACE("(%d, %d, %d, %p)\n", texunit, target, pname, params );
  ENTER_GL();
  func_glMultiTexParameterfvEXT( texunit, target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexParameteriEXT( GLenum texunit, GLenum target, GLenum pname, GLint param ) {
  void (*func_glMultiTexParameteriEXT)( GLenum, GLenum, GLenum, GLint ) = extension_funcs[EXT_glMultiTexParameteriEXT];
  TRACE("(%d, %d, %d, %d)\n", texunit, target, pname, param );
  ENTER_GL();
  func_glMultiTexParameteriEXT( texunit, target, pname, param );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexParameterivEXT( GLenum texunit, GLenum target, GLenum pname, GLint* params ) {
  void (*func_glMultiTexParameterivEXT)( GLenum, GLenum, GLenum, GLint* ) = extension_funcs[EXT_glMultiTexParameterivEXT];
  TRACE("(%d, %d, %d, %p)\n", texunit, target, pname, params );
  ENTER_GL();
  func_glMultiTexParameterivEXT( texunit, target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexRenderbufferEXT( GLenum texunit, GLenum target, GLuint renderbuffer ) {
  void (*func_glMultiTexRenderbufferEXT)( GLenum, GLenum, GLuint ) = extension_funcs[EXT_glMultiTexRenderbufferEXT];
  TRACE("(%d, %d, %d)\n", texunit, target, renderbuffer );
  ENTER_GL();
  func_glMultiTexRenderbufferEXT( texunit, target, renderbuffer );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexSubImage1DEXT( GLenum texunit, GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, GLvoid* pixels ) {
  void (*func_glMultiTexSubImage1DEXT)( GLenum, GLenum, GLint, GLint, GLsizei, GLenum, GLenum, GLvoid* ) = extension_funcs[EXT_glMultiTexSubImage1DEXT];
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %p)\n", texunit, target, level, xoffset, width, format, type, pixels );
  ENTER_GL();
  func_glMultiTexSubImage1DEXT( texunit, target, level, xoffset, width, format, type, pixels );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexSubImage2DEXT( GLenum texunit, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid* pixels ) {
  void (*func_glMultiTexSubImage2DEXT)( GLenum, GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, GLvoid* ) = extension_funcs[EXT_glMultiTexSubImage2DEXT];
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %d, %p)\n", texunit, target, level, xoffset, yoffset, width, height, format, type, pixels );
  ENTER_GL();
  func_glMultiTexSubImage2DEXT( texunit, target, level, xoffset, yoffset, width, height, format, type, pixels );
  LEAVE_GL();
}

static void WINAPI wine_glMultiTexSubImage3DEXT( GLenum texunit, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, GLvoid* pixels ) {
  void (*func_glMultiTexSubImage3DEXT)( GLenum, GLenum, GLint, GLint, GLint, GLint, GLsizei, GLsizei, GLsizei, GLenum, GLenum, GLvoid* ) = extension_funcs[EXT_glMultiTexSubImage3DEXT];
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %p)\n", texunit, target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels );
  ENTER_GL();
  func_glMultiTexSubImage3DEXT( texunit, target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels );
  LEAVE_GL();
}

static void WINAPI wine_glNamedBufferDataEXT( GLuint buffer, INT_PTR size, GLvoid* data, GLenum usage ) {
  void (*func_glNamedBufferDataEXT)( GLuint, INT_PTR, GLvoid*, GLenum ) = extension_funcs[EXT_glNamedBufferDataEXT];
  TRACE("(%d, %ld, %p, %d)\n", buffer, size, data, usage );
  ENTER_GL();
  func_glNamedBufferDataEXT( buffer, size, data, usage );
  LEAVE_GL();
}

static void WINAPI wine_glNamedBufferSubDataEXT( GLuint buffer, INT_PTR offset, INT_PTR size, GLvoid* data ) {
  void (*func_glNamedBufferSubDataEXT)( GLuint, INT_PTR, INT_PTR, GLvoid* ) = extension_funcs[EXT_glNamedBufferSubDataEXT];
  TRACE("(%d, %ld, %ld, %p)\n", buffer, offset, size, data );
  ENTER_GL();
  func_glNamedBufferSubDataEXT( buffer, offset, size, data );
  LEAVE_GL();
}

static void WINAPI wine_glNamedCopyBufferSubDataEXT( GLuint readBuffer, GLuint writeBuffer, INT_PTR readOffset, INT_PTR writeOffset, INT_PTR size ) {
  void (*func_glNamedCopyBufferSubDataEXT)( GLuint, GLuint, INT_PTR, INT_PTR, INT_PTR ) = extension_funcs[EXT_glNamedCopyBufferSubDataEXT];
  TRACE("(%d, %d, %ld, %ld, %ld)\n", readBuffer, writeBuffer, readOffset, writeOffset, size );
  ENTER_GL();
  func_glNamedCopyBufferSubDataEXT( readBuffer, writeBuffer, readOffset, writeOffset, size );
  LEAVE_GL();
}

static void WINAPI wine_glNamedFramebufferRenderbufferEXT( GLuint framebuffer, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer ) {
  void (*func_glNamedFramebufferRenderbufferEXT)( GLuint, GLenum, GLenum, GLuint ) = extension_funcs[EXT_glNamedFramebufferRenderbufferEXT];
  TRACE("(%d, %d, %d, %d)\n", framebuffer, attachment, renderbuffertarget, renderbuffer );
  ENTER_GL();
  func_glNamedFramebufferRenderbufferEXT( framebuffer, attachment, renderbuffertarget, renderbuffer );
  LEAVE_GL();
}

static void WINAPI wine_glNamedFramebufferTexture1DEXT( GLuint framebuffer, GLenum attachment, GLenum textarget, GLuint texture, GLint level ) {
  void (*func_glNamedFramebufferTexture1DEXT)( GLuint, GLenum, GLenum, GLuint, GLint ) = extension_funcs[EXT_glNamedFramebufferTexture1DEXT];
  TRACE("(%d, %d, %d, %d, %d)\n", framebuffer, attachment, textarget, texture, level );
  ENTER_GL();
  func_glNamedFramebufferTexture1DEXT( framebuffer, attachment, textarget, texture, level );
  LEAVE_GL();
}

static void WINAPI wine_glNamedFramebufferTexture2DEXT( GLuint framebuffer, GLenum attachment, GLenum textarget, GLuint texture, GLint level ) {
  void (*func_glNamedFramebufferTexture2DEXT)( GLuint, GLenum, GLenum, GLuint, GLint ) = extension_funcs[EXT_glNamedFramebufferTexture2DEXT];
  TRACE("(%d, %d, %d, %d, %d)\n", framebuffer, attachment, textarget, texture, level );
  ENTER_GL();
  func_glNamedFramebufferTexture2DEXT( framebuffer, attachment, textarget, texture, level );
  LEAVE_GL();
}

static void WINAPI wine_glNamedFramebufferTexture3DEXT( GLuint framebuffer, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint zoffset ) {
  void (*func_glNamedFramebufferTexture3DEXT)( GLuint, GLenum, GLenum, GLuint, GLint, GLint ) = extension_funcs[EXT_glNamedFramebufferTexture3DEXT];
  TRACE("(%d, %d, %d, %d, %d, %d)\n", framebuffer, attachment, textarget, texture, level, zoffset );
  ENTER_GL();
  func_glNamedFramebufferTexture3DEXT( framebuffer, attachment, textarget, texture, level, zoffset );
  LEAVE_GL();
}

static void WINAPI wine_glNamedFramebufferTextureEXT( GLuint framebuffer, GLenum attachment, GLuint texture, GLint level ) {
  void (*func_glNamedFramebufferTextureEXT)( GLuint, GLenum, GLuint, GLint ) = extension_funcs[EXT_glNamedFramebufferTextureEXT];
  TRACE("(%d, %d, %d, %d)\n", framebuffer, attachment, texture, level );
  ENTER_GL();
  func_glNamedFramebufferTextureEXT( framebuffer, attachment, texture, level );
  LEAVE_GL();
}

static void WINAPI wine_glNamedFramebufferTextureFaceEXT( GLuint framebuffer, GLenum attachment, GLuint texture, GLint level, GLenum face ) {
  void (*func_glNamedFramebufferTextureFaceEXT)( GLuint, GLenum, GLuint, GLint, GLenum ) = extension_funcs[EXT_glNamedFramebufferTextureFaceEXT];
  TRACE("(%d, %d, %d, %d, %d)\n", framebuffer, attachment, texture, level, face );
  ENTER_GL();
  func_glNamedFramebufferTextureFaceEXT( framebuffer, attachment, texture, level, face );
  LEAVE_GL();
}

static void WINAPI wine_glNamedFramebufferTextureLayerEXT( GLuint framebuffer, GLenum attachment, GLuint texture, GLint level, GLint layer ) {
  void (*func_glNamedFramebufferTextureLayerEXT)( GLuint, GLenum, GLuint, GLint, GLint ) = extension_funcs[EXT_glNamedFramebufferTextureLayerEXT];
  TRACE("(%d, %d, %d, %d, %d)\n", framebuffer, attachment, texture, level, layer );
  ENTER_GL();
  func_glNamedFramebufferTextureLayerEXT( framebuffer, attachment, texture, level, layer );
  LEAVE_GL();
}

static void WINAPI wine_glNamedProgramLocalParameter4dEXT( GLuint program, GLenum target, GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w ) {
  void (*func_glNamedProgramLocalParameter4dEXT)( GLuint, GLenum, GLuint, GLdouble, GLdouble, GLdouble, GLdouble ) = extension_funcs[EXT_glNamedProgramLocalParameter4dEXT];
  TRACE("(%d, %d, %d, %f, %f, %f, %f)\n", program, target, index, x, y, z, w );
  ENTER_GL();
  func_glNamedProgramLocalParameter4dEXT( program, target, index, x, y, z, w );
  LEAVE_GL();
}

static void WINAPI wine_glNamedProgramLocalParameter4dvEXT( GLuint program, GLenum target, GLuint index, GLdouble* params ) {
  void (*func_glNamedProgramLocalParameter4dvEXT)( GLuint, GLenum, GLuint, GLdouble* ) = extension_funcs[EXT_glNamedProgramLocalParameter4dvEXT];
  TRACE("(%d, %d, %d, %p)\n", program, target, index, params );
  ENTER_GL();
  func_glNamedProgramLocalParameter4dvEXT( program, target, index, params );
  LEAVE_GL();
}

static void WINAPI wine_glNamedProgramLocalParameter4fEXT( GLuint program, GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w ) {
  void (*func_glNamedProgramLocalParameter4fEXT)( GLuint, GLenum, GLuint, GLfloat, GLfloat, GLfloat, GLfloat ) = extension_funcs[EXT_glNamedProgramLocalParameter4fEXT];
  TRACE("(%d, %d, %d, %f, %f, %f, %f)\n", program, target, index, x, y, z, w );
  ENTER_GL();
  func_glNamedProgramLocalParameter4fEXT( program, target, index, x, y, z, w );
  LEAVE_GL();
}

static void WINAPI wine_glNamedProgramLocalParameter4fvEXT( GLuint program, GLenum target, GLuint index, GLfloat* params ) {
  void (*func_glNamedProgramLocalParameter4fvEXT)( GLuint, GLenum, GLuint, GLfloat* ) = extension_funcs[EXT_glNamedProgramLocalParameter4fvEXT];
  TRACE("(%d, %d, %d, %p)\n", program, target, index, params );
  ENTER_GL();
  func_glNamedProgramLocalParameter4fvEXT( program, target, index, params );
  LEAVE_GL();
}

static void WINAPI wine_glNamedProgramLocalParameterI4iEXT( GLuint program, GLenum target, GLuint index, GLint x, GLint y, GLint z, GLint w ) {
  void (*func_glNamedProgramLocalParameterI4iEXT)( GLuint, GLenum, GLuint, GLint, GLint, GLint, GLint ) = extension_funcs[EXT_glNamedProgramLocalParameterI4iEXT];
  TRACE("(%d, %d, %d, %d, %d, %d, %d)\n", program, target, index, x, y, z, w );
  ENTER_GL();
  func_glNamedProgramLocalParameterI4iEXT( program, target, index, x, y, z, w );
  LEAVE_GL();
}

static void WINAPI wine_glNamedProgramLocalParameterI4ivEXT( GLuint program, GLenum target, GLuint index, GLint* params ) {
  void (*func_glNamedProgramLocalParameterI4ivEXT)( GLuint, GLenum, GLuint, GLint* ) = extension_funcs[EXT_glNamedProgramLocalParameterI4ivEXT];
  TRACE("(%d, %d, %d, %p)\n", program, target, index, params );
  ENTER_GL();
  func_glNamedProgramLocalParameterI4ivEXT( program, target, index, params );
  LEAVE_GL();
}

static void WINAPI wine_glNamedProgramLocalParameterI4uiEXT( GLuint program, GLenum target, GLuint index, GLuint x, GLuint y, GLuint z, GLuint w ) {
  void (*func_glNamedProgramLocalParameterI4uiEXT)( GLuint, GLenum, GLuint, GLuint, GLuint, GLuint, GLuint ) = extension_funcs[EXT_glNamedProgramLocalParameterI4uiEXT];
  TRACE("(%d, %d, %d, %d, %d, %d, %d)\n", program, target, index, x, y, z, w );
  ENTER_GL();
  func_glNamedProgramLocalParameterI4uiEXT( program, target, index, x, y, z, w );
  LEAVE_GL();
}

static void WINAPI wine_glNamedProgramLocalParameterI4uivEXT( GLuint program, GLenum target, GLuint index, GLuint* params ) {
  void (*func_glNamedProgramLocalParameterI4uivEXT)( GLuint, GLenum, GLuint, GLuint* ) = extension_funcs[EXT_glNamedProgramLocalParameterI4uivEXT];
  TRACE("(%d, %d, %d, %p)\n", program, target, index, params );
  ENTER_GL();
  func_glNamedProgramLocalParameterI4uivEXT( program, target, index, params );
  LEAVE_GL();
}

static void WINAPI wine_glNamedProgramLocalParameters4fvEXT( GLuint program, GLenum target, GLuint index, GLsizei count, GLfloat* params ) {
  void (*func_glNamedProgramLocalParameters4fvEXT)( GLuint, GLenum, GLuint, GLsizei, GLfloat* ) = extension_funcs[EXT_glNamedProgramLocalParameters4fvEXT];
  TRACE("(%d, %d, %d, %d, %p)\n", program, target, index, count, params );
  ENTER_GL();
  func_glNamedProgramLocalParameters4fvEXT( program, target, index, count, params );
  LEAVE_GL();
}

static void WINAPI wine_glNamedProgramLocalParametersI4ivEXT( GLuint program, GLenum target, GLuint index, GLsizei count, GLint* params ) {
  void (*func_glNamedProgramLocalParametersI4ivEXT)( GLuint, GLenum, GLuint, GLsizei, GLint* ) = extension_funcs[EXT_glNamedProgramLocalParametersI4ivEXT];
  TRACE("(%d, %d, %d, %d, %p)\n", program, target, index, count, params );
  ENTER_GL();
  func_glNamedProgramLocalParametersI4ivEXT( program, target, index, count, params );
  LEAVE_GL();
}

static void WINAPI wine_glNamedProgramLocalParametersI4uivEXT( GLuint program, GLenum target, GLuint index, GLsizei count, GLuint* params ) {
  void (*func_glNamedProgramLocalParametersI4uivEXT)( GLuint, GLenum, GLuint, GLsizei, GLuint* ) = extension_funcs[EXT_glNamedProgramLocalParametersI4uivEXT];
  TRACE("(%d, %d, %d, %d, %p)\n", program, target, index, count, params );
  ENTER_GL();
  func_glNamedProgramLocalParametersI4uivEXT( program, target, index, count, params );
  LEAVE_GL();
}

static void WINAPI wine_glNamedProgramStringEXT( GLuint program, GLenum target, GLenum format, GLsizei len, GLvoid* string ) {
  void (*func_glNamedProgramStringEXT)( GLuint, GLenum, GLenum, GLsizei, GLvoid* ) = extension_funcs[EXT_glNamedProgramStringEXT];
  TRACE("(%d, %d, %d, %d, %p)\n", program, target, format, len, string );
  ENTER_GL();
  func_glNamedProgramStringEXT( program, target, format, len, string );
  LEAVE_GL();
}

static void WINAPI wine_glNamedRenderbufferStorageEXT( GLuint renderbuffer, GLenum internalformat, GLsizei width, GLsizei height ) {
  void (*func_glNamedRenderbufferStorageEXT)( GLuint, GLenum, GLsizei, GLsizei ) = extension_funcs[EXT_glNamedRenderbufferStorageEXT];
  TRACE("(%d, %d, %d, %d)\n", renderbuffer, internalformat, width, height );
  ENTER_GL();
  func_glNamedRenderbufferStorageEXT( renderbuffer, internalformat, width, height );
  LEAVE_GL();
}

static void WINAPI wine_glNamedRenderbufferStorageMultisampleCoverageEXT( GLuint renderbuffer, GLsizei coverageSamples, GLsizei colorSamples, GLenum internalformat, GLsizei width, GLsizei height ) {
  void (*func_glNamedRenderbufferStorageMultisampleCoverageEXT)( GLuint, GLsizei, GLsizei, GLenum, GLsizei, GLsizei ) = extension_funcs[EXT_glNamedRenderbufferStorageMultisampleCoverageEXT];
  TRACE("(%d, %d, %d, %d, %d, %d)\n", renderbuffer, coverageSamples, colorSamples, internalformat, width, height );
  ENTER_GL();
  func_glNamedRenderbufferStorageMultisampleCoverageEXT( renderbuffer, coverageSamples, colorSamples, internalformat, width, height );
  LEAVE_GL();
}

static void WINAPI wine_glNamedRenderbufferStorageMultisampleEXT( GLuint renderbuffer, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height ) {
  void (*func_glNamedRenderbufferStorageMultisampleEXT)( GLuint, GLsizei, GLenum, GLsizei, GLsizei ) = extension_funcs[EXT_glNamedRenderbufferStorageMultisampleEXT];
  TRACE("(%d, %d, %d, %d, %d)\n", renderbuffer, samples, internalformat, width, height );
  ENTER_GL();
  func_glNamedRenderbufferStorageMultisampleEXT( renderbuffer, samples, internalformat, width, height );
  LEAVE_GL();
}

static void WINAPI wine_glNamedStringARB( GLenum type, GLint namelen, char* name, GLint stringlen, char* string ) {
  void (*func_glNamedStringARB)( GLenum, GLint, char*, GLint, char* ) = extension_funcs[EXT_glNamedStringARB];
  TRACE("(%d, %d, %p, %d, %p)\n", type, namelen, name, stringlen, string );
  ENTER_GL();
  func_glNamedStringARB( type, namelen, name, stringlen, string );
  LEAVE_GL();
}

static GLuint WINAPI wine_glNewBufferRegion( GLenum type ) {
  GLuint ret_value;
  GLuint (*func_glNewBufferRegion)( GLenum ) = extension_funcs[EXT_glNewBufferRegion];
  TRACE("(%d)\n", type );
  ENTER_GL();
  ret_value = func_glNewBufferRegion( type );
  LEAVE_GL();
  return ret_value;
}

static GLuint WINAPI wine_glNewObjectBufferATI( GLsizei size, GLvoid* pointer, GLenum usage ) {
  GLuint ret_value;
  GLuint (*func_glNewObjectBufferATI)( GLsizei, GLvoid*, GLenum ) = extension_funcs[EXT_glNewObjectBufferATI];
  TRACE("(%d, %p, %d)\n", size, pointer, usage );
  ENTER_GL();
  ret_value = func_glNewObjectBufferATI( size, pointer, usage );
  LEAVE_GL();
  return ret_value;
}

static void WINAPI wine_glNormal3fVertex3fSUN( GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z ) {
  void (*func_glNormal3fVertex3fSUN)( GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat ) = extension_funcs[EXT_glNormal3fVertex3fSUN];
  TRACE("(%f, %f, %f, %f, %f, %f)\n", nx, ny, nz, x, y, z );
  ENTER_GL();
  func_glNormal3fVertex3fSUN( nx, ny, nz, x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glNormal3fVertex3fvSUN( GLfloat* n, GLfloat* v ) {
  void (*func_glNormal3fVertex3fvSUN)( GLfloat*, GLfloat* ) = extension_funcs[EXT_glNormal3fVertex3fvSUN];
  TRACE("(%p, %p)\n", n, v );
  ENTER_GL();
  func_glNormal3fVertex3fvSUN( n, v );
  LEAVE_GL();
}

static void WINAPI wine_glNormal3hNV( unsigned short nx, unsigned short ny, unsigned short nz ) {
  void (*func_glNormal3hNV)( unsigned short, unsigned short, unsigned short ) = extension_funcs[EXT_glNormal3hNV];
  TRACE("(%d, %d, %d)\n", nx, ny, nz );
  ENTER_GL();
  func_glNormal3hNV( nx, ny, nz );
  LEAVE_GL();
}

static void WINAPI wine_glNormal3hvNV( unsigned short* v ) {
  void (*func_glNormal3hvNV)( unsigned short* ) = extension_funcs[EXT_glNormal3hvNV];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glNormal3hvNV( v );
  LEAVE_GL();
}

static void WINAPI wine_glNormalFormatNV( GLenum type, GLsizei stride ) {
  void (*func_glNormalFormatNV)( GLenum, GLsizei ) = extension_funcs[EXT_glNormalFormatNV];
  TRACE("(%d, %d)\n", type, stride );
  ENTER_GL();
  func_glNormalFormatNV( type, stride );
  LEAVE_GL();
}

static void WINAPI wine_glNormalP3ui( GLenum type, GLuint coords ) {
  void (*func_glNormalP3ui)( GLenum, GLuint ) = extension_funcs[EXT_glNormalP3ui];
  TRACE("(%d, %d)\n", type, coords );
  ENTER_GL();
  func_glNormalP3ui( type, coords );
  LEAVE_GL();
}

static void WINAPI wine_glNormalP3uiv( GLenum type, GLuint* coords ) {
  void (*func_glNormalP3uiv)( GLenum, GLuint* ) = extension_funcs[EXT_glNormalP3uiv];
  TRACE("(%d, %p)\n", type, coords );
  ENTER_GL();
  func_glNormalP3uiv( type, coords );
  LEAVE_GL();
}

static void WINAPI wine_glNormalPointerEXT( GLenum type, GLsizei stride, GLsizei count, GLvoid* pointer ) {
  void (*func_glNormalPointerEXT)( GLenum, GLsizei, GLsizei, GLvoid* ) = extension_funcs[EXT_glNormalPointerEXT];
  TRACE("(%d, %d, %d, %p)\n", type, stride, count, pointer );
  ENTER_GL();
  func_glNormalPointerEXT( type, stride, count, pointer );
  LEAVE_GL();
}

static void WINAPI wine_glNormalPointerListIBM( GLenum type, GLint stride, GLvoid** pointer, GLint ptrstride ) {
  void (*func_glNormalPointerListIBM)( GLenum, GLint, GLvoid**, GLint ) = extension_funcs[EXT_glNormalPointerListIBM];
  TRACE("(%d, %d, %p, %d)\n", type, stride, pointer, ptrstride );
  ENTER_GL();
  func_glNormalPointerListIBM( type, stride, pointer, ptrstride );
  LEAVE_GL();
}

static void WINAPI wine_glNormalPointervINTEL( GLenum type, GLvoid** pointer ) {
  void (*func_glNormalPointervINTEL)( GLenum, GLvoid** ) = extension_funcs[EXT_glNormalPointervINTEL];
  TRACE("(%d, %p)\n", type, pointer );
  ENTER_GL();
  func_glNormalPointervINTEL( type, pointer );
  LEAVE_GL();
}

static void WINAPI wine_glNormalStream3bATI( GLenum stream, GLbyte nx, GLbyte ny, GLbyte nz ) {
  void (*func_glNormalStream3bATI)( GLenum, GLbyte, GLbyte, GLbyte ) = extension_funcs[EXT_glNormalStream3bATI];
  TRACE("(%d, %d, %d, %d)\n", stream, nx, ny, nz );
  ENTER_GL();
  func_glNormalStream3bATI( stream, nx, ny, nz );
  LEAVE_GL();
}

static void WINAPI wine_glNormalStream3bvATI( GLenum stream, GLbyte* coords ) {
  void (*func_glNormalStream3bvATI)( GLenum, GLbyte* ) = extension_funcs[EXT_glNormalStream3bvATI];
  TRACE("(%d, %p)\n", stream, coords );
  ENTER_GL();
  func_glNormalStream3bvATI( stream, coords );
  LEAVE_GL();
}

static void WINAPI wine_glNormalStream3dATI( GLenum stream, GLdouble nx, GLdouble ny, GLdouble nz ) {
  void (*func_glNormalStream3dATI)( GLenum, GLdouble, GLdouble, GLdouble ) = extension_funcs[EXT_glNormalStream3dATI];
  TRACE("(%d, %f, %f, %f)\n", stream, nx, ny, nz );
  ENTER_GL();
  func_glNormalStream3dATI( stream, nx, ny, nz );
  LEAVE_GL();
}

static void WINAPI wine_glNormalStream3dvATI( GLenum stream, GLdouble* coords ) {
  void (*func_glNormalStream3dvATI)( GLenum, GLdouble* ) = extension_funcs[EXT_glNormalStream3dvATI];
  TRACE("(%d, %p)\n", stream, coords );
  ENTER_GL();
  func_glNormalStream3dvATI( stream, coords );
  LEAVE_GL();
}

static void WINAPI wine_glNormalStream3fATI( GLenum stream, GLfloat nx, GLfloat ny, GLfloat nz ) {
  void (*func_glNormalStream3fATI)( GLenum, GLfloat, GLfloat, GLfloat ) = extension_funcs[EXT_glNormalStream3fATI];
  TRACE("(%d, %f, %f, %f)\n", stream, nx, ny, nz );
  ENTER_GL();
  func_glNormalStream3fATI( stream, nx, ny, nz );
  LEAVE_GL();
}

static void WINAPI wine_glNormalStream3fvATI( GLenum stream, GLfloat* coords ) {
  void (*func_glNormalStream3fvATI)( GLenum, GLfloat* ) = extension_funcs[EXT_glNormalStream3fvATI];
  TRACE("(%d, %p)\n", stream, coords );
  ENTER_GL();
  func_glNormalStream3fvATI( stream, coords );
  LEAVE_GL();
}

static void WINAPI wine_glNormalStream3iATI( GLenum stream, GLint nx, GLint ny, GLint nz ) {
  void (*func_glNormalStream3iATI)( GLenum, GLint, GLint, GLint ) = extension_funcs[EXT_glNormalStream3iATI];
  TRACE("(%d, %d, %d, %d)\n", stream, nx, ny, nz );
  ENTER_GL();
  func_glNormalStream3iATI( stream, nx, ny, nz );
  LEAVE_GL();
}

static void WINAPI wine_glNormalStream3ivATI( GLenum stream, GLint* coords ) {
  void (*func_glNormalStream3ivATI)( GLenum, GLint* ) = extension_funcs[EXT_glNormalStream3ivATI];
  TRACE("(%d, %p)\n", stream, coords );
  ENTER_GL();
  func_glNormalStream3ivATI( stream, coords );
  LEAVE_GL();
}

static void WINAPI wine_glNormalStream3sATI( GLenum stream, GLshort nx, GLshort ny, GLshort nz ) {
  void (*func_glNormalStream3sATI)( GLenum, GLshort, GLshort, GLshort ) = extension_funcs[EXT_glNormalStream3sATI];
  TRACE("(%d, %d, %d, %d)\n", stream, nx, ny, nz );
  ENTER_GL();
  func_glNormalStream3sATI( stream, nx, ny, nz );
  LEAVE_GL();
}

static void WINAPI wine_glNormalStream3svATI( GLenum stream, GLshort* coords ) {
  void (*func_glNormalStream3svATI)( GLenum, GLshort* ) = extension_funcs[EXT_glNormalStream3svATI];
  TRACE("(%d, %p)\n", stream, coords );
  ENTER_GL();
  func_glNormalStream3svATI( stream, coords );
  LEAVE_GL();
}

static GLenum WINAPI wine_glObjectPurgeableAPPLE( GLenum objectType, GLuint name, GLenum option ) {
  GLenum ret_value;
  GLenum (*func_glObjectPurgeableAPPLE)( GLenum, GLuint, GLenum ) = extension_funcs[EXT_glObjectPurgeableAPPLE];
  TRACE("(%d, %d, %d)\n", objectType, name, option );
  ENTER_GL();
  ret_value = func_glObjectPurgeableAPPLE( objectType, name, option );
  LEAVE_GL();
  return ret_value;
}

static GLenum WINAPI wine_glObjectUnpurgeableAPPLE( GLenum objectType, GLuint name, GLenum option ) {
  GLenum ret_value;
  GLenum (*func_glObjectUnpurgeableAPPLE)( GLenum, GLuint, GLenum ) = extension_funcs[EXT_glObjectUnpurgeableAPPLE];
  TRACE("(%d, %d, %d)\n", objectType, name, option );
  ENTER_GL();
  ret_value = func_glObjectUnpurgeableAPPLE( objectType, name, option );
  LEAVE_GL();
  return ret_value;
}

static void WINAPI wine_glPNTrianglesfATI( GLenum pname, GLfloat param ) {
  void (*func_glPNTrianglesfATI)( GLenum, GLfloat ) = extension_funcs[EXT_glPNTrianglesfATI];
  TRACE("(%d, %f)\n", pname, param );
  ENTER_GL();
  func_glPNTrianglesfATI( pname, param );
  LEAVE_GL();
}

static void WINAPI wine_glPNTrianglesiATI( GLenum pname, GLint param ) {
  void (*func_glPNTrianglesiATI)( GLenum, GLint ) = extension_funcs[EXT_glPNTrianglesiATI];
  TRACE("(%d, %d)\n", pname, param );
  ENTER_GL();
  func_glPNTrianglesiATI( pname, param );
  LEAVE_GL();
}

static void WINAPI wine_glPassTexCoordATI( GLuint dst, GLuint coord, GLenum swizzle ) {
  void (*func_glPassTexCoordATI)( GLuint, GLuint, GLenum ) = extension_funcs[EXT_glPassTexCoordATI];
  TRACE("(%d, %d, %d)\n", dst, coord, swizzle );
  ENTER_GL();
  func_glPassTexCoordATI( dst, coord, swizzle );
  LEAVE_GL();
}

static void WINAPI wine_glPatchParameterfv( GLenum pname, GLfloat* values ) {
  void (*func_glPatchParameterfv)( GLenum, GLfloat* ) = extension_funcs[EXT_glPatchParameterfv];
  TRACE("(%d, %p)\n", pname, values );
  ENTER_GL();
  func_glPatchParameterfv( pname, values );
  LEAVE_GL();
}

static void WINAPI wine_glPatchParameteri( GLenum pname, GLint value ) {
  void (*func_glPatchParameteri)( GLenum, GLint ) = extension_funcs[EXT_glPatchParameteri];
  TRACE("(%d, %d)\n", pname, value );
  ENTER_GL();
  func_glPatchParameteri( pname, value );
  LEAVE_GL();
}

static void WINAPI wine_glPauseTransformFeedback( void ) {
  void (*func_glPauseTransformFeedback)( void ) = extension_funcs[EXT_glPauseTransformFeedback];
  TRACE("()\n");
  ENTER_GL();
  func_glPauseTransformFeedback( );
  LEAVE_GL();
}

static void WINAPI wine_glPauseTransformFeedbackNV( void ) {
  void (*func_glPauseTransformFeedbackNV)( void ) = extension_funcs[EXT_glPauseTransformFeedbackNV];
  TRACE("()\n");
  ENTER_GL();
  func_glPauseTransformFeedbackNV( );
  LEAVE_GL();
}

static void WINAPI wine_glPixelDataRangeNV( GLenum target, GLsizei length, GLvoid* pointer ) {
  void (*func_glPixelDataRangeNV)( GLenum, GLsizei, GLvoid* ) = extension_funcs[EXT_glPixelDataRangeNV];
  TRACE("(%d, %d, %p)\n", target, length, pointer );
  ENTER_GL();
  func_glPixelDataRangeNV( target, length, pointer );
  LEAVE_GL();
}

static void WINAPI wine_glPixelTexGenParameterfSGIS( GLenum pname, GLfloat param ) {
  void (*func_glPixelTexGenParameterfSGIS)( GLenum, GLfloat ) = extension_funcs[EXT_glPixelTexGenParameterfSGIS];
  TRACE("(%d, %f)\n", pname, param );
  ENTER_GL();
  func_glPixelTexGenParameterfSGIS( pname, param );
  LEAVE_GL();
}

static void WINAPI wine_glPixelTexGenParameterfvSGIS( GLenum pname, GLfloat* params ) {
  void (*func_glPixelTexGenParameterfvSGIS)( GLenum, GLfloat* ) = extension_funcs[EXT_glPixelTexGenParameterfvSGIS];
  TRACE("(%d, %p)\n", pname, params );
  ENTER_GL();
  func_glPixelTexGenParameterfvSGIS( pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glPixelTexGenParameteriSGIS( GLenum pname, GLint param ) {
  void (*func_glPixelTexGenParameteriSGIS)( GLenum, GLint ) = extension_funcs[EXT_glPixelTexGenParameteriSGIS];
  TRACE("(%d, %d)\n", pname, param );
  ENTER_GL();
  func_glPixelTexGenParameteriSGIS( pname, param );
  LEAVE_GL();
}

static void WINAPI wine_glPixelTexGenParameterivSGIS( GLenum pname, GLint* params ) {
  void (*func_glPixelTexGenParameterivSGIS)( GLenum, GLint* ) = extension_funcs[EXT_glPixelTexGenParameterivSGIS];
  TRACE("(%d, %p)\n", pname, params );
  ENTER_GL();
  func_glPixelTexGenParameterivSGIS( pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glPixelTexGenSGIX( GLenum mode ) {
  void (*func_glPixelTexGenSGIX)( GLenum ) = extension_funcs[EXT_glPixelTexGenSGIX];
  TRACE("(%d)\n", mode );
  ENTER_GL();
  func_glPixelTexGenSGIX( mode );
  LEAVE_GL();
}

static void WINAPI wine_glPixelTransformParameterfEXT( GLenum target, GLenum pname, GLfloat param ) {
  void (*func_glPixelTransformParameterfEXT)( GLenum, GLenum, GLfloat ) = extension_funcs[EXT_glPixelTransformParameterfEXT];
  TRACE("(%d, %d, %f)\n", target, pname, param );
  ENTER_GL();
  func_glPixelTransformParameterfEXT( target, pname, param );
  LEAVE_GL();
}

static void WINAPI wine_glPixelTransformParameterfvEXT( GLenum target, GLenum pname, GLfloat* params ) {
  void (*func_glPixelTransformParameterfvEXT)( GLenum, GLenum, GLfloat* ) = extension_funcs[EXT_glPixelTransformParameterfvEXT];
  TRACE("(%d, %d, %p)\n", target, pname, params );
  ENTER_GL();
  func_glPixelTransformParameterfvEXT( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glPixelTransformParameteriEXT( GLenum target, GLenum pname, GLint param ) {
  void (*func_glPixelTransformParameteriEXT)( GLenum, GLenum, GLint ) = extension_funcs[EXT_glPixelTransformParameteriEXT];
  TRACE("(%d, %d, %d)\n", target, pname, param );
  ENTER_GL();
  func_glPixelTransformParameteriEXT( target, pname, param );
  LEAVE_GL();
}

static void WINAPI wine_glPixelTransformParameterivEXT( GLenum target, GLenum pname, GLint* params ) {
  void (*func_glPixelTransformParameterivEXT)( GLenum, GLenum, GLint* ) = extension_funcs[EXT_glPixelTransformParameterivEXT];
  TRACE("(%d, %d, %p)\n", target, pname, params );
  ENTER_GL();
  func_glPixelTransformParameterivEXT( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glPointParameterf( GLenum pname, GLfloat param ) {
  void (*func_glPointParameterf)( GLenum, GLfloat ) = extension_funcs[EXT_glPointParameterf];
  TRACE("(%d, %f)\n", pname, param );
  ENTER_GL();
  func_glPointParameterf( pname, param );
  LEAVE_GL();
}

static void WINAPI wine_glPointParameterfARB( GLenum pname, GLfloat param ) {
  void (*func_glPointParameterfARB)( GLenum, GLfloat ) = extension_funcs[EXT_glPointParameterfARB];
  TRACE("(%d, %f)\n", pname, param );
  ENTER_GL();
  func_glPointParameterfARB( pname, param );
  LEAVE_GL();
}

static void WINAPI wine_glPointParameterfEXT( GLenum pname, GLfloat param ) {
  void (*func_glPointParameterfEXT)( GLenum, GLfloat ) = extension_funcs[EXT_glPointParameterfEXT];
  TRACE("(%d, %f)\n", pname, param );
  ENTER_GL();
  func_glPointParameterfEXT( pname, param );
  LEAVE_GL();
}

static void WINAPI wine_glPointParameterfSGIS( GLenum pname, GLfloat param ) {
  void (*func_glPointParameterfSGIS)( GLenum, GLfloat ) = extension_funcs[EXT_glPointParameterfSGIS];
  TRACE("(%d, %f)\n", pname, param );
  ENTER_GL();
  func_glPointParameterfSGIS( pname, param );
  LEAVE_GL();
}

static void WINAPI wine_glPointParameterfv( GLenum pname, GLfloat* params ) {
  void (*func_glPointParameterfv)( GLenum, GLfloat* ) = extension_funcs[EXT_glPointParameterfv];
  TRACE("(%d, %p)\n", pname, params );
  ENTER_GL();
  func_glPointParameterfv( pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glPointParameterfvARB( GLenum pname, GLfloat* params ) {
  void (*func_glPointParameterfvARB)( GLenum, GLfloat* ) = extension_funcs[EXT_glPointParameterfvARB];
  TRACE("(%d, %p)\n", pname, params );
  ENTER_GL();
  func_glPointParameterfvARB( pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glPointParameterfvEXT( GLenum pname, GLfloat* params ) {
  void (*func_glPointParameterfvEXT)( GLenum, GLfloat* ) = extension_funcs[EXT_glPointParameterfvEXT];
  TRACE("(%d, %p)\n", pname, params );
  ENTER_GL();
  func_glPointParameterfvEXT( pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glPointParameterfvSGIS( GLenum pname, GLfloat* params ) {
  void (*func_glPointParameterfvSGIS)( GLenum, GLfloat* ) = extension_funcs[EXT_glPointParameterfvSGIS];
  TRACE("(%d, %p)\n", pname, params );
  ENTER_GL();
  func_glPointParameterfvSGIS( pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glPointParameteri( GLenum pname, GLint param ) {
  void (*func_glPointParameteri)( GLenum, GLint ) = extension_funcs[EXT_glPointParameteri];
  TRACE("(%d, %d)\n", pname, param );
  ENTER_GL();
  func_glPointParameteri( pname, param );
  LEAVE_GL();
}

static void WINAPI wine_glPointParameteriNV( GLenum pname, GLint param ) {
  void (*func_glPointParameteriNV)( GLenum, GLint ) = extension_funcs[EXT_glPointParameteriNV];
  TRACE("(%d, %d)\n", pname, param );
  ENTER_GL();
  func_glPointParameteriNV( pname, param );
  LEAVE_GL();
}

static void WINAPI wine_glPointParameteriv( GLenum pname, GLint* params ) {
  void (*func_glPointParameteriv)( GLenum, GLint* ) = extension_funcs[EXT_glPointParameteriv];
  TRACE("(%d, %p)\n", pname, params );
  ENTER_GL();
  func_glPointParameteriv( pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glPointParameterivNV( GLenum pname, GLint* params ) {
  void (*func_glPointParameterivNV)( GLenum, GLint* ) = extension_funcs[EXT_glPointParameterivNV];
  TRACE("(%d, %p)\n", pname, params );
  ENTER_GL();
  func_glPointParameterivNV( pname, params );
  LEAVE_GL();
}

static GLint WINAPI wine_glPollAsyncSGIX( GLuint* markerp ) {
  GLint ret_value;
  GLint (*func_glPollAsyncSGIX)( GLuint* ) = extension_funcs[EXT_glPollAsyncSGIX];
  TRACE("(%p)\n", markerp );
  ENTER_GL();
  ret_value = func_glPollAsyncSGIX( markerp );
  LEAVE_GL();
  return ret_value;
}

static GLint WINAPI wine_glPollInstrumentsSGIX( GLint* marker_p ) {
  GLint ret_value;
  GLint (*func_glPollInstrumentsSGIX)( GLint* ) = extension_funcs[EXT_glPollInstrumentsSGIX];
  TRACE("(%p)\n", marker_p );
  ENTER_GL();
  ret_value = func_glPollInstrumentsSGIX( marker_p );
  LEAVE_GL();
  return ret_value;
}

static void WINAPI wine_glPolygonOffsetEXT( GLfloat factor, GLfloat bias ) {
  void (*func_glPolygonOffsetEXT)( GLfloat, GLfloat ) = extension_funcs[EXT_glPolygonOffsetEXT];
  TRACE("(%f, %f)\n", factor, bias );
  ENTER_GL();
  func_glPolygonOffsetEXT( factor, bias );
  LEAVE_GL();
}

static void WINAPI wine_glPresentFrameDualFillNV( GLuint video_slot, UINT64 minPresentTime, GLuint beginPresentTimeId, GLuint presentDurationId, GLenum type, GLenum target0, GLuint fill0, GLenum target1, GLuint fill1, GLenum target2, GLuint fill2, GLenum target3, GLuint fill3 ) {
  void (*func_glPresentFrameDualFillNV)( GLuint, UINT64, GLuint, GLuint, GLenum, GLenum, GLuint, GLenum, GLuint, GLenum, GLuint, GLenum, GLuint ) = extension_funcs[EXT_glPresentFrameDualFillNV];
  TRACE("(%d, %s, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d)\n", video_slot, wine_dbgstr_longlong(minPresentTime), beginPresentTimeId, presentDurationId, type, target0, fill0, target1, fill1, target2, fill2, target3, fill3 );
  ENTER_GL();
  func_glPresentFrameDualFillNV( video_slot, minPresentTime, beginPresentTimeId, presentDurationId, type, target0, fill0, target1, fill1, target2, fill2, target3, fill3 );
  LEAVE_GL();
}

static void WINAPI wine_glPresentFrameKeyedNV( GLuint video_slot, UINT64 minPresentTime, GLuint beginPresentTimeId, GLuint presentDurationId, GLenum type, GLenum target0, GLuint fill0, GLuint key0, GLenum target1, GLuint fill1, GLuint key1 ) {
  void (*func_glPresentFrameKeyedNV)( GLuint, UINT64, GLuint, GLuint, GLenum, GLenum, GLuint, GLuint, GLenum, GLuint, GLuint ) = extension_funcs[EXT_glPresentFrameKeyedNV];
  TRACE("(%d, %s, %d, %d, %d, %d, %d, %d, %d, %d, %d)\n", video_slot, wine_dbgstr_longlong(minPresentTime), beginPresentTimeId, presentDurationId, type, target0, fill0, key0, target1, fill1, key1 );
  ENTER_GL();
  func_glPresentFrameKeyedNV( video_slot, minPresentTime, beginPresentTimeId, presentDurationId, type, target0, fill0, key0, target1, fill1, key1 );
  LEAVE_GL();
}

static void WINAPI wine_glPrimitiveRestartIndex( GLuint index ) {
  void (*func_glPrimitiveRestartIndex)( GLuint ) = extension_funcs[EXT_glPrimitiveRestartIndex];
  TRACE("(%d)\n", index );
  ENTER_GL();
  func_glPrimitiveRestartIndex( index );
  LEAVE_GL();
}

static void WINAPI wine_glPrimitiveRestartIndexNV( GLuint index ) {
  void (*func_glPrimitiveRestartIndexNV)( GLuint ) = extension_funcs[EXT_glPrimitiveRestartIndexNV];
  TRACE("(%d)\n", index );
  ENTER_GL();
  func_glPrimitiveRestartIndexNV( index );
  LEAVE_GL();
}

static void WINAPI wine_glPrimitiveRestartNV( void ) {
  void (*func_glPrimitiveRestartNV)( void ) = extension_funcs[EXT_glPrimitiveRestartNV];
  TRACE("()\n");
  ENTER_GL();
  func_glPrimitiveRestartNV( );
  LEAVE_GL();
}

static void WINAPI wine_glPrioritizeTexturesEXT( GLsizei n, GLuint* textures, GLclampf* priorities ) {
  void (*func_glPrioritizeTexturesEXT)( GLsizei, GLuint*, GLclampf* ) = extension_funcs[EXT_glPrioritizeTexturesEXT];
  TRACE("(%d, %p, %p)\n", n, textures, priorities );
  ENTER_GL();
  func_glPrioritizeTexturesEXT( n, textures, priorities );
  LEAVE_GL();
}

static void WINAPI wine_glProgramBinary( GLuint program, GLenum binaryFormat, GLvoid* binary, GLsizei length ) {
  void (*func_glProgramBinary)( GLuint, GLenum, GLvoid*, GLsizei ) = extension_funcs[EXT_glProgramBinary];
  TRACE("(%d, %d, %p, %d)\n", program, binaryFormat, binary, length );
  ENTER_GL();
  func_glProgramBinary( program, binaryFormat, binary, length );
  LEAVE_GL();
}

static void WINAPI wine_glProgramBufferParametersIivNV( GLenum target, GLuint buffer, GLuint index, GLsizei count, GLint* params ) {
  void (*func_glProgramBufferParametersIivNV)( GLenum, GLuint, GLuint, GLsizei, GLint* ) = extension_funcs[EXT_glProgramBufferParametersIivNV];
  TRACE("(%d, %d, %d, %d, %p)\n", target, buffer, index, count, params );
  ENTER_GL();
  func_glProgramBufferParametersIivNV( target, buffer, index, count, params );
  LEAVE_GL();
}

static void WINAPI wine_glProgramBufferParametersIuivNV( GLenum target, GLuint buffer, GLuint index, GLsizei count, GLuint* params ) {
  void (*func_glProgramBufferParametersIuivNV)( GLenum, GLuint, GLuint, GLsizei, GLuint* ) = extension_funcs[EXT_glProgramBufferParametersIuivNV];
  TRACE("(%d, %d, %d, %d, %p)\n", target, buffer, index, count, params );
  ENTER_GL();
  func_glProgramBufferParametersIuivNV( target, buffer, index, count, params );
  LEAVE_GL();
}

static void WINAPI wine_glProgramBufferParametersfvNV( GLenum target, GLuint buffer, GLuint index, GLsizei count, GLfloat* params ) {
  void (*func_glProgramBufferParametersfvNV)( GLenum, GLuint, GLuint, GLsizei, GLfloat* ) = extension_funcs[EXT_glProgramBufferParametersfvNV];
  TRACE("(%d, %d, %d, %d, %p)\n", target, buffer, index, count, params );
  ENTER_GL();
  func_glProgramBufferParametersfvNV( target, buffer, index, count, params );
  LEAVE_GL();
}

static void WINAPI wine_glProgramEnvParameter4dARB( GLenum target, GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w ) {
  void (*func_glProgramEnvParameter4dARB)( GLenum, GLuint, GLdouble, GLdouble, GLdouble, GLdouble ) = extension_funcs[EXT_glProgramEnvParameter4dARB];
  TRACE("(%d, %d, %f, %f, %f, %f)\n", target, index, x, y, z, w );
  ENTER_GL();
  func_glProgramEnvParameter4dARB( target, index, x, y, z, w );
  LEAVE_GL();
}

static void WINAPI wine_glProgramEnvParameter4dvARB( GLenum target, GLuint index, GLdouble* params ) {
  void (*func_glProgramEnvParameter4dvARB)( GLenum, GLuint, GLdouble* ) = extension_funcs[EXT_glProgramEnvParameter4dvARB];
  TRACE("(%d, %d, %p)\n", target, index, params );
  ENTER_GL();
  func_glProgramEnvParameter4dvARB( target, index, params );
  LEAVE_GL();
}

static void WINAPI wine_glProgramEnvParameter4fARB( GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w ) {
  void (*func_glProgramEnvParameter4fARB)( GLenum, GLuint, GLfloat, GLfloat, GLfloat, GLfloat ) = extension_funcs[EXT_glProgramEnvParameter4fARB];
  TRACE("(%d, %d, %f, %f, %f, %f)\n", target, index, x, y, z, w );
  ENTER_GL();
  func_glProgramEnvParameter4fARB( target, index, x, y, z, w );
  LEAVE_GL();
}

static void WINAPI wine_glProgramEnvParameter4fvARB( GLenum target, GLuint index, GLfloat* params ) {
  void (*func_glProgramEnvParameter4fvARB)( GLenum, GLuint, GLfloat* ) = extension_funcs[EXT_glProgramEnvParameter4fvARB];
  TRACE("(%d, %d, %p)\n", target, index, params );
  ENTER_GL();
  func_glProgramEnvParameter4fvARB( target, index, params );
  LEAVE_GL();
}

static void WINAPI wine_glProgramEnvParameterI4iNV( GLenum target, GLuint index, GLint x, GLint y, GLint z, GLint w ) {
  void (*func_glProgramEnvParameterI4iNV)( GLenum, GLuint, GLint, GLint, GLint, GLint ) = extension_funcs[EXT_glProgramEnvParameterI4iNV];
  TRACE("(%d, %d, %d, %d, %d, %d)\n", target, index, x, y, z, w );
  ENTER_GL();
  func_glProgramEnvParameterI4iNV( target, index, x, y, z, w );
  LEAVE_GL();
}

static void WINAPI wine_glProgramEnvParameterI4ivNV( GLenum target, GLuint index, GLint* params ) {
  void (*func_glProgramEnvParameterI4ivNV)( GLenum, GLuint, GLint* ) = extension_funcs[EXT_glProgramEnvParameterI4ivNV];
  TRACE("(%d, %d, %p)\n", target, index, params );
  ENTER_GL();
  func_glProgramEnvParameterI4ivNV( target, index, params );
  LEAVE_GL();
}

static void WINAPI wine_glProgramEnvParameterI4uiNV( GLenum target, GLuint index, GLuint x, GLuint y, GLuint z, GLuint w ) {
  void (*func_glProgramEnvParameterI4uiNV)( GLenum, GLuint, GLuint, GLuint, GLuint, GLuint ) = extension_funcs[EXT_glProgramEnvParameterI4uiNV];
  TRACE("(%d, %d, %d, %d, %d, %d)\n", target, index, x, y, z, w );
  ENTER_GL();
  func_glProgramEnvParameterI4uiNV( target, index, x, y, z, w );
  LEAVE_GL();
}

static void WINAPI wine_glProgramEnvParameterI4uivNV( GLenum target, GLuint index, GLuint* params ) {
  void (*func_glProgramEnvParameterI4uivNV)( GLenum, GLuint, GLuint* ) = extension_funcs[EXT_glProgramEnvParameterI4uivNV];
  TRACE("(%d, %d, %p)\n", target, index, params );
  ENTER_GL();
  func_glProgramEnvParameterI4uivNV( target, index, params );
  LEAVE_GL();
}

static void WINAPI wine_glProgramEnvParameters4fvEXT( GLenum target, GLuint index, GLsizei count, GLfloat* params ) {
  void (*func_glProgramEnvParameters4fvEXT)( GLenum, GLuint, GLsizei, GLfloat* ) = extension_funcs[EXT_glProgramEnvParameters4fvEXT];
  TRACE("(%d, %d, %d, %p)\n", target, index, count, params );
  ENTER_GL();
  func_glProgramEnvParameters4fvEXT( target, index, count, params );
  LEAVE_GL();
}

static void WINAPI wine_glProgramEnvParametersI4ivNV( GLenum target, GLuint index, GLsizei count, GLint* params ) {
  void (*func_glProgramEnvParametersI4ivNV)( GLenum, GLuint, GLsizei, GLint* ) = extension_funcs[EXT_glProgramEnvParametersI4ivNV];
  TRACE("(%d, %d, %d, %p)\n", target, index, count, params );
  ENTER_GL();
  func_glProgramEnvParametersI4ivNV( target, index, count, params );
  LEAVE_GL();
}

static void WINAPI wine_glProgramEnvParametersI4uivNV( GLenum target, GLuint index, GLsizei count, GLuint* params ) {
  void (*func_glProgramEnvParametersI4uivNV)( GLenum, GLuint, GLsizei, GLuint* ) = extension_funcs[EXT_glProgramEnvParametersI4uivNV];
  TRACE("(%d, %d, %d, %p)\n", target, index, count, params );
  ENTER_GL();
  func_glProgramEnvParametersI4uivNV( target, index, count, params );
  LEAVE_GL();
}

static void WINAPI wine_glProgramLocalParameter4dARB( GLenum target, GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w ) {
  void (*func_glProgramLocalParameter4dARB)( GLenum, GLuint, GLdouble, GLdouble, GLdouble, GLdouble ) = extension_funcs[EXT_glProgramLocalParameter4dARB];
  TRACE("(%d, %d, %f, %f, %f, %f)\n", target, index, x, y, z, w );
  ENTER_GL();
  func_glProgramLocalParameter4dARB( target, index, x, y, z, w );
  LEAVE_GL();
}

static void WINAPI wine_glProgramLocalParameter4dvARB( GLenum target, GLuint index, GLdouble* params ) {
  void (*func_glProgramLocalParameter4dvARB)( GLenum, GLuint, GLdouble* ) = extension_funcs[EXT_glProgramLocalParameter4dvARB];
  TRACE("(%d, %d, %p)\n", target, index, params );
  ENTER_GL();
  func_glProgramLocalParameter4dvARB( target, index, params );
  LEAVE_GL();
}

static void WINAPI wine_glProgramLocalParameter4fARB( GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w ) {
  void (*func_glProgramLocalParameter4fARB)( GLenum, GLuint, GLfloat, GLfloat, GLfloat, GLfloat ) = extension_funcs[EXT_glProgramLocalParameter4fARB];
  TRACE("(%d, %d, %f, %f, %f, %f)\n", target, index, x, y, z, w );
  ENTER_GL();
  func_glProgramLocalParameter4fARB( target, index, x, y, z, w );
  LEAVE_GL();
}

static void WINAPI wine_glProgramLocalParameter4fvARB( GLenum target, GLuint index, GLfloat* params ) {
  void (*func_glProgramLocalParameter4fvARB)( GLenum, GLuint, GLfloat* ) = extension_funcs[EXT_glProgramLocalParameter4fvARB];
  TRACE("(%d, %d, %p)\n", target, index, params );
  ENTER_GL();
  func_glProgramLocalParameter4fvARB( target, index, params );
  LEAVE_GL();
}

static void WINAPI wine_glProgramLocalParameterI4iNV( GLenum target, GLuint index, GLint x, GLint y, GLint z, GLint w ) {
  void (*func_glProgramLocalParameterI4iNV)( GLenum, GLuint, GLint, GLint, GLint, GLint ) = extension_funcs[EXT_glProgramLocalParameterI4iNV];
  TRACE("(%d, %d, %d, %d, %d, %d)\n", target, index, x, y, z, w );
  ENTER_GL();
  func_glProgramLocalParameterI4iNV( target, index, x, y, z, w );
  LEAVE_GL();
}

static void WINAPI wine_glProgramLocalParameterI4ivNV( GLenum target, GLuint index, GLint* params ) {
  void (*func_glProgramLocalParameterI4ivNV)( GLenum, GLuint, GLint* ) = extension_funcs[EXT_glProgramLocalParameterI4ivNV];
  TRACE("(%d, %d, %p)\n", target, index, params );
  ENTER_GL();
  func_glProgramLocalParameterI4ivNV( target, index, params );
  LEAVE_GL();
}

static void WINAPI wine_glProgramLocalParameterI4uiNV( GLenum target, GLuint index, GLuint x, GLuint y, GLuint z, GLuint w ) {
  void (*func_glProgramLocalParameterI4uiNV)( GLenum, GLuint, GLuint, GLuint, GLuint, GLuint ) = extension_funcs[EXT_glProgramLocalParameterI4uiNV];
  TRACE("(%d, %d, %d, %d, %d, %d)\n", target, index, x, y, z, w );
  ENTER_GL();
  func_glProgramLocalParameterI4uiNV( target, index, x, y, z, w );
  LEAVE_GL();
}

static void WINAPI wine_glProgramLocalParameterI4uivNV( GLenum target, GLuint index, GLuint* params ) {
  void (*func_glProgramLocalParameterI4uivNV)( GLenum, GLuint, GLuint* ) = extension_funcs[EXT_glProgramLocalParameterI4uivNV];
  TRACE("(%d, %d, %p)\n", target, index, params );
  ENTER_GL();
  func_glProgramLocalParameterI4uivNV( target, index, params );
  LEAVE_GL();
}

static void WINAPI wine_glProgramLocalParameters4fvEXT( GLenum target, GLuint index, GLsizei count, GLfloat* params ) {
  void (*func_glProgramLocalParameters4fvEXT)( GLenum, GLuint, GLsizei, GLfloat* ) = extension_funcs[EXT_glProgramLocalParameters4fvEXT];
  TRACE("(%d, %d, %d, %p)\n", target, index, count, params );
  ENTER_GL();
  func_glProgramLocalParameters4fvEXT( target, index, count, params );
  LEAVE_GL();
}

static void WINAPI wine_glProgramLocalParametersI4ivNV( GLenum target, GLuint index, GLsizei count, GLint* params ) {
  void (*func_glProgramLocalParametersI4ivNV)( GLenum, GLuint, GLsizei, GLint* ) = extension_funcs[EXT_glProgramLocalParametersI4ivNV];
  TRACE("(%d, %d, %d, %p)\n", target, index, count, params );
  ENTER_GL();
  func_glProgramLocalParametersI4ivNV( target, index, count, params );
  LEAVE_GL();
}

static void WINAPI wine_glProgramLocalParametersI4uivNV( GLenum target, GLuint index, GLsizei count, GLuint* params ) {
  void (*func_glProgramLocalParametersI4uivNV)( GLenum, GLuint, GLsizei, GLuint* ) = extension_funcs[EXT_glProgramLocalParametersI4uivNV];
  TRACE("(%d, %d, %d, %p)\n", target, index, count, params );
  ENTER_GL();
  func_glProgramLocalParametersI4uivNV( target, index, count, params );
  LEAVE_GL();
}

static void WINAPI wine_glProgramNamedParameter4dNV( GLuint id, GLsizei len, GLubyte* name, GLdouble x, GLdouble y, GLdouble z, GLdouble w ) {
  void (*func_glProgramNamedParameter4dNV)( GLuint, GLsizei, GLubyte*, GLdouble, GLdouble, GLdouble, GLdouble ) = extension_funcs[EXT_glProgramNamedParameter4dNV];
  TRACE("(%d, %d, %p, %f, %f, %f, %f)\n", id, len, name, x, y, z, w );
  ENTER_GL();
  func_glProgramNamedParameter4dNV( id, len, name, x, y, z, w );
  LEAVE_GL();
}

static void WINAPI wine_glProgramNamedParameter4dvNV( GLuint id, GLsizei len, GLubyte* name, GLdouble* v ) {
  void (*func_glProgramNamedParameter4dvNV)( GLuint, GLsizei, GLubyte*, GLdouble* ) = extension_funcs[EXT_glProgramNamedParameter4dvNV];
  TRACE("(%d, %d, %p, %p)\n", id, len, name, v );
  ENTER_GL();
  func_glProgramNamedParameter4dvNV( id, len, name, v );
  LEAVE_GL();
}

static void WINAPI wine_glProgramNamedParameter4fNV( GLuint id, GLsizei len, GLubyte* name, GLfloat x, GLfloat y, GLfloat z, GLfloat w ) {
  void (*func_glProgramNamedParameter4fNV)( GLuint, GLsizei, GLubyte*, GLfloat, GLfloat, GLfloat, GLfloat ) = extension_funcs[EXT_glProgramNamedParameter4fNV];
  TRACE("(%d, %d, %p, %f, %f, %f, %f)\n", id, len, name, x, y, z, w );
  ENTER_GL();
  func_glProgramNamedParameter4fNV( id, len, name, x, y, z, w );
  LEAVE_GL();
}

static void WINAPI wine_glProgramNamedParameter4fvNV( GLuint id, GLsizei len, GLubyte* name, GLfloat* v ) {
  void (*func_glProgramNamedParameter4fvNV)( GLuint, GLsizei, GLubyte*, GLfloat* ) = extension_funcs[EXT_glProgramNamedParameter4fvNV];
  TRACE("(%d, %d, %p, %p)\n", id, len, name, v );
  ENTER_GL();
  func_glProgramNamedParameter4fvNV( id, len, name, v );
  LEAVE_GL();
}

static void WINAPI wine_glProgramParameter4dNV( GLenum target, GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w ) {
  void (*func_glProgramParameter4dNV)( GLenum, GLuint, GLdouble, GLdouble, GLdouble, GLdouble ) = extension_funcs[EXT_glProgramParameter4dNV];
  TRACE("(%d, %d, %f, %f, %f, %f)\n", target, index, x, y, z, w );
  ENTER_GL();
  func_glProgramParameter4dNV( target, index, x, y, z, w );
  LEAVE_GL();
}

static void WINAPI wine_glProgramParameter4dvNV( GLenum target, GLuint index, GLdouble* v ) {
  void (*func_glProgramParameter4dvNV)( GLenum, GLuint, GLdouble* ) = extension_funcs[EXT_glProgramParameter4dvNV];
  TRACE("(%d, %d, %p)\n", target, index, v );
  ENTER_GL();
  func_glProgramParameter4dvNV( target, index, v );
  LEAVE_GL();
}

static void WINAPI wine_glProgramParameter4fNV( GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w ) {
  void (*func_glProgramParameter4fNV)( GLenum, GLuint, GLfloat, GLfloat, GLfloat, GLfloat ) = extension_funcs[EXT_glProgramParameter4fNV];
  TRACE("(%d, %d, %f, %f, %f, %f)\n", target, index, x, y, z, w );
  ENTER_GL();
  func_glProgramParameter4fNV( target, index, x, y, z, w );
  LEAVE_GL();
}

static void WINAPI wine_glProgramParameter4fvNV( GLenum target, GLuint index, GLfloat* v ) {
  void (*func_glProgramParameter4fvNV)( GLenum, GLuint, GLfloat* ) = extension_funcs[EXT_glProgramParameter4fvNV];
  TRACE("(%d, %d, %p)\n", target, index, v );
  ENTER_GL();
  func_glProgramParameter4fvNV( target, index, v );
  LEAVE_GL();
}

static void WINAPI wine_glProgramParameteri( GLuint program, GLenum pname, GLint value ) {
  void (*func_glProgramParameteri)( GLuint, GLenum, GLint ) = extension_funcs[EXT_glProgramParameteri];
  TRACE("(%d, %d, %d)\n", program, pname, value );
  ENTER_GL();
  func_glProgramParameteri( program, pname, value );
  LEAVE_GL();
}

static void WINAPI wine_glProgramParameteriARB( GLuint program, GLenum pname, GLint value ) {
  void (*func_glProgramParameteriARB)( GLuint, GLenum, GLint ) = extension_funcs[EXT_glProgramParameteriARB];
  TRACE("(%d, %d, %d)\n", program, pname, value );
  ENTER_GL();
  func_glProgramParameteriARB( program, pname, value );
  LEAVE_GL();
}

static void WINAPI wine_glProgramParameteriEXT( GLuint program, GLenum pname, GLint value ) {
  void (*func_glProgramParameteriEXT)( GLuint, GLenum, GLint ) = extension_funcs[EXT_glProgramParameteriEXT];
  TRACE("(%d, %d, %d)\n", program, pname, value );
  ENTER_GL();
  func_glProgramParameteriEXT( program, pname, value );
  LEAVE_GL();
}

static void WINAPI wine_glProgramParameters4dvNV( GLenum target, GLuint index, GLuint count, GLdouble* v ) {
  void (*func_glProgramParameters4dvNV)( GLenum, GLuint, GLuint, GLdouble* ) = extension_funcs[EXT_glProgramParameters4dvNV];
  TRACE("(%d, %d, %d, %p)\n", target, index, count, v );
  ENTER_GL();
  func_glProgramParameters4dvNV( target, index, count, v );
  LEAVE_GL();
}

static void WINAPI wine_glProgramParameters4fvNV( GLenum target, GLuint index, GLuint count, GLfloat* v ) {
  void (*func_glProgramParameters4fvNV)( GLenum, GLuint, GLuint, GLfloat* ) = extension_funcs[EXT_glProgramParameters4fvNV];
  TRACE("(%d, %d, %d, %p)\n", target, index, count, v );
  ENTER_GL();
  func_glProgramParameters4fvNV( target, index, count, v );
  LEAVE_GL();
}

static void WINAPI wine_glProgramStringARB( GLenum target, GLenum format, GLsizei len, GLvoid* string ) {
  void (*func_glProgramStringARB)( GLenum, GLenum, GLsizei, GLvoid* ) = extension_funcs[EXT_glProgramStringARB];
  TRACE("(%d, %d, %d, %p)\n", target, format, len, string );
  ENTER_GL();
  func_glProgramStringARB( target, format, len, string );
  LEAVE_GL();
}

static void WINAPI wine_glProgramSubroutineParametersuivNV( GLenum target, GLsizei count, GLuint* params ) {
  void (*func_glProgramSubroutineParametersuivNV)( GLenum, GLsizei, GLuint* ) = extension_funcs[EXT_glProgramSubroutineParametersuivNV];
  TRACE("(%d, %d, %p)\n", target, count, params );
  ENTER_GL();
  func_glProgramSubroutineParametersuivNV( target, count, params );
  LEAVE_GL();
}

static void WINAPI wine_glProgramUniform1d( GLuint program, GLint location, GLdouble v0 ) {
  void (*func_glProgramUniform1d)( GLuint, GLint, GLdouble ) = extension_funcs[EXT_glProgramUniform1d];
  TRACE("(%d, %d, %f)\n", program, location, v0 );
  ENTER_GL();
  func_glProgramUniform1d( program, location, v0 );
  LEAVE_GL();
}

static void WINAPI wine_glProgramUniform1dEXT( GLuint program, GLint location, GLdouble x ) {
  void (*func_glProgramUniform1dEXT)( GLuint, GLint, GLdouble ) = extension_funcs[EXT_glProgramUniform1dEXT];
  TRACE("(%d, %d, %f)\n", program, location, x );
  ENTER_GL();
  func_glProgramUniform1dEXT( program, location, x );
  LEAVE_GL();
}

static void WINAPI wine_glProgramUniform1dv( GLuint program, GLint location, GLsizei count, GLdouble* value ) {
  void (*func_glProgramUniform1dv)( GLuint, GLint, GLsizei, GLdouble* ) = extension_funcs[EXT_glProgramUniform1dv];
  TRACE("(%d, %d, %d, %p)\n", program, location, count, value );
  ENTER_GL();
  func_glProgramUniform1dv( program, location, count, value );
  LEAVE_GL();
}

static void WINAPI wine_glProgramUniform1dvEXT( GLuint program, GLint location, GLsizei count, GLdouble* value ) {
  void (*func_glProgramUniform1dvEXT)( GLuint, GLint, GLsizei, GLdouble* ) = extension_funcs[EXT_glProgramUniform1dvEXT];
  TRACE("(%d, %d, %d, %p)\n", program, location, count, value );
  ENTER_GL();
  func_glProgramUniform1dvEXT( program, location, count, value );
  LEAVE_GL();
}

static void WINAPI wine_glProgramUniform1f( GLuint program, GLint location, GLfloat v0 ) {
  void (*func_glProgramUniform1f)( GLuint, GLint, GLfloat ) = extension_funcs[EXT_glProgramUniform1f];
  TRACE("(%d, %d, %f)\n", program, location, v0 );
  ENTER_GL();
  func_glProgramUniform1f( program, location, v0 );
  LEAVE_GL();
}

static void WINAPI wine_glProgramUniform1fEXT( GLuint program, GLint location, GLfloat v0 ) {
  void (*func_glProgramUniform1fEXT)( GLuint, GLint, GLfloat ) = extension_funcs[EXT_glProgramUniform1fEXT];
  TRACE("(%d, %d, %f)\n", program, location, v0 );
  ENTER_GL();
  func_glProgramUniform1fEXT( program, location, v0 );
  LEAVE_GL();
}

static void WINAPI wine_glProgramUniform1fv( GLuint program, GLint location, GLsizei count, GLfloat* value ) {
  void (*func_glProgramUniform1fv)( GLuint, GLint, GLsizei, GLfloat* ) = extension_funcs[EXT_glProgramUniform1fv];
  TRACE("(%d, %d, %d, %p)\n", program, location, count, value );
  ENTER_GL();
  func_glProgramUniform1fv( program, location, count, value );
  LEAVE_GL();
}

static void WINAPI wine_glProgramUniform1fvEXT( GLuint program, GLint location, GLsizei count, GLfloat* value ) {
  void (*func_glProgramUniform1fvEXT)( GLuint, GLint, GLsizei, GLfloat* ) = extension_funcs[EXT_glProgramUniform1fvEXT];
  TRACE("(%d, %d, %d, %p)\n", program, location, count, value );
  ENTER_GL();
  func_glProgramUniform1fvEXT( program, location, count, value );
  LEAVE_GL();
}

static void WINAPI wine_glProgramUniform1i( GLuint program, GLint location, GLint v0 ) {
  void (*func_glProgramUniform1i)( GLuint, GLint, GLint ) = extension_funcs[EXT_glProgramUniform1i];
  TRACE("(%d, %d, %d)\n", program, location, v0 );
  ENTER_GL();
  func_glProgramUniform1i( program, location, v0 );
  LEAVE_GL();
}

static void WINAPI wine_glProgramUniform1i64NV( GLuint program, GLint location, INT64 x ) {
  void (*func_glProgramUniform1i64NV)( GLuint, GLint, INT64 ) = extension_funcs[EXT_glProgramUniform1i64NV];
  TRACE("(%d, %d, %s)\n", program, location, wine_dbgstr_longlong(x) );
  ENTER_GL();
  func_glProgramUniform1i64NV( program, location, x );
  LEAVE_GL();
}

static void WINAPI wine_glProgramUniform1i64vNV( GLuint program, GLint location, GLsizei count, INT64* value ) {
  void (*func_glProgramUniform1i64vNV)( GLuint, GLint, GLsizei, INT64* ) = extension_funcs[EXT_glProgramUniform1i64vNV];
  TRACE("(%d, %d, %d, %p)\n", program, location, count, value );
  ENTER_GL();
  func_glProgramUniform1i64vNV( program, location, count, value );
  LEAVE_GL();
}

static void WINAPI wine_glProgramUniform1iEXT( GLuint program, GLint location, GLint v0 ) {
  void (*func_glProgramUniform1iEXT)( GLuint, GLint, GLint ) = extension_funcs[EXT_glProgramUniform1iEXT];
  TRACE("(%d, %d, %d)\n", program, location, v0 );
  ENTER_GL();
  func_glProgramUniform1iEXT( program, location, v0 );
  LEAVE_GL();
}

static void WINAPI wine_glProgramUniform1iv( GLuint program, GLint location, GLsizei count, GLint* value ) {
  void (*func_glProgramUniform1iv)( GLuint, GLint, GLsizei, GLint* ) = extension_funcs[EXT_glProgramUniform1iv];
  TRACE("(%d, %d, %d, %p)\n", program, location, count, value );
  ENTER_GL();
  func_glProgramUniform1iv( program, location, count, value );
  LEAVE_GL();
}

static void WINAPI wine_glProgramUniform1ivEXT( GLuint program, GLint location, GLsizei count, GLint* value ) {
  void (*func_glProgramUniform1ivEXT)( GLuint, GLint, GLsizei, GLint* ) = extension_funcs[EXT_glProgramUniform1ivEXT];
  TRACE("(%d, %d, %d, %p)\n", program, location, count, value );
  ENTER_GL();
  func_glProgramUniform1ivEXT( program, location, count, value );
  LEAVE_GL();
}

static void WINAPI wine_glProgramUniform1ui( GLuint program, GLint location, GLuint v0 ) {
  void (*func_glProgramUniform1ui)( GLuint, GLint, GLuint ) = extension_funcs[EXT_glProgramUniform1ui];
  TRACE("(%d, %d, %d)\n", program, location, v0 );
  ENTER_GL();
  func_glProgramUniform1ui( program, location, v0 );
  LEAVE_GL();
}

static void WINAPI wine_glProgramUniform1ui64NV( GLuint program, GLint location, UINT64 x ) {
  void (*func_glProgramUniform1ui64NV)( GLuint, GLint, UINT64 ) = extension_funcs[EXT_glProgramUniform1ui64NV];
  TRACE("(%d, %d, %s)\n", program, location, wine_dbgstr_longlong(x) );
  ENTER_GL();
  func_glProgramUniform1ui64NV( program, location, x );
  LEAVE_GL();
}

static void WINAPI wine_glProgramUniform1ui64vNV( GLuint program, GLint location, GLsizei count, UINT64* value ) {
  void (*func_glProgramUniform1ui64vNV)( GLuint, GLint, GLsizei, UINT64* ) = extension_funcs[EXT_glProgramUniform1ui64vNV];
  TRACE("(%d, %d, %d, %p)\n", program, location, count, value );
  ENTER_GL();
  func_glProgramUniform1ui64vNV( program, location, count, value );
  LEAVE_GL();
}

static void WINAPI wine_glProgramUniform1uiEXT( GLuint program, GLint location, GLuint v0 ) {
  void (*func_glProgramUniform1uiEXT)( GLuint, GLint, GLuint ) = extension_funcs[EXT_glProgramUniform1uiEXT];
  TRACE("(%d, %d, %d)\n", program, location, v0 );
  ENTER_GL();
  func_glProgramUniform1uiEXT( program, location, v0 );
  LEAVE_GL();
}

static void WINAPI wine_glProgramUniform1uiv( GLuint program, GLint location, GLsizei count, GLuint* value ) {
  void (*func_glProgramUniform1uiv)( GLuint, GLint, GLsizei, GLuint* ) = extension_funcs[EXT_glProgramUniform1uiv];
  TRACE("(%d, %d, %d, %p)\n", program, location, count, value );
  ENTER_GL();
  func_glProgramUniform1uiv( program, location, count, value );
  LEAVE_GL();
}

static void WINAPI wine_glProgramUniform1uivEXT( GLuint program, GLint location, GLsizei count, GLuint* value ) {
  void (*func_glProgramUniform1uivEXT)( GLuint, GLint, GLsizei, GLuint* ) = extension_funcs[EXT_glProgramUniform1uivEXT];
  TRACE("(%d, %d, %d, %p)\n", program, location, count, value );
  ENTER_GL();
  func_glProgramUniform1uivEXT( program, location, count, value );
  LEAVE_GL();
}

static void WINAPI wine_glProgramUniform2d( GLuint program, GLint location, GLdouble v0, GLdouble v1 ) {
  void (*func_glProgramUniform2d)( GLuint, GLint, GLdouble, GLdouble ) = extension_funcs[EXT_glProgramUniform2d];
  TRACE("(%d, %d, %f, %f)\n", program, location, v0, v1 );
  ENTER_GL();
  func_glProgramUniform2d( program, location, v0, v1 );
  LEAVE_GL();
}

static void WINAPI wine_glProgramUniform2dEXT( GLuint program, GLint location, GLdouble x, GLdouble y ) {
  void (*func_glProgramUniform2dEXT)( GLuint, GLint, GLdouble, GLdouble ) = extension_funcs[EXT_glProgramUniform2dEXT];
  TRACE("(%d, %d, %f, %f)\n", program, location, x, y );
  ENTER_GL();
  func_glProgramUniform2dEXT( program, location, x, y );
  LEAVE_GL();
}

static void WINAPI wine_glProgramUniform2dv( GLuint program, GLint location, GLsizei count, GLdouble* value ) {
  void (*func_glProgramUniform2dv)( GLuint, GLint, GLsizei, GLdouble* ) = extension_funcs[EXT_glProgramUniform2dv];
  TRACE("(%d, %d, %d, %p)\n", program, location, count, value );
  ENTER_GL();
  func_glProgramUniform2dv( program, location, count, value );
  LEAVE_GL();
}

static void WINAPI wine_glProgramUniform2dvEXT( GLuint program, GLint location, GLsizei count, GLdouble* value ) {
  void (*func_glProgramUniform2dvEXT)( GLuint, GLint, GLsizei, GLdouble* ) = extension_funcs[EXT_glProgramUniform2dvEXT];
  TRACE("(%d, %d, %d, %p)\n", program, location, count, value );
  ENTER_GL();
  func_glProgramUniform2dvEXT( program, location, count, value );
  LEAVE_GL();
}

static void WINAPI wine_glProgramUniform2f( GLuint program, GLint location, GLfloat v0, GLfloat v1 ) {
  void (*func_glProgramUniform2f)( GLuint, GLint, GLfloat, GLfloat ) = extension_funcs[EXT_glProgramUniform2f];
  TRACE("(%d, %d, %f, %f)\n", program, location, v0, v1 );
  ENTER_GL();
  func_glProgramUniform2f( program, location, v0, v1 );
  LEAVE_GL();
}

static void WINAPI wine_glProgramUniform2fEXT( GLuint program, GLint location, GLfloat v0, GLfloat v1 ) {
  void (*func_glProgramUniform2fEXT)( GLuint, GLint, GLfloat, GLfloat ) = extension_funcs[EXT_glProgramUniform2fEXT];
  TRACE("(%d, %d, %f, %f)\n", program, location, v0, v1 );
  ENTER_GL();
  func_glProgramUniform2fEXT( program, location, v0, v1 );
  LEAVE_GL();
}

static void WINAPI wine_glProgramUniform2fv( GLuint program, GLint location, GLsizei count, GLfloat* value ) {
  void (*func_glProgramUniform2fv)( GLuint, GLint, GLsizei, GLfloat* ) = extension_funcs[EXT_glProgramUniform2fv];
  TRACE("(%d, %d, %d, %p)\n", program, location, count, value );
  ENTER_GL();
  func_glProgramUniform2fv( program, location, count, value );
  LEAVE_GL();
}

static void WINAPI wine_glProgramUniform2fvEXT( GLuint program, GLint location, GLsizei count, GLfloat* value ) {
  void (*func_glProgramUniform2fvEXT)( GLuint, GLint, GLsizei, GLfloat* ) = extension_funcs[EXT_glProgramUniform2fvEXT];
  TRACE("(%d, %d, %d, %p)\n", program, location, count, value );
  ENTER_GL();
  func_glProgramUniform2fvEXT( program, location, count, value );
  LEAVE_GL();
}

static void WINAPI wine_glProgramUniform2i( GLuint program, GLint location, GLint v0, GLint v1 ) {
  void (*func_glProgramUniform2i)( GLuint, GLint, GLint, GLint ) = extension_funcs[EXT_glProgramUniform2i];
  TRACE("(%d, %d, %d, %d)\n", program, location, v0, v1 );
  ENTER_GL();
  func_glProgramUniform2i( program, location, v0, v1 );
  LEAVE_GL();
}

static void WINAPI wine_glProgramUniform2i64NV( GLuint program, GLint location, INT64 x, INT64 y ) {
  void (*func_glProgramUniform2i64NV)( GLuint, GLint, INT64, INT64 ) = extension_funcs[EXT_glProgramUniform2i64NV];
  TRACE("(%d, %d, %s, %s)\n", program, location, wine_dbgstr_longlong(x), wine_dbgstr_longlong(y) );
  ENTER_GL();
  func_glProgramUniform2i64NV( program, location, x, y );
  LEAVE_GL();
}

static void WINAPI wine_glProgramUniform2i64vNV( GLuint program, GLint location, GLsizei count, INT64* value ) {
  void (*func_glProgramUniform2i64vNV)( GLuint, GLint, GLsizei, INT64* ) = extension_funcs[EXT_glProgramUniform2i64vNV];
  TRACE("(%d, %d, %d, %p)\n", program, location, count, value );
  ENTER_GL();
  func_glProgramUniform2i64vNV( program, location, count, value );
  LEAVE_GL();
}

static void WINAPI wine_glProgramUniform2iEXT( GLuint program, GLint location, GLint v0, GLint v1 ) {
  void (*func_glProgramUniform2iEXT)( GLuint, GLint, GLint, GLint ) = extension_funcs[EXT_glProgramUniform2iEXT];
  TRACE("(%d, %d, %d, %d)\n", program, location, v0, v1 );
  ENTER_GL();
  func_glProgramUniform2iEXT( program, location, v0, v1 );
  LEAVE_GL();
}

static void WINAPI wine_glProgramUniform2iv( GLuint program, GLint location, GLsizei count, GLint* value ) {
  void (*func_glProgramUniform2iv)( GLuint, GLint, GLsizei, GLint* ) = extension_funcs[EXT_glProgramUniform2iv];
  TRACE("(%d, %d, %d, %p)\n", program, location, count, value );
  ENTER_GL();
  func_glProgramUniform2iv( program, location, count, value );
  LEAVE_GL();
}

static void WINAPI wine_glProgramUniform2ivEXT( GLuint program, GLint location, GLsizei count, GLint* value ) {
  void (*func_glProgramUniform2ivEXT)( GLuint, GLint, GLsizei, GLint* ) = extension_funcs[EXT_glProgramUniform2ivEXT];
  TRACE("(%d, %d, %d, %p)\n", program, location, count, value );
  ENTER_GL();
  func_glProgramUniform2ivEXT( program, location, count, value );
  LEAVE_GL();
}

static void WINAPI wine_glProgramUniform2ui( GLuint program, GLint location, GLuint v0, GLuint v1 ) {
  void (*func_glProgramUniform2ui)( GLuint, GLint, GLuint, GLuint ) = extension_funcs[EXT_glProgramUniform2ui];
  TRACE("(%d, %d, %d, %d)\n", program, location, v0, v1 );
  ENTER_GL();
  func_glProgramUniform2ui( program, location, v0, v1 );
  LEAVE_GL();
}

static void WINAPI wine_glProgramUniform2ui64NV( GLuint program, GLint location, UINT64 x, UINT64 y ) {
  void (*func_glProgramUniform2ui64NV)( GLuint, GLint, UINT64, UINT64 ) = extension_funcs[EXT_glProgramUniform2ui64NV];
  TRACE("(%d, %d, %s, %s)\n", program, location, wine_dbgstr_longlong(x), wine_dbgstr_longlong(y) );
  ENTER_GL();
  func_glProgramUniform2ui64NV( program, location, x, y );
  LEAVE_GL();
}

static void WINAPI wine_glProgramUniform2ui64vNV( GLuint program, GLint location, GLsizei count, UINT64* value ) {
  void (*func_glProgramUniform2ui64vNV)( GLuint, GLint, GLsizei, UINT64* ) = extension_funcs[EXT_glProgramUniform2ui64vNV];
  TRACE("(%d, %d, %d, %p)\n", program, location, count, value );
  ENTER_GL();
  func_glProgramUniform2ui64vNV( program, location, count, value );
  LEAVE_GL();
}

static void WINAPI wine_glProgramUniform2uiEXT( GLuint program, GLint location, GLuint v0, GLuint v1 ) {
  void (*func_glProgramUniform2uiEXT)( GLuint, GLint, GLuint, GLuint ) = extension_funcs[EXT_glProgramUniform2uiEXT];
  TRACE("(%d, %d, %d, %d)\n", program, location, v0, v1 );
  ENTER_GL();
  func_glProgramUniform2uiEXT( program, location, v0, v1 );
  LEAVE_GL();
}

static void WINAPI wine_glProgramUniform2uiv( GLuint program, GLint location, GLsizei count, GLuint* value ) {
  void (*func_glProgramUniform2uiv)( GLuint, GLint, GLsizei, GLuint* ) = extension_funcs[EXT_glProgramUniform2uiv];
  TRACE("(%d, %d, %d, %p)\n", program, location, count, value );
  ENTER_GL();
  func_glProgramUniform2uiv( program, location, count, value );
  LEAVE_GL();
}

static void WINAPI wine_glProgramUniform2uivEXT( GLuint program, GLint location, GLsizei count, GLuint* value ) {
  void (*func_glProgramUniform2uivEXT)( GLuint, GLint, GLsizei, GLuint* ) = extension_funcs[EXT_glProgramUniform2uivEXT];
  TRACE("(%d, %d, %d, %p)\n", program, location, count, value );
  ENTER_GL();
  func_glProgramUniform2uivEXT( program, location, count, value );
  LEAVE_GL();
}

static void WINAPI wine_glProgramUniform3d( GLuint program, GLint location, GLdouble v0, GLdouble v1, GLdouble v2 ) {
  void (*func_glProgramUniform3d)( GLuint, GLint, GLdouble, GLdouble, GLdouble ) = extension_funcs[EXT_glProgramUniform3d];
  TRACE("(%d, %d, %f, %f, %f)\n", program, location, v0, v1, v2 );
  ENTER_GL();
  func_glProgramUniform3d( program, location, v0, v1, v2 );
  LEAVE_GL();
}

static void WINAPI wine_glProgramUniform3dEXT( GLuint program, GLint location, GLdouble x, GLdouble y, GLdouble z ) {
  void (*func_glProgramUniform3dEXT)( GLuint, GLint, GLdouble, GLdouble, GLdouble ) = extension_funcs[EXT_glProgramUniform3dEXT];
  TRACE("(%d, %d, %f, %f, %f)\n", program, location, x, y, z );
  ENTER_GL();
  func_glProgramUniform3dEXT( program, location, x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glProgramUniform3dv( GLuint program, GLint location, GLsizei count, GLdouble* value ) {
  void (*func_glProgramUniform3dv)( GLuint, GLint, GLsizei, GLdouble* ) = extension_funcs[EXT_glProgramUniform3dv];
  TRACE("(%d, %d, %d, %p)\n", program, location, count, value );
  ENTER_GL();
  func_glProgramUniform3dv( program, location, count, value );
  LEAVE_GL();
}

static void WINAPI wine_glProgramUniform3dvEXT( GLuint program, GLint location, GLsizei count, GLdouble* value ) {
  void (*func_glProgramUniform3dvEXT)( GLuint, GLint, GLsizei, GLdouble* ) = extension_funcs[EXT_glProgramUniform3dvEXT];
  TRACE("(%d, %d, %d, %p)\n", program, location, count, value );
  ENTER_GL();
  func_glProgramUniform3dvEXT( program, location, count, value );
  LEAVE_GL();
}

static void WINAPI wine_glProgramUniform3f( GLuint program, GLint location, GLfloat v0, GLfloat v1, GLfloat v2 ) {
  void (*func_glProgramUniform3f)( GLuint, GLint, GLfloat, GLfloat, GLfloat ) = extension_funcs[EXT_glProgramUniform3f];
  TRACE("(%d, %d, %f, %f, %f)\n", program, location, v0, v1, v2 );
  ENTER_GL();
  func_glProgramUniform3f( program, location, v0, v1, v2 );
  LEAVE_GL();
}

static void WINAPI wine_glProgramUniform3fEXT( GLuint program, GLint location, GLfloat v0, GLfloat v1, GLfloat v2 ) {
  void (*func_glProgramUniform3fEXT)( GLuint, GLint, GLfloat, GLfloat, GLfloat ) = extension_funcs[EXT_glProgramUniform3fEXT];
  TRACE("(%d, %d, %f, %f, %f)\n", program, location, v0, v1, v2 );
  ENTER_GL();
  func_glProgramUniform3fEXT( program, location, v0, v1, v2 );
  LEAVE_GL();
}

static void WINAPI wine_glProgramUniform3fv( GLuint program, GLint location, GLsizei count, GLfloat* value ) {
  void (*func_glProgramUniform3fv)( GLuint, GLint, GLsizei, GLfloat* ) = extension_funcs[EXT_glProgramUniform3fv];
  TRACE("(%d, %d, %d, %p)\n", program, location, count, value );
  ENTER_GL();
  func_glProgramUniform3fv( program, location, count, value );
  LEAVE_GL();
}

static void WINAPI wine_glProgramUniform3fvEXT( GLuint program, GLint location, GLsizei count, GLfloat* value ) {
  void (*func_glProgramUniform3fvEXT)( GLuint, GLint, GLsizei, GLfloat* ) = extension_funcs[EXT_glProgramUniform3fvEXT];
  TRACE("(%d, %d, %d, %p)\n", program, location, count, value );
  ENTER_GL();
  func_glProgramUniform3fvEXT( program, location, count, value );
  LEAVE_GL();
}

static void WINAPI wine_glProgramUniform3i( GLuint program, GLint location, GLint v0, GLint v1, GLint v2 ) {
  void (*func_glProgramUniform3i)( GLuint, GLint, GLint, GLint, GLint ) = extension_funcs[EXT_glProgramUniform3i];
  TRACE("(%d, %d, %d, %d, %d)\n", program, location, v0, v1, v2 );
  ENTER_GL();
  func_glProgramUniform3i( program, location, v0, v1, v2 );
  LEAVE_GL();
}

static void WINAPI wine_glProgramUniform3i64NV( GLuint program, GLint location, INT64 x, INT64 y, INT64 z ) {
  void (*func_glProgramUniform3i64NV)( GLuint, GLint, INT64, INT64, INT64 ) = extension_funcs[EXT_glProgramUniform3i64NV];
  TRACE("(%d, %d, %s, %s, %s)\n", program, location, wine_dbgstr_longlong(x), wine_dbgstr_longlong(y), wine_dbgstr_longlong(z) );
  ENTER_GL();
  func_glProgramUniform3i64NV( program, location, x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glProgramUniform3i64vNV( GLuint program, GLint location, GLsizei count, INT64* value ) {
  void (*func_glProgramUniform3i64vNV)( GLuint, GLint, GLsizei, INT64* ) = extension_funcs[EXT_glProgramUniform3i64vNV];
  TRACE("(%d, %d, %d, %p)\n", program, location, count, value );
  ENTER_GL();
  func_glProgramUniform3i64vNV( program, location, count, value );
  LEAVE_GL();
}

static void WINAPI wine_glProgramUniform3iEXT( GLuint program, GLint location, GLint v0, GLint v1, GLint v2 ) {
  void (*func_glProgramUniform3iEXT)( GLuint, GLint, GLint, GLint, GLint ) = extension_funcs[EXT_glProgramUniform3iEXT];
  TRACE("(%d, %d, %d, %d, %d)\n", program, location, v0, v1, v2 );
  ENTER_GL();
  func_glProgramUniform3iEXT( program, location, v0, v1, v2 );
  LEAVE_GL();
}

static void WINAPI wine_glProgramUniform3iv( GLuint program, GLint location, GLsizei count, GLint* value ) {
  void (*func_glProgramUniform3iv)( GLuint, GLint, GLsizei, GLint* ) = extension_funcs[EXT_glProgramUniform3iv];
  TRACE("(%d, %d, %d, %p)\n", program, location, count, value );
  ENTER_GL();
  func_glProgramUniform3iv( program, location, count, value );
  LEAVE_GL();
}

static void WINAPI wine_glProgramUniform3ivEXT( GLuint program, GLint location, GLsizei count, GLint* value ) {
  void (*func_glProgramUniform3ivEXT)( GLuint, GLint, GLsizei, GLint* ) = extension_funcs[EXT_glProgramUniform3ivEXT];
  TRACE("(%d, %d, %d, %p)\n", program, location, count, value );
  ENTER_GL();
  func_glProgramUniform3ivEXT( program, location, count, value );
  LEAVE_GL();
}

static void WINAPI wine_glProgramUniform3ui( GLuint program, GLint location, GLuint v0, GLuint v1, GLuint v2 ) {
  void (*func_glProgramUniform3ui)( GLuint, GLint, GLuint, GLuint, GLuint ) = extension_funcs[EXT_glProgramUniform3ui];
  TRACE("(%d, %d, %d, %d, %d)\n", program, location, v0, v1, v2 );
  ENTER_GL();
  func_glProgramUniform3ui( program, location, v0, v1, v2 );
  LEAVE_GL();
}

static void WINAPI wine_glProgramUniform3ui64NV( GLuint program, GLint location, UINT64 x, UINT64 y, UINT64 z ) {
  void (*func_glProgramUniform3ui64NV)( GLuint, GLint, UINT64, UINT64, UINT64 ) = extension_funcs[EXT_glProgramUniform3ui64NV];
  TRACE("(%d, %d, %s, %s, %s)\n", program, location, wine_dbgstr_longlong(x), wine_dbgstr_longlong(y), wine_dbgstr_longlong(z) );
  ENTER_GL();
  func_glProgramUniform3ui64NV( program, location, x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glProgramUniform3ui64vNV( GLuint program, GLint location, GLsizei count, UINT64* value ) {
  void (*func_glProgramUniform3ui64vNV)( GLuint, GLint, GLsizei, UINT64* ) = extension_funcs[EXT_glProgramUniform3ui64vNV];
  TRACE("(%d, %d, %d, %p)\n", program, location, count, value );
  ENTER_GL();
  func_glProgramUniform3ui64vNV( program, location, count, value );
  LEAVE_GL();
}

static void WINAPI wine_glProgramUniform3uiEXT( GLuint program, GLint location, GLuint v0, GLuint v1, GLuint v2 ) {
  void (*func_glProgramUniform3uiEXT)( GLuint, GLint, GLuint, GLuint, GLuint ) = extension_funcs[EXT_glProgramUniform3uiEXT];
  TRACE("(%d, %d, %d, %d, %d)\n", program, location, v0, v1, v2 );
  ENTER_GL();
  func_glProgramUniform3uiEXT( program, location, v0, v1, v2 );
  LEAVE_GL();
}

static void WINAPI wine_glProgramUniform3uiv( GLuint program, GLint location, GLsizei count, GLuint* value ) {
  void (*func_glProgramUniform3uiv)( GLuint, GLint, GLsizei, GLuint* ) = extension_funcs[EXT_glProgramUniform3uiv];
  TRACE("(%d, %d, %d, %p)\n", program, location, count, value );
  ENTER_GL();
  func_glProgramUniform3uiv( program, location, count, value );
  LEAVE_GL();
}

static void WINAPI wine_glProgramUniform3uivEXT( GLuint program, GLint location, GLsizei count, GLuint* value ) {
  void (*func_glProgramUniform3uivEXT)( GLuint, GLint, GLsizei, GLuint* ) = extension_funcs[EXT_glProgramUniform3uivEXT];
  TRACE("(%d, %d, %d, %p)\n", program, location, count, value );
  ENTER_GL();
  func_glProgramUniform3uivEXT( program, location, count, value );
  LEAVE_GL();
}

static void WINAPI wine_glProgramUniform4d( GLuint program, GLint location, GLdouble v0, GLdouble v1, GLdouble v2, GLdouble v3 ) {
  void (*func_glProgramUniform4d)( GLuint, GLint, GLdouble, GLdouble, GLdouble, GLdouble ) = extension_funcs[EXT_glProgramUniform4d];
  TRACE("(%d, %d, %f, %f, %f, %f)\n", program, location, v0, v1, v2, v3 );
  ENTER_GL();
  func_glProgramUniform4d( program, location, v0, v1, v2, v3 );
  LEAVE_GL();
}

static void WINAPI wine_glProgramUniform4dEXT( GLuint program, GLint location, GLdouble x, GLdouble y, GLdouble z, GLdouble w ) {
  void (*func_glProgramUniform4dEXT)( GLuint, GLint, GLdouble, GLdouble, GLdouble, GLdouble ) = extension_funcs[EXT_glProgramUniform4dEXT];
  TRACE("(%d, %d, %f, %f, %f, %f)\n", program, location, x, y, z, w );
  ENTER_GL();
  func_glProgramUniform4dEXT( program, location, x, y, z, w );
  LEAVE_GL();
}

static void WINAPI wine_glProgramUniform4dv( GLuint program, GLint location, GLsizei count, GLdouble* value ) {
  void (*func_glProgramUniform4dv)( GLuint, GLint, GLsizei, GLdouble* ) = extension_funcs[EXT_glProgramUniform4dv];
  TRACE("(%d, %d, %d, %p)\n", program, location, count, value );
  ENTER_GL();
  func_glProgramUniform4dv( program, location, count, value );
  LEAVE_GL();
}

static void WINAPI wine_glProgramUniform4dvEXT( GLuint program, GLint location, GLsizei count, GLdouble* value ) {
  void (*func_glProgramUniform4dvEXT)( GLuint, GLint, GLsizei, GLdouble* ) = extension_funcs[EXT_glProgramUniform4dvEXT];
  TRACE("(%d, %d, %d, %p)\n", program, location, count, value );
  ENTER_GL();
  func_glProgramUniform4dvEXT( program, location, count, value );
  LEAVE_GL();
}

static void WINAPI wine_glProgramUniform4f( GLuint program, GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3 ) {
  void (*func_glProgramUniform4f)( GLuint, GLint, GLfloat, GLfloat, GLfloat, GLfloat ) = extension_funcs[EXT_glProgramUniform4f];
  TRACE("(%d, %d, %f, %f, %f, %f)\n", program, location, v0, v1, v2, v3 );
  ENTER_GL();
  func_glProgramUniform4f( program, location, v0, v1, v2, v3 );
  LEAVE_GL();
}

static void WINAPI wine_glProgramUniform4fEXT( GLuint program, GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3 ) {
  void (*func_glProgramUniform4fEXT)( GLuint, GLint, GLfloat, GLfloat, GLfloat, GLfloat ) = extension_funcs[EXT_glProgramUniform4fEXT];
  TRACE("(%d, %d, %f, %f, %f, %f)\n", program, location, v0, v1, v2, v3 );
  ENTER_GL();
  func_glProgramUniform4fEXT( program, location, v0, v1, v2, v3 );
  LEAVE_GL();
}

static void WINAPI wine_glProgramUniform4fv( GLuint program, GLint location, GLsizei count, GLfloat* value ) {
  void (*func_glProgramUniform4fv)( GLuint, GLint, GLsizei, GLfloat* ) = extension_funcs[EXT_glProgramUniform4fv];
  TRACE("(%d, %d, %d, %p)\n", program, location, count, value );
  ENTER_GL();
  func_glProgramUniform4fv( program, location, count, value );
  LEAVE_GL();
}

static void WINAPI wine_glProgramUniform4fvEXT( GLuint program, GLint location, GLsizei count, GLfloat* value ) {
  void (*func_glProgramUniform4fvEXT)( GLuint, GLint, GLsizei, GLfloat* ) = extension_funcs[EXT_glProgramUniform4fvEXT];
  TRACE("(%d, %d, %d, %p)\n", program, location, count, value );
  ENTER_GL();
  func_glProgramUniform4fvEXT( program, location, count, value );
  LEAVE_GL();
}

static void WINAPI wine_glProgramUniform4i( GLuint program, GLint location, GLint v0, GLint v1, GLint v2, GLint v3 ) {
  void (*func_glProgramUniform4i)( GLuint, GLint, GLint, GLint, GLint, GLint ) = extension_funcs[EXT_glProgramUniform4i];
  TRACE("(%d, %d, %d, %d, %d, %d)\n", program, location, v0, v1, v2, v3 );
  ENTER_GL();
  func_glProgramUniform4i( program, location, v0, v1, v2, v3 );
  LEAVE_GL();
}

static void WINAPI wine_glProgramUniform4i64NV( GLuint program, GLint location, INT64 x, INT64 y, INT64 z, INT64 w ) {
  void (*func_glProgramUniform4i64NV)( GLuint, GLint, INT64, INT64, INT64, INT64 ) = extension_funcs[EXT_glProgramUniform4i64NV];
  TRACE("(%d, %d, %s, %s, %s, %s)\n", program, location, wine_dbgstr_longlong(x), wine_dbgstr_longlong(y), wine_dbgstr_longlong(z), wine_dbgstr_longlong(w) );
  ENTER_GL();
  func_glProgramUniform4i64NV( program, location, x, y, z, w );
  LEAVE_GL();
}

static void WINAPI wine_glProgramUniform4i64vNV( GLuint program, GLint location, GLsizei count, INT64* value ) {
  void (*func_glProgramUniform4i64vNV)( GLuint, GLint, GLsizei, INT64* ) = extension_funcs[EXT_glProgramUniform4i64vNV];
  TRACE("(%d, %d, %d, %p)\n", program, location, count, value );
  ENTER_GL();
  func_glProgramUniform4i64vNV( program, location, count, value );
  LEAVE_GL();
}

static void WINAPI wine_glProgramUniform4iEXT( GLuint program, GLint location, GLint v0, GLint v1, GLint v2, GLint v3 ) {
  void (*func_glProgramUniform4iEXT)( GLuint, GLint, GLint, GLint, GLint, GLint ) = extension_funcs[EXT_glProgramUniform4iEXT];
  TRACE("(%d, %d, %d, %d, %d, %d)\n", program, location, v0, v1, v2, v3 );
  ENTER_GL();
  func_glProgramUniform4iEXT( program, location, v0, v1, v2, v3 );
  LEAVE_GL();
}

static void WINAPI wine_glProgramUniform4iv( GLuint program, GLint location, GLsizei count, GLint* value ) {
  void (*func_glProgramUniform4iv)( GLuint, GLint, GLsizei, GLint* ) = extension_funcs[EXT_glProgramUniform4iv];
  TRACE("(%d, %d, %d, %p)\n", program, location, count, value );
  ENTER_GL();
  func_glProgramUniform4iv( program, location, count, value );
  LEAVE_GL();
}

static void WINAPI wine_glProgramUniform4ivEXT( GLuint program, GLint location, GLsizei count, GLint* value ) {
  void (*func_glProgramUniform4ivEXT)( GLuint, GLint, GLsizei, GLint* ) = extension_funcs[EXT_glProgramUniform4ivEXT];
  TRACE("(%d, %d, %d, %p)\n", program, location, count, value );
  ENTER_GL();
  func_glProgramUniform4ivEXT( program, location, count, value );
  LEAVE_GL();
}

static void WINAPI wine_glProgramUniform4ui( GLuint program, GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3 ) {
  void (*func_glProgramUniform4ui)( GLuint, GLint, GLuint, GLuint, GLuint, GLuint ) = extension_funcs[EXT_glProgramUniform4ui];
  TRACE("(%d, %d, %d, %d, %d, %d)\n", program, location, v0, v1, v2, v3 );
  ENTER_GL();
  func_glProgramUniform4ui( program, location, v0, v1, v2, v3 );
  LEAVE_GL();
}

static void WINAPI wine_glProgramUniform4ui64NV( GLuint program, GLint location, UINT64 x, UINT64 y, UINT64 z, UINT64 w ) {
  void (*func_glProgramUniform4ui64NV)( GLuint, GLint, UINT64, UINT64, UINT64, UINT64 ) = extension_funcs[EXT_glProgramUniform4ui64NV];
  TRACE("(%d, %d, %s, %s, %s, %s)\n", program, location, wine_dbgstr_longlong(x), wine_dbgstr_longlong(y), wine_dbgstr_longlong(z), wine_dbgstr_longlong(w) );
  ENTER_GL();
  func_glProgramUniform4ui64NV( program, location, x, y, z, w );
  LEAVE_GL();
}

static void WINAPI wine_glProgramUniform4ui64vNV( GLuint program, GLint location, GLsizei count, UINT64* value ) {
  void (*func_glProgramUniform4ui64vNV)( GLuint, GLint, GLsizei, UINT64* ) = extension_funcs[EXT_glProgramUniform4ui64vNV];
  TRACE("(%d, %d, %d, %p)\n", program, location, count, value );
  ENTER_GL();
  func_glProgramUniform4ui64vNV( program, location, count, value );
  LEAVE_GL();
}

static void WINAPI wine_glProgramUniform4uiEXT( GLuint program, GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3 ) {
  void (*func_glProgramUniform4uiEXT)( GLuint, GLint, GLuint, GLuint, GLuint, GLuint ) = extension_funcs[EXT_glProgramUniform4uiEXT];
  TRACE("(%d, %d, %d, %d, %d, %d)\n", program, location, v0, v1, v2, v3 );
  ENTER_GL();
  func_glProgramUniform4uiEXT( program, location, v0, v1, v2, v3 );
  LEAVE_GL();
}

static void WINAPI wine_glProgramUniform4uiv( GLuint program, GLint location, GLsizei count, GLuint* value ) {
  void (*func_glProgramUniform4uiv)( GLuint, GLint, GLsizei, GLuint* ) = extension_funcs[EXT_glProgramUniform4uiv];
  TRACE("(%d, %d, %d, %p)\n", program, location, count, value );
  ENTER_GL();
  func_glProgramUniform4uiv( program, location, count, value );
  LEAVE_GL();
}

static void WINAPI wine_glProgramUniform4uivEXT( GLuint program, GLint location, GLsizei count, GLuint* value ) {
  void (*func_glProgramUniform4uivEXT)( GLuint, GLint, GLsizei, GLuint* ) = extension_funcs[EXT_glProgramUniform4uivEXT];
  TRACE("(%d, %d, %d, %p)\n", program, location, count, value );
  ENTER_GL();
  func_glProgramUniform4uivEXT( program, location, count, value );
  LEAVE_GL();
}

static void WINAPI wine_glProgramUniformMatrix2dv( GLuint program, GLint location, GLsizei count, GLboolean transpose, GLdouble* value ) {
  void (*func_glProgramUniformMatrix2dv)( GLuint, GLint, GLsizei, GLboolean, GLdouble* ) = extension_funcs[EXT_glProgramUniformMatrix2dv];
  TRACE("(%d, %d, %d, %d, %p)\n", program, location, count, transpose, value );
  ENTER_GL();
  func_glProgramUniformMatrix2dv( program, location, count, transpose, value );
  LEAVE_GL();
}

static void WINAPI wine_glProgramUniformMatrix2dvEXT( GLuint program, GLint location, GLsizei count, GLboolean transpose, GLdouble* value ) {
  void (*func_glProgramUniformMatrix2dvEXT)( GLuint, GLint, GLsizei, GLboolean, GLdouble* ) = extension_funcs[EXT_glProgramUniformMatrix2dvEXT];
  TRACE("(%d, %d, %d, %d, %p)\n", program, location, count, transpose, value );
  ENTER_GL();
  func_glProgramUniformMatrix2dvEXT( program, location, count, transpose, value );
  LEAVE_GL();
}

static void WINAPI wine_glProgramUniformMatrix2fv( GLuint program, GLint location, GLsizei count, GLboolean transpose, GLfloat* value ) {
  void (*func_glProgramUniformMatrix2fv)( GLuint, GLint, GLsizei, GLboolean, GLfloat* ) = extension_funcs[EXT_glProgramUniformMatrix2fv];
  TRACE("(%d, %d, %d, %d, %p)\n", program, location, count, transpose, value );
  ENTER_GL();
  func_glProgramUniformMatrix2fv( program, location, count, transpose, value );
  LEAVE_GL();
}

static void WINAPI wine_glProgramUniformMatrix2fvEXT( GLuint program, GLint location, GLsizei count, GLboolean transpose, GLfloat* value ) {
  void (*func_glProgramUniformMatrix2fvEXT)( GLuint, GLint, GLsizei, GLboolean, GLfloat* ) = extension_funcs[EXT_glProgramUniformMatrix2fvEXT];
  TRACE("(%d, %d, %d, %d, %p)\n", program, location, count, transpose, value );
  ENTER_GL();
  func_glProgramUniformMatrix2fvEXT( program, location, count, transpose, value );
  LEAVE_GL();
}

static void WINAPI wine_glProgramUniformMatrix2x3dv( GLuint program, GLint location, GLsizei count, GLboolean transpose, GLdouble* value ) {
  void (*func_glProgramUniformMatrix2x3dv)( GLuint, GLint, GLsizei, GLboolean, GLdouble* ) = extension_funcs[EXT_glProgramUniformMatrix2x3dv];
  TRACE("(%d, %d, %d, %d, %p)\n", program, location, count, transpose, value );
  ENTER_GL();
  func_glProgramUniformMatrix2x3dv( program, location, count, transpose, value );
  LEAVE_GL();
}

static void WINAPI wine_glProgramUniformMatrix2x3dvEXT( GLuint program, GLint location, GLsizei count, GLboolean transpose, GLdouble* value ) {
  void (*func_glProgramUniformMatrix2x3dvEXT)( GLuint, GLint, GLsizei, GLboolean, GLdouble* ) = extension_funcs[EXT_glProgramUniformMatrix2x3dvEXT];
  TRACE("(%d, %d, %d, %d, %p)\n", program, location, count, transpose, value );
  ENTER_GL();
  func_glProgramUniformMatrix2x3dvEXT( program, location, count, transpose, value );
  LEAVE_GL();
}

static void WINAPI wine_glProgramUniformMatrix2x3fv( GLuint program, GLint location, GLsizei count, GLboolean transpose, GLfloat* value ) {
  void (*func_glProgramUniformMatrix2x3fv)( GLuint, GLint, GLsizei, GLboolean, GLfloat* ) = extension_funcs[EXT_glProgramUniformMatrix2x3fv];
  TRACE("(%d, %d, %d, %d, %p)\n", program, location, count, transpose, value );
  ENTER_GL();
  func_glProgramUniformMatrix2x3fv( program, location, count, transpose, value );
  LEAVE_GL();
}

static void WINAPI wine_glProgramUniformMatrix2x3fvEXT( GLuint program, GLint location, GLsizei count, GLboolean transpose, GLfloat* value ) {
  void (*func_glProgramUniformMatrix2x3fvEXT)( GLuint, GLint, GLsizei, GLboolean, GLfloat* ) = extension_funcs[EXT_glProgramUniformMatrix2x3fvEXT];
  TRACE("(%d, %d, %d, %d, %p)\n", program, location, count, transpose, value );
  ENTER_GL();
  func_glProgramUniformMatrix2x3fvEXT( program, location, count, transpose, value );
  LEAVE_GL();
}

static void WINAPI wine_glProgramUniformMatrix2x4dv( GLuint program, GLint location, GLsizei count, GLboolean transpose, GLdouble* value ) {
  void (*func_glProgramUniformMatrix2x4dv)( GLuint, GLint, GLsizei, GLboolean, GLdouble* ) = extension_funcs[EXT_glProgramUniformMatrix2x4dv];
  TRACE("(%d, %d, %d, %d, %p)\n", program, location, count, transpose, value );
  ENTER_GL();
  func_glProgramUniformMatrix2x4dv( program, location, count, transpose, value );
  LEAVE_GL();
}

static void WINAPI wine_glProgramUniformMatrix2x4dvEXT( GLuint program, GLint location, GLsizei count, GLboolean transpose, GLdouble* value ) {
  void (*func_glProgramUniformMatrix2x4dvEXT)( GLuint, GLint, GLsizei, GLboolean, GLdouble* ) = extension_funcs[EXT_glProgramUniformMatrix2x4dvEXT];
  TRACE("(%d, %d, %d, %d, %p)\n", program, location, count, transpose, value );
  ENTER_GL();
  func_glProgramUniformMatrix2x4dvEXT( program, location, count, transpose, value );
  LEAVE_GL();
}

static void WINAPI wine_glProgramUniformMatrix2x4fv( GLuint program, GLint location, GLsizei count, GLboolean transpose, GLfloat* value ) {
  void (*func_glProgramUniformMatrix2x4fv)( GLuint, GLint, GLsizei, GLboolean, GLfloat* ) = extension_funcs[EXT_glProgramUniformMatrix2x4fv];
  TRACE("(%d, %d, %d, %d, %p)\n", program, location, count, transpose, value );
  ENTER_GL();
  func_glProgramUniformMatrix2x4fv( program, location, count, transpose, value );
  LEAVE_GL();
}

static void WINAPI wine_glProgramUniformMatrix2x4fvEXT( GLuint program, GLint location, GLsizei count, GLboolean transpose, GLfloat* value ) {
  void (*func_glProgramUniformMatrix2x4fvEXT)( GLuint, GLint, GLsizei, GLboolean, GLfloat* ) = extension_funcs[EXT_glProgramUniformMatrix2x4fvEXT];
  TRACE("(%d, %d, %d, %d, %p)\n", program, location, count, transpose, value );
  ENTER_GL();
  func_glProgramUniformMatrix2x4fvEXT( program, location, count, transpose, value );
  LEAVE_GL();
}

static void WINAPI wine_glProgramUniformMatrix3dv( GLuint program, GLint location, GLsizei count, GLboolean transpose, GLdouble* value ) {
  void (*func_glProgramUniformMatrix3dv)( GLuint, GLint, GLsizei, GLboolean, GLdouble* ) = extension_funcs[EXT_glProgramUniformMatrix3dv];
  TRACE("(%d, %d, %d, %d, %p)\n", program, location, count, transpose, value );
  ENTER_GL();
  func_glProgramUniformMatrix3dv( program, location, count, transpose, value );
  LEAVE_GL();
}

static void WINAPI wine_glProgramUniformMatrix3dvEXT( GLuint program, GLint location, GLsizei count, GLboolean transpose, GLdouble* value ) {
  void (*func_glProgramUniformMatrix3dvEXT)( GLuint, GLint, GLsizei, GLboolean, GLdouble* ) = extension_funcs[EXT_glProgramUniformMatrix3dvEXT];
  TRACE("(%d, %d, %d, %d, %p)\n", program, location, count, transpose, value );
  ENTER_GL();
  func_glProgramUniformMatrix3dvEXT( program, location, count, transpose, value );
  LEAVE_GL();
}

static void WINAPI wine_glProgramUniformMatrix3fv( GLuint program, GLint location, GLsizei count, GLboolean transpose, GLfloat* value ) {
  void (*func_glProgramUniformMatrix3fv)( GLuint, GLint, GLsizei, GLboolean, GLfloat* ) = extension_funcs[EXT_glProgramUniformMatrix3fv];
  TRACE("(%d, %d, %d, %d, %p)\n", program, location, count, transpose, value );
  ENTER_GL();
  func_glProgramUniformMatrix3fv( program, location, count, transpose, value );
  LEAVE_GL();
}

static void WINAPI wine_glProgramUniformMatrix3fvEXT( GLuint program, GLint location, GLsizei count, GLboolean transpose, GLfloat* value ) {
  void (*func_glProgramUniformMatrix3fvEXT)( GLuint, GLint, GLsizei, GLboolean, GLfloat* ) = extension_funcs[EXT_glProgramUniformMatrix3fvEXT];
  TRACE("(%d, %d, %d, %d, %p)\n", program, location, count, transpose, value );
  ENTER_GL();
  func_glProgramUniformMatrix3fvEXT( program, location, count, transpose, value );
  LEAVE_GL();
}

static void WINAPI wine_glProgramUniformMatrix3x2dv( GLuint program, GLint location, GLsizei count, GLboolean transpose, GLdouble* value ) {
  void (*func_glProgramUniformMatrix3x2dv)( GLuint, GLint, GLsizei, GLboolean, GLdouble* ) = extension_funcs[EXT_glProgramUniformMatrix3x2dv];
  TRACE("(%d, %d, %d, %d, %p)\n", program, location, count, transpose, value );
  ENTER_GL();
  func_glProgramUniformMatrix3x2dv( program, location, count, transpose, value );
  LEAVE_GL();
}

static void WINAPI wine_glProgramUniformMatrix3x2dvEXT( GLuint program, GLint location, GLsizei count, GLboolean transpose, GLdouble* value ) {
  void (*func_glProgramUniformMatrix3x2dvEXT)( GLuint, GLint, GLsizei, GLboolean, GLdouble* ) = extension_funcs[EXT_glProgramUniformMatrix3x2dvEXT];
  TRACE("(%d, %d, %d, %d, %p)\n", program, location, count, transpose, value );
  ENTER_GL();
  func_glProgramUniformMatrix3x2dvEXT( program, location, count, transpose, value );
  LEAVE_GL();
}

static void WINAPI wine_glProgramUniformMatrix3x2fv( GLuint program, GLint location, GLsizei count, GLboolean transpose, GLfloat* value ) {
  void (*func_glProgramUniformMatrix3x2fv)( GLuint, GLint, GLsizei, GLboolean, GLfloat* ) = extension_funcs[EXT_glProgramUniformMatrix3x2fv];
  TRACE("(%d, %d, %d, %d, %p)\n", program, location, count, transpose, value );
  ENTER_GL();
  func_glProgramUniformMatrix3x2fv( program, location, count, transpose, value );
  LEAVE_GL();
}

static void WINAPI wine_glProgramUniformMatrix3x2fvEXT( GLuint program, GLint location, GLsizei count, GLboolean transpose, GLfloat* value ) {
  void (*func_glProgramUniformMatrix3x2fvEXT)( GLuint, GLint, GLsizei, GLboolean, GLfloat* ) = extension_funcs[EXT_glProgramUniformMatrix3x2fvEXT];
  TRACE("(%d, %d, %d, %d, %p)\n", program, location, count, transpose, value );
  ENTER_GL();
  func_glProgramUniformMatrix3x2fvEXT( program, location, count, transpose, value );
  LEAVE_GL();
}

static void WINAPI wine_glProgramUniformMatrix3x4dv( GLuint program, GLint location, GLsizei count, GLboolean transpose, GLdouble* value ) {
  void (*func_glProgramUniformMatrix3x4dv)( GLuint, GLint, GLsizei, GLboolean, GLdouble* ) = extension_funcs[EXT_glProgramUniformMatrix3x4dv];
  TRACE("(%d, %d, %d, %d, %p)\n", program, location, count, transpose, value );
  ENTER_GL();
  func_glProgramUniformMatrix3x4dv( program, location, count, transpose, value );
  LEAVE_GL();
}

static void WINAPI wine_glProgramUniformMatrix3x4dvEXT( GLuint program, GLint location, GLsizei count, GLboolean transpose, GLdouble* value ) {
  void (*func_glProgramUniformMatrix3x4dvEXT)( GLuint, GLint, GLsizei, GLboolean, GLdouble* ) = extension_funcs[EXT_glProgramUniformMatrix3x4dvEXT];
  TRACE("(%d, %d, %d, %d, %p)\n", program, location, count, transpose, value );
  ENTER_GL();
  func_glProgramUniformMatrix3x4dvEXT( program, location, count, transpose, value );
  LEAVE_GL();
}

static void WINAPI wine_glProgramUniformMatrix3x4fv( GLuint program, GLint location, GLsizei count, GLboolean transpose, GLfloat* value ) {
  void (*func_glProgramUniformMatrix3x4fv)( GLuint, GLint, GLsizei, GLboolean, GLfloat* ) = extension_funcs[EXT_glProgramUniformMatrix3x4fv];
  TRACE("(%d, %d, %d, %d, %p)\n", program, location, count, transpose, value );
  ENTER_GL();
  func_glProgramUniformMatrix3x4fv( program, location, count, transpose, value );
  LEAVE_GL();
}

static void WINAPI wine_glProgramUniformMatrix3x4fvEXT( GLuint program, GLint location, GLsizei count, GLboolean transpose, GLfloat* value ) {
  void (*func_glProgramUniformMatrix3x4fvEXT)( GLuint, GLint, GLsizei, GLboolean, GLfloat* ) = extension_funcs[EXT_glProgramUniformMatrix3x4fvEXT];
  TRACE("(%d, %d, %d, %d, %p)\n", program, location, count, transpose, value );
  ENTER_GL();
  func_glProgramUniformMatrix3x4fvEXT( program, location, count, transpose, value );
  LEAVE_GL();
}

static void WINAPI wine_glProgramUniformMatrix4dv( GLuint program, GLint location, GLsizei count, GLboolean transpose, GLdouble* value ) {
  void (*func_glProgramUniformMatrix4dv)( GLuint, GLint, GLsizei, GLboolean, GLdouble* ) = extension_funcs[EXT_glProgramUniformMatrix4dv];
  TRACE("(%d, %d, %d, %d, %p)\n", program, location, count, transpose, value );
  ENTER_GL();
  func_glProgramUniformMatrix4dv( program, location, count, transpose, value );
  LEAVE_GL();
}

static void WINAPI wine_glProgramUniformMatrix4dvEXT( GLuint program, GLint location, GLsizei count, GLboolean transpose, GLdouble* value ) {
  void (*func_glProgramUniformMatrix4dvEXT)( GLuint, GLint, GLsizei, GLboolean, GLdouble* ) = extension_funcs[EXT_glProgramUniformMatrix4dvEXT];
  TRACE("(%d, %d, %d, %d, %p)\n", program, location, count, transpose, value );
  ENTER_GL();
  func_glProgramUniformMatrix4dvEXT( program, location, count, transpose, value );
  LEAVE_GL();
}

static void WINAPI wine_glProgramUniformMatrix4fv( GLuint program, GLint location, GLsizei count, GLboolean transpose, GLfloat* value ) {
  void (*func_glProgramUniformMatrix4fv)( GLuint, GLint, GLsizei, GLboolean, GLfloat* ) = extension_funcs[EXT_glProgramUniformMatrix4fv];
  TRACE("(%d, %d, %d, %d, %p)\n", program, location, count, transpose, value );
  ENTER_GL();
  func_glProgramUniformMatrix4fv( program, location, count, transpose, value );
  LEAVE_GL();
}

static void WINAPI wine_glProgramUniformMatrix4fvEXT( GLuint program, GLint location, GLsizei count, GLboolean transpose, GLfloat* value ) {
  void (*func_glProgramUniformMatrix4fvEXT)( GLuint, GLint, GLsizei, GLboolean, GLfloat* ) = extension_funcs[EXT_glProgramUniformMatrix4fvEXT];
  TRACE("(%d, %d, %d, %d, %p)\n", program, location, count, transpose, value );
  ENTER_GL();
  func_glProgramUniformMatrix4fvEXT( program, location, count, transpose, value );
  LEAVE_GL();
}

static void WINAPI wine_glProgramUniformMatrix4x2dv( GLuint program, GLint location, GLsizei count, GLboolean transpose, GLdouble* value ) {
  void (*func_glProgramUniformMatrix4x2dv)( GLuint, GLint, GLsizei, GLboolean, GLdouble* ) = extension_funcs[EXT_glProgramUniformMatrix4x2dv];
  TRACE("(%d, %d, %d, %d, %p)\n", program, location, count, transpose, value );
  ENTER_GL();
  func_glProgramUniformMatrix4x2dv( program, location, count, transpose, value );
  LEAVE_GL();
}

static void WINAPI wine_glProgramUniformMatrix4x2dvEXT( GLuint program, GLint location, GLsizei count, GLboolean transpose, GLdouble* value ) {
  void (*func_glProgramUniformMatrix4x2dvEXT)( GLuint, GLint, GLsizei, GLboolean, GLdouble* ) = extension_funcs[EXT_glProgramUniformMatrix4x2dvEXT];
  TRACE("(%d, %d, %d, %d, %p)\n", program, location, count, transpose, value );
  ENTER_GL();
  func_glProgramUniformMatrix4x2dvEXT( program, location, count, transpose, value );
  LEAVE_GL();
}

static void WINAPI wine_glProgramUniformMatrix4x2fv( GLuint program, GLint location, GLsizei count, GLboolean transpose, GLfloat* value ) {
  void (*func_glProgramUniformMatrix4x2fv)( GLuint, GLint, GLsizei, GLboolean, GLfloat* ) = extension_funcs[EXT_glProgramUniformMatrix4x2fv];
  TRACE("(%d, %d, %d, %d, %p)\n", program, location, count, transpose, value );
  ENTER_GL();
  func_glProgramUniformMatrix4x2fv( program, location, count, transpose, value );
  LEAVE_GL();
}

static void WINAPI wine_glProgramUniformMatrix4x2fvEXT( GLuint program, GLint location, GLsizei count, GLboolean transpose, GLfloat* value ) {
  void (*func_glProgramUniformMatrix4x2fvEXT)( GLuint, GLint, GLsizei, GLboolean, GLfloat* ) = extension_funcs[EXT_glProgramUniformMatrix4x2fvEXT];
  TRACE("(%d, %d, %d, %d, %p)\n", program, location, count, transpose, value );
  ENTER_GL();
  func_glProgramUniformMatrix4x2fvEXT( program, location, count, transpose, value );
  LEAVE_GL();
}

static void WINAPI wine_glProgramUniformMatrix4x3dv( GLuint program, GLint location, GLsizei count, GLboolean transpose, GLdouble* value ) {
  void (*func_glProgramUniformMatrix4x3dv)( GLuint, GLint, GLsizei, GLboolean, GLdouble* ) = extension_funcs[EXT_glProgramUniformMatrix4x3dv];
  TRACE("(%d, %d, %d, %d, %p)\n", program, location, count, transpose, value );
  ENTER_GL();
  func_glProgramUniformMatrix4x3dv( program, location, count, transpose, value );
  LEAVE_GL();
}

static void WINAPI wine_glProgramUniformMatrix4x3dvEXT( GLuint program, GLint location, GLsizei count, GLboolean transpose, GLdouble* value ) {
  void (*func_glProgramUniformMatrix4x3dvEXT)( GLuint, GLint, GLsizei, GLboolean, GLdouble* ) = extension_funcs[EXT_glProgramUniformMatrix4x3dvEXT];
  TRACE("(%d, %d, %d, %d, %p)\n", program, location, count, transpose, value );
  ENTER_GL();
  func_glProgramUniformMatrix4x3dvEXT( program, location, count, transpose, value );
  LEAVE_GL();
}

static void WINAPI wine_glProgramUniformMatrix4x3fv( GLuint program, GLint location, GLsizei count, GLboolean transpose, GLfloat* value ) {
  void (*func_glProgramUniformMatrix4x3fv)( GLuint, GLint, GLsizei, GLboolean, GLfloat* ) = extension_funcs[EXT_glProgramUniformMatrix4x3fv];
  TRACE("(%d, %d, %d, %d, %p)\n", program, location, count, transpose, value );
  ENTER_GL();
  func_glProgramUniformMatrix4x3fv( program, location, count, transpose, value );
  LEAVE_GL();
}

static void WINAPI wine_glProgramUniformMatrix4x3fvEXT( GLuint program, GLint location, GLsizei count, GLboolean transpose, GLfloat* value ) {
  void (*func_glProgramUniformMatrix4x3fvEXT)( GLuint, GLint, GLsizei, GLboolean, GLfloat* ) = extension_funcs[EXT_glProgramUniformMatrix4x3fvEXT];
  TRACE("(%d, %d, %d, %d, %p)\n", program, location, count, transpose, value );
  ENTER_GL();
  func_glProgramUniformMatrix4x3fvEXT( program, location, count, transpose, value );
  LEAVE_GL();
}

static void WINAPI wine_glProgramUniformui64NV( GLuint program, GLint location, UINT64 value ) {
  void (*func_glProgramUniformui64NV)( GLuint, GLint, UINT64 ) = extension_funcs[EXT_glProgramUniformui64NV];
  TRACE("(%d, %d, %s)\n", program, location, wine_dbgstr_longlong(value) );
  ENTER_GL();
  func_glProgramUniformui64NV( program, location, value );
  LEAVE_GL();
}

static void WINAPI wine_glProgramUniformui64vNV( GLuint program, GLint location, GLsizei count, UINT64* value ) {
  void (*func_glProgramUniformui64vNV)( GLuint, GLint, GLsizei, UINT64* ) = extension_funcs[EXT_glProgramUniformui64vNV];
  TRACE("(%d, %d, %d, %p)\n", program, location, count, value );
  ENTER_GL();
  func_glProgramUniformui64vNV( program, location, count, value );
  LEAVE_GL();
}

static void WINAPI wine_glProgramVertexLimitNV( GLenum target, GLint limit ) {
  void (*func_glProgramVertexLimitNV)( GLenum, GLint ) = extension_funcs[EXT_glProgramVertexLimitNV];
  TRACE("(%d, %d)\n", target, limit );
  ENTER_GL();
  func_glProgramVertexLimitNV( target, limit );
  LEAVE_GL();
}

static void WINAPI wine_glProvokingVertex( GLenum mode ) {
  void (*func_glProvokingVertex)( GLenum ) = extension_funcs[EXT_glProvokingVertex];
  TRACE("(%d)\n", mode );
  ENTER_GL();
  func_glProvokingVertex( mode );
  LEAVE_GL();
}

static void WINAPI wine_glProvokingVertexEXT( GLenum mode ) {
  void (*func_glProvokingVertexEXT)( GLenum ) = extension_funcs[EXT_glProvokingVertexEXT];
  TRACE("(%d)\n", mode );
  ENTER_GL();
  func_glProvokingVertexEXT( mode );
  LEAVE_GL();
}

static void WINAPI wine_glPushClientAttribDefaultEXT( GLbitfield mask ) {
  void (*func_glPushClientAttribDefaultEXT)( GLbitfield ) = extension_funcs[EXT_glPushClientAttribDefaultEXT];
  TRACE("(%d)\n", mask );
  ENTER_GL();
  func_glPushClientAttribDefaultEXT( mask );
  LEAVE_GL();
}

static void WINAPI wine_glQueryCounter( GLuint id, GLenum target ) {
  void (*func_glQueryCounter)( GLuint, GLenum ) = extension_funcs[EXT_glQueryCounter];
  TRACE("(%d, %d)\n", id, target );
  ENTER_GL();
  func_glQueryCounter( id, target );
  LEAVE_GL();
}

static void WINAPI wine_glReadBufferRegion( GLenum region, GLint x, GLint y, GLsizei width, GLsizei height ) {
  void (*func_glReadBufferRegion)( GLenum, GLint, GLint, GLsizei, GLsizei ) = extension_funcs[EXT_glReadBufferRegion];
  TRACE("(%d, %d, %d, %d, %d)\n", region, x, y, width, height );
  ENTER_GL();
  func_glReadBufferRegion( region, x, y, width, height );
  LEAVE_GL();
}

static void WINAPI wine_glReadInstrumentsSGIX( GLint marker ) {
  void (*func_glReadInstrumentsSGIX)( GLint ) = extension_funcs[EXT_glReadInstrumentsSGIX];
  TRACE("(%d)\n", marker );
  ENTER_GL();
  func_glReadInstrumentsSGIX( marker );
  LEAVE_GL();
}

static void WINAPI wine_glReadnPixelsARB( GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLsizei bufSize, GLvoid* data ) {
  void (*func_glReadnPixelsARB)( GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, GLsizei, GLvoid* ) = extension_funcs[EXT_glReadnPixelsARB];
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %p)\n", x, y, width, height, format, type, bufSize, data );
  ENTER_GL();
  func_glReadnPixelsARB( x, y, width, height, format, type, bufSize, data );
  LEAVE_GL();
}

static void WINAPI wine_glReferencePlaneSGIX( GLdouble* equation ) {
  void (*func_glReferencePlaneSGIX)( GLdouble* ) = extension_funcs[EXT_glReferencePlaneSGIX];
  TRACE("(%p)\n", equation );
  ENTER_GL();
  func_glReferencePlaneSGIX( equation );
  LEAVE_GL();
}

static void WINAPI wine_glReleaseShaderCompiler( void ) {
  void (*func_glReleaseShaderCompiler)( void ) = extension_funcs[EXT_glReleaseShaderCompiler];
  TRACE("()\n");
  ENTER_GL();
  func_glReleaseShaderCompiler( );
  LEAVE_GL();
}

static void WINAPI wine_glRenderbufferStorage( GLenum target, GLenum internalformat, GLsizei width, GLsizei height ) {
  void (*func_glRenderbufferStorage)( GLenum, GLenum, GLsizei, GLsizei ) = extension_funcs[EXT_glRenderbufferStorage];
  TRACE("(%d, %d, %d, %d)\n", target, internalformat, width, height );
  ENTER_GL();
  func_glRenderbufferStorage( target, internalformat, width, height );
  LEAVE_GL();
}

static void WINAPI wine_glRenderbufferStorageEXT( GLenum target, GLenum internalformat, GLsizei width, GLsizei height ) {
  void (*func_glRenderbufferStorageEXT)( GLenum, GLenum, GLsizei, GLsizei ) = extension_funcs[EXT_glRenderbufferStorageEXT];
  TRACE("(%d, %d, %d, %d)\n", target, internalformat, width, height );
  ENTER_GL();
  func_glRenderbufferStorageEXT( target, internalformat, width, height );
  LEAVE_GL();
}

static void WINAPI wine_glRenderbufferStorageMultisample( GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height ) {
  void (*func_glRenderbufferStorageMultisample)( GLenum, GLsizei, GLenum, GLsizei, GLsizei ) = extension_funcs[EXT_glRenderbufferStorageMultisample];
  TRACE("(%d, %d, %d, %d, %d)\n", target, samples, internalformat, width, height );
  ENTER_GL();
  func_glRenderbufferStorageMultisample( target, samples, internalformat, width, height );
  LEAVE_GL();
}

static void WINAPI wine_glRenderbufferStorageMultisampleCoverageNV( GLenum target, GLsizei coverageSamples, GLsizei colorSamples, GLenum internalformat, GLsizei width, GLsizei height ) {
  void (*func_glRenderbufferStorageMultisampleCoverageNV)( GLenum, GLsizei, GLsizei, GLenum, GLsizei, GLsizei ) = extension_funcs[EXT_glRenderbufferStorageMultisampleCoverageNV];
  TRACE("(%d, %d, %d, %d, %d, %d)\n", target, coverageSamples, colorSamples, internalformat, width, height );
  ENTER_GL();
  func_glRenderbufferStorageMultisampleCoverageNV( target, coverageSamples, colorSamples, internalformat, width, height );
  LEAVE_GL();
}

static void WINAPI wine_glRenderbufferStorageMultisampleEXT( GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height ) {
  void (*func_glRenderbufferStorageMultisampleEXT)( GLenum, GLsizei, GLenum, GLsizei, GLsizei ) = extension_funcs[EXT_glRenderbufferStorageMultisampleEXT];
  TRACE("(%d, %d, %d, %d, %d)\n", target, samples, internalformat, width, height );
  ENTER_GL();
  func_glRenderbufferStorageMultisampleEXT( target, samples, internalformat, width, height );
  LEAVE_GL();
}

static void WINAPI wine_glReplacementCodePointerSUN( GLenum type, GLsizei stride, GLvoid** pointer ) {
  void (*func_glReplacementCodePointerSUN)( GLenum, GLsizei, GLvoid** ) = extension_funcs[EXT_glReplacementCodePointerSUN];
  TRACE("(%d, %d, %p)\n", type, stride, pointer );
  ENTER_GL();
  func_glReplacementCodePointerSUN( type, stride, pointer );
  LEAVE_GL();
}

static void WINAPI wine_glReplacementCodeubSUN( GLubyte code ) {
  void (*func_glReplacementCodeubSUN)( GLubyte ) = extension_funcs[EXT_glReplacementCodeubSUN];
  TRACE("(%d)\n", code );
  ENTER_GL();
  func_glReplacementCodeubSUN( code );
  LEAVE_GL();
}

static void WINAPI wine_glReplacementCodeubvSUN( GLubyte* code ) {
  void (*func_glReplacementCodeubvSUN)( GLubyte* ) = extension_funcs[EXT_glReplacementCodeubvSUN];
  TRACE("(%p)\n", code );
  ENTER_GL();
  func_glReplacementCodeubvSUN( code );
  LEAVE_GL();
}

static void WINAPI wine_glReplacementCodeuiColor3fVertex3fSUN( GLuint rc, GLfloat r, GLfloat g, GLfloat b, GLfloat x, GLfloat y, GLfloat z ) {
  void (*func_glReplacementCodeuiColor3fVertex3fSUN)( GLuint, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat ) = extension_funcs[EXT_glReplacementCodeuiColor3fVertex3fSUN];
  TRACE("(%d, %f, %f, %f, %f, %f, %f)\n", rc, r, g, b, x, y, z );
  ENTER_GL();
  func_glReplacementCodeuiColor3fVertex3fSUN( rc, r, g, b, x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glReplacementCodeuiColor3fVertex3fvSUN( GLuint* rc, GLfloat* c, GLfloat* v ) {
  void (*func_glReplacementCodeuiColor3fVertex3fvSUN)( GLuint*, GLfloat*, GLfloat* ) = extension_funcs[EXT_glReplacementCodeuiColor3fVertex3fvSUN];
  TRACE("(%p, %p, %p)\n", rc, c, v );
  ENTER_GL();
  func_glReplacementCodeuiColor3fVertex3fvSUN( rc, c, v );
  LEAVE_GL();
}

static void WINAPI wine_glReplacementCodeuiColor4fNormal3fVertex3fSUN( GLuint rc, GLfloat r, GLfloat g, GLfloat b, GLfloat a, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z ) {
  void (*func_glReplacementCodeuiColor4fNormal3fVertex3fSUN)( GLuint, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat ) = extension_funcs[EXT_glReplacementCodeuiColor4fNormal3fVertex3fSUN];
  TRACE("(%d, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f)\n", rc, r, g, b, a, nx, ny, nz, x, y, z );
  ENTER_GL();
  func_glReplacementCodeuiColor4fNormal3fVertex3fSUN( rc, r, g, b, a, nx, ny, nz, x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glReplacementCodeuiColor4fNormal3fVertex3fvSUN( GLuint* rc, GLfloat* c, GLfloat* n, GLfloat* v ) {
  void (*func_glReplacementCodeuiColor4fNormal3fVertex3fvSUN)( GLuint*, GLfloat*, GLfloat*, GLfloat* ) = extension_funcs[EXT_glReplacementCodeuiColor4fNormal3fVertex3fvSUN];
  TRACE("(%p, %p, %p, %p)\n", rc, c, n, v );
  ENTER_GL();
  func_glReplacementCodeuiColor4fNormal3fVertex3fvSUN( rc, c, n, v );
  LEAVE_GL();
}

static void WINAPI wine_glReplacementCodeuiColor4ubVertex3fSUN( GLuint rc, GLubyte r, GLubyte g, GLubyte b, GLubyte a, GLfloat x, GLfloat y, GLfloat z ) {
  void (*func_glReplacementCodeuiColor4ubVertex3fSUN)( GLuint, GLubyte, GLubyte, GLubyte, GLubyte, GLfloat, GLfloat, GLfloat ) = extension_funcs[EXT_glReplacementCodeuiColor4ubVertex3fSUN];
  TRACE("(%d, %d, %d, %d, %d, %f, %f, %f)\n", rc, r, g, b, a, x, y, z );
  ENTER_GL();
  func_glReplacementCodeuiColor4ubVertex3fSUN( rc, r, g, b, a, x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glReplacementCodeuiColor4ubVertex3fvSUN( GLuint* rc, GLubyte* c, GLfloat* v ) {
  void (*func_glReplacementCodeuiColor4ubVertex3fvSUN)( GLuint*, GLubyte*, GLfloat* ) = extension_funcs[EXT_glReplacementCodeuiColor4ubVertex3fvSUN];
  TRACE("(%p, %p, %p)\n", rc, c, v );
  ENTER_GL();
  func_glReplacementCodeuiColor4ubVertex3fvSUN( rc, c, v );
  LEAVE_GL();
}

static void WINAPI wine_glReplacementCodeuiNormal3fVertex3fSUN( GLuint rc, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z ) {
  void (*func_glReplacementCodeuiNormal3fVertex3fSUN)( GLuint, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat ) = extension_funcs[EXT_glReplacementCodeuiNormal3fVertex3fSUN];
  TRACE("(%d, %f, %f, %f, %f, %f, %f)\n", rc, nx, ny, nz, x, y, z );
  ENTER_GL();
  func_glReplacementCodeuiNormal3fVertex3fSUN( rc, nx, ny, nz, x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glReplacementCodeuiNormal3fVertex3fvSUN( GLuint* rc, GLfloat* n, GLfloat* v ) {
  void (*func_glReplacementCodeuiNormal3fVertex3fvSUN)( GLuint*, GLfloat*, GLfloat* ) = extension_funcs[EXT_glReplacementCodeuiNormal3fVertex3fvSUN];
  TRACE("(%p, %p, %p)\n", rc, n, v );
  ENTER_GL();
  func_glReplacementCodeuiNormal3fVertex3fvSUN( rc, n, v );
  LEAVE_GL();
}

static void WINAPI wine_glReplacementCodeuiSUN( GLuint code ) {
  void (*func_glReplacementCodeuiSUN)( GLuint ) = extension_funcs[EXT_glReplacementCodeuiSUN];
  TRACE("(%d)\n", code );
  ENTER_GL();
  func_glReplacementCodeuiSUN( code );
  LEAVE_GL();
}

static void WINAPI wine_glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fSUN( GLuint rc, GLfloat s, GLfloat t, GLfloat r, GLfloat g, GLfloat b, GLfloat a, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z ) {
  void (*func_glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fSUN)( GLuint, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat ) = extension_funcs[EXT_glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fSUN];
  TRACE("(%d, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f)\n", rc, s, t, r, g, b, a, nx, ny, nz, x, y, z );
  ENTER_GL();
  func_glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fSUN( rc, s, t, r, g, b, a, nx, ny, nz, x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fvSUN( GLuint* rc, GLfloat* tc, GLfloat* c, GLfloat* n, GLfloat* v ) {
  void (*func_glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fvSUN)( GLuint*, GLfloat*, GLfloat*, GLfloat*, GLfloat* ) = extension_funcs[EXT_glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fvSUN];
  TRACE("(%p, %p, %p, %p, %p)\n", rc, tc, c, n, v );
  ENTER_GL();
  func_glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fvSUN( rc, tc, c, n, v );
  LEAVE_GL();
}

static void WINAPI wine_glReplacementCodeuiTexCoord2fNormal3fVertex3fSUN( GLuint rc, GLfloat s, GLfloat t, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z ) {
  void (*func_glReplacementCodeuiTexCoord2fNormal3fVertex3fSUN)( GLuint, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat ) = extension_funcs[EXT_glReplacementCodeuiTexCoord2fNormal3fVertex3fSUN];
  TRACE("(%d, %f, %f, %f, %f, %f, %f, %f, %f)\n", rc, s, t, nx, ny, nz, x, y, z );
  ENTER_GL();
  func_glReplacementCodeuiTexCoord2fNormal3fVertex3fSUN( rc, s, t, nx, ny, nz, x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glReplacementCodeuiTexCoord2fNormal3fVertex3fvSUN( GLuint* rc, GLfloat* tc, GLfloat* n, GLfloat* v ) {
  void (*func_glReplacementCodeuiTexCoord2fNormal3fVertex3fvSUN)( GLuint*, GLfloat*, GLfloat*, GLfloat* ) = extension_funcs[EXT_glReplacementCodeuiTexCoord2fNormal3fVertex3fvSUN];
  TRACE("(%p, %p, %p, %p)\n", rc, tc, n, v );
  ENTER_GL();
  func_glReplacementCodeuiTexCoord2fNormal3fVertex3fvSUN( rc, tc, n, v );
  LEAVE_GL();
}

static void WINAPI wine_glReplacementCodeuiTexCoord2fVertex3fSUN( GLuint rc, GLfloat s, GLfloat t, GLfloat x, GLfloat y, GLfloat z ) {
  void (*func_glReplacementCodeuiTexCoord2fVertex3fSUN)( GLuint, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat ) = extension_funcs[EXT_glReplacementCodeuiTexCoord2fVertex3fSUN];
  TRACE("(%d, %f, %f, %f, %f, %f)\n", rc, s, t, x, y, z );
  ENTER_GL();
  func_glReplacementCodeuiTexCoord2fVertex3fSUN( rc, s, t, x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glReplacementCodeuiTexCoord2fVertex3fvSUN( GLuint* rc, GLfloat* tc, GLfloat* v ) {
  void (*func_glReplacementCodeuiTexCoord2fVertex3fvSUN)( GLuint*, GLfloat*, GLfloat* ) = extension_funcs[EXT_glReplacementCodeuiTexCoord2fVertex3fvSUN];
  TRACE("(%p, %p, %p)\n", rc, tc, v );
  ENTER_GL();
  func_glReplacementCodeuiTexCoord2fVertex3fvSUN( rc, tc, v );
  LEAVE_GL();
}

static void WINAPI wine_glReplacementCodeuiVertex3fSUN( GLuint rc, GLfloat x, GLfloat y, GLfloat z ) {
  void (*func_glReplacementCodeuiVertex3fSUN)( GLuint, GLfloat, GLfloat, GLfloat ) = extension_funcs[EXT_glReplacementCodeuiVertex3fSUN];
  TRACE("(%d, %f, %f, %f)\n", rc, x, y, z );
  ENTER_GL();
  func_glReplacementCodeuiVertex3fSUN( rc, x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glReplacementCodeuiVertex3fvSUN( GLuint* rc, GLfloat* v ) {
  void (*func_glReplacementCodeuiVertex3fvSUN)( GLuint*, GLfloat* ) = extension_funcs[EXT_glReplacementCodeuiVertex3fvSUN];
  TRACE("(%p, %p)\n", rc, v );
  ENTER_GL();
  func_glReplacementCodeuiVertex3fvSUN( rc, v );
  LEAVE_GL();
}

static void WINAPI wine_glReplacementCodeuivSUN( GLuint* code ) {
  void (*func_glReplacementCodeuivSUN)( GLuint* ) = extension_funcs[EXT_glReplacementCodeuivSUN];
  TRACE("(%p)\n", code );
  ENTER_GL();
  func_glReplacementCodeuivSUN( code );
  LEAVE_GL();
}

static void WINAPI wine_glReplacementCodeusSUN( GLushort code ) {
  void (*func_glReplacementCodeusSUN)( GLushort ) = extension_funcs[EXT_glReplacementCodeusSUN];
  TRACE("(%d)\n", code );
  ENTER_GL();
  func_glReplacementCodeusSUN( code );
  LEAVE_GL();
}

static void WINAPI wine_glReplacementCodeusvSUN( GLushort* code ) {
  void (*func_glReplacementCodeusvSUN)( GLushort* ) = extension_funcs[EXT_glReplacementCodeusvSUN];
  TRACE("(%p)\n", code );
  ENTER_GL();
  func_glReplacementCodeusvSUN( code );
  LEAVE_GL();
}

static void WINAPI wine_glRequestResidentProgramsNV( GLsizei n, GLuint* programs ) {
  void (*func_glRequestResidentProgramsNV)( GLsizei, GLuint* ) = extension_funcs[EXT_glRequestResidentProgramsNV];
  TRACE("(%d, %p)\n", n, programs );
  ENTER_GL();
  func_glRequestResidentProgramsNV( n, programs );
  LEAVE_GL();
}

static void WINAPI wine_glResetHistogram( GLenum target ) {
  void (*func_glResetHistogram)( GLenum ) = extension_funcs[EXT_glResetHistogram];
  TRACE("(%d)\n", target );
  ENTER_GL();
  func_glResetHistogram( target );
  LEAVE_GL();
}

static void WINAPI wine_glResetHistogramEXT( GLenum target ) {
  void (*func_glResetHistogramEXT)( GLenum ) = extension_funcs[EXT_glResetHistogramEXT];
  TRACE("(%d)\n", target );
  ENTER_GL();
  func_glResetHistogramEXT( target );
  LEAVE_GL();
}

static void WINAPI wine_glResetMinmax( GLenum target ) {
  void (*func_glResetMinmax)( GLenum ) = extension_funcs[EXT_glResetMinmax];
  TRACE("(%d)\n", target );
  ENTER_GL();
  func_glResetMinmax( target );
  LEAVE_GL();
}

static void WINAPI wine_glResetMinmaxEXT( GLenum target ) {
  void (*func_glResetMinmaxEXT)( GLenum ) = extension_funcs[EXT_glResetMinmaxEXT];
  TRACE("(%d)\n", target );
  ENTER_GL();
  func_glResetMinmaxEXT( target );
  LEAVE_GL();
}

static void WINAPI wine_glResizeBuffersMESA( void ) {
  void (*func_glResizeBuffersMESA)( void ) = extension_funcs[EXT_glResizeBuffersMESA];
  TRACE("()\n");
  ENTER_GL();
  func_glResizeBuffersMESA( );
  LEAVE_GL();
}

static void WINAPI wine_glResumeTransformFeedback( void ) {
  void (*func_glResumeTransformFeedback)( void ) = extension_funcs[EXT_glResumeTransformFeedback];
  TRACE("()\n");
  ENTER_GL();
  func_glResumeTransformFeedback( );
  LEAVE_GL();
}

static void WINAPI wine_glResumeTransformFeedbackNV( void ) {
  void (*func_glResumeTransformFeedbackNV)( void ) = extension_funcs[EXT_glResumeTransformFeedbackNV];
  TRACE("()\n");
  ENTER_GL();
  func_glResumeTransformFeedbackNV( );
  LEAVE_GL();
}

static void WINAPI wine_glSampleCoverage( GLclampf value, GLboolean invert ) {
  void (*func_glSampleCoverage)( GLclampf, GLboolean ) = extension_funcs[EXT_glSampleCoverage];
  TRACE("(%f, %d)\n", value, invert );
  ENTER_GL();
  func_glSampleCoverage( value, invert );
  LEAVE_GL();
}

static void WINAPI wine_glSampleCoverageARB( GLclampf value, GLboolean invert ) {
  void (*func_glSampleCoverageARB)( GLclampf, GLboolean ) = extension_funcs[EXT_glSampleCoverageARB];
  TRACE("(%f, %d)\n", value, invert );
  ENTER_GL();
  func_glSampleCoverageARB( value, invert );
  LEAVE_GL();
}

static void WINAPI wine_glSampleMapATI( GLuint dst, GLuint interp, GLenum swizzle ) {
  void (*func_glSampleMapATI)( GLuint, GLuint, GLenum ) = extension_funcs[EXT_glSampleMapATI];
  TRACE("(%d, %d, %d)\n", dst, interp, swizzle );
  ENTER_GL();
  func_glSampleMapATI( dst, interp, swizzle );
  LEAVE_GL();
}

static void WINAPI wine_glSampleMaskEXT( GLclampf value, GLboolean invert ) {
  void (*func_glSampleMaskEXT)( GLclampf, GLboolean ) = extension_funcs[EXT_glSampleMaskEXT];
  TRACE("(%f, %d)\n", value, invert );
  ENTER_GL();
  func_glSampleMaskEXT( value, invert );
  LEAVE_GL();
}

static void WINAPI wine_glSampleMaskIndexedNV( GLuint index, GLbitfield mask ) {
  void (*func_glSampleMaskIndexedNV)( GLuint, GLbitfield ) = extension_funcs[EXT_glSampleMaskIndexedNV];
  TRACE("(%d, %d)\n", index, mask );
  ENTER_GL();
  func_glSampleMaskIndexedNV( index, mask );
  LEAVE_GL();
}

static void WINAPI wine_glSampleMaskSGIS( GLclampf value, GLboolean invert ) {
  void (*func_glSampleMaskSGIS)( GLclampf, GLboolean ) = extension_funcs[EXT_glSampleMaskSGIS];
  TRACE("(%f, %d)\n", value, invert );
  ENTER_GL();
  func_glSampleMaskSGIS( value, invert );
  LEAVE_GL();
}

static void WINAPI wine_glSampleMaski( GLuint index, GLbitfield mask ) {
  void (*func_glSampleMaski)( GLuint, GLbitfield ) = extension_funcs[EXT_glSampleMaski];
  TRACE("(%d, %d)\n", index, mask );
  ENTER_GL();
  func_glSampleMaski( index, mask );
  LEAVE_GL();
}

static void WINAPI wine_glSamplePatternEXT( GLenum pattern ) {
  void (*func_glSamplePatternEXT)( GLenum ) = extension_funcs[EXT_glSamplePatternEXT];
  TRACE("(%d)\n", pattern );
  ENTER_GL();
  func_glSamplePatternEXT( pattern );
  LEAVE_GL();
}

static void WINAPI wine_glSamplePatternSGIS( GLenum pattern ) {
  void (*func_glSamplePatternSGIS)( GLenum ) = extension_funcs[EXT_glSamplePatternSGIS];
  TRACE("(%d)\n", pattern );
  ENTER_GL();
  func_glSamplePatternSGIS( pattern );
  LEAVE_GL();
}

static void WINAPI wine_glSamplerParameterIiv( GLuint sampler, GLenum pname, GLint* param ) {
  void (*func_glSamplerParameterIiv)( GLuint, GLenum, GLint* ) = extension_funcs[EXT_glSamplerParameterIiv];
  TRACE("(%d, %d, %p)\n", sampler, pname, param );
  ENTER_GL();
  func_glSamplerParameterIiv( sampler, pname, param );
  LEAVE_GL();
}

static void WINAPI wine_glSamplerParameterIuiv( GLuint sampler, GLenum pname, GLuint* param ) {
  void (*func_glSamplerParameterIuiv)( GLuint, GLenum, GLuint* ) = extension_funcs[EXT_glSamplerParameterIuiv];
  TRACE("(%d, %d, %p)\n", sampler, pname, param );
  ENTER_GL();
  func_glSamplerParameterIuiv( sampler, pname, param );
  LEAVE_GL();
}

static void WINAPI wine_glSamplerParameterf( GLuint sampler, GLenum pname, GLfloat param ) {
  void (*func_glSamplerParameterf)( GLuint, GLenum, GLfloat ) = extension_funcs[EXT_glSamplerParameterf];
  TRACE("(%d, %d, %f)\n", sampler, pname, param );
  ENTER_GL();
  func_glSamplerParameterf( sampler, pname, param );
  LEAVE_GL();
}

static void WINAPI wine_glSamplerParameterfv( GLuint sampler, GLenum pname, GLfloat* param ) {
  void (*func_glSamplerParameterfv)( GLuint, GLenum, GLfloat* ) = extension_funcs[EXT_glSamplerParameterfv];
  TRACE("(%d, %d, %p)\n", sampler, pname, param );
  ENTER_GL();
  func_glSamplerParameterfv( sampler, pname, param );
  LEAVE_GL();
}

static void WINAPI wine_glSamplerParameteri( GLuint sampler, GLenum pname, GLint param ) {
  void (*func_glSamplerParameteri)( GLuint, GLenum, GLint ) = extension_funcs[EXT_glSamplerParameteri];
  TRACE("(%d, %d, %d)\n", sampler, pname, param );
  ENTER_GL();
  func_glSamplerParameteri( sampler, pname, param );
  LEAVE_GL();
}

static void WINAPI wine_glSamplerParameteriv( GLuint sampler, GLenum pname, GLint* param ) {
  void (*func_glSamplerParameteriv)( GLuint, GLenum, GLint* ) = extension_funcs[EXT_glSamplerParameteriv];
  TRACE("(%d, %d, %p)\n", sampler, pname, param );
  ENTER_GL();
  func_glSamplerParameteriv( sampler, pname, param );
  LEAVE_GL();
}

static void WINAPI wine_glScissorArrayv( GLuint first, GLsizei count, GLint* v ) {
  void (*func_glScissorArrayv)( GLuint, GLsizei, GLint* ) = extension_funcs[EXT_glScissorArrayv];
  TRACE("(%d, %d, %p)\n", first, count, v );
  ENTER_GL();
  func_glScissorArrayv( first, count, v );
  LEAVE_GL();
}

static void WINAPI wine_glScissorIndexed( GLuint index, GLint left, GLint bottom, GLsizei width, GLsizei height ) {
  void (*func_glScissorIndexed)( GLuint, GLint, GLint, GLsizei, GLsizei ) = extension_funcs[EXT_glScissorIndexed];
  TRACE("(%d, %d, %d, %d, %d)\n", index, left, bottom, width, height );
  ENTER_GL();
  func_glScissorIndexed( index, left, bottom, width, height );
  LEAVE_GL();
}

static void WINAPI wine_glScissorIndexedv( GLuint index, GLint* v ) {
  void (*func_glScissorIndexedv)( GLuint, GLint* ) = extension_funcs[EXT_glScissorIndexedv];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glScissorIndexedv( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glSecondaryColor3b( GLbyte red, GLbyte green, GLbyte blue ) {
  void (*func_glSecondaryColor3b)( GLbyte, GLbyte, GLbyte ) = extension_funcs[EXT_glSecondaryColor3b];
  TRACE("(%d, %d, %d)\n", red, green, blue );
  ENTER_GL();
  func_glSecondaryColor3b( red, green, blue );
  LEAVE_GL();
}

static void WINAPI wine_glSecondaryColor3bEXT( GLbyte red, GLbyte green, GLbyte blue ) {
  void (*func_glSecondaryColor3bEXT)( GLbyte, GLbyte, GLbyte ) = extension_funcs[EXT_glSecondaryColor3bEXT];
  TRACE("(%d, %d, %d)\n", red, green, blue );
  ENTER_GL();
  func_glSecondaryColor3bEXT( red, green, blue );
  LEAVE_GL();
}

static void WINAPI wine_glSecondaryColor3bv( GLbyte* v ) {
  void (*func_glSecondaryColor3bv)( GLbyte* ) = extension_funcs[EXT_glSecondaryColor3bv];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glSecondaryColor3bv( v );
  LEAVE_GL();
}

static void WINAPI wine_glSecondaryColor3bvEXT( GLbyte* v ) {
  void (*func_glSecondaryColor3bvEXT)( GLbyte* ) = extension_funcs[EXT_glSecondaryColor3bvEXT];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glSecondaryColor3bvEXT( v );
  LEAVE_GL();
}

static void WINAPI wine_glSecondaryColor3d( GLdouble red, GLdouble green, GLdouble blue ) {
  void (*func_glSecondaryColor3d)( GLdouble, GLdouble, GLdouble ) = extension_funcs[EXT_glSecondaryColor3d];
  TRACE("(%f, %f, %f)\n", red, green, blue );
  ENTER_GL();
  func_glSecondaryColor3d( red, green, blue );
  LEAVE_GL();
}

static void WINAPI wine_glSecondaryColor3dEXT( GLdouble red, GLdouble green, GLdouble blue ) {
  void (*func_glSecondaryColor3dEXT)( GLdouble, GLdouble, GLdouble ) = extension_funcs[EXT_glSecondaryColor3dEXT];
  TRACE("(%f, %f, %f)\n", red, green, blue );
  ENTER_GL();
  func_glSecondaryColor3dEXT( red, green, blue );
  LEAVE_GL();
}

static void WINAPI wine_glSecondaryColor3dv( GLdouble* v ) {
  void (*func_glSecondaryColor3dv)( GLdouble* ) = extension_funcs[EXT_glSecondaryColor3dv];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glSecondaryColor3dv( v );
  LEAVE_GL();
}

static void WINAPI wine_glSecondaryColor3dvEXT( GLdouble* v ) {
  void (*func_glSecondaryColor3dvEXT)( GLdouble* ) = extension_funcs[EXT_glSecondaryColor3dvEXT];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glSecondaryColor3dvEXT( v );
  LEAVE_GL();
}

static void WINAPI wine_glSecondaryColor3f( GLfloat red, GLfloat green, GLfloat blue ) {
  void (*func_glSecondaryColor3f)( GLfloat, GLfloat, GLfloat ) = extension_funcs[EXT_glSecondaryColor3f];
  TRACE("(%f, %f, %f)\n", red, green, blue );
  ENTER_GL();
  func_glSecondaryColor3f( red, green, blue );
  LEAVE_GL();
}

static void WINAPI wine_glSecondaryColor3fEXT( GLfloat red, GLfloat green, GLfloat blue ) {
  void (*func_glSecondaryColor3fEXT)( GLfloat, GLfloat, GLfloat ) = extension_funcs[EXT_glSecondaryColor3fEXT];
  TRACE("(%f, %f, %f)\n", red, green, blue );
  ENTER_GL();
  func_glSecondaryColor3fEXT( red, green, blue );
  LEAVE_GL();
}

static void WINAPI wine_glSecondaryColor3fv( GLfloat* v ) {
  void (*func_glSecondaryColor3fv)( GLfloat* ) = extension_funcs[EXT_glSecondaryColor3fv];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glSecondaryColor3fv( v );
  LEAVE_GL();
}

static void WINAPI wine_glSecondaryColor3fvEXT( GLfloat* v ) {
  void (*func_glSecondaryColor3fvEXT)( GLfloat* ) = extension_funcs[EXT_glSecondaryColor3fvEXT];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glSecondaryColor3fvEXT( v );
  LEAVE_GL();
}

static void WINAPI wine_glSecondaryColor3hNV( unsigned short red, unsigned short green, unsigned short blue ) {
  void (*func_glSecondaryColor3hNV)( unsigned short, unsigned short, unsigned short ) = extension_funcs[EXT_glSecondaryColor3hNV];
  TRACE("(%d, %d, %d)\n", red, green, blue );
  ENTER_GL();
  func_glSecondaryColor3hNV( red, green, blue );
  LEAVE_GL();
}

static void WINAPI wine_glSecondaryColor3hvNV( unsigned short* v ) {
  void (*func_glSecondaryColor3hvNV)( unsigned short* ) = extension_funcs[EXT_glSecondaryColor3hvNV];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glSecondaryColor3hvNV( v );
  LEAVE_GL();
}

static void WINAPI wine_glSecondaryColor3i( GLint red, GLint green, GLint blue ) {
  void (*func_glSecondaryColor3i)( GLint, GLint, GLint ) = extension_funcs[EXT_glSecondaryColor3i];
  TRACE("(%d, %d, %d)\n", red, green, blue );
  ENTER_GL();
  func_glSecondaryColor3i( red, green, blue );
  LEAVE_GL();
}

static void WINAPI wine_glSecondaryColor3iEXT( GLint red, GLint green, GLint blue ) {
  void (*func_glSecondaryColor3iEXT)( GLint, GLint, GLint ) = extension_funcs[EXT_glSecondaryColor3iEXT];
  TRACE("(%d, %d, %d)\n", red, green, blue );
  ENTER_GL();
  func_glSecondaryColor3iEXT( red, green, blue );
  LEAVE_GL();
}

static void WINAPI wine_glSecondaryColor3iv( GLint* v ) {
  void (*func_glSecondaryColor3iv)( GLint* ) = extension_funcs[EXT_glSecondaryColor3iv];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glSecondaryColor3iv( v );
  LEAVE_GL();
}

static void WINAPI wine_glSecondaryColor3ivEXT( GLint* v ) {
  void (*func_glSecondaryColor3ivEXT)( GLint* ) = extension_funcs[EXT_glSecondaryColor3ivEXT];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glSecondaryColor3ivEXT( v );
  LEAVE_GL();
}

static void WINAPI wine_glSecondaryColor3s( GLshort red, GLshort green, GLshort blue ) {
  void (*func_glSecondaryColor3s)( GLshort, GLshort, GLshort ) = extension_funcs[EXT_glSecondaryColor3s];
  TRACE("(%d, %d, %d)\n", red, green, blue );
  ENTER_GL();
  func_glSecondaryColor3s( red, green, blue );
  LEAVE_GL();
}

static void WINAPI wine_glSecondaryColor3sEXT( GLshort red, GLshort green, GLshort blue ) {
  void (*func_glSecondaryColor3sEXT)( GLshort, GLshort, GLshort ) = extension_funcs[EXT_glSecondaryColor3sEXT];
  TRACE("(%d, %d, %d)\n", red, green, blue );
  ENTER_GL();
  func_glSecondaryColor3sEXT( red, green, blue );
  LEAVE_GL();
}

static void WINAPI wine_glSecondaryColor3sv( GLshort* v ) {
  void (*func_glSecondaryColor3sv)( GLshort* ) = extension_funcs[EXT_glSecondaryColor3sv];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glSecondaryColor3sv( v );
  LEAVE_GL();
}

static void WINAPI wine_glSecondaryColor3svEXT( GLshort* v ) {
  void (*func_glSecondaryColor3svEXT)( GLshort* ) = extension_funcs[EXT_glSecondaryColor3svEXT];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glSecondaryColor3svEXT( v );
  LEAVE_GL();
}

static void WINAPI wine_glSecondaryColor3ub( GLubyte red, GLubyte green, GLubyte blue ) {
  void (*func_glSecondaryColor3ub)( GLubyte, GLubyte, GLubyte ) = extension_funcs[EXT_glSecondaryColor3ub];
  TRACE("(%d, %d, %d)\n", red, green, blue );
  ENTER_GL();
  func_glSecondaryColor3ub( red, green, blue );
  LEAVE_GL();
}

static void WINAPI wine_glSecondaryColor3ubEXT( GLubyte red, GLubyte green, GLubyte blue ) {
  void (*func_glSecondaryColor3ubEXT)( GLubyte, GLubyte, GLubyte ) = extension_funcs[EXT_glSecondaryColor3ubEXT];
  TRACE("(%d, %d, %d)\n", red, green, blue );
  ENTER_GL();
  func_glSecondaryColor3ubEXT( red, green, blue );
  LEAVE_GL();
}

static void WINAPI wine_glSecondaryColor3ubv( GLubyte* v ) {
  void (*func_glSecondaryColor3ubv)( GLubyte* ) = extension_funcs[EXT_glSecondaryColor3ubv];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glSecondaryColor3ubv( v );
  LEAVE_GL();
}

static void WINAPI wine_glSecondaryColor3ubvEXT( GLubyte* v ) {
  void (*func_glSecondaryColor3ubvEXT)( GLubyte* ) = extension_funcs[EXT_glSecondaryColor3ubvEXT];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glSecondaryColor3ubvEXT( v );
  LEAVE_GL();
}

static void WINAPI wine_glSecondaryColor3ui( GLuint red, GLuint green, GLuint blue ) {
  void (*func_glSecondaryColor3ui)( GLuint, GLuint, GLuint ) = extension_funcs[EXT_glSecondaryColor3ui];
  TRACE("(%d, %d, %d)\n", red, green, blue );
  ENTER_GL();
  func_glSecondaryColor3ui( red, green, blue );
  LEAVE_GL();
}

static void WINAPI wine_glSecondaryColor3uiEXT( GLuint red, GLuint green, GLuint blue ) {
  void (*func_glSecondaryColor3uiEXT)( GLuint, GLuint, GLuint ) = extension_funcs[EXT_glSecondaryColor3uiEXT];
  TRACE("(%d, %d, %d)\n", red, green, blue );
  ENTER_GL();
  func_glSecondaryColor3uiEXT( red, green, blue );
  LEAVE_GL();
}

static void WINAPI wine_glSecondaryColor3uiv( GLuint* v ) {
  void (*func_glSecondaryColor3uiv)( GLuint* ) = extension_funcs[EXT_glSecondaryColor3uiv];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glSecondaryColor3uiv( v );
  LEAVE_GL();
}

static void WINAPI wine_glSecondaryColor3uivEXT( GLuint* v ) {
  void (*func_glSecondaryColor3uivEXT)( GLuint* ) = extension_funcs[EXT_glSecondaryColor3uivEXT];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glSecondaryColor3uivEXT( v );
  LEAVE_GL();
}

static void WINAPI wine_glSecondaryColor3us( GLushort red, GLushort green, GLushort blue ) {
  void (*func_glSecondaryColor3us)( GLushort, GLushort, GLushort ) = extension_funcs[EXT_glSecondaryColor3us];
  TRACE("(%d, %d, %d)\n", red, green, blue );
  ENTER_GL();
  func_glSecondaryColor3us( red, green, blue );
  LEAVE_GL();
}

static void WINAPI wine_glSecondaryColor3usEXT( GLushort red, GLushort green, GLushort blue ) {
  void (*func_glSecondaryColor3usEXT)( GLushort, GLushort, GLushort ) = extension_funcs[EXT_glSecondaryColor3usEXT];
  TRACE("(%d, %d, %d)\n", red, green, blue );
  ENTER_GL();
  func_glSecondaryColor3usEXT( red, green, blue );
  LEAVE_GL();
}

static void WINAPI wine_glSecondaryColor3usv( GLushort* v ) {
  void (*func_glSecondaryColor3usv)( GLushort* ) = extension_funcs[EXT_glSecondaryColor3usv];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glSecondaryColor3usv( v );
  LEAVE_GL();
}

static void WINAPI wine_glSecondaryColor3usvEXT( GLushort* v ) {
  void (*func_glSecondaryColor3usvEXT)( GLushort* ) = extension_funcs[EXT_glSecondaryColor3usvEXT];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glSecondaryColor3usvEXT( v );
  LEAVE_GL();
}

static void WINAPI wine_glSecondaryColorFormatNV( GLint size, GLenum type, GLsizei stride ) {
  void (*func_glSecondaryColorFormatNV)( GLint, GLenum, GLsizei ) = extension_funcs[EXT_glSecondaryColorFormatNV];
  TRACE("(%d, %d, %d)\n", size, type, stride );
  ENTER_GL();
  func_glSecondaryColorFormatNV( size, type, stride );
  LEAVE_GL();
}

static void WINAPI wine_glSecondaryColorP3ui( GLenum type, GLuint color ) {
  void (*func_glSecondaryColorP3ui)( GLenum, GLuint ) = extension_funcs[EXT_glSecondaryColorP3ui];
  TRACE("(%d, %d)\n", type, color );
  ENTER_GL();
  func_glSecondaryColorP3ui( type, color );
  LEAVE_GL();
}

static void WINAPI wine_glSecondaryColorP3uiv( GLenum type, GLuint* color ) {
  void (*func_glSecondaryColorP3uiv)( GLenum, GLuint* ) = extension_funcs[EXT_glSecondaryColorP3uiv];
  TRACE("(%d, %p)\n", type, color );
  ENTER_GL();
  func_glSecondaryColorP3uiv( type, color );
  LEAVE_GL();
}

static void WINAPI wine_glSecondaryColorPointer( GLint size, GLenum type, GLsizei stride, GLvoid* pointer ) {
  void (*func_glSecondaryColorPointer)( GLint, GLenum, GLsizei, GLvoid* ) = extension_funcs[EXT_glSecondaryColorPointer];
  TRACE("(%d, %d, %d, %p)\n", size, type, stride, pointer );
  ENTER_GL();
  func_glSecondaryColorPointer( size, type, stride, pointer );
  LEAVE_GL();
}

static void WINAPI wine_glSecondaryColorPointerEXT( GLint size, GLenum type, GLsizei stride, GLvoid* pointer ) {
  void (*func_glSecondaryColorPointerEXT)( GLint, GLenum, GLsizei, GLvoid* ) = extension_funcs[EXT_glSecondaryColorPointerEXT];
  TRACE("(%d, %d, %d, %p)\n", size, type, stride, pointer );
  ENTER_GL();
  func_glSecondaryColorPointerEXT( size, type, stride, pointer );
  LEAVE_GL();
}

static void WINAPI wine_glSecondaryColorPointerListIBM( GLint size, GLenum type, GLint stride, GLvoid** pointer, GLint ptrstride ) {
  void (*func_glSecondaryColorPointerListIBM)( GLint, GLenum, GLint, GLvoid**, GLint ) = extension_funcs[EXT_glSecondaryColorPointerListIBM];
  TRACE("(%d, %d, %d, %p, %d)\n", size, type, stride, pointer, ptrstride );
  ENTER_GL();
  func_glSecondaryColorPointerListIBM( size, type, stride, pointer, ptrstride );
  LEAVE_GL();
}

static void WINAPI wine_glSelectPerfMonitorCountersAMD( GLuint monitor, GLboolean enable, GLuint group, GLint numCounters, GLuint* counterList ) {
  void (*func_glSelectPerfMonitorCountersAMD)( GLuint, GLboolean, GLuint, GLint, GLuint* ) = extension_funcs[EXT_glSelectPerfMonitorCountersAMD];
  TRACE("(%d, %d, %d, %d, %p)\n", monitor, enable, group, numCounters, counterList );
  ENTER_GL();
  func_glSelectPerfMonitorCountersAMD( monitor, enable, group, numCounters, counterList );
  LEAVE_GL();
}

static void WINAPI wine_glSelectTextureCoordSetSGIS( GLenum target ) {
  void (*func_glSelectTextureCoordSetSGIS)( GLenum ) = extension_funcs[EXT_glSelectTextureCoordSetSGIS];
  TRACE("(%d)\n", target );
  ENTER_GL();
  func_glSelectTextureCoordSetSGIS( target );
  LEAVE_GL();
}

static void WINAPI wine_glSelectTextureSGIS( GLenum target ) {
  void (*func_glSelectTextureSGIS)( GLenum ) = extension_funcs[EXT_glSelectTextureSGIS];
  TRACE("(%d)\n", target );
  ENTER_GL();
  func_glSelectTextureSGIS( target );
  LEAVE_GL();
}

static void WINAPI wine_glSeparableFilter2D( GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid* row, GLvoid* column ) {
  void (*func_glSeparableFilter2D)( GLenum, GLenum, GLsizei, GLsizei, GLenum, GLenum, GLvoid*, GLvoid* ) = extension_funcs[EXT_glSeparableFilter2D];
  TRACE("(%d, %d, %d, %d, %d, %d, %p, %p)\n", target, internalformat, width, height, format, type, row, column );
  ENTER_GL();
  func_glSeparableFilter2D( target, internalformat, width, height, format, type, row, column );
  LEAVE_GL();
}

static void WINAPI wine_glSeparableFilter2DEXT( GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid* row, GLvoid* column ) {
  void (*func_glSeparableFilter2DEXT)( GLenum, GLenum, GLsizei, GLsizei, GLenum, GLenum, GLvoid*, GLvoid* ) = extension_funcs[EXT_glSeparableFilter2DEXT];
  TRACE("(%d, %d, %d, %d, %d, %d, %p, %p)\n", target, internalformat, width, height, format, type, row, column );
  ENTER_GL();
  func_glSeparableFilter2DEXT( target, internalformat, width, height, format, type, row, column );
  LEAVE_GL();
}

static void WINAPI wine_glSetFenceAPPLE( GLuint fence ) {
  void (*func_glSetFenceAPPLE)( GLuint ) = extension_funcs[EXT_glSetFenceAPPLE];
  TRACE("(%d)\n", fence );
  ENTER_GL();
  func_glSetFenceAPPLE( fence );
  LEAVE_GL();
}

static void WINAPI wine_glSetFenceNV( GLuint fence, GLenum condition ) {
  void (*func_glSetFenceNV)( GLuint, GLenum ) = extension_funcs[EXT_glSetFenceNV];
  TRACE("(%d, %d)\n", fence, condition );
  ENTER_GL();
  func_glSetFenceNV( fence, condition );
  LEAVE_GL();
}

static void WINAPI wine_glSetFragmentShaderConstantATI( GLuint dst, GLfloat* value ) {
  void (*func_glSetFragmentShaderConstantATI)( GLuint, GLfloat* ) = extension_funcs[EXT_glSetFragmentShaderConstantATI];
  TRACE("(%d, %p)\n", dst, value );
  ENTER_GL();
  func_glSetFragmentShaderConstantATI( dst, value );
  LEAVE_GL();
}

static void WINAPI wine_glSetInvariantEXT( GLuint id, GLenum type, GLvoid* addr ) {
  void (*func_glSetInvariantEXT)( GLuint, GLenum, GLvoid* ) = extension_funcs[EXT_glSetInvariantEXT];
  TRACE("(%d, %d, %p)\n", id, type, addr );
  ENTER_GL();
  func_glSetInvariantEXT( id, type, addr );
  LEAVE_GL();
}

static void WINAPI wine_glSetLocalConstantEXT( GLuint id, GLenum type, GLvoid* addr ) {
  void (*func_glSetLocalConstantEXT)( GLuint, GLenum, GLvoid* ) = extension_funcs[EXT_glSetLocalConstantEXT];
  TRACE("(%d, %d, %p)\n", id, type, addr );
  ENTER_GL();
  func_glSetLocalConstantEXT( id, type, addr );
  LEAVE_GL();
}

static void WINAPI wine_glShaderBinary( GLsizei count, GLuint* shaders, GLenum binaryformat, GLvoid* binary, GLsizei length ) {
  void (*func_glShaderBinary)( GLsizei, GLuint*, GLenum, GLvoid*, GLsizei ) = extension_funcs[EXT_glShaderBinary];
  TRACE("(%d, %p, %d, %p, %d)\n", count, shaders, binaryformat, binary, length );
  ENTER_GL();
  func_glShaderBinary( count, shaders, binaryformat, binary, length );
  LEAVE_GL();
}

static void WINAPI wine_glShaderOp1EXT( GLenum op, GLuint res, GLuint arg1 ) {
  void (*func_glShaderOp1EXT)( GLenum, GLuint, GLuint ) = extension_funcs[EXT_glShaderOp1EXT];
  TRACE("(%d, %d, %d)\n", op, res, arg1 );
  ENTER_GL();
  func_glShaderOp1EXT( op, res, arg1 );
  LEAVE_GL();
}

static void WINAPI wine_glShaderOp2EXT( GLenum op, GLuint res, GLuint arg1, GLuint arg2 ) {
  void (*func_glShaderOp2EXT)( GLenum, GLuint, GLuint, GLuint ) = extension_funcs[EXT_glShaderOp2EXT];
  TRACE("(%d, %d, %d, %d)\n", op, res, arg1, arg2 );
  ENTER_GL();
  func_glShaderOp2EXT( op, res, arg1, arg2 );
  LEAVE_GL();
}

static void WINAPI wine_glShaderOp3EXT( GLenum op, GLuint res, GLuint arg1, GLuint arg2, GLuint arg3 ) {
  void (*func_glShaderOp3EXT)( GLenum, GLuint, GLuint, GLuint, GLuint ) = extension_funcs[EXT_glShaderOp3EXT];
  TRACE("(%d, %d, %d, %d, %d)\n", op, res, arg1, arg2, arg3 );
  ENTER_GL();
  func_glShaderOp3EXT( op, res, arg1, arg2, arg3 );
  LEAVE_GL();
}

static void WINAPI wine_glShaderSource( GLuint shader, GLsizei count, char** string, GLint* length ) {
  void (*func_glShaderSource)( GLuint, GLsizei, char**, GLint* ) = extension_funcs[EXT_glShaderSource];
  TRACE("(%d, %d, %p, %p)\n", shader, count, string, length );
  ENTER_GL();
  func_glShaderSource( shader, count, string, length );
  LEAVE_GL();
}

static void WINAPI wine_glShaderSourceARB( unsigned int shaderObj, GLsizei count, char** string, GLint* length ) {
  void (*func_glShaderSourceARB)( unsigned int, GLsizei, char**, GLint* ) = extension_funcs[EXT_glShaderSourceARB];
  TRACE("(%d, %d, %p, %p)\n", shaderObj, count, string, length );
  ENTER_GL();
  func_glShaderSourceARB( shaderObj, count, string, length );
  LEAVE_GL();
}

static void WINAPI wine_glSharpenTexFuncSGIS( GLenum target, GLsizei n, GLfloat* points ) {
  void (*func_glSharpenTexFuncSGIS)( GLenum, GLsizei, GLfloat* ) = extension_funcs[EXT_glSharpenTexFuncSGIS];
  TRACE("(%d, %d, %p)\n", target, n, points );
  ENTER_GL();
  func_glSharpenTexFuncSGIS( target, n, points );
  LEAVE_GL();
}

static void WINAPI wine_glSpriteParameterfSGIX( GLenum pname, GLfloat param ) {
  void (*func_glSpriteParameterfSGIX)( GLenum, GLfloat ) = extension_funcs[EXT_glSpriteParameterfSGIX];
  TRACE("(%d, %f)\n", pname, param );
  ENTER_GL();
  func_glSpriteParameterfSGIX( pname, param );
  LEAVE_GL();
}

static void WINAPI wine_glSpriteParameterfvSGIX( GLenum pname, GLfloat* params ) {
  void (*func_glSpriteParameterfvSGIX)( GLenum, GLfloat* ) = extension_funcs[EXT_glSpriteParameterfvSGIX];
  TRACE("(%d, %p)\n", pname, params );
  ENTER_GL();
  func_glSpriteParameterfvSGIX( pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glSpriteParameteriSGIX( GLenum pname, GLint param ) {
  void (*func_glSpriteParameteriSGIX)( GLenum, GLint ) = extension_funcs[EXT_glSpriteParameteriSGIX];
  TRACE("(%d, %d)\n", pname, param );
  ENTER_GL();
  func_glSpriteParameteriSGIX( pname, param );
  LEAVE_GL();
}

static void WINAPI wine_glSpriteParameterivSGIX( GLenum pname, GLint* params ) {
  void (*func_glSpriteParameterivSGIX)( GLenum, GLint* ) = extension_funcs[EXT_glSpriteParameterivSGIX];
  TRACE("(%d, %p)\n", pname, params );
  ENTER_GL();
  func_glSpriteParameterivSGIX( pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glStartInstrumentsSGIX( void ) {
  void (*func_glStartInstrumentsSGIX)( void ) = extension_funcs[EXT_glStartInstrumentsSGIX];
  TRACE("()\n");
  ENTER_GL();
  func_glStartInstrumentsSGIX( );
  LEAVE_GL();
}

static void WINAPI wine_glStencilClearTagEXT( GLsizei stencilTagBits, GLuint stencilClearTag ) {
  void (*func_glStencilClearTagEXT)( GLsizei, GLuint ) = extension_funcs[EXT_glStencilClearTagEXT];
  TRACE("(%d, %d)\n", stencilTagBits, stencilClearTag );
  ENTER_GL();
  func_glStencilClearTagEXT( stencilTagBits, stencilClearTag );
  LEAVE_GL();
}

static void WINAPI wine_glStencilFuncSeparate( GLenum face, GLenum func, GLint ref, GLuint mask ) {
  void (*func_glStencilFuncSeparate)( GLenum, GLenum, GLint, GLuint ) = extension_funcs[EXT_glStencilFuncSeparate];
  TRACE("(%d, %d, %d, %d)\n", face, func, ref, mask );
  ENTER_GL();
  func_glStencilFuncSeparate( face, func, ref, mask );
  LEAVE_GL();
}

static void WINAPI wine_glStencilFuncSeparateATI( GLenum frontfunc, GLenum backfunc, GLint ref, GLuint mask ) {
  void (*func_glStencilFuncSeparateATI)( GLenum, GLenum, GLint, GLuint ) = extension_funcs[EXT_glStencilFuncSeparateATI];
  TRACE("(%d, %d, %d, %d)\n", frontfunc, backfunc, ref, mask );
  ENTER_GL();
  func_glStencilFuncSeparateATI( frontfunc, backfunc, ref, mask );
  LEAVE_GL();
}

static void WINAPI wine_glStencilMaskSeparate( GLenum face, GLuint mask ) {
  void (*func_glStencilMaskSeparate)( GLenum, GLuint ) = extension_funcs[EXT_glStencilMaskSeparate];
  TRACE("(%d, %d)\n", face, mask );
  ENTER_GL();
  func_glStencilMaskSeparate( face, mask );
  LEAVE_GL();
}

static void WINAPI wine_glStencilOpSeparate( GLenum face, GLenum sfail, GLenum dpfail, GLenum dppass ) {
  void (*func_glStencilOpSeparate)( GLenum, GLenum, GLenum, GLenum ) = extension_funcs[EXT_glStencilOpSeparate];
  TRACE("(%d, %d, %d, %d)\n", face, sfail, dpfail, dppass );
  ENTER_GL();
  func_glStencilOpSeparate( face, sfail, dpfail, dppass );
  LEAVE_GL();
}

static void WINAPI wine_glStencilOpSeparateATI( GLenum face, GLenum sfail, GLenum dpfail, GLenum dppass ) {
  void (*func_glStencilOpSeparateATI)( GLenum, GLenum, GLenum, GLenum ) = extension_funcs[EXT_glStencilOpSeparateATI];
  TRACE("(%d, %d, %d, %d)\n", face, sfail, dpfail, dppass );
  ENTER_GL();
  func_glStencilOpSeparateATI( face, sfail, dpfail, dppass );
  LEAVE_GL();
}

static void WINAPI wine_glStopInstrumentsSGIX( GLint marker ) {
  void (*func_glStopInstrumentsSGIX)( GLint ) = extension_funcs[EXT_glStopInstrumentsSGIX];
  TRACE("(%d)\n", marker );
  ENTER_GL();
  func_glStopInstrumentsSGIX( marker );
  LEAVE_GL();
}

static void WINAPI wine_glStringMarkerGREMEDY( GLsizei len, GLvoid* string ) {
  void (*func_glStringMarkerGREMEDY)( GLsizei, GLvoid* ) = extension_funcs[EXT_glStringMarkerGREMEDY];
  TRACE("(%d, %p)\n", len, string );
  ENTER_GL();
  func_glStringMarkerGREMEDY( len, string );
  LEAVE_GL();
}

static void WINAPI wine_glSwizzleEXT( GLuint res, GLuint in, GLenum outX, GLenum outY, GLenum outZ, GLenum outW ) {
  void (*func_glSwizzleEXT)( GLuint, GLuint, GLenum, GLenum, GLenum, GLenum ) = extension_funcs[EXT_glSwizzleEXT];
  TRACE("(%d, %d, %d, %d, %d, %d)\n", res, in, outX, outY, outZ, outW );
  ENTER_GL();
  func_glSwizzleEXT( res, in, outX, outY, outZ, outW );
  LEAVE_GL();
}

static void WINAPI wine_glTagSampleBufferSGIX( void ) {
  void (*func_glTagSampleBufferSGIX)( void ) = extension_funcs[EXT_glTagSampleBufferSGIX];
  TRACE("()\n");
  ENTER_GL();
  func_glTagSampleBufferSGIX( );
  LEAVE_GL();
}

static void WINAPI wine_glTangent3bEXT( GLbyte tx, GLbyte ty, GLbyte tz ) {
  void (*func_glTangent3bEXT)( GLbyte, GLbyte, GLbyte ) = extension_funcs[EXT_glTangent3bEXT];
  TRACE("(%d, %d, %d)\n", tx, ty, tz );
  ENTER_GL();
  func_glTangent3bEXT( tx, ty, tz );
  LEAVE_GL();
}

static void WINAPI wine_glTangent3bvEXT( GLbyte* v ) {
  void (*func_glTangent3bvEXT)( GLbyte* ) = extension_funcs[EXT_glTangent3bvEXT];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glTangent3bvEXT( v );
  LEAVE_GL();
}

static void WINAPI wine_glTangent3dEXT( GLdouble tx, GLdouble ty, GLdouble tz ) {
  void (*func_glTangent3dEXT)( GLdouble, GLdouble, GLdouble ) = extension_funcs[EXT_glTangent3dEXT];
  TRACE("(%f, %f, %f)\n", tx, ty, tz );
  ENTER_GL();
  func_glTangent3dEXT( tx, ty, tz );
  LEAVE_GL();
}

static void WINAPI wine_glTangent3dvEXT( GLdouble* v ) {
  void (*func_glTangent3dvEXT)( GLdouble* ) = extension_funcs[EXT_glTangent3dvEXT];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glTangent3dvEXT( v );
  LEAVE_GL();
}

static void WINAPI wine_glTangent3fEXT( GLfloat tx, GLfloat ty, GLfloat tz ) {
  void (*func_glTangent3fEXT)( GLfloat, GLfloat, GLfloat ) = extension_funcs[EXT_glTangent3fEXT];
  TRACE("(%f, %f, %f)\n", tx, ty, tz );
  ENTER_GL();
  func_glTangent3fEXT( tx, ty, tz );
  LEAVE_GL();
}

static void WINAPI wine_glTangent3fvEXT( GLfloat* v ) {
  void (*func_glTangent3fvEXT)( GLfloat* ) = extension_funcs[EXT_glTangent3fvEXT];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glTangent3fvEXT( v );
  LEAVE_GL();
}

static void WINAPI wine_glTangent3iEXT( GLint tx, GLint ty, GLint tz ) {
  void (*func_glTangent3iEXT)( GLint, GLint, GLint ) = extension_funcs[EXT_glTangent3iEXT];
  TRACE("(%d, %d, %d)\n", tx, ty, tz );
  ENTER_GL();
  func_glTangent3iEXT( tx, ty, tz );
  LEAVE_GL();
}

static void WINAPI wine_glTangent3ivEXT( GLint* v ) {
  void (*func_glTangent3ivEXT)( GLint* ) = extension_funcs[EXT_glTangent3ivEXT];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glTangent3ivEXT( v );
  LEAVE_GL();
}

static void WINAPI wine_glTangent3sEXT( GLshort tx, GLshort ty, GLshort tz ) {
  void (*func_glTangent3sEXT)( GLshort, GLshort, GLshort ) = extension_funcs[EXT_glTangent3sEXT];
  TRACE("(%d, %d, %d)\n", tx, ty, tz );
  ENTER_GL();
  func_glTangent3sEXT( tx, ty, tz );
  LEAVE_GL();
}

static void WINAPI wine_glTangent3svEXT( GLshort* v ) {
  void (*func_glTangent3svEXT)( GLshort* ) = extension_funcs[EXT_glTangent3svEXT];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glTangent3svEXT( v );
  LEAVE_GL();
}

static void WINAPI wine_glTangentPointerEXT( GLenum type, GLsizei stride, GLvoid* pointer ) {
  void (*func_glTangentPointerEXT)( GLenum, GLsizei, GLvoid* ) = extension_funcs[EXT_glTangentPointerEXT];
  TRACE("(%d, %d, %p)\n", type, stride, pointer );
  ENTER_GL();
  func_glTangentPointerEXT( type, stride, pointer );
  LEAVE_GL();
}

static void WINAPI wine_glTbufferMask3DFX( GLuint mask ) {
  void (*func_glTbufferMask3DFX)( GLuint ) = extension_funcs[EXT_glTbufferMask3DFX];
  TRACE("(%d)\n", mask );
  ENTER_GL();
  func_glTbufferMask3DFX( mask );
  LEAVE_GL();
}

static void WINAPI wine_glTessellationFactorAMD( GLfloat factor ) {
  void (*func_glTessellationFactorAMD)( GLfloat ) = extension_funcs[EXT_glTessellationFactorAMD];
  TRACE("(%f)\n", factor );
  ENTER_GL();
  func_glTessellationFactorAMD( factor );
  LEAVE_GL();
}

static void WINAPI wine_glTessellationModeAMD( GLenum mode ) {
  void (*func_glTessellationModeAMD)( GLenum ) = extension_funcs[EXT_glTessellationModeAMD];
  TRACE("(%d)\n", mode );
  ENTER_GL();
  func_glTessellationModeAMD( mode );
  LEAVE_GL();
}

static GLboolean WINAPI wine_glTestFenceAPPLE( GLuint fence ) {
  GLboolean ret_value;
  GLboolean (*func_glTestFenceAPPLE)( GLuint ) = extension_funcs[EXT_glTestFenceAPPLE];
  TRACE("(%d)\n", fence );
  ENTER_GL();
  ret_value = func_glTestFenceAPPLE( fence );
  LEAVE_GL();
  return ret_value;
}

static GLboolean WINAPI wine_glTestFenceNV( GLuint fence ) {
  GLboolean ret_value;
  GLboolean (*func_glTestFenceNV)( GLuint ) = extension_funcs[EXT_glTestFenceNV];
  TRACE("(%d)\n", fence );
  ENTER_GL();
  ret_value = func_glTestFenceNV( fence );
  LEAVE_GL();
  return ret_value;
}

static GLboolean WINAPI wine_glTestObjectAPPLE( GLenum object, GLuint name ) {
  GLboolean ret_value;
  GLboolean (*func_glTestObjectAPPLE)( GLenum, GLuint ) = extension_funcs[EXT_glTestObjectAPPLE];
  TRACE("(%d, %d)\n", object, name );
  ENTER_GL();
  ret_value = func_glTestObjectAPPLE( object, name );
  LEAVE_GL();
  return ret_value;
}

static void WINAPI wine_glTexBuffer( GLenum target, GLenum internalformat, GLuint buffer ) {
  void (*func_glTexBuffer)( GLenum, GLenum, GLuint ) = extension_funcs[EXT_glTexBuffer];
  TRACE("(%d, %d, %d)\n", target, internalformat, buffer );
  ENTER_GL();
  func_glTexBuffer( target, internalformat, buffer );
  LEAVE_GL();
}

static void WINAPI wine_glTexBufferARB( GLenum target, GLenum internalformat, GLuint buffer ) {
  void (*func_glTexBufferARB)( GLenum, GLenum, GLuint ) = extension_funcs[EXT_glTexBufferARB];
  TRACE("(%d, %d, %d)\n", target, internalformat, buffer );
  ENTER_GL();
  func_glTexBufferARB( target, internalformat, buffer );
  LEAVE_GL();
}

static void WINAPI wine_glTexBufferEXT( GLenum target, GLenum internalformat, GLuint buffer ) {
  void (*func_glTexBufferEXT)( GLenum, GLenum, GLuint ) = extension_funcs[EXT_glTexBufferEXT];
  TRACE("(%d, %d, %d)\n", target, internalformat, buffer );
  ENTER_GL();
  func_glTexBufferEXT( target, internalformat, buffer );
  LEAVE_GL();
}

static void WINAPI wine_glTexBumpParameterfvATI( GLenum pname, GLfloat* param ) {
  void (*func_glTexBumpParameterfvATI)( GLenum, GLfloat* ) = extension_funcs[EXT_glTexBumpParameterfvATI];
  TRACE("(%d, %p)\n", pname, param );
  ENTER_GL();
  func_glTexBumpParameterfvATI( pname, param );
  LEAVE_GL();
}

static void WINAPI wine_glTexBumpParameterivATI( GLenum pname, GLint* param ) {
  void (*func_glTexBumpParameterivATI)( GLenum, GLint* ) = extension_funcs[EXT_glTexBumpParameterivATI];
  TRACE("(%d, %p)\n", pname, param );
  ENTER_GL();
  func_glTexBumpParameterivATI( pname, param );
  LEAVE_GL();
}

static void WINAPI wine_glTexCoord1hNV( unsigned short s ) {
  void (*func_glTexCoord1hNV)( unsigned short ) = extension_funcs[EXT_glTexCoord1hNV];
  TRACE("(%d)\n", s );
  ENTER_GL();
  func_glTexCoord1hNV( s );
  LEAVE_GL();
}

static void WINAPI wine_glTexCoord1hvNV( unsigned short* v ) {
  void (*func_glTexCoord1hvNV)( unsigned short* ) = extension_funcs[EXT_glTexCoord1hvNV];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glTexCoord1hvNV( v );
  LEAVE_GL();
}

static void WINAPI wine_glTexCoord2fColor3fVertex3fSUN( GLfloat s, GLfloat t, GLfloat r, GLfloat g, GLfloat b, GLfloat x, GLfloat y, GLfloat z ) {
  void (*func_glTexCoord2fColor3fVertex3fSUN)( GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat ) = extension_funcs[EXT_glTexCoord2fColor3fVertex3fSUN];
  TRACE("(%f, %f, %f, %f, %f, %f, %f, %f)\n", s, t, r, g, b, x, y, z );
  ENTER_GL();
  func_glTexCoord2fColor3fVertex3fSUN( s, t, r, g, b, x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glTexCoord2fColor3fVertex3fvSUN( GLfloat* tc, GLfloat* c, GLfloat* v ) {
  void (*func_glTexCoord2fColor3fVertex3fvSUN)( GLfloat*, GLfloat*, GLfloat* ) = extension_funcs[EXT_glTexCoord2fColor3fVertex3fvSUN];
  TRACE("(%p, %p, %p)\n", tc, c, v );
  ENTER_GL();
  func_glTexCoord2fColor3fVertex3fvSUN( tc, c, v );
  LEAVE_GL();
}

static void WINAPI wine_glTexCoord2fColor4fNormal3fVertex3fSUN( GLfloat s, GLfloat t, GLfloat r, GLfloat g, GLfloat b, GLfloat a, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z ) {
  void (*func_glTexCoord2fColor4fNormal3fVertex3fSUN)( GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat ) = extension_funcs[EXT_glTexCoord2fColor4fNormal3fVertex3fSUN];
  TRACE("(%f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f)\n", s, t, r, g, b, a, nx, ny, nz, x, y, z );
  ENTER_GL();
  func_glTexCoord2fColor4fNormal3fVertex3fSUN( s, t, r, g, b, a, nx, ny, nz, x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glTexCoord2fColor4fNormal3fVertex3fvSUN( GLfloat* tc, GLfloat* c, GLfloat* n, GLfloat* v ) {
  void (*func_glTexCoord2fColor4fNormal3fVertex3fvSUN)( GLfloat*, GLfloat*, GLfloat*, GLfloat* ) = extension_funcs[EXT_glTexCoord2fColor4fNormal3fVertex3fvSUN];
  TRACE("(%p, %p, %p, %p)\n", tc, c, n, v );
  ENTER_GL();
  func_glTexCoord2fColor4fNormal3fVertex3fvSUN( tc, c, n, v );
  LEAVE_GL();
}

static void WINAPI wine_glTexCoord2fColor4ubVertex3fSUN( GLfloat s, GLfloat t, GLubyte r, GLubyte g, GLubyte b, GLubyte a, GLfloat x, GLfloat y, GLfloat z ) {
  void (*func_glTexCoord2fColor4ubVertex3fSUN)( GLfloat, GLfloat, GLubyte, GLubyte, GLubyte, GLubyte, GLfloat, GLfloat, GLfloat ) = extension_funcs[EXT_glTexCoord2fColor4ubVertex3fSUN];
  TRACE("(%f, %f, %d, %d, %d, %d, %f, %f, %f)\n", s, t, r, g, b, a, x, y, z );
  ENTER_GL();
  func_glTexCoord2fColor4ubVertex3fSUN( s, t, r, g, b, a, x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glTexCoord2fColor4ubVertex3fvSUN( GLfloat* tc, GLubyte* c, GLfloat* v ) {
  void (*func_glTexCoord2fColor4ubVertex3fvSUN)( GLfloat*, GLubyte*, GLfloat* ) = extension_funcs[EXT_glTexCoord2fColor4ubVertex3fvSUN];
  TRACE("(%p, %p, %p)\n", tc, c, v );
  ENTER_GL();
  func_glTexCoord2fColor4ubVertex3fvSUN( tc, c, v );
  LEAVE_GL();
}

static void WINAPI wine_glTexCoord2fNormal3fVertex3fSUN( GLfloat s, GLfloat t, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z ) {
  void (*func_glTexCoord2fNormal3fVertex3fSUN)( GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat ) = extension_funcs[EXT_glTexCoord2fNormal3fVertex3fSUN];
  TRACE("(%f, %f, %f, %f, %f, %f, %f, %f)\n", s, t, nx, ny, nz, x, y, z );
  ENTER_GL();
  func_glTexCoord2fNormal3fVertex3fSUN( s, t, nx, ny, nz, x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glTexCoord2fNormal3fVertex3fvSUN( GLfloat* tc, GLfloat* n, GLfloat* v ) {
  void (*func_glTexCoord2fNormal3fVertex3fvSUN)( GLfloat*, GLfloat*, GLfloat* ) = extension_funcs[EXT_glTexCoord2fNormal3fVertex3fvSUN];
  TRACE("(%p, %p, %p)\n", tc, n, v );
  ENTER_GL();
  func_glTexCoord2fNormal3fVertex3fvSUN( tc, n, v );
  LEAVE_GL();
}

static void WINAPI wine_glTexCoord2fVertex3fSUN( GLfloat s, GLfloat t, GLfloat x, GLfloat y, GLfloat z ) {
  void (*func_glTexCoord2fVertex3fSUN)( GLfloat, GLfloat, GLfloat, GLfloat, GLfloat ) = extension_funcs[EXT_glTexCoord2fVertex3fSUN];
  TRACE("(%f, %f, %f, %f, %f)\n", s, t, x, y, z );
  ENTER_GL();
  func_glTexCoord2fVertex3fSUN( s, t, x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glTexCoord2fVertex3fvSUN( GLfloat* tc, GLfloat* v ) {
  void (*func_glTexCoord2fVertex3fvSUN)( GLfloat*, GLfloat* ) = extension_funcs[EXT_glTexCoord2fVertex3fvSUN];
  TRACE("(%p, %p)\n", tc, v );
  ENTER_GL();
  func_glTexCoord2fVertex3fvSUN( tc, v );
  LEAVE_GL();
}

static void WINAPI wine_glTexCoord2hNV( unsigned short s, unsigned short t ) {
  void (*func_glTexCoord2hNV)( unsigned short, unsigned short ) = extension_funcs[EXT_glTexCoord2hNV];
  TRACE("(%d, %d)\n", s, t );
  ENTER_GL();
  func_glTexCoord2hNV( s, t );
  LEAVE_GL();
}

static void WINAPI wine_glTexCoord2hvNV( unsigned short* v ) {
  void (*func_glTexCoord2hvNV)( unsigned short* ) = extension_funcs[EXT_glTexCoord2hvNV];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glTexCoord2hvNV( v );
  LEAVE_GL();
}

static void WINAPI wine_glTexCoord3hNV( unsigned short s, unsigned short t, unsigned short r ) {
  void (*func_glTexCoord3hNV)( unsigned short, unsigned short, unsigned short ) = extension_funcs[EXT_glTexCoord3hNV];
  TRACE("(%d, %d, %d)\n", s, t, r );
  ENTER_GL();
  func_glTexCoord3hNV( s, t, r );
  LEAVE_GL();
}

static void WINAPI wine_glTexCoord3hvNV( unsigned short* v ) {
  void (*func_glTexCoord3hvNV)( unsigned short* ) = extension_funcs[EXT_glTexCoord3hvNV];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glTexCoord3hvNV( v );
  LEAVE_GL();
}

static void WINAPI wine_glTexCoord4fColor4fNormal3fVertex4fSUN( GLfloat s, GLfloat t, GLfloat p, GLfloat q, GLfloat r, GLfloat g, GLfloat b, GLfloat a, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z, GLfloat w ) {
  void (*func_glTexCoord4fColor4fNormal3fVertex4fSUN)( GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat ) = extension_funcs[EXT_glTexCoord4fColor4fNormal3fVertex4fSUN];
  TRACE("(%f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f)\n", s, t, p, q, r, g, b, a, nx, ny, nz, x, y, z, w );
  ENTER_GL();
  func_glTexCoord4fColor4fNormal3fVertex4fSUN( s, t, p, q, r, g, b, a, nx, ny, nz, x, y, z, w );
  LEAVE_GL();
}

static void WINAPI wine_glTexCoord4fColor4fNormal3fVertex4fvSUN( GLfloat* tc, GLfloat* c, GLfloat* n, GLfloat* v ) {
  void (*func_glTexCoord4fColor4fNormal3fVertex4fvSUN)( GLfloat*, GLfloat*, GLfloat*, GLfloat* ) = extension_funcs[EXT_glTexCoord4fColor4fNormal3fVertex4fvSUN];
  TRACE("(%p, %p, %p, %p)\n", tc, c, n, v );
  ENTER_GL();
  func_glTexCoord4fColor4fNormal3fVertex4fvSUN( tc, c, n, v );
  LEAVE_GL();
}

static void WINAPI wine_glTexCoord4fVertex4fSUN( GLfloat s, GLfloat t, GLfloat p, GLfloat q, GLfloat x, GLfloat y, GLfloat z, GLfloat w ) {
  void (*func_glTexCoord4fVertex4fSUN)( GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat ) = extension_funcs[EXT_glTexCoord4fVertex4fSUN];
  TRACE("(%f, %f, %f, %f, %f, %f, %f, %f)\n", s, t, p, q, x, y, z, w );
  ENTER_GL();
  func_glTexCoord4fVertex4fSUN( s, t, p, q, x, y, z, w );
  LEAVE_GL();
}

static void WINAPI wine_glTexCoord4fVertex4fvSUN( GLfloat* tc, GLfloat* v ) {
  void (*func_glTexCoord4fVertex4fvSUN)( GLfloat*, GLfloat* ) = extension_funcs[EXT_glTexCoord4fVertex4fvSUN];
  TRACE("(%p, %p)\n", tc, v );
  ENTER_GL();
  func_glTexCoord4fVertex4fvSUN( tc, v );
  LEAVE_GL();
}

static void WINAPI wine_glTexCoord4hNV( unsigned short s, unsigned short t, unsigned short r, unsigned short q ) {
  void (*func_glTexCoord4hNV)( unsigned short, unsigned short, unsigned short, unsigned short ) = extension_funcs[EXT_glTexCoord4hNV];
  TRACE("(%d, %d, %d, %d)\n", s, t, r, q );
  ENTER_GL();
  func_glTexCoord4hNV( s, t, r, q );
  LEAVE_GL();
}

static void WINAPI wine_glTexCoord4hvNV( unsigned short* v ) {
  void (*func_glTexCoord4hvNV)( unsigned short* ) = extension_funcs[EXT_glTexCoord4hvNV];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glTexCoord4hvNV( v );
  LEAVE_GL();
}

static void WINAPI wine_glTexCoordFormatNV( GLint size, GLenum type, GLsizei stride ) {
  void (*func_glTexCoordFormatNV)( GLint, GLenum, GLsizei ) = extension_funcs[EXT_glTexCoordFormatNV];
  TRACE("(%d, %d, %d)\n", size, type, stride );
  ENTER_GL();
  func_glTexCoordFormatNV( size, type, stride );
  LEAVE_GL();
}

static void WINAPI wine_glTexCoordP1ui( GLenum type, GLuint coords ) {
  void (*func_glTexCoordP1ui)( GLenum, GLuint ) = extension_funcs[EXT_glTexCoordP1ui];
  TRACE("(%d, %d)\n", type, coords );
  ENTER_GL();
  func_glTexCoordP1ui( type, coords );
  LEAVE_GL();
}

static void WINAPI wine_glTexCoordP1uiv( GLenum type, GLuint* coords ) {
  void (*func_glTexCoordP1uiv)( GLenum, GLuint* ) = extension_funcs[EXT_glTexCoordP1uiv];
  TRACE("(%d, %p)\n", type, coords );
  ENTER_GL();
  func_glTexCoordP1uiv( type, coords );
  LEAVE_GL();
}

static void WINAPI wine_glTexCoordP2ui( GLenum type, GLuint coords ) {
  void (*func_glTexCoordP2ui)( GLenum, GLuint ) = extension_funcs[EXT_glTexCoordP2ui];
  TRACE("(%d, %d)\n", type, coords );
  ENTER_GL();
  func_glTexCoordP2ui( type, coords );
  LEAVE_GL();
}

static void WINAPI wine_glTexCoordP2uiv( GLenum type, GLuint* coords ) {
  void (*func_glTexCoordP2uiv)( GLenum, GLuint* ) = extension_funcs[EXT_glTexCoordP2uiv];
  TRACE("(%d, %p)\n", type, coords );
  ENTER_GL();
  func_glTexCoordP2uiv( type, coords );
  LEAVE_GL();
}

static void WINAPI wine_glTexCoordP3ui( GLenum type, GLuint coords ) {
  void (*func_glTexCoordP3ui)( GLenum, GLuint ) = extension_funcs[EXT_glTexCoordP3ui];
  TRACE("(%d, %d)\n", type, coords );
  ENTER_GL();
  func_glTexCoordP3ui( type, coords );
  LEAVE_GL();
}

static void WINAPI wine_glTexCoordP3uiv( GLenum type, GLuint* coords ) {
  void (*func_glTexCoordP3uiv)( GLenum, GLuint* ) = extension_funcs[EXT_glTexCoordP3uiv];
  TRACE("(%d, %p)\n", type, coords );
  ENTER_GL();
  func_glTexCoordP3uiv( type, coords );
  LEAVE_GL();
}

static void WINAPI wine_glTexCoordP4ui( GLenum type, GLuint coords ) {
  void (*func_glTexCoordP4ui)( GLenum, GLuint ) = extension_funcs[EXT_glTexCoordP4ui];
  TRACE("(%d, %d)\n", type, coords );
  ENTER_GL();
  func_glTexCoordP4ui( type, coords );
  LEAVE_GL();
}

static void WINAPI wine_glTexCoordP4uiv( GLenum type, GLuint* coords ) {
  void (*func_glTexCoordP4uiv)( GLenum, GLuint* ) = extension_funcs[EXT_glTexCoordP4uiv];
  TRACE("(%d, %p)\n", type, coords );
  ENTER_GL();
  func_glTexCoordP4uiv( type, coords );
  LEAVE_GL();
}

static void WINAPI wine_glTexCoordPointerEXT( GLint size, GLenum type, GLsizei stride, GLsizei count, GLvoid* pointer ) {
  void (*func_glTexCoordPointerEXT)( GLint, GLenum, GLsizei, GLsizei, GLvoid* ) = extension_funcs[EXT_glTexCoordPointerEXT];
  TRACE("(%d, %d, %d, %d, %p)\n", size, type, stride, count, pointer );
  ENTER_GL();
  func_glTexCoordPointerEXT( size, type, stride, count, pointer );
  LEAVE_GL();
}

static void WINAPI wine_glTexCoordPointerListIBM( GLint size, GLenum type, GLint stride, GLvoid** pointer, GLint ptrstride ) {
  void (*func_glTexCoordPointerListIBM)( GLint, GLenum, GLint, GLvoid**, GLint ) = extension_funcs[EXT_glTexCoordPointerListIBM];
  TRACE("(%d, %d, %d, %p, %d)\n", size, type, stride, pointer, ptrstride );
  ENTER_GL();
  func_glTexCoordPointerListIBM( size, type, stride, pointer, ptrstride );
  LEAVE_GL();
}

static void WINAPI wine_glTexCoordPointervINTEL( GLint size, GLenum type, GLvoid** pointer ) {
  void (*func_glTexCoordPointervINTEL)( GLint, GLenum, GLvoid** ) = extension_funcs[EXT_glTexCoordPointervINTEL];
  TRACE("(%d, %d, %p)\n", size, type, pointer );
  ENTER_GL();
  func_glTexCoordPointervINTEL( size, type, pointer );
  LEAVE_GL();
}

static void WINAPI wine_glTexFilterFuncSGIS( GLenum target, GLenum filter, GLsizei n, GLfloat* weights ) {
  void (*func_glTexFilterFuncSGIS)( GLenum, GLenum, GLsizei, GLfloat* ) = extension_funcs[EXT_glTexFilterFuncSGIS];
  TRACE("(%d, %d, %d, %p)\n", target, filter, n, weights );
  ENTER_GL();
  func_glTexFilterFuncSGIS( target, filter, n, weights );
  LEAVE_GL();
}

static void WINAPI wine_glTexImage2DMultisample( GLenum target, GLsizei samples, GLint internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations ) {
  void (*func_glTexImage2DMultisample)( GLenum, GLsizei, GLint, GLsizei, GLsizei, GLboolean ) = extension_funcs[EXT_glTexImage2DMultisample];
  TRACE("(%d, %d, %d, %d, %d, %d)\n", target, samples, internalformat, width, height, fixedsamplelocations );
  ENTER_GL();
  func_glTexImage2DMultisample( target, samples, internalformat, width, height, fixedsamplelocations );
  LEAVE_GL();
}

static void WINAPI wine_glTexImage3D( GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, GLvoid* pixels ) {
  void (*func_glTexImage3D)( GLenum, GLint, GLint, GLsizei, GLsizei, GLsizei, GLint, GLenum, GLenum, GLvoid* ) = extension_funcs[EXT_glTexImage3D];
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %d, %p)\n", target, level, internalformat, width, height, depth, border, format, type, pixels );
  ENTER_GL();
  func_glTexImage3D( target, level, internalformat, width, height, depth, border, format, type, pixels );
  LEAVE_GL();
}

static void WINAPI wine_glTexImage3DEXT( GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, GLvoid* pixels ) {
  void (*func_glTexImage3DEXT)( GLenum, GLint, GLenum, GLsizei, GLsizei, GLsizei, GLint, GLenum, GLenum, GLvoid* ) = extension_funcs[EXT_glTexImage3DEXT];
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %d, %p)\n", target, level, internalformat, width, height, depth, border, format, type, pixels );
  ENTER_GL();
  func_glTexImage3DEXT( target, level, internalformat, width, height, depth, border, format, type, pixels );
  LEAVE_GL();
}

static void WINAPI wine_glTexImage3DMultisample( GLenum target, GLsizei samples, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedsamplelocations ) {
  void (*func_glTexImage3DMultisample)( GLenum, GLsizei, GLint, GLsizei, GLsizei, GLsizei, GLboolean ) = extension_funcs[EXT_glTexImage3DMultisample];
  TRACE("(%d, %d, %d, %d, %d, %d, %d)\n", target, samples, internalformat, width, height, depth, fixedsamplelocations );
  ENTER_GL();
  func_glTexImage3DMultisample( target, samples, internalformat, width, height, depth, fixedsamplelocations );
  LEAVE_GL();
}

static void WINAPI wine_glTexImage4DSGIS( GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLsizei size4d, GLint border, GLenum format, GLenum type, GLvoid* pixels ) {
  void (*func_glTexImage4DSGIS)( GLenum, GLint, GLenum, GLsizei, GLsizei, GLsizei, GLsizei, GLint, GLenum, GLenum, GLvoid* ) = extension_funcs[EXT_glTexImage4DSGIS];
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %p)\n", target, level, internalformat, width, height, depth, size4d, border, format, type, pixels );
  ENTER_GL();
  func_glTexImage4DSGIS( target, level, internalformat, width, height, depth, size4d, border, format, type, pixels );
  LEAVE_GL();
}

static void WINAPI wine_glTexParameterIiv( GLenum target, GLenum pname, GLint* params ) {
  void (*func_glTexParameterIiv)( GLenum, GLenum, GLint* ) = extension_funcs[EXT_glTexParameterIiv];
  TRACE("(%d, %d, %p)\n", target, pname, params );
  ENTER_GL();
  func_glTexParameterIiv( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glTexParameterIivEXT( GLenum target, GLenum pname, GLint* params ) {
  void (*func_glTexParameterIivEXT)( GLenum, GLenum, GLint* ) = extension_funcs[EXT_glTexParameterIivEXT];
  TRACE("(%d, %d, %p)\n", target, pname, params );
  ENTER_GL();
  func_glTexParameterIivEXT( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glTexParameterIuiv( GLenum target, GLenum pname, GLuint* params ) {
  void (*func_glTexParameterIuiv)( GLenum, GLenum, GLuint* ) = extension_funcs[EXT_glTexParameterIuiv];
  TRACE("(%d, %d, %p)\n", target, pname, params );
  ENTER_GL();
  func_glTexParameterIuiv( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glTexParameterIuivEXT( GLenum target, GLenum pname, GLuint* params ) {
  void (*func_glTexParameterIuivEXT)( GLenum, GLenum, GLuint* ) = extension_funcs[EXT_glTexParameterIuivEXT];
  TRACE("(%d, %d, %p)\n", target, pname, params );
  ENTER_GL();
  func_glTexParameterIuivEXT( target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glTexRenderbufferNV( GLenum target, GLuint renderbuffer ) {
  void (*func_glTexRenderbufferNV)( GLenum, GLuint ) = extension_funcs[EXT_glTexRenderbufferNV];
  TRACE("(%d, %d)\n", target, renderbuffer );
  ENTER_GL();
  func_glTexRenderbufferNV( target, renderbuffer );
  LEAVE_GL();
}

static void WINAPI wine_glTexSubImage1DEXT( GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, GLvoid* pixels ) {
  void (*func_glTexSubImage1DEXT)( GLenum, GLint, GLint, GLsizei, GLenum, GLenum, GLvoid* ) = extension_funcs[EXT_glTexSubImage1DEXT];
  TRACE("(%d, %d, %d, %d, %d, %d, %p)\n", target, level, xoffset, width, format, type, pixels );
  ENTER_GL();
  func_glTexSubImage1DEXT( target, level, xoffset, width, format, type, pixels );
  LEAVE_GL();
}

static void WINAPI wine_glTexSubImage2DEXT( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid* pixels ) {
  void (*func_glTexSubImage2DEXT)( GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, GLvoid* ) = extension_funcs[EXT_glTexSubImage2DEXT];
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %p)\n", target, level, xoffset, yoffset, width, height, format, type, pixels );
  ENTER_GL();
  func_glTexSubImage2DEXT( target, level, xoffset, yoffset, width, height, format, type, pixels );
  LEAVE_GL();
}

static void WINAPI wine_glTexSubImage3D( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, GLvoid* pixels ) {
  void (*func_glTexSubImage3D)( GLenum, GLint, GLint, GLint, GLint, GLsizei, GLsizei, GLsizei, GLenum, GLenum, GLvoid* ) = extension_funcs[EXT_glTexSubImage3D];
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %p)\n", target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels );
  ENTER_GL();
  func_glTexSubImage3D( target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels );
  LEAVE_GL();
}

static void WINAPI wine_glTexSubImage3DEXT( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, GLvoid* pixels ) {
  void (*func_glTexSubImage3DEXT)( GLenum, GLint, GLint, GLint, GLint, GLsizei, GLsizei, GLsizei, GLenum, GLenum, GLvoid* ) = extension_funcs[EXT_glTexSubImage3DEXT];
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %p)\n", target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels );
  ENTER_GL();
  func_glTexSubImage3DEXT( target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels );
  LEAVE_GL();
}

static void WINAPI wine_glTexSubImage4DSGIS( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint woffset, GLsizei width, GLsizei height, GLsizei depth, GLsizei size4d, GLenum format, GLenum type, GLvoid* pixels ) {
  void (*func_glTexSubImage4DSGIS)( GLenum, GLint, GLint, GLint, GLint, GLint, GLsizei, GLsizei, GLsizei, GLsizei, GLenum, GLenum, GLvoid* ) = extension_funcs[EXT_glTexSubImage4DSGIS];
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %p)\n", target, level, xoffset, yoffset, zoffset, woffset, width, height, depth, size4d, format, type, pixels );
  ENTER_GL();
  func_glTexSubImage4DSGIS( target, level, xoffset, yoffset, zoffset, woffset, width, height, depth, size4d, format, type, pixels );
  LEAVE_GL();
}

static void WINAPI wine_glTextureBarrierNV( void ) {
  void (*func_glTextureBarrierNV)( void ) = extension_funcs[EXT_glTextureBarrierNV];
  TRACE("()\n");
  ENTER_GL();
  func_glTextureBarrierNV( );
  LEAVE_GL();
}

static void WINAPI wine_glTextureBufferEXT( GLuint texture, GLenum target, GLenum internalformat, GLuint buffer ) {
  void (*func_glTextureBufferEXT)( GLuint, GLenum, GLenum, GLuint ) = extension_funcs[EXT_glTextureBufferEXT];
  TRACE("(%d, %d, %d, %d)\n", texture, target, internalformat, buffer );
  ENTER_GL();
  func_glTextureBufferEXT( texture, target, internalformat, buffer );
  LEAVE_GL();
}

static void WINAPI wine_glTextureColorMaskSGIS( GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha ) {
  void (*func_glTextureColorMaskSGIS)( GLboolean, GLboolean, GLboolean, GLboolean ) = extension_funcs[EXT_glTextureColorMaskSGIS];
  TRACE("(%d, %d, %d, %d)\n", red, green, blue, alpha );
  ENTER_GL();
  func_glTextureColorMaskSGIS( red, green, blue, alpha );
  LEAVE_GL();
}

static void WINAPI wine_glTextureImage1DEXT( GLuint texture, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLenum format, GLenum type, GLvoid* pixels ) {
  void (*func_glTextureImage1DEXT)( GLuint, GLenum, GLint, GLenum, GLsizei, GLint, GLenum, GLenum, GLvoid* ) = extension_funcs[EXT_glTextureImage1DEXT];
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %p)\n", texture, target, level, internalformat, width, border, format, type, pixels );
  ENTER_GL();
  func_glTextureImage1DEXT( texture, target, level, internalformat, width, border, format, type, pixels );
  LEAVE_GL();
}

static void WINAPI wine_glTextureImage2DEXT( GLuint texture, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, GLvoid* pixels ) {
  void (*func_glTextureImage2DEXT)( GLuint, GLenum, GLint, GLenum, GLsizei, GLsizei, GLint, GLenum, GLenum, GLvoid* ) = extension_funcs[EXT_glTextureImage2DEXT];
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %d, %p)\n", texture, target, level, internalformat, width, height, border, format, type, pixels );
  ENTER_GL();
  func_glTextureImage2DEXT( texture, target, level, internalformat, width, height, border, format, type, pixels );
  LEAVE_GL();
}

static void WINAPI wine_glTextureImage3DEXT( GLuint texture, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, GLvoid* pixels ) {
  void (*func_glTextureImage3DEXT)( GLuint, GLenum, GLint, GLenum, GLsizei, GLsizei, GLsizei, GLint, GLenum, GLenum, GLvoid* ) = extension_funcs[EXT_glTextureImage3DEXT];
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %p)\n", texture, target, level, internalformat, width, height, depth, border, format, type, pixels );
  ENTER_GL();
  func_glTextureImage3DEXT( texture, target, level, internalformat, width, height, depth, border, format, type, pixels );
  LEAVE_GL();
}

static void WINAPI wine_glTextureLightEXT( GLenum pname ) {
  void (*func_glTextureLightEXT)( GLenum ) = extension_funcs[EXT_glTextureLightEXT];
  TRACE("(%d)\n", pname );
  ENTER_GL();
  func_glTextureLightEXT( pname );
  LEAVE_GL();
}

static void WINAPI wine_glTextureMaterialEXT( GLenum face, GLenum mode ) {
  void (*func_glTextureMaterialEXT)( GLenum, GLenum ) = extension_funcs[EXT_glTextureMaterialEXT];
  TRACE("(%d, %d)\n", face, mode );
  ENTER_GL();
  func_glTextureMaterialEXT( face, mode );
  LEAVE_GL();
}

static void WINAPI wine_glTextureNormalEXT( GLenum mode ) {
  void (*func_glTextureNormalEXT)( GLenum ) = extension_funcs[EXT_glTextureNormalEXT];
  TRACE("(%d)\n", mode );
  ENTER_GL();
  func_glTextureNormalEXT( mode );
  LEAVE_GL();
}

static void WINAPI wine_glTextureParameterIivEXT( GLuint texture, GLenum target, GLenum pname, GLint* params ) {
  void (*func_glTextureParameterIivEXT)( GLuint, GLenum, GLenum, GLint* ) = extension_funcs[EXT_glTextureParameterIivEXT];
  TRACE("(%d, %d, %d, %p)\n", texture, target, pname, params );
  ENTER_GL();
  func_glTextureParameterIivEXT( texture, target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glTextureParameterIuivEXT( GLuint texture, GLenum target, GLenum pname, GLuint* params ) {
  void (*func_glTextureParameterIuivEXT)( GLuint, GLenum, GLenum, GLuint* ) = extension_funcs[EXT_glTextureParameterIuivEXT];
  TRACE("(%d, %d, %d, %p)\n", texture, target, pname, params );
  ENTER_GL();
  func_glTextureParameterIuivEXT( texture, target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glTextureParameterfEXT( GLuint texture, GLenum target, GLenum pname, GLfloat param ) {
  void (*func_glTextureParameterfEXT)( GLuint, GLenum, GLenum, GLfloat ) = extension_funcs[EXT_glTextureParameterfEXT];
  TRACE("(%d, %d, %d, %f)\n", texture, target, pname, param );
  ENTER_GL();
  func_glTextureParameterfEXT( texture, target, pname, param );
  LEAVE_GL();
}

static void WINAPI wine_glTextureParameterfvEXT( GLuint texture, GLenum target, GLenum pname, GLfloat* params ) {
  void (*func_glTextureParameterfvEXT)( GLuint, GLenum, GLenum, GLfloat* ) = extension_funcs[EXT_glTextureParameterfvEXT];
  TRACE("(%d, %d, %d, %p)\n", texture, target, pname, params );
  ENTER_GL();
  func_glTextureParameterfvEXT( texture, target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glTextureParameteriEXT( GLuint texture, GLenum target, GLenum pname, GLint param ) {
  void (*func_glTextureParameteriEXT)( GLuint, GLenum, GLenum, GLint ) = extension_funcs[EXT_glTextureParameteriEXT];
  TRACE("(%d, %d, %d, %d)\n", texture, target, pname, param );
  ENTER_GL();
  func_glTextureParameteriEXT( texture, target, pname, param );
  LEAVE_GL();
}

static void WINAPI wine_glTextureParameterivEXT( GLuint texture, GLenum target, GLenum pname, GLint* params ) {
  void (*func_glTextureParameterivEXT)( GLuint, GLenum, GLenum, GLint* ) = extension_funcs[EXT_glTextureParameterivEXT];
  TRACE("(%d, %d, %d, %p)\n", texture, target, pname, params );
  ENTER_GL();
  func_glTextureParameterivEXT( texture, target, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glTextureRangeAPPLE( GLenum target, GLsizei length, GLvoid* pointer ) {
  void (*func_glTextureRangeAPPLE)( GLenum, GLsizei, GLvoid* ) = extension_funcs[EXT_glTextureRangeAPPLE];
  TRACE("(%d, %d, %p)\n", target, length, pointer );
  ENTER_GL();
  func_glTextureRangeAPPLE( target, length, pointer );
  LEAVE_GL();
}

static void WINAPI wine_glTextureRenderbufferEXT( GLuint texture, GLenum target, GLuint renderbuffer ) {
  void (*func_glTextureRenderbufferEXT)( GLuint, GLenum, GLuint ) = extension_funcs[EXT_glTextureRenderbufferEXT];
  TRACE("(%d, %d, %d)\n", texture, target, renderbuffer );
  ENTER_GL();
  func_glTextureRenderbufferEXT( texture, target, renderbuffer );
  LEAVE_GL();
}

static void WINAPI wine_glTextureSubImage1DEXT( GLuint texture, GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, GLvoid* pixels ) {
  void (*func_glTextureSubImage1DEXT)( GLuint, GLenum, GLint, GLint, GLsizei, GLenum, GLenum, GLvoid* ) = extension_funcs[EXT_glTextureSubImage1DEXT];
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %p)\n", texture, target, level, xoffset, width, format, type, pixels );
  ENTER_GL();
  func_glTextureSubImage1DEXT( texture, target, level, xoffset, width, format, type, pixels );
  LEAVE_GL();
}

static void WINAPI wine_glTextureSubImage2DEXT( GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid* pixels ) {
  void (*func_glTextureSubImage2DEXT)( GLuint, GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, GLvoid* ) = extension_funcs[EXT_glTextureSubImage2DEXT];
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %d, %p)\n", texture, target, level, xoffset, yoffset, width, height, format, type, pixels );
  ENTER_GL();
  func_glTextureSubImage2DEXT( texture, target, level, xoffset, yoffset, width, height, format, type, pixels );
  LEAVE_GL();
}

static void WINAPI wine_glTextureSubImage3DEXT( GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, GLvoid* pixels ) {
  void (*func_glTextureSubImage3DEXT)( GLuint, GLenum, GLint, GLint, GLint, GLint, GLsizei, GLsizei, GLsizei, GLenum, GLenum, GLvoid* ) = extension_funcs[EXT_glTextureSubImage3DEXT];
  TRACE("(%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %p)\n", texture, target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels );
  ENTER_GL();
  func_glTextureSubImage3DEXT( texture, target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels );
  LEAVE_GL();
}

static void WINAPI wine_glTrackMatrixNV( GLenum target, GLuint address, GLenum matrix, GLenum transform ) {
  void (*func_glTrackMatrixNV)( GLenum, GLuint, GLenum, GLenum ) = extension_funcs[EXT_glTrackMatrixNV];
  TRACE("(%d, %d, %d, %d)\n", target, address, matrix, transform );
  ENTER_GL();
  func_glTrackMatrixNV( target, address, matrix, transform );
  LEAVE_GL();
}

static void WINAPI wine_glTransformFeedbackAttribsNV( GLuint count, GLint* attribs, GLenum bufferMode ) {
  void (*func_glTransformFeedbackAttribsNV)( GLuint, GLint*, GLenum ) = extension_funcs[EXT_glTransformFeedbackAttribsNV];
  TRACE("(%d, %p, %d)\n", count, attribs, bufferMode );
  ENTER_GL();
  func_glTransformFeedbackAttribsNV( count, attribs, bufferMode );
  LEAVE_GL();
}

static void WINAPI wine_glTransformFeedbackStreamAttribsNV( GLsizei count, GLint* attribs, GLsizei nbuffers, GLint* bufstreams, GLenum bufferMode ) {
  void (*func_glTransformFeedbackStreamAttribsNV)( GLsizei, GLint*, GLsizei, GLint*, GLenum ) = extension_funcs[EXT_glTransformFeedbackStreamAttribsNV];
  TRACE("(%d, %p, %d, %p, %d)\n", count, attribs, nbuffers, bufstreams, bufferMode );
  ENTER_GL();
  func_glTransformFeedbackStreamAttribsNV( count, attribs, nbuffers, bufstreams, bufferMode );
  LEAVE_GL();
}

static void WINAPI wine_glTransformFeedbackVaryings( GLuint program, GLsizei count, char** varyings, GLenum bufferMode ) {
  void (*func_glTransformFeedbackVaryings)( GLuint, GLsizei, char**, GLenum ) = extension_funcs[EXT_glTransformFeedbackVaryings];
  TRACE("(%d, %d, %p, %d)\n", program, count, varyings, bufferMode );
  ENTER_GL();
  func_glTransformFeedbackVaryings( program, count, varyings, bufferMode );
  LEAVE_GL();
}

static void WINAPI wine_glTransformFeedbackVaryingsEXT( GLuint program, GLsizei count, char** varyings, GLenum bufferMode ) {
  void (*func_glTransformFeedbackVaryingsEXT)( GLuint, GLsizei, char**, GLenum ) = extension_funcs[EXT_glTransformFeedbackVaryingsEXT];
  TRACE("(%d, %d, %p, %d)\n", program, count, varyings, bufferMode );
  ENTER_GL();
  func_glTransformFeedbackVaryingsEXT( program, count, varyings, bufferMode );
  LEAVE_GL();
}

static void WINAPI wine_glTransformFeedbackVaryingsNV( GLuint program, GLsizei count, GLint* locations, GLenum bufferMode ) {
  void (*func_glTransformFeedbackVaryingsNV)( GLuint, GLsizei, GLint*, GLenum ) = extension_funcs[EXT_glTransformFeedbackVaryingsNV];
  TRACE("(%d, %d, %p, %d)\n", program, count, locations, bufferMode );
  ENTER_GL();
  func_glTransformFeedbackVaryingsNV( program, count, locations, bufferMode );
  LEAVE_GL();
}

static void WINAPI wine_glUniform1d( GLint location, GLdouble x ) {
  void (*func_glUniform1d)( GLint, GLdouble ) = extension_funcs[EXT_glUniform1d];
  TRACE("(%d, %f)\n", location, x );
  ENTER_GL();
  func_glUniform1d( location, x );
  LEAVE_GL();
}

static void WINAPI wine_glUniform1dv( GLint location, GLsizei count, GLdouble* value ) {
  void (*func_glUniform1dv)( GLint, GLsizei, GLdouble* ) = extension_funcs[EXT_glUniform1dv];
  TRACE("(%d, %d, %p)\n", location, count, value );
  ENTER_GL();
  func_glUniform1dv( location, count, value );
  LEAVE_GL();
}

static void WINAPI wine_glUniform1f( GLint location, GLfloat v0 ) {
  void (*func_glUniform1f)( GLint, GLfloat ) = extension_funcs[EXT_glUniform1f];
  TRACE("(%d, %f)\n", location, v0 );
  ENTER_GL();
  func_glUniform1f( location, v0 );
  LEAVE_GL();
}

static void WINAPI wine_glUniform1fARB( GLint location, GLfloat v0 ) {
  void (*func_glUniform1fARB)( GLint, GLfloat ) = extension_funcs[EXT_glUniform1fARB];
  TRACE("(%d, %f)\n", location, v0 );
  ENTER_GL();
  func_glUniform1fARB( location, v0 );
  LEAVE_GL();
}

static void WINAPI wine_glUniform1fv( GLint location, GLsizei count, GLfloat* value ) {
  void (*func_glUniform1fv)( GLint, GLsizei, GLfloat* ) = extension_funcs[EXT_glUniform1fv];
  TRACE("(%d, %d, %p)\n", location, count, value );
  ENTER_GL();
  func_glUniform1fv( location, count, value );
  LEAVE_GL();
}

static void WINAPI wine_glUniform1fvARB( GLint location, GLsizei count, GLfloat* value ) {
  void (*func_glUniform1fvARB)( GLint, GLsizei, GLfloat* ) = extension_funcs[EXT_glUniform1fvARB];
  TRACE("(%d, %d, %p)\n", location, count, value );
  ENTER_GL();
  func_glUniform1fvARB( location, count, value );
  LEAVE_GL();
}

static void WINAPI wine_glUniform1i( GLint location, GLint v0 ) {
  void (*func_glUniform1i)( GLint, GLint ) = extension_funcs[EXT_glUniform1i];
  TRACE("(%d, %d)\n", location, v0 );
  ENTER_GL();
  func_glUniform1i( location, v0 );
  LEAVE_GL();
}

static void WINAPI wine_glUniform1i64NV( GLint location, INT64 x ) {
  void (*func_glUniform1i64NV)( GLint, INT64 ) = extension_funcs[EXT_glUniform1i64NV];
  TRACE("(%d, %s)\n", location, wine_dbgstr_longlong(x) );
  ENTER_GL();
  func_glUniform1i64NV( location, x );
  LEAVE_GL();
}

static void WINAPI wine_glUniform1i64vNV( GLint location, GLsizei count, INT64* value ) {
  void (*func_glUniform1i64vNV)( GLint, GLsizei, INT64* ) = extension_funcs[EXT_glUniform1i64vNV];
  TRACE("(%d, %d, %p)\n", location, count, value );
  ENTER_GL();
  func_glUniform1i64vNV( location, count, value );
  LEAVE_GL();
}

static void WINAPI wine_glUniform1iARB( GLint location, GLint v0 ) {
  void (*func_glUniform1iARB)( GLint, GLint ) = extension_funcs[EXT_glUniform1iARB];
  TRACE("(%d, %d)\n", location, v0 );
  ENTER_GL();
  func_glUniform1iARB( location, v0 );
  LEAVE_GL();
}

static void WINAPI wine_glUniform1iv( GLint location, GLsizei count, GLint* value ) {
  void (*func_glUniform1iv)( GLint, GLsizei, GLint* ) = extension_funcs[EXT_glUniform1iv];
  TRACE("(%d, %d, %p)\n", location, count, value );
  ENTER_GL();
  func_glUniform1iv( location, count, value );
  LEAVE_GL();
}

static void WINAPI wine_glUniform1ivARB( GLint location, GLsizei count, GLint* value ) {
  void (*func_glUniform1ivARB)( GLint, GLsizei, GLint* ) = extension_funcs[EXT_glUniform1ivARB];
  TRACE("(%d, %d, %p)\n", location, count, value );
  ENTER_GL();
  func_glUniform1ivARB( location, count, value );
  LEAVE_GL();
}

static void WINAPI wine_glUniform1ui( GLint location, GLuint v0 ) {
  void (*func_glUniform1ui)( GLint, GLuint ) = extension_funcs[EXT_glUniform1ui];
  TRACE("(%d, %d)\n", location, v0 );
  ENTER_GL();
  func_glUniform1ui( location, v0 );
  LEAVE_GL();
}

static void WINAPI wine_glUniform1ui64NV( GLint location, UINT64 x ) {
  void (*func_glUniform1ui64NV)( GLint, UINT64 ) = extension_funcs[EXT_glUniform1ui64NV];
  TRACE("(%d, %s)\n", location, wine_dbgstr_longlong(x) );
  ENTER_GL();
  func_glUniform1ui64NV( location, x );
  LEAVE_GL();
}

static void WINAPI wine_glUniform1ui64vNV( GLint location, GLsizei count, UINT64* value ) {
  void (*func_glUniform1ui64vNV)( GLint, GLsizei, UINT64* ) = extension_funcs[EXT_glUniform1ui64vNV];
  TRACE("(%d, %d, %p)\n", location, count, value );
  ENTER_GL();
  func_glUniform1ui64vNV( location, count, value );
  LEAVE_GL();
}

static void WINAPI wine_glUniform1uiEXT( GLint location, GLuint v0 ) {
  void (*func_glUniform1uiEXT)( GLint, GLuint ) = extension_funcs[EXT_glUniform1uiEXT];
  TRACE("(%d, %d)\n", location, v0 );
  ENTER_GL();
  func_glUniform1uiEXT( location, v0 );
  LEAVE_GL();
}

static void WINAPI wine_glUniform1uiv( GLint location, GLsizei count, GLuint* value ) {
  void (*func_glUniform1uiv)( GLint, GLsizei, GLuint* ) = extension_funcs[EXT_glUniform1uiv];
  TRACE("(%d, %d, %p)\n", location, count, value );
  ENTER_GL();
  func_glUniform1uiv( location, count, value );
  LEAVE_GL();
}

static void WINAPI wine_glUniform1uivEXT( GLint location, GLsizei count, GLuint* value ) {
  void (*func_glUniform1uivEXT)( GLint, GLsizei, GLuint* ) = extension_funcs[EXT_glUniform1uivEXT];
  TRACE("(%d, %d, %p)\n", location, count, value );
  ENTER_GL();
  func_glUniform1uivEXT( location, count, value );
  LEAVE_GL();
}

static void WINAPI wine_glUniform2d( GLint location, GLdouble x, GLdouble y ) {
  void (*func_glUniform2d)( GLint, GLdouble, GLdouble ) = extension_funcs[EXT_glUniform2d];
  TRACE("(%d, %f, %f)\n", location, x, y );
  ENTER_GL();
  func_glUniform2d( location, x, y );
  LEAVE_GL();
}

static void WINAPI wine_glUniform2dv( GLint location, GLsizei count, GLdouble* value ) {
  void (*func_glUniform2dv)( GLint, GLsizei, GLdouble* ) = extension_funcs[EXT_glUniform2dv];
  TRACE("(%d, %d, %p)\n", location, count, value );
  ENTER_GL();
  func_glUniform2dv( location, count, value );
  LEAVE_GL();
}

static void WINAPI wine_glUniform2f( GLint location, GLfloat v0, GLfloat v1 ) {
  void (*func_glUniform2f)( GLint, GLfloat, GLfloat ) = extension_funcs[EXT_glUniform2f];
  TRACE("(%d, %f, %f)\n", location, v0, v1 );
  ENTER_GL();
  func_glUniform2f( location, v0, v1 );
  LEAVE_GL();
}

static void WINAPI wine_glUniform2fARB( GLint location, GLfloat v0, GLfloat v1 ) {
  void (*func_glUniform2fARB)( GLint, GLfloat, GLfloat ) = extension_funcs[EXT_glUniform2fARB];
  TRACE("(%d, %f, %f)\n", location, v0, v1 );
  ENTER_GL();
  func_glUniform2fARB( location, v0, v1 );
  LEAVE_GL();
}

static void WINAPI wine_glUniform2fv( GLint location, GLsizei count, GLfloat* value ) {
  void (*func_glUniform2fv)( GLint, GLsizei, GLfloat* ) = extension_funcs[EXT_glUniform2fv];
  TRACE("(%d, %d, %p)\n", location, count, value );
  ENTER_GL();
  func_glUniform2fv( location, count, value );
  LEAVE_GL();
}

static void WINAPI wine_glUniform2fvARB( GLint location, GLsizei count, GLfloat* value ) {
  void (*func_glUniform2fvARB)( GLint, GLsizei, GLfloat* ) = extension_funcs[EXT_glUniform2fvARB];
  TRACE("(%d, %d, %p)\n", location, count, value );
  ENTER_GL();
  func_glUniform2fvARB( location, count, value );
  LEAVE_GL();
}

static void WINAPI wine_glUniform2i( GLint location, GLint v0, GLint v1 ) {
  void (*func_glUniform2i)( GLint, GLint, GLint ) = extension_funcs[EXT_glUniform2i];
  TRACE("(%d, %d, %d)\n", location, v0, v1 );
  ENTER_GL();
  func_glUniform2i( location, v0, v1 );
  LEAVE_GL();
}

static void WINAPI wine_glUniform2i64NV( GLint location, INT64 x, INT64 y ) {
  void (*func_glUniform2i64NV)( GLint, INT64, INT64 ) = extension_funcs[EXT_glUniform2i64NV];
  TRACE("(%d, %s, %s)\n", location, wine_dbgstr_longlong(x), wine_dbgstr_longlong(y) );
  ENTER_GL();
  func_glUniform2i64NV( location, x, y );
  LEAVE_GL();
}

static void WINAPI wine_glUniform2i64vNV( GLint location, GLsizei count, INT64* value ) {
  void (*func_glUniform2i64vNV)( GLint, GLsizei, INT64* ) = extension_funcs[EXT_glUniform2i64vNV];
  TRACE("(%d, %d, %p)\n", location, count, value );
  ENTER_GL();
  func_glUniform2i64vNV( location, count, value );
  LEAVE_GL();
}

static void WINAPI wine_glUniform2iARB( GLint location, GLint v0, GLint v1 ) {
  void (*func_glUniform2iARB)( GLint, GLint, GLint ) = extension_funcs[EXT_glUniform2iARB];
  TRACE("(%d, %d, %d)\n", location, v0, v1 );
  ENTER_GL();
  func_glUniform2iARB( location, v0, v1 );
  LEAVE_GL();
}

static void WINAPI wine_glUniform2iv( GLint location, GLsizei count, GLint* value ) {
  void (*func_glUniform2iv)( GLint, GLsizei, GLint* ) = extension_funcs[EXT_glUniform2iv];
  TRACE("(%d, %d, %p)\n", location, count, value );
  ENTER_GL();
  func_glUniform2iv( location, count, value );
  LEAVE_GL();
}

static void WINAPI wine_glUniform2ivARB( GLint location, GLsizei count, GLint* value ) {
  void (*func_glUniform2ivARB)( GLint, GLsizei, GLint* ) = extension_funcs[EXT_glUniform2ivARB];
  TRACE("(%d, %d, %p)\n", location, count, value );
  ENTER_GL();
  func_glUniform2ivARB( location, count, value );
  LEAVE_GL();
}

static void WINAPI wine_glUniform2ui( GLint location, GLuint v0, GLuint v1 ) {
  void (*func_glUniform2ui)( GLint, GLuint, GLuint ) = extension_funcs[EXT_glUniform2ui];
  TRACE("(%d, %d, %d)\n", location, v0, v1 );
  ENTER_GL();
  func_glUniform2ui( location, v0, v1 );
  LEAVE_GL();
}

static void WINAPI wine_glUniform2ui64NV( GLint location, UINT64 x, UINT64 y ) {
  void (*func_glUniform2ui64NV)( GLint, UINT64, UINT64 ) = extension_funcs[EXT_glUniform2ui64NV];
  TRACE("(%d, %s, %s)\n", location, wine_dbgstr_longlong(x), wine_dbgstr_longlong(y) );
  ENTER_GL();
  func_glUniform2ui64NV( location, x, y );
  LEAVE_GL();
}

static void WINAPI wine_glUniform2ui64vNV( GLint location, GLsizei count, UINT64* value ) {
  void (*func_glUniform2ui64vNV)( GLint, GLsizei, UINT64* ) = extension_funcs[EXT_glUniform2ui64vNV];
  TRACE("(%d, %d, %p)\n", location, count, value );
  ENTER_GL();
  func_glUniform2ui64vNV( location, count, value );
  LEAVE_GL();
}

static void WINAPI wine_glUniform2uiEXT( GLint location, GLuint v0, GLuint v1 ) {
  void (*func_glUniform2uiEXT)( GLint, GLuint, GLuint ) = extension_funcs[EXT_glUniform2uiEXT];
  TRACE("(%d, %d, %d)\n", location, v0, v1 );
  ENTER_GL();
  func_glUniform2uiEXT( location, v0, v1 );
  LEAVE_GL();
}

static void WINAPI wine_glUniform2uiv( GLint location, GLsizei count, GLuint* value ) {
  void (*func_glUniform2uiv)( GLint, GLsizei, GLuint* ) = extension_funcs[EXT_glUniform2uiv];
  TRACE("(%d, %d, %p)\n", location, count, value );
  ENTER_GL();
  func_glUniform2uiv( location, count, value );
  LEAVE_GL();
}

static void WINAPI wine_glUniform2uivEXT( GLint location, GLsizei count, GLuint* value ) {
  void (*func_glUniform2uivEXT)( GLint, GLsizei, GLuint* ) = extension_funcs[EXT_glUniform2uivEXT];
  TRACE("(%d, %d, %p)\n", location, count, value );
  ENTER_GL();
  func_glUniform2uivEXT( location, count, value );
  LEAVE_GL();
}

static void WINAPI wine_glUniform3d( GLint location, GLdouble x, GLdouble y, GLdouble z ) {
  void (*func_glUniform3d)( GLint, GLdouble, GLdouble, GLdouble ) = extension_funcs[EXT_glUniform3d];
  TRACE("(%d, %f, %f, %f)\n", location, x, y, z );
  ENTER_GL();
  func_glUniform3d( location, x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glUniform3dv( GLint location, GLsizei count, GLdouble* value ) {
  void (*func_glUniform3dv)( GLint, GLsizei, GLdouble* ) = extension_funcs[EXT_glUniform3dv];
  TRACE("(%d, %d, %p)\n", location, count, value );
  ENTER_GL();
  func_glUniform3dv( location, count, value );
  LEAVE_GL();
}

static void WINAPI wine_glUniform3f( GLint location, GLfloat v0, GLfloat v1, GLfloat v2 ) {
  void (*func_glUniform3f)( GLint, GLfloat, GLfloat, GLfloat ) = extension_funcs[EXT_glUniform3f];
  TRACE("(%d, %f, %f, %f)\n", location, v0, v1, v2 );
  ENTER_GL();
  func_glUniform3f( location, v0, v1, v2 );
  LEAVE_GL();
}

static void WINAPI wine_glUniform3fARB( GLint location, GLfloat v0, GLfloat v1, GLfloat v2 ) {
  void (*func_glUniform3fARB)( GLint, GLfloat, GLfloat, GLfloat ) = extension_funcs[EXT_glUniform3fARB];
  TRACE("(%d, %f, %f, %f)\n", location, v0, v1, v2 );
  ENTER_GL();
  func_glUniform3fARB( location, v0, v1, v2 );
  LEAVE_GL();
}

static void WINAPI wine_glUniform3fv( GLint location, GLsizei count, GLfloat* value ) {
  void (*func_glUniform3fv)( GLint, GLsizei, GLfloat* ) = extension_funcs[EXT_glUniform3fv];
  TRACE("(%d, %d, %p)\n", location, count, value );
  ENTER_GL();
  func_glUniform3fv( location, count, value );
  LEAVE_GL();
}

static void WINAPI wine_glUniform3fvARB( GLint location, GLsizei count, GLfloat* value ) {
  void (*func_glUniform3fvARB)( GLint, GLsizei, GLfloat* ) = extension_funcs[EXT_glUniform3fvARB];
  TRACE("(%d, %d, %p)\n", location, count, value );
  ENTER_GL();
  func_glUniform3fvARB( location, count, value );
  LEAVE_GL();
}

static void WINAPI wine_glUniform3i( GLint location, GLint v0, GLint v1, GLint v2 ) {
  void (*func_glUniform3i)( GLint, GLint, GLint, GLint ) = extension_funcs[EXT_glUniform3i];
  TRACE("(%d, %d, %d, %d)\n", location, v0, v1, v2 );
  ENTER_GL();
  func_glUniform3i( location, v0, v1, v2 );
  LEAVE_GL();
}

static void WINAPI wine_glUniform3i64NV( GLint location, INT64 x, INT64 y, INT64 z ) {
  void (*func_glUniform3i64NV)( GLint, INT64, INT64, INT64 ) = extension_funcs[EXT_glUniform3i64NV];
  TRACE("(%d, %s, %s, %s)\n", location, wine_dbgstr_longlong(x), wine_dbgstr_longlong(y), wine_dbgstr_longlong(z) );
  ENTER_GL();
  func_glUniform3i64NV( location, x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glUniform3i64vNV( GLint location, GLsizei count, INT64* value ) {
  void (*func_glUniform3i64vNV)( GLint, GLsizei, INT64* ) = extension_funcs[EXT_glUniform3i64vNV];
  TRACE("(%d, %d, %p)\n", location, count, value );
  ENTER_GL();
  func_glUniform3i64vNV( location, count, value );
  LEAVE_GL();
}

static void WINAPI wine_glUniform3iARB( GLint location, GLint v0, GLint v1, GLint v2 ) {
  void (*func_glUniform3iARB)( GLint, GLint, GLint, GLint ) = extension_funcs[EXT_glUniform3iARB];
  TRACE("(%d, %d, %d, %d)\n", location, v0, v1, v2 );
  ENTER_GL();
  func_glUniform3iARB( location, v0, v1, v2 );
  LEAVE_GL();
}

static void WINAPI wine_glUniform3iv( GLint location, GLsizei count, GLint* value ) {
  void (*func_glUniform3iv)( GLint, GLsizei, GLint* ) = extension_funcs[EXT_glUniform3iv];
  TRACE("(%d, %d, %p)\n", location, count, value );
  ENTER_GL();
  func_glUniform3iv( location, count, value );
  LEAVE_GL();
}

static void WINAPI wine_glUniform3ivARB( GLint location, GLsizei count, GLint* value ) {
  void (*func_glUniform3ivARB)( GLint, GLsizei, GLint* ) = extension_funcs[EXT_glUniform3ivARB];
  TRACE("(%d, %d, %p)\n", location, count, value );
  ENTER_GL();
  func_glUniform3ivARB( location, count, value );
  LEAVE_GL();
}

static void WINAPI wine_glUniform3ui( GLint location, GLuint v0, GLuint v1, GLuint v2 ) {
  void (*func_glUniform3ui)( GLint, GLuint, GLuint, GLuint ) = extension_funcs[EXT_glUniform3ui];
  TRACE("(%d, %d, %d, %d)\n", location, v0, v1, v2 );
  ENTER_GL();
  func_glUniform3ui( location, v0, v1, v2 );
  LEAVE_GL();
}

static void WINAPI wine_glUniform3ui64NV( GLint location, UINT64 x, UINT64 y, UINT64 z ) {
  void (*func_glUniform3ui64NV)( GLint, UINT64, UINT64, UINT64 ) = extension_funcs[EXT_glUniform3ui64NV];
  TRACE("(%d, %s, %s, %s)\n", location, wine_dbgstr_longlong(x), wine_dbgstr_longlong(y), wine_dbgstr_longlong(z) );
  ENTER_GL();
  func_glUniform3ui64NV( location, x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glUniform3ui64vNV( GLint location, GLsizei count, UINT64* value ) {
  void (*func_glUniform3ui64vNV)( GLint, GLsizei, UINT64* ) = extension_funcs[EXT_glUniform3ui64vNV];
  TRACE("(%d, %d, %p)\n", location, count, value );
  ENTER_GL();
  func_glUniform3ui64vNV( location, count, value );
  LEAVE_GL();
}

static void WINAPI wine_glUniform3uiEXT( GLint location, GLuint v0, GLuint v1, GLuint v2 ) {
  void (*func_glUniform3uiEXT)( GLint, GLuint, GLuint, GLuint ) = extension_funcs[EXT_glUniform3uiEXT];
  TRACE("(%d, %d, %d, %d)\n", location, v0, v1, v2 );
  ENTER_GL();
  func_glUniform3uiEXT( location, v0, v1, v2 );
  LEAVE_GL();
}

static void WINAPI wine_glUniform3uiv( GLint location, GLsizei count, GLuint* value ) {
  void (*func_glUniform3uiv)( GLint, GLsizei, GLuint* ) = extension_funcs[EXT_glUniform3uiv];
  TRACE("(%d, %d, %p)\n", location, count, value );
  ENTER_GL();
  func_glUniform3uiv( location, count, value );
  LEAVE_GL();
}

static void WINAPI wine_glUniform3uivEXT( GLint location, GLsizei count, GLuint* value ) {
  void (*func_glUniform3uivEXT)( GLint, GLsizei, GLuint* ) = extension_funcs[EXT_glUniform3uivEXT];
  TRACE("(%d, %d, %p)\n", location, count, value );
  ENTER_GL();
  func_glUniform3uivEXT( location, count, value );
  LEAVE_GL();
}

static void WINAPI wine_glUniform4d( GLint location, GLdouble x, GLdouble y, GLdouble z, GLdouble w ) {
  void (*func_glUniform4d)( GLint, GLdouble, GLdouble, GLdouble, GLdouble ) = extension_funcs[EXT_glUniform4d];
  TRACE("(%d, %f, %f, %f, %f)\n", location, x, y, z, w );
  ENTER_GL();
  func_glUniform4d( location, x, y, z, w );
  LEAVE_GL();
}

static void WINAPI wine_glUniform4dv( GLint location, GLsizei count, GLdouble* value ) {
  void (*func_glUniform4dv)( GLint, GLsizei, GLdouble* ) = extension_funcs[EXT_glUniform4dv];
  TRACE("(%d, %d, %p)\n", location, count, value );
  ENTER_GL();
  func_glUniform4dv( location, count, value );
  LEAVE_GL();
}

static void WINAPI wine_glUniform4f( GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3 ) {
  void (*func_glUniform4f)( GLint, GLfloat, GLfloat, GLfloat, GLfloat ) = extension_funcs[EXT_glUniform4f];
  TRACE("(%d, %f, %f, %f, %f)\n", location, v0, v1, v2, v3 );
  ENTER_GL();
  func_glUniform4f( location, v0, v1, v2, v3 );
  LEAVE_GL();
}

static void WINAPI wine_glUniform4fARB( GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3 ) {
  void (*func_glUniform4fARB)( GLint, GLfloat, GLfloat, GLfloat, GLfloat ) = extension_funcs[EXT_glUniform4fARB];
  TRACE("(%d, %f, %f, %f, %f)\n", location, v0, v1, v2, v3 );
  ENTER_GL();
  func_glUniform4fARB( location, v0, v1, v2, v3 );
  LEAVE_GL();
}

static void WINAPI wine_glUniform4fv( GLint location, GLsizei count, GLfloat* value ) {
  void (*func_glUniform4fv)( GLint, GLsizei, GLfloat* ) = extension_funcs[EXT_glUniform4fv];
  TRACE("(%d, %d, %p)\n", location, count, value );
  ENTER_GL();
  func_glUniform4fv( location, count, value );
  LEAVE_GL();
}

static void WINAPI wine_glUniform4fvARB( GLint location, GLsizei count, GLfloat* value ) {
  void (*func_glUniform4fvARB)( GLint, GLsizei, GLfloat* ) = extension_funcs[EXT_glUniform4fvARB];
  TRACE("(%d, %d, %p)\n", location, count, value );
  ENTER_GL();
  func_glUniform4fvARB( location, count, value );
  LEAVE_GL();
}

static void WINAPI wine_glUniform4i( GLint location, GLint v0, GLint v1, GLint v2, GLint v3 ) {
  void (*func_glUniform4i)( GLint, GLint, GLint, GLint, GLint ) = extension_funcs[EXT_glUniform4i];
  TRACE("(%d, %d, %d, %d, %d)\n", location, v0, v1, v2, v3 );
  ENTER_GL();
  func_glUniform4i( location, v0, v1, v2, v3 );
  LEAVE_GL();
}

static void WINAPI wine_glUniform4i64NV( GLint location, INT64 x, INT64 y, INT64 z, INT64 w ) {
  void (*func_glUniform4i64NV)( GLint, INT64, INT64, INT64, INT64 ) = extension_funcs[EXT_glUniform4i64NV];
  TRACE("(%d, %s, %s, %s, %s)\n", location, wine_dbgstr_longlong(x), wine_dbgstr_longlong(y), wine_dbgstr_longlong(z), wine_dbgstr_longlong(w) );
  ENTER_GL();
  func_glUniform4i64NV( location, x, y, z, w );
  LEAVE_GL();
}

static void WINAPI wine_glUniform4i64vNV( GLint location, GLsizei count, INT64* value ) {
  void (*func_glUniform4i64vNV)( GLint, GLsizei, INT64* ) = extension_funcs[EXT_glUniform4i64vNV];
  TRACE("(%d, %d, %p)\n", location, count, value );
  ENTER_GL();
  func_glUniform4i64vNV( location, count, value );
  LEAVE_GL();
}

static void WINAPI wine_glUniform4iARB( GLint location, GLint v0, GLint v1, GLint v2, GLint v3 ) {
  void (*func_glUniform4iARB)( GLint, GLint, GLint, GLint, GLint ) = extension_funcs[EXT_glUniform4iARB];
  TRACE("(%d, %d, %d, %d, %d)\n", location, v0, v1, v2, v3 );
  ENTER_GL();
  func_glUniform4iARB( location, v0, v1, v2, v3 );
  LEAVE_GL();
}

static void WINAPI wine_glUniform4iv( GLint location, GLsizei count, GLint* value ) {
  void (*func_glUniform4iv)( GLint, GLsizei, GLint* ) = extension_funcs[EXT_glUniform4iv];
  TRACE("(%d, %d, %p)\n", location, count, value );
  ENTER_GL();
  func_glUniform4iv( location, count, value );
  LEAVE_GL();
}

static void WINAPI wine_glUniform4ivARB( GLint location, GLsizei count, GLint* value ) {
  void (*func_glUniform4ivARB)( GLint, GLsizei, GLint* ) = extension_funcs[EXT_glUniform4ivARB];
  TRACE("(%d, %d, %p)\n", location, count, value );
  ENTER_GL();
  func_glUniform4ivARB( location, count, value );
  LEAVE_GL();
}

static void WINAPI wine_glUniform4ui( GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3 ) {
  void (*func_glUniform4ui)( GLint, GLuint, GLuint, GLuint, GLuint ) = extension_funcs[EXT_glUniform4ui];
  TRACE("(%d, %d, %d, %d, %d)\n", location, v0, v1, v2, v3 );
  ENTER_GL();
  func_glUniform4ui( location, v0, v1, v2, v3 );
  LEAVE_GL();
}

static void WINAPI wine_glUniform4ui64NV( GLint location, UINT64 x, UINT64 y, UINT64 z, UINT64 w ) {
  void (*func_glUniform4ui64NV)( GLint, UINT64, UINT64, UINT64, UINT64 ) = extension_funcs[EXT_glUniform4ui64NV];
  TRACE("(%d, %s, %s, %s, %s)\n", location, wine_dbgstr_longlong(x), wine_dbgstr_longlong(y), wine_dbgstr_longlong(z), wine_dbgstr_longlong(w) );
  ENTER_GL();
  func_glUniform4ui64NV( location, x, y, z, w );
  LEAVE_GL();
}

static void WINAPI wine_glUniform4ui64vNV( GLint location, GLsizei count, UINT64* value ) {
  void (*func_glUniform4ui64vNV)( GLint, GLsizei, UINT64* ) = extension_funcs[EXT_glUniform4ui64vNV];
  TRACE("(%d, %d, %p)\n", location, count, value );
  ENTER_GL();
  func_glUniform4ui64vNV( location, count, value );
  LEAVE_GL();
}

static void WINAPI wine_glUniform4uiEXT( GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3 ) {
  void (*func_glUniform4uiEXT)( GLint, GLuint, GLuint, GLuint, GLuint ) = extension_funcs[EXT_glUniform4uiEXT];
  TRACE("(%d, %d, %d, %d, %d)\n", location, v0, v1, v2, v3 );
  ENTER_GL();
  func_glUniform4uiEXT( location, v0, v1, v2, v3 );
  LEAVE_GL();
}

static void WINAPI wine_glUniform4uiv( GLint location, GLsizei count, GLuint* value ) {
  void (*func_glUniform4uiv)( GLint, GLsizei, GLuint* ) = extension_funcs[EXT_glUniform4uiv];
  TRACE("(%d, %d, %p)\n", location, count, value );
  ENTER_GL();
  func_glUniform4uiv( location, count, value );
  LEAVE_GL();
}

static void WINAPI wine_glUniform4uivEXT( GLint location, GLsizei count, GLuint* value ) {
  void (*func_glUniform4uivEXT)( GLint, GLsizei, GLuint* ) = extension_funcs[EXT_glUniform4uivEXT];
  TRACE("(%d, %d, %p)\n", location, count, value );
  ENTER_GL();
  func_glUniform4uivEXT( location, count, value );
  LEAVE_GL();
}

static void WINAPI wine_glUniformBlockBinding( GLuint program, GLuint uniformBlockIndex, GLuint uniformBlockBinding ) {
  void (*func_glUniformBlockBinding)( GLuint, GLuint, GLuint ) = extension_funcs[EXT_glUniformBlockBinding];
  TRACE("(%d, %d, %d)\n", program, uniformBlockIndex, uniformBlockBinding );
  ENTER_GL();
  func_glUniformBlockBinding( program, uniformBlockIndex, uniformBlockBinding );
  LEAVE_GL();
}

static void WINAPI wine_glUniformBufferEXT( GLuint program, GLint location, GLuint buffer ) {
  void (*func_glUniformBufferEXT)( GLuint, GLint, GLuint ) = extension_funcs[EXT_glUniformBufferEXT];
  TRACE("(%d, %d, %d)\n", program, location, buffer );
  ENTER_GL();
  func_glUniformBufferEXT( program, location, buffer );
  LEAVE_GL();
}

static void WINAPI wine_glUniformMatrix2dv( GLint location, GLsizei count, GLboolean transpose, GLdouble* value ) {
  void (*func_glUniformMatrix2dv)( GLint, GLsizei, GLboolean, GLdouble* ) = extension_funcs[EXT_glUniformMatrix2dv];
  TRACE("(%d, %d, %d, %p)\n", location, count, transpose, value );
  ENTER_GL();
  func_glUniformMatrix2dv( location, count, transpose, value );
  LEAVE_GL();
}

static void WINAPI wine_glUniformMatrix2fv( GLint location, GLsizei count, GLboolean transpose, GLfloat* value ) {
  void (*func_glUniformMatrix2fv)( GLint, GLsizei, GLboolean, GLfloat* ) = extension_funcs[EXT_glUniformMatrix2fv];
  TRACE("(%d, %d, %d, %p)\n", location, count, transpose, value );
  ENTER_GL();
  func_glUniformMatrix2fv( location, count, transpose, value );
  LEAVE_GL();
}

static void WINAPI wine_glUniformMatrix2fvARB( GLint location, GLsizei count, GLboolean transpose, GLfloat* value ) {
  void (*func_glUniformMatrix2fvARB)( GLint, GLsizei, GLboolean, GLfloat* ) = extension_funcs[EXT_glUniformMatrix2fvARB];
  TRACE("(%d, %d, %d, %p)\n", location, count, transpose, value );
  ENTER_GL();
  func_glUniformMatrix2fvARB( location, count, transpose, value );
  LEAVE_GL();
}

static void WINAPI wine_glUniformMatrix2x3dv( GLint location, GLsizei count, GLboolean transpose, GLdouble* value ) {
  void (*func_glUniformMatrix2x3dv)( GLint, GLsizei, GLboolean, GLdouble* ) = extension_funcs[EXT_glUniformMatrix2x3dv];
  TRACE("(%d, %d, %d, %p)\n", location, count, transpose, value );
  ENTER_GL();
  func_glUniformMatrix2x3dv( location, count, transpose, value );
  LEAVE_GL();
}

static void WINAPI wine_glUniformMatrix2x3fv( GLint location, GLsizei count, GLboolean transpose, GLfloat* value ) {
  void (*func_glUniformMatrix2x3fv)( GLint, GLsizei, GLboolean, GLfloat* ) = extension_funcs[EXT_glUniformMatrix2x3fv];
  TRACE("(%d, %d, %d, %p)\n", location, count, transpose, value );
  ENTER_GL();
  func_glUniformMatrix2x3fv( location, count, transpose, value );
  LEAVE_GL();
}

static void WINAPI wine_glUniformMatrix2x4dv( GLint location, GLsizei count, GLboolean transpose, GLdouble* value ) {
  void (*func_glUniformMatrix2x4dv)( GLint, GLsizei, GLboolean, GLdouble* ) = extension_funcs[EXT_glUniformMatrix2x4dv];
  TRACE("(%d, %d, %d, %p)\n", location, count, transpose, value );
  ENTER_GL();
  func_glUniformMatrix2x4dv( location, count, transpose, value );
  LEAVE_GL();
}

static void WINAPI wine_glUniformMatrix2x4fv( GLint location, GLsizei count, GLboolean transpose, GLfloat* value ) {
  void (*func_glUniformMatrix2x4fv)( GLint, GLsizei, GLboolean, GLfloat* ) = extension_funcs[EXT_glUniformMatrix2x4fv];
  TRACE("(%d, %d, %d, %p)\n", location, count, transpose, value );
  ENTER_GL();
  func_glUniformMatrix2x4fv( location, count, transpose, value );
  LEAVE_GL();
}

static void WINAPI wine_glUniformMatrix3dv( GLint location, GLsizei count, GLboolean transpose, GLdouble* value ) {
  void (*func_glUniformMatrix3dv)( GLint, GLsizei, GLboolean, GLdouble* ) = extension_funcs[EXT_glUniformMatrix3dv];
  TRACE("(%d, %d, %d, %p)\n", location, count, transpose, value );
  ENTER_GL();
  func_glUniformMatrix3dv( location, count, transpose, value );
  LEAVE_GL();
}

static void WINAPI wine_glUniformMatrix3fv( GLint location, GLsizei count, GLboolean transpose, GLfloat* value ) {
  void (*func_glUniformMatrix3fv)( GLint, GLsizei, GLboolean, GLfloat* ) = extension_funcs[EXT_glUniformMatrix3fv];
  TRACE("(%d, %d, %d, %p)\n", location, count, transpose, value );
  ENTER_GL();
  func_glUniformMatrix3fv( location, count, transpose, value );
  LEAVE_GL();
}

static void WINAPI wine_glUniformMatrix3fvARB( GLint location, GLsizei count, GLboolean transpose, GLfloat* value ) {
  void (*func_glUniformMatrix3fvARB)( GLint, GLsizei, GLboolean, GLfloat* ) = extension_funcs[EXT_glUniformMatrix3fvARB];
  TRACE("(%d, %d, %d, %p)\n", location, count, transpose, value );
  ENTER_GL();
  func_glUniformMatrix3fvARB( location, count, transpose, value );
  LEAVE_GL();
}

static void WINAPI wine_glUniformMatrix3x2dv( GLint location, GLsizei count, GLboolean transpose, GLdouble* value ) {
  void (*func_glUniformMatrix3x2dv)( GLint, GLsizei, GLboolean, GLdouble* ) = extension_funcs[EXT_glUniformMatrix3x2dv];
  TRACE("(%d, %d, %d, %p)\n", location, count, transpose, value );
  ENTER_GL();
  func_glUniformMatrix3x2dv( location, count, transpose, value );
  LEAVE_GL();
}

static void WINAPI wine_glUniformMatrix3x2fv( GLint location, GLsizei count, GLboolean transpose, GLfloat* value ) {
  void (*func_glUniformMatrix3x2fv)( GLint, GLsizei, GLboolean, GLfloat* ) = extension_funcs[EXT_glUniformMatrix3x2fv];
  TRACE("(%d, %d, %d, %p)\n", location, count, transpose, value );
  ENTER_GL();
  func_glUniformMatrix3x2fv( location, count, transpose, value );
  LEAVE_GL();
}

static void WINAPI wine_glUniformMatrix3x4dv( GLint location, GLsizei count, GLboolean transpose, GLdouble* value ) {
  void (*func_glUniformMatrix3x4dv)( GLint, GLsizei, GLboolean, GLdouble* ) = extension_funcs[EXT_glUniformMatrix3x4dv];
  TRACE("(%d, %d, %d, %p)\n", location, count, transpose, value );
  ENTER_GL();
  func_glUniformMatrix3x4dv( location, count, transpose, value );
  LEAVE_GL();
}

static void WINAPI wine_glUniformMatrix3x4fv( GLint location, GLsizei count, GLboolean transpose, GLfloat* value ) {
  void (*func_glUniformMatrix3x4fv)( GLint, GLsizei, GLboolean, GLfloat* ) = extension_funcs[EXT_glUniformMatrix3x4fv];
  TRACE("(%d, %d, %d, %p)\n", location, count, transpose, value );
  ENTER_GL();
  func_glUniformMatrix3x4fv( location, count, transpose, value );
  LEAVE_GL();
}

static void WINAPI wine_glUniformMatrix4dv( GLint location, GLsizei count, GLboolean transpose, GLdouble* value ) {
  void (*func_glUniformMatrix4dv)( GLint, GLsizei, GLboolean, GLdouble* ) = extension_funcs[EXT_glUniformMatrix4dv];
  TRACE("(%d, %d, %d, %p)\n", location, count, transpose, value );
  ENTER_GL();
  func_glUniformMatrix4dv( location, count, transpose, value );
  LEAVE_GL();
}

static void WINAPI wine_glUniformMatrix4fv( GLint location, GLsizei count, GLboolean transpose, GLfloat* value ) {
  void (*func_glUniformMatrix4fv)( GLint, GLsizei, GLboolean, GLfloat* ) = extension_funcs[EXT_glUniformMatrix4fv];
  TRACE("(%d, %d, %d, %p)\n", location, count, transpose, value );
  ENTER_GL();
  func_glUniformMatrix4fv( location, count, transpose, value );
  LEAVE_GL();
}

static void WINAPI wine_glUniformMatrix4fvARB( GLint location, GLsizei count, GLboolean transpose, GLfloat* value ) {
  void (*func_glUniformMatrix4fvARB)( GLint, GLsizei, GLboolean, GLfloat* ) = extension_funcs[EXT_glUniformMatrix4fvARB];
  TRACE("(%d, %d, %d, %p)\n", location, count, transpose, value );
  ENTER_GL();
  func_glUniformMatrix4fvARB( location, count, transpose, value );
  LEAVE_GL();
}

static void WINAPI wine_glUniformMatrix4x2dv( GLint location, GLsizei count, GLboolean transpose, GLdouble* value ) {
  void (*func_glUniformMatrix4x2dv)( GLint, GLsizei, GLboolean, GLdouble* ) = extension_funcs[EXT_glUniformMatrix4x2dv];
  TRACE("(%d, %d, %d, %p)\n", location, count, transpose, value );
  ENTER_GL();
  func_glUniformMatrix4x2dv( location, count, transpose, value );
  LEAVE_GL();
}

static void WINAPI wine_glUniformMatrix4x2fv( GLint location, GLsizei count, GLboolean transpose, GLfloat* value ) {
  void (*func_glUniformMatrix4x2fv)( GLint, GLsizei, GLboolean, GLfloat* ) = extension_funcs[EXT_glUniformMatrix4x2fv];
  TRACE("(%d, %d, %d, %p)\n", location, count, transpose, value );
  ENTER_GL();
  func_glUniformMatrix4x2fv( location, count, transpose, value );
  LEAVE_GL();
}

static void WINAPI wine_glUniformMatrix4x3dv( GLint location, GLsizei count, GLboolean transpose, GLdouble* value ) {
  void (*func_glUniformMatrix4x3dv)( GLint, GLsizei, GLboolean, GLdouble* ) = extension_funcs[EXT_glUniformMatrix4x3dv];
  TRACE("(%d, %d, %d, %p)\n", location, count, transpose, value );
  ENTER_GL();
  func_glUniformMatrix4x3dv( location, count, transpose, value );
  LEAVE_GL();
}

static void WINAPI wine_glUniformMatrix4x3fv( GLint location, GLsizei count, GLboolean transpose, GLfloat* value ) {
  void (*func_glUniformMatrix4x3fv)( GLint, GLsizei, GLboolean, GLfloat* ) = extension_funcs[EXT_glUniformMatrix4x3fv];
  TRACE("(%d, %d, %d, %p)\n", location, count, transpose, value );
  ENTER_GL();
  func_glUniformMatrix4x3fv( location, count, transpose, value );
  LEAVE_GL();
}

static void WINAPI wine_glUniformSubroutinesuiv( GLenum shadertype, GLsizei count, GLuint* indices ) {
  void (*func_glUniformSubroutinesuiv)( GLenum, GLsizei, GLuint* ) = extension_funcs[EXT_glUniformSubroutinesuiv];
  TRACE("(%d, %d, %p)\n", shadertype, count, indices );
  ENTER_GL();
  func_glUniformSubroutinesuiv( shadertype, count, indices );
  LEAVE_GL();
}

static void WINAPI wine_glUniformui64NV( GLint location, UINT64 value ) {
  void (*func_glUniformui64NV)( GLint, UINT64 ) = extension_funcs[EXT_glUniformui64NV];
  TRACE("(%d, %s)\n", location, wine_dbgstr_longlong(value) );
  ENTER_GL();
  func_glUniformui64NV( location, value );
  LEAVE_GL();
}

static void WINAPI wine_glUniformui64vNV( GLint location, GLsizei count, UINT64* value ) {
  void (*func_glUniformui64vNV)( GLint, GLsizei, UINT64* ) = extension_funcs[EXT_glUniformui64vNV];
  TRACE("(%d, %d, %p)\n", location, count, value );
  ENTER_GL();
  func_glUniformui64vNV( location, count, value );
  LEAVE_GL();
}

static void WINAPI wine_glUnlockArraysEXT( void ) {
  void (*func_glUnlockArraysEXT)( void ) = extension_funcs[EXT_glUnlockArraysEXT];
  TRACE("()\n");
  ENTER_GL();
  func_glUnlockArraysEXT( );
  LEAVE_GL();
}

static GLboolean WINAPI wine_glUnmapBuffer( GLenum target ) {
  GLboolean ret_value;
  GLboolean (*func_glUnmapBuffer)( GLenum ) = extension_funcs[EXT_glUnmapBuffer];
  TRACE("(%d)\n", target );
  ENTER_GL();
  ret_value = func_glUnmapBuffer( target );
  LEAVE_GL();
  return ret_value;
}

static GLboolean WINAPI wine_glUnmapBufferARB( GLenum target ) {
  GLboolean ret_value;
  GLboolean (*func_glUnmapBufferARB)( GLenum ) = extension_funcs[EXT_glUnmapBufferARB];
  TRACE("(%d)\n", target );
  ENTER_GL();
  ret_value = func_glUnmapBufferARB( target );
  LEAVE_GL();
  return ret_value;
}

static GLboolean WINAPI wine_glUnmapNamedBufferEXT( GLuint buffer ) {
  GLboolean ret_value;
  GLboolean (*func_glUnmapNamedBufferEXT)( GLuint ) = extension_funcs[EXT_glUnmapNamedBufferEXT];
  TRACE("(%d)\n", buffer );
  ENTER_GL();
  ret_value = func_glUnmapNamedBufferEXT( buffer );
  LEAVE_GL();
  return ret_value;
}

static void WINAPI wine_glUnmapObjectBufferATI( GLuint buffer ) {
  void (*func_glUnmapObjectBufferATI)( GLuint ) = extension_funcs[EXT_glUnmapObjectBufferATI];
  TRACE("(%d)\n", buffer );
  ENTER_GL();
  func_glUnmapObjectBufferATI( buffer );
  LEAVE_GL();
}

static void WINAPI wine_glUpdateObjectBufferATI( GLuint buffer, GLuint offset, GLsizei size, GLvoid* pointer, GLenum preserve ) {
  void (*func_glUpdateObjectBufferATI)( GLuint, GLuint, GLsizei, GLvoid*, GLenum ) = extension_funcs[EXT_glUpdateObjectBufferATI];
  TRACE("(%d, %d, %d, %p, %d)\n", buffer, offset, size, pointer, preserve );
  ENTER_GL();
  func_glUpdateObjectBufferATI( buffer, offset, size, pointer, preserve );
  LEAVE_GL();
}

static void WINAPI wine_glUseProgram( GLuint program ) {
  void (*func_glUseProgram)( GLuint ) = extension_funcs[EXT_glUseProgram];
  TRACE("(%d)\n", program );
  ENTER_GL();
  func_glUseProgram( program );
  LEAVE_GL();
}

static void WINAPI wine_glUseProgramObjectARB( unsigned int programObj ) {
  void (*func_glUseProgramObjectARB)( unsigned int ) = extension_funcs[EXT_glUseProgramObjectARB];
  TRACE("(%d)\n", programObj );
  ENTER_GL();
  func_glUseProgramObjectARB( programObj );
  LEAVE_GL();
}

static void WINAPI wine_glUseProgramStages( GLuint pipeline, GLbitfield stages, GLuint program ) {
  void (*func_glUseProgramStages)( GLuint, GLbitfield, GLuint ) = extension_funcs[EXT_glUseProgramStages];
  TRACE("(%d, %d, %d)\n", pipeline, stages, program );
  ENTER_GL();
  func_glUseProgramStages( pipeline, stages, program );
  LEAVE_GL();
}

static void WINAPI wine_glUseShaderProgramEXT( GLenum type, GLuint program ) {
  void (*func_glUseShaderProgramEXT)( GLenum, GLuint ) = extension_funcs[EXT_glUseShaderProgramEXT];
  TRACE("(%d, %d)\n", type, program );
  ENTER_GL();
  func_glUseShaderProgramEXT( type, program );
  LEAVE_GL();
}

static void WINAPI wine_glVDPAUFiniNV( void ) {
  void (*func_glVDPAUFiniNV)( void ) = extension_funcs[EXT_glVDPAUFiniNV];
  TRACE("()\n");
  ENTER_GL();
  func_glVDPAUFiniNV( );
  LEAVE_GL();
}

static void WINAPI wine_glVDPAUGetSurfaceivNV( INT_PTR surface, GLenum pname, GLsizei bufSize, GLsizei* length, GLint* values ) {
  void (*func_glVDPAUGetSurfaceivNV)( INT_PTR, GLenum, GLsizei, GLsizei*, GLint* ) = extension_funcs[EXT_glVDPAUGetSurfaceivNV];
  TRACE("(%ld, %d, %d, %p, %p)\n", surface, pname, bufSize, length, values );
  ENTER_GL();
  func_glVDPAUGetSurfaceivNV( surface, pname, bufSize, length, values );
  LEAVE_GL();
}

static void WINAPI wine_glVDPAUInitNV( GLvoid* vdpDevice, GLvoid* getProcAddress ) {
  void (*func_glVDPAUInitNV)( GLvoid*, GLvoid* ) = extension_funcs[EXT_glVDPAUInitNV];
  TRACE("(%p, %p)\n", vdpDevice, getProcAddress );
  ENTER_GL();
  func_glVDPAUInitNV( vdpDevice, getProcAddress );
  LEAVE_GL();
}

static void WINAPI wine_glVDPAUIsSurfaceNV( INT_PTR surface ) {
  void (*func_glVDPAUIsSurfaceNV)( INT_PTR ) = extension_funcs[EXT_glVDPAUIsSurfaceNV];
  TRACE("(%ld)\n", surface );
  ENTER_GL();
  func_glVDPAUIsSurfaceNV( surface );
  LEAVE_GL();
}

static void WINAPI wine_glVDPAUMapSurfacesNV( GLsizei numSurfaces, INT_PTR* surfaces ) {
  void (*func_glVDPAUMapSurfacesNV)( GLsizei, INT_PTR* ) = extension_funcs[EXT_glVDPAUMapSurfacesNV];
  TRACE("(%d, %p)\n", numSurfaces, surfaces );
  ENTER_GL();
  func_glVDPAUMapSurfacesNV( numSurfaces, surfaces );
  LEAVE_GL();
}

static INT_PTR WINAPI wine_glVDPAURegisterOutputSurfaceNV( GLvoid* vdpSurface, GLenum target, GLsizei numTextureNames, GLuint* textureNames ) {
  INT_PTR ret_value;
  INT_PTR (*func_glVDPAURegisterOutputSurfaceNV)( GLvoid*, GLenum, GLsizei, GLuint* ) = extension_funcs[EXT_glVDPAURegisterOutputSurfaceNV];
  TRACE("(%p, %d, %d, %p)\n", vdpSurface, target, numTextureNames, textureNames );
  ENTER_GL();
  ret_value = func_glVDPAURegisterOutputSurfaceNV( vdpSurface, target, numTextureNames, textureNames );
  LEAVE_GL();
  return ret_value;
}

static INT_PTR WINAPI wine_glVDPAURegisterVideoSurfaceNV( GLvoid* vdpSurface, GLenum target, GLsizei numTextureNames, GLuint* textureNames ) {
  INT_PTR ret_value;
  INT_PTR (*func_glVDPAURegisterVideoSurfaceNV)( GLvoid*, GLenum, GLsizei, GLuint* ) = extension_funcs[EXT_glVDPAURegisterVideoSurfaceNV];
  TRACE("(%p, %d, %d, %p)\n", vdpSurface, target, numTextureNames, textureNames );
  ENTER_GL();
  ret_value = func_glVDPAURegisterVideoSurfaceNV( vdpSurface, target, numTextureNames, textureNames );
  LEAVE_GL();
  return ret_value;
}

static void WINAPI wine_glVDPAUSurfaceAccessNV( INT_PTR surface, GLenum access ) {
  void (*func_glVDPAUSurfaceAccessNV)( INT_PTR, GLenum ) = extension_funcs[EXT_glVDPAUSurfaceAccessNV];
  TRACE("(%ld, %d)\n", surface, access );
  ENTER_GL();
  func_glVDPAUSurfaceAccessNV( surface, access );
  LEAVE_GL();
}

static void WINAPI wine_glVDPAUUnmapSurfacesNV( GLsizei numSurface, INT_PTR* surfaces ) {
  void (*func_glVDPAUUnmapSurfacesNV)( GLsizei, INT_PTR* ) = extension_funcs[EXT_glVDPAUUnmapSurfacesNV];
  TRACE("(%d, %p)\n", numSurface, surfaces );
  ENTER_GL();
  func_glVDPAUUnmapSurfacesNV( numSurface, surfaces );
  LEAVE_GL();
}

static void WINAPI wine_glVDPAUUnregisterSurfaceNV( INT_PTR surface ) {
  void (*func_glVDPAUUnregisterSurfaceNV)( INT_PTR ) = extension_funcs[EXT_glVDPAUUnregisterSurfaceNV];
  TRACE("(%ld)\n", surface );
  ENTER_GL();
  func_glVDPAUUnregisterSurfaceNV( surface );
  LEAVE_GL();
}

static void WINAPI wine_glValidateProgram( GLuint program ) {
  void (*func_glValidateProgram)( GLuint ) = extension_funcs[EXT_glValidateProgram];
  TRACE("(%d)\n", program );
  ENTER_GL();
  func_glValidateProgram( program );
  LEAVE_GL();
}

static void WINAPI wine_glValidateProgramARB( unsigned int programObj ) {
  void (*func_glValidateProgramARB)( unsigned int ) = extension_funcs[EXT_glValidateProgramARB];
  TRACE("(%d)\n", programObj );
  ENTER_GL();
  func_glValidateProgramARB( programObj );
  LEAVE_GL();
}

static void WINAPI wine_glValidateProgramPipeline( GLuint pipeline ) {
  void (*func_glValidateProgramPipeline)( GLuint ) = extension_funcs[EXT_glValidateProgramPipeline];
  TRACE("(%d)\n", pipeline );
  ENTER_GL();
  func_glValidateProgramPipeline( pipeline );
  LEAVE_GL();
}

static void WINAPI wine_glVariantArrayObjectATI( GLuint id, GLenum type, GLsizei stride, GLuint buffer, GLuint offset ) {
  void (*func_glVariantArrayObjectATI)( GLuint, GLenum, GLsizei, GLuint, GLuint ) = extension_funcs[EXT_glVariantArrayObjectATI];
  TRACE("(%d, %d, %d, %d, %d)\n", id, type, stride, buffer, offset );
  ENTER_GL();
  func_glVariantArrayObjectATI( id, type, stride, buffer, offset );
  LEAVE_GL();
}

static void WINAPI wine_glVariantPointerEXT( GLuint id, GLenum type, GLuint stride, GLvoid* addr ) {
  void (*func_glVariantPointerEXT)( GLuint, GLenum, GLuint, GLvoid* ) = extension_funcs[EXT_glVariantPointerEXT];
  TRACE("(%d, %d, %d, %p)\n", id, type, stride, addr );
  ENTER_GL();
  func_glVariantPointerEXT( id, type, stride, addr );
  LEAVE_GL();
}

static void WINAPI wine_glVariantbvEXT( GLuint id, GLbyte* addr ) {
  void (*func_glVariantbvEXT)( GLuint, GLbyte* ) = extension_funcs[EXT_glVariantbvEXT];
  TRACE("(%d, %p)\n", id, addr );
  ENTER_GL();
  func_glVariantbvEXT( id, addr );
  LEAVE_GL();
}

static void WINAPI wine_glVariantdvEXT( GLuint id, GLdouble* addr ) {
  void (*func_glVariantdvEXT)( GLuint, GLdouble* ) = extension_funcs[EXT_glVariantdvEXT];
  TRACE("(%d, %p)\n", id, addr );
  ENTER_GL();
  func_glVariantdvEXT( id, addr );
  LEAVE_GL();
}

static void WINAPI wine_glVariantfvEXT( GLuint id, GLfloat* addr ) {
  void (*func_glVariantfvEXT)( GLuint, GLfloat* ) = extension_funcs[EXT_glVariantfvEXT];
  TRACE("(%d, %p)\n", id, addr );
  ENTER_GL();
  func_glVariantfvEXT( id, addr );
  LEAVE_GL();
}

static void WINAPI wine_glVariantivEXT( GLuint id, GLint* addr ) {
  void (*func_glVariantivEXT)( GLuint, GLint* ) = extension_funcs[EXT_glVariantivEXT];
  TRACE("(%d, %p)\n", id, addr );
  ENTER_GL();
  func_glVariantivEXT( id, addr );
  LEAVE_GL();
}

static void WINAPI wine_glVariantsvEXT( GLuint id, GLshort* addr ) {
  void (*func_glVariantsvEXT)( GLuint, GLshort* ) = extension_funcs[EXT_glVariantsvEXT];
  TRACE("(%d, %p)\n", id, addr );
  ENTER_GL();
  func_glVariantsvEXT( id, addr );
  LEAVE_GL();
}

static void WINAPI wine_glVariantubvEXT( GLuint id, GLubyte* addr ) {
  void (*func_glVariantubvEXT)( GLuint, GLubyte* ) = extension_funcs[EXT_glVariantubvEXT];
  TRACE("(%d, %p)\n", id, addr );
  ENTER_GL();
  func_glVariantubvEXT( id, addr );
  LEAVE_GL();
}

static void WINAPI wine_glVariantuivEXT( GLuint id, GLuint* addr ) {
  void (*func_glVariantuivEXT)( GLuint, GLuint* ) = extension_funcs[EXT_glVariantuivEXT];
  TRACE("(%d, %p)\n", id, addr );
  ENTER_GL();
  func_glVariantuivEXT( id, addr );
  LEAVE_GL();
}

static void WINAPI wine_glVariantusvEXT( GLuint id, GLushort* addr ) {
  void (*func_glVariantusvEXT)( GLuint, GLushort* ) = extension_funcs[EXT_glVariantusvEXT];
  TRACE("(%d, %p)\n", id, addr );
  ENTER_GL();
  func_glVariantusvEXT( id, addr );
  LEAVE_GL();
}

static void WINAPI wine_glVertex2hNV( unsigned short x, unsigned short y ) {
  void (*func_glVertex2hNV)( unsigned short, unsigned short ) = extension_funcs[EXT_glVertex2hNV];
  TRACE("(%d, %d)\n", x, y );
  ENTER_GL();
  func_glVertex2hNV( x, y );
  LEAVE_GL();
}

static void WINAPI wine_glVertex2hvNV( unsigned short* v ) {
  void (*func_glVertex2hvNV)( unsigned short* ) = extension_funcs[EXT_glVertex2hvNV];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glVertex2hvNV( v );
  LEAVE_GL();
}

static void WINAPI wine_glVertex3hNV( unsigned short x, unsigned short y, unsigned short z ) {
  void (*func_glVertex3hNV)( unsigned short, unsigned short, unsigned short ) = extension_funcs[EXT_glVertex3hNV];
  TRACE("(%d, %d, %d)\n", x, y, z );
  ENTER_GL();
  func_glVertex3hNV( x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glVertex3hvNV( unsigned short* v ) {
  void (*func_glVertex3hvNV)( unsigned short* ) = extension_funcs[EXT_glVertex3hvNV];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glVertex3hvNV( v );
  LEAVE_GL();
}

static void WINAPI wine_glVertex4hNV( unsigned short x, unsigned short y, unsigned short z, unsigned short w ) {
  void (*func_glVertex4hNV)( unsigned short, unsigned short, unsigned short, unsigned short ) = extension_funcs[EXT_glVertex4hNV];
  TRACE("(%d, %d, %d, %d)\n", x, y, z, w );
  ENTER_GL();
  func_glVertex4hNV( x, y, z, w );
  LEAVE_GL();
}

static void WINAPI wine_glVertex4hvNV( unsigned short* v ) {
  void (*func_glVertex4hvNV)( unsigned short* ) = extension_funcs[EXT_glVertex4hvNV];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glVertex4hvNV( v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexArrayParameteriAPPLE( GLenum pname, GLint param ) {
  void (*func_glVertexArrayParameteriAPPLE)( GLenum, GLint ) = extension_funcs[EXT_glVertexArrayParameteriAPPLE];
  TRACE("(%d, %d)\n", pname, param );
  ENTER_GL();
  func_glVertexArrayParameteriAPPLE( pname, param );
  LEAVE_GL();
}

static void WINAPI wine_glVertexArrayRangeAPPLE( GLsizei length, GLvoid* pointer ) {
  void (*func_glVertexArrayRangeAPPLE)( GLsizei, GLvoid* ) = extension_funcs[EXT_glVertexArrayRangeAPPLE];
  TRACE("(%d, %p)\n", length, pointer );
  ENTER_GL();
  func_glVertexArrayRangeAPPLE( length, pointer );
  LEAVE_GL();
}

static void WINAPI wine_glVertexArrayRangeNV( GLsizei length, GLvoid* pointer ) {
  void (*func_glVertexArrayRangeNV)( GLsizei, GLvoid* ) = extension_funcs[EXT_glVertexArrayRangeNV];
  TRACE("(%d, %p)\n", length, pointer );
  ENTER_GL();
  func_glVertexArrayRangeNV( length, pointer );
  LEAVE_GL();
}

static void WINAPI wine_glVertexArrayVertexAttribLOffsetEXT( GLuint vaobj, GLuint buffer, GLuint index, GLint size, GLenum type, GLsizei stride, INT_PTR offset ) {
  void (*func_glVertexArrayVertexAttribLOffsetEXT)( GLuint, GLuint, GLuint, GLint, GLenum, GLsizei, INT_PTR ) = extension_funcs[EXT_glVertexArrayVertexAttribLOffsetEXT];
  TRACE("(%d, %d, %d, %d, %d, %d, %ld)\n", vaobj, buffer, index, size, type, stride, offset );
  ENTER_GL();
  func_glVertexArrayVertexAttribLOffsetEXT( vaobj, buffer, index, size, type, stride, offset );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib1d( GLuint index, GLdouble x ) {
  void (*func_glVertexAttrib1d)( GLuint, GLdouble ) = extension_funcs[EXT_glVertexAttrib1d];
  TRACE("(%d, %f)\n", index, x );
  ENTER_GL();
  func_glVertexAttrib1d( index, x );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib1dARB( GLuint index, GLdouble x ) {
  void (*func_glVertexAttrib1dARB)( GLuint, GLdouble ) = extension_funcs[EXT_glVertexAttrib1dARB];
  TRACE("(%d, %f)\n", index, x );
  ENTER_GL();
  func_glVertexAttrib1dARB( index, x );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib1dNV( GLuint index, GLdouble x ) {
  void (*func_glVertexAttrib1dNV)( GLuint, GLdouble ) = extension_funcs[EXT_glVertexAttrib1dNV];
  TRACE("(%d, %f)\n", index, x );
  ENTER_GL();
  func_glVertexAttrib1dNV( index, x );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib1dv( GLuint index, GLdouble* v ) {
  void (*func_glVertexAttrib1dv)( GLuint, GLdouble* ) = extension_funcs[EXT_glVertexAttrib1dv];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib1dv( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib1dvARB( GLuint index, GLdouble* v ) {
  void (*func_glVertexAttrib1dvARB)( GLuint, GLdouble* ) = extension_funcs[EXT_glVertexAttrib1dvARB];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib1dvARB( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib1dvNV( GLuint index, GLdouble* v ) {
  void (*func_glVertexAttrib1dvNV)( GLuint, GLdouble* ) = extension_funcs[EXT_glVertexAttrib1dvNV];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib1dvNV( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib1f( GLuint index, GLfloat x ) {
  void (*func_glVertexAttrib1f)( GLuint, GLfloat ) = extension_funcs[EXT_glVertexAttrib1f];
  TRACE("(%d, %f)\n", index, x );
  ENTER_GL();
  func_glVertexAttrib1f( index, x );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib1fARB( GLuint index, GLfloat x ) {
  void (*func_glVertexAttrib1fARB)( GLuint, GLfloat ) = extension_funcs[EXT_glVertexAttrib1fARB];
  TRACE("(%d, %f)\n", index, x );
  ENTER_GL();
  func_glVertexAttrib1fARB( index, x );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib1fNV( GLuint index, GLfloat x ) {
  void (*func_glVertexAttrib1fNV)( GLuint, GLfloat ) = extension_funcs[EXT_glVertexAttrib1fNV];
  TRACE("(%d, %f)\n", index, x );
  ENTER_GL();
  func_glVertexAttrib1fNV( index, x );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib1fv( GLuint index, GLfloat* v ) {
  void (*func_glVertexAttrib1fv)( GLuint, GLfloat* ) = extension_funcs[EXT_glVertexAttrib1fv];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib1fv( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib1fvARB( GLuint index, GLfloat* v ) {
  void (*func_glVertexAttrib1fvARB)( GLuint, GLfloat* ) = extension_funcs[EXT_glVertexAttrib1fvARB];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib1fvARB( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib1fvNV( GLuint index, GLfloat* v ) {
  void (*func_glVertexAttrib1fvNV)( GLuint, GLfloat* ) = extension_funcs[EXT_glVertexAttrib1fvNV];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib1fvNV( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib1hNV( GLuint index, unsigned short x ) {
  void (*func_glVertexAttrib1hNV)( GLuint, unsigned short ) = extension_funcs[EXT_glVertexAttrib1hNV];
  TRACE("(%d, %d)\n", index, x );
  ENTER_GL();
  func_glVertexAttrib1hNV( index, x );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib1hvNV( GLuint index, unsigned short* v ) {
  void (*func_glVertexAttrib1hvNV)( GLuint, unsigned short* ) = extension_funcs[EXT_glVertexAttrib1hvNV];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib1hvNV( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib1s( GLuint index, GLshort x ) {
  void (*func_glVertexAttrib1s)( GLuint, GLshort ) = extension_funcs[EXT_glVertexAttrib1s];
  TRACE("(%d, %d)\n", index, x );
  ENTER_GL();
  func_glVertexAttrib1s( index, x );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib1sARB( GLuint index, GLshort x ) {
  void (*func_glVertexAttrib1sARB)( GLuint, GLshort ) = extension_funcs[EXT_glVertexAttrib1sARB];
  TRACE("(%d, %d)\n", index, x );
  ENTER_GL();
  func_glVertexAttrib1sARB( index, x );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib1sNV( GLuint index, GLshort x ) {
  void (*func_glVertexAttrib1sNV)( GLuint, GLshort ) = extension_funcs[EXT_glVertexAttrib1sNV];
  TRACE("(%d, %d)\n", index, x );
  ENTER_GL();
  func_glVertexAttrib1sNV( index, x );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib1sv( GLuint index, GLshort* v ) {
  void (*func_glVertexAttrib1sv)( GLuint, GLshort* ) = extension_funcs[EXT_glVertexAttrib1sv];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib1sv( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib1svARB( GLuint index, GLshort* v ) {
  void (*func_glVertexAttrib1svARB)( GLuint, GLshort* ) = extension_funcs[EXT_glVertexAttrib1svARB];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib1svARB( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib1svNV( GLuint index, GLshort* v ) {
  void (*func_glVertexAttrib1svNV)( GLuint, GLshort* ) = extension_funcs[EXT_glVertexAttrib1svNV];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib1svNV( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib2d( GLuint index, GLdouble x, GLdouble y ) {
  void (*func_glVertexAttrib2d)( GLuint, GLdouble, GLdouble ) = extension_funcs[EXT_glVertexAttrib2d];
  TRACE("(%d, %f, %f)\n", index, x, y );
  ENTER_GL();
  func_glVertexAttrib2d( index, x, y );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib2dARB( GLuint index, GLdouble x, GLdouble y ) {
  void (*func_glVertexAttrib2dARB)( GLuint, GLdouble, GLdouble ) = extension_funcs[EXT_glVertexAttrib2dARB];
  TRACE("(%d, %f, %f)\n", index, x, y );
  ENTER_GL();
  func_glVertexAttrib2dARB( index, x, y );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib2dNV( GLuint index, GLdouble x, GLdouble y ) {
  void (*func_glVertexAttrib2dNV)( GLuint, GLdouble, GLdouble ) = extension_funcs[EXT_glVertexAttrib2dNV];
  TRACE("(%d, %f, %f)\n", index, x, y );
  ENTER_GL();
  func_glVertexAttrib2dNV( index, x, y );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib2dv( GLuint index, GLdouble* v ) {
  void (*func_glVertexAttrib2dv)( GLuint, GLdouble* ) = extension_funcs[EXT_glVertexAttrib2dv];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib2dv( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib2dvARB( GLuint index, GLdouble* v ) {
  void (*func_glVertexAttrib2dvARB)( GLuint, GLdouble* ) = extension_funcs[EXT_glVertexAttrib2dvARB];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib2dvARB( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib2dvNV( GLuint index, GLdouble* v ) {
  void (*func_glVertexAttrib2dvNV)( GLuint, GLdouble* ) = extension_funcs[EXT_glVertexAttrib2dvNV];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib2dvNV( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib2f( GLuint index, GLfloat x, GLfloat y ) {
  void (*func_glVertexAttrib2f)( GLuint, GLfloat, GLfloat ) = extension_funcs[EXT_glVertexAttrib2f];
  TRACE("(%d, %f, %f)\n", index, x, y );
  ENTER_GL();
  func_glVertexAttrib2f( index, x, y );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib2fARB( GLuint index, GLfloat x, GLfloat y ) {
  void (*func_glVertexAttrib2fARB)( GLuint, GLfloat, GLfloat ) = extension_funcs[EXT_glVertexAttrib2fARB];
  TRACE("(%d, %f, %f)\n", index, x, y );
  ENTER_GL();
  func_glVertexAttrib2fARB( index, x, y );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib2fNV( GLuint index, GLfloat x, GLfloat y ) {
  void (*func_glVertexAttrib2fNV)( GLuint, GLfloat, GLfloat ) = extension_funcs[EXT_glVertexAttrib2fNV];
  TRACE("(%d, %f, %f)\n", index, x, y );
  ENTER_GL();
  func_glVertexAttrib2fNV( index, x, y );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib2fv( GLuint index, GLfloat* v ) {
  void (*func_glVertexAttrib2fv)( GLuint, GLfloat* ) = extension_funcs[EXT_glVertexAttrib2fv];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib2fv( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib2fvARB( GLuint index, GLfloat* v ) {
  void (*func_glVertexAttrib2fvARB)( GLuint, GLfloat* ) = extension_funcs[EXT_glVertexAttrib2fvARB];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib2fvARB( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib2fvNV( GLuint index, GLfloat* v ) {
  void (*func_glVertexAttrib2fvNV)( GLuint, GLfloat* ) = extension_funcs[EXT_glVertexAttrib2fvNV];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib2fvNV( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib2hNV( GLuint index, unsigned short x, unsigned short y ) {
  void (*func_glVertexAttrib2hNV)( GLuint, unsigned short, unsigned short ) = extension_funcs[EXT_glVertexAttrib2hNV];
  TRACE("(%d, %d, %d)\n", index, x, y );
  ENTER_GL();
  func_glVertexAttrib2hNV( index, x, y );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib2hvNV( GLuint index, unsigned short* v ) {
  void (*func_glVertexAttrib2hvNV)( GLuint, unsigned short* ) = extension_funcs[EXT_glVertexAttrib2hvNV];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib2hvNV( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib2s( GLuint index, GLshort x, GLshort y ) {
  void (*func_glVertexAttrib2s)( GLuint, GLshort, GLshort ) = extension_funcs[EXT_glVertexAttrib2s];
  TRACE("(%d, %d, %d)\n", index, x, y );
  ENTER_GL();
  func_glVertexAttrib2s( index, x, y );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib2sARB( GLuint index, GLshort x, GLshort y ) {
  void (*func_glVertexAttrib2sARB)( GLuint, GLshort, GLshort ) = extension_funcs[EXT_glVertexAttrib2sARB];
  TRACE("(%d, %d, %d)\n", index, x, y );
  ENTER_GL();
  func_glVertexAttrib2sARB( index, x, y );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib2sNV( GLuint index, GLshort x, GLshort y ) {
  void (*func_glVertexAttrib2sNV)( GLuint, GLshort, GLshort ) = extension_funcs[EXT_glVertexAttrib2sNV];
  TRACE("(%d, %d, %d)\n", index, x, y );
  ENTER_GL();
  func_glVertexAttrib2sNV( index, x, y );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib2sv( GLuint index, GLshort* v ) {
  void (*func_glVertexAttrib2sv)( GLuint, GLshort* ) = extension_funcs[EXT_glVertexAttrib2sv];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib2sv( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib2svARB( GLuint index, GLshort* v ) {
  void (*func_glVertexAttrib2svARB)( GLuint, GLshort* ) = extension_funcs[EXT_glVertexAttrib2svARB];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib2svARB( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib2svNV( GLuint index, GLshort* v ) {
  void (*func_glVertexAttrib2svNV)( GLuint, GLshort* ) = extension_funcs[EXT_glVertexAttrib2svNV];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib2svNV( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib3d( GLuint index, GLdouble x, GLdouble y, GLdouble z ) {
  void (*func_glVertexAttrib3d)( GLuint, GLdouble, GLdouble, GLdouble ) = extension_funcs[EXT_glVertexAttrib3d];
  TRACE("(%d, %f, %f, %f)\n", index, x, y, z );
  ENTER_GL();
  func_glVertexAttrib3d( index, x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib3dARB( GLuint index, GLdouble x, GLdouble y, GLdouble z ) {
  void (*func_glVertexAttrib3dARB)( GLuint, GLdouble, GLdouble, GLdouble ) = extension_funcs[EXT_glVertexAttrib3dARB];
  TRACE("(%d, %f, %f, %f)\n", index, x, y, z );
  ENTER_GL();
  func_glVertexAttrib3dARB( index, x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib3dNV( GLuint index, GLdouble x, GLdouble y, GLdouble z ) {
  void (*func_glVertexAttrib3dNV)( GLuint, GLdouble, GLdouble, GLdouble ) = extension_funcs[EXT_glVertexAttrib3dNV];
  TRACE("(%d, %f, %f, %f)\n", index, x, y, z );
  ENTER_GL();
  func_glVertexAttrib3dNV( index, x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib3dv( GLuint index, GLdouble* v ) {
  void (*func_glVertexAttrib3dv)( GLuint, GLdouble* ) = extension_funcs[EXT_glVertexAttrib3dv];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib3dv( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib3dvARB( GLuint index, GLdouble* v ) {
  void (*func_glVertexAttrib3dvARB)( GLuint, GLdouble* ) = extension_funcs[EXT_glVertexAttrib3dvARB];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib3dvARB( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib3dvNV( GLuint index, GLdouble* v ) {
  void (*func_glVertexAttrib3dvNV)( GLuint, GLdouble* ) = extension_funcs[EXT_glVertexAttrib3dvNV];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib3dvNV( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib3f( GLuint index, GLfloat x, GLfloat y, GLfloat z ) {
  void (*func_glVertexAttrib3f)( GLuint, GLfloat, GLfloat, GLfloat ) = extension_funcs[EXT_glVertexAttrib3f];
  TRACE("(%d, %f, %f, %f)\n", index, x, y, z );
  ENTER_GL();
  func_glVertexAttrib3f( index, x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib3fARB( GLuint index, GLfloat x, GLfloat y, GLfloat z ) {
  void (*func_glVertexAttrib3fARB)( GLuint, GLfloat, GLfloat, GLfloat ) = extension_funcs[EXT_glVertexAttrib3fARB];
  TRACE("(%d, %f, %f, %f)\n", index, x, y, z );
  ENTER_GL();
  func_glVertexAttrib3fARB( index, x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib3fNV( GLuint index, GLfloat x, GLfloat y, GLfloat z ) {
  void (*func_glVertexAttrib3fNV)( GLuint, GLfloat, GLfloat, GLfloat ) = extension_funcs[EXT_glVertexAttrib3fNV];
  TRACE("(%d, %f, %f, %f)\n", index, x, y, z );
  ENTER_GL();
  func_glVertexAttrib3fNV( index, x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib3fv( GLuint index, GLfloat* v ) {
  void (*func_glVertexAttrib3fv)( GLuint, GLfloat* ) = extension_funcs[EXT_glVertexAttrib3fv];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib3fv( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib3fvARB( GLuint index, GLfloat* v ) {
  void (*func_glVertexAttrib3fvARB)( GLuint, GLfloat* ) = extension_funcs[EXT_glVertexAttrib3fvARB];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib3fvARB( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib3fvNV( GLuint index, GLfloat* v ) {
  void (*func_glVertexAttrib3fvNV)( GLuint, GLfloat* ) = extension_funcs[EXT_glVertexAttrib3fvNV];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib3fvNV( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib3hNV( GLuint index, unsigned short x, unsigned short y, unsigned short z ) {
  void (*func_glVertexAttrib3hNV)( GLuint, unsigned short, unsigned short, unsigned short ) = extension_funcs[EXT_glVertexAttrib3hNV];
  TRACE("(%d, %d, %d, %d)\n", index, x, y, z );
  ENTER_GL();
  func_glVertexAttrib3hNV( index, x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib3hvNV( GLuint index, unsigned short* v ) {
  void (*func_glVertexAttrib3hvNV)( GLuint, unsigned short* ) = extension_funcs[EXT_glVertexAttrib3hvNV];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib3hvNV( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib3s( GLuint index, GLshort x, GLshort y, GLshort z ) {
  void (*func_glVertexAttrib3s)( GLuint, GLshort, GLshort, GLshort ) = extension_funcs[EXT_glVertexAttrib3s];
  TRACE("(%d, %d, %d, %d)\n", index, x, y, z );
  ENTER_GL();
  func_glVertexAttrib3s( index, x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib3sARB( GLuint index, GLshort x, GLshort y, GLshort z ) {
  void (*func_glVertexAttrib3sARB)( GLuint, GLshort, GLshort, GLshort ) = extension_funcs[EXT_glVertexAttrib3sARB];
  TRACE("(%d, %d, %d, %d)\n", index, x, y, z );
  ENTER_GL();
  func_glVertexAttrib3sARB( index, x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib3sNV( GLuint index, GLshort x, GLshort y, GLshort z ) {
  void (*func_glVertexAttrib3sNV)( GLuint, GLshort, GLshort, GLshort ) = extension_funcs[EXT_glVertexAttrib3sNV];
  TRACE("(%d, %d, %d, %d)\n", index, x, y, z );
  ENTER_GL();
  func_glVertexAttrib3sNV( index, x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib3sv( GLuint index, GLshort* v ) {
  void (*func_glVertexAttrib3sv)( GLuint, GLshort* ) = extension_funcs[EXT_glVertexAttrib3sv];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib3sv( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib3svARB( GLuint index, GLshort* v ) {
  void (*func_glVertexAttrib3svARB)( GLuint, GLshort* ) = extension_funcs[EXT_glVertexAttrib3svARB];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib3svARB( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib3svNV( GLuint index, GLshort* v ) {
  void (*func_glVertexAttrib3svNV)( GLuint, GLshort* ) = extension_funcs[EXT_glVertexAttrib3svNV];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib3svNV( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4Nbv( GLuint index, GLbyte* v ) {
  void (*func_glVertexAttrib4Nbv)( GLuint, GLbyte* ) = extension_funcs[EXT_glVertexAttrib4Nbv];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib4Nbv( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4NbvARB( GLuint index, GLbyte* v ) {
  void (*func_glVertexAttrib4NbvARB)( GLuint, GLbyte* ) = extension_funcs[EXT_glVertexAttrib4NbvARB];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib4NbvARB( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4Niv( GLuint index, GLint* v ) {
  void (*func_glVertexAttrib4Niv)( GLuint, GLint* ) = extension_funcs[EXT_glVertexAttrib4Niv];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib4Niv( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4NivARB( GLuint index, GLint* v ) {
  void (*func_glVertexAttrib4NivARB)( GLuint, GLint* ) = extension_funcs[EXT_glVertexAttrib4NivARB];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib4NivARB( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4Nsv( GLuint index, GLshort* v ) {
  void (*func_glVertexAttrib4Nsv)( GLuint, GLshort* ) = extension_funcs[EXT_glVertexAttrib4Nsv];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib4Nsv( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4NsvARB( GLuint index, GLshort* v ) {
  void (*func_glVertexAttrib4NsvARB)( GLuint, GLshort* ) = extension_funcs[EXT_glVertexAttrib4NsvARB];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib4NsvARB( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4Nub( GLuint index, GLubyte x, GLubyte y, GLubyte z, GLubyte w ) {
  void (*func_glVertexAttrib4Nub)( GLuint, GLubyte, GLubyte, GLubyte, GLubyte ) = extension_funcs[EXT_glVertexAttrib4Nub];
  TRACE("(%d, %d, %d, %d, %d)\n", index, x, y, z, w );
  ENTER_GL();
  func_glVertexAttrib4Nub( index, x, y, z, w );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4NubARB( GLuint index, GLubyte x, GLubyte y, GLubyte z, GLubyte w ) {
  void (*func_glVertexAttrib4NubARB)( GLuint, GLubyte, GLubyte, GLubyte, GLubyte ) = extension_funcs[EXT_glVertexAttrib4NubARB];
  TRACE("(%d, %d, %d, %d, %d)\n", index, x, y, z, w );
  ENTER_GL();
  func_glVertexAttrib4NubARB( index, x, y, z, w );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4Nubv( GLuint index, GLubyte* v ) {
  void (*func_glVertexAttrib4Nubv)( GLuint, GLubyte* ) = extension_funcs[EXT_glVertexAttrib4Nubv];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib4Nubv( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4NubvARB( GLuint index, GLubyte* v ) {
  void (*func_glVertexAttrib4NubvARB)( GLuint, GLubyte* ) = extension_funcs[EXT_glVertexAttrib4NubvARB];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib4NubvARB( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4Nuiv( GLuint index, GLuint* v ) {
  void (*func_glVertexAttrib4Nuiv)( GLuint, GLuint* ) = extension_funcs[EXT_glVertexAttrib4Nuiv];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib4Nuiv( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4NuivARB( GLuint index, GLuint* v ) {
  void (*func_glVertexAttrib4NuivARB)( GLuint, GLuint* ) = extension_funcs[EXT_glVertexAttrib4NuivARB];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib4NuivARB( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4Nusv( GLuint index, GLushort* v ) {
  void (*func_glVertexAttrib4Nusv)( GLuint, GLushort* ) = extension_funcs[EXT_glVertexAttrib4Nusv];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib4Nusv( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4NusvARB( GLuint index, GLushort* v ) {
  void (*func_glVertexAttrib4NusvARB)( GLuint, GLushort* ) = extension_funcs[EXT_glVertexAttrib4NusvARB];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib4NusvARB( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4bv( GLuint index, GLbyte* v ) {
  void (*func_glVertexAttrib4bv)( GLuint, GLbyte* ) = extension_funcs[EXT_glVertexAttrib4bv];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib4bv( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4bvARB( GLuint index, GLbyte* v ) {
  void (*func_glVertexAttrib4bvARB)( GLuint, GLbyte* ) = extension_funcs[EXT_glVertexAttrib4bvARB];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib4bvARB( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4d( GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w ) {
  void (*func_glVertexAttrib4d)( GLuint, GLdouble, GLdouble, GLdouble, GLdouble ) = extension_funcs[EXT_glVertexAttrib4d];
  TRACE("(%d, %f, %f, %f, %f)\n", index, x, y, z, w );
  ENTER_GL();
  func_glVertexAttrib4d( index, x, y, z, w );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4dARB( GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w ) {
  void (*func_glVertexAttrib4dARB)( GLuint, GLdouble, GLdouble, GLdouble, GLdouble ) = extension_funcs[EXT_glVertexAttrib4dARB];
  TRACE("(%d, %f, %f, %f, %f)\n", index, x, y, z, w );
  ENTER_GL();
  func_glVertexAttrib4dARB( index, x, y, z, w );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4dNV( GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w ) {
  void (*func_glVertexAttrib4dNV)( GLuint, GLdouble, GLdouble, GLdouble, GLdouble ) = extension_funcs[EXT_glVertexAttrib4dNV];
  TRACE("(%d, %f, %f, %f, %f)\n", index, x, y, z, w );
  ENTER_GL();
  func_glVertexAttrib4dNV( index, x, y, z, w );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4dv( GLuint index, GLdouble* v ) {
  void (*func_glVertexAttrib4dv)( GLuint, GLdouble* ) = extension_funcs[EXT_glVertexAttrib4dv];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib4dv( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4dvARB( GLuint index, GLdouble* v ) {
  void (*func_glVertexAttrib4dvARB)( GLuint, GLdouble* ) = extension_funcs[EXT_glVertexAttrib4dvARB];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib4dvARB( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4dvNV( GLuint index, GLdouble* v ) {
  void (*func_glVertexAttrib4dvNV)( GLuint, GLdouble* ) = extension_funcs[EXT_glVertexAttrib4dvNV];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib4dvNV( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4f( GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w ) {
  void (*func_glVertexAttrib4f)( GLuint, GLfloat, GLfloat, GLfloat, GLfloat ) = extension_funcs[EXT_glVertexAttrib4f];
  TRACE("(%d, %f, %f, %f, %f)\n", index, x, y, z, w );
  ENTER_GL();
  func_glVertexAttrib4f( index, x, y, z, w );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4fARB( GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w ) {
  void (*func_glVertexAttrib4fARB)( GLuint, GLfloat, GLfloat, GLfloat, GLfloat ) = extension_funcs[EXT_glVertexAttrib4fARB];
  TRACE("(%d, %f, %f, %f, %f)\n", index, x, y, z, w );
  ENTER_GL();
  func_glVertexAttrib4fARB( index, x, y, z, w );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4fNV( GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w ) {
  void (*func_glVertexAttrib4fNV)( GLuint, GLfloat, GLfloat, GLfloat, GLfloat ) = extension_funcs[EXT_glVertexAttrib4fNV];
  TRACE("(%d, %f, %f, %f, %f)\n", index, x, y, z, w );
  ENTER_GL();
  func_glVertexAttrib4fNV( index, x, y, z, w );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4fv( GLuint index, GLfloat* v ) {
  void (*func_glVertexAttrib4fv)( GLuint, GLfloat* ) = extension_funcs[EXT_glVertexAttrib4fv];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib4fv( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4fvARB( GLuint index, GLfloat* v ) {
  void (*func_glVertexAttrib4fvARB)( GLuint, GLfloat* ) = extension_funcs[EXT_glVertexAttrib4fvARB];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib4fvARB( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4fvNV( GLuint index, GLfloat* v ) {
  void (*func_glVertexAttrib4fvNV)( GLuint, GLfloat* ) = extension_funcs[EXT_glVertexAttrib4fvNV];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib4fvNV( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4hNV( GLuint index, unsigned short x, unsigned short y, unsigned short z, unsigned short w ) {
  void (*func_glVertexAttrib4hNV)( GLuint, unsigned short, unsigned short, unsigned short, unsigned short ) = extension_funcs[EXT_glVertexAttrib4hNV];
  TRACE("(%d, %d, %d, %d, %d)\n", index, x, y, z, w );
  ENTER_GL();
  func_glVertexAttrib4hNV( index, x, y, z, w );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4hvNV( GLuint index, unsigned short* v ) {
  void (*func_glVertexAttrib4hvNV)( GLuint, unsigned short* ) = extension_funcs[EXT_glVertexAttrib4hvNV];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib4hvNV( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4iv( GLuint index, GLint* v ) {
  void (*func_glVertexAttrib4iv)( GLuint, GLint* ) = extension_funcs[EXT_glVertexAttrib4iv];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib4iv( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4ivARB( GLuint index, GLint* v ) {
  void (*func_glVertexAttrib4ivARB)( GLuint, GLint* ) = extension_funcs[EXT_glVertexAttrib4ivARB];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib4ivARB( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4s( GLuint index, GLshort x, GLshort y, GLshort z, GLshort w ) {
  void (*func_glVertexAttrib4s)( GLuint, GLshort, GLshort, GLshort, GLshort ) = extension_funcs[EXT_glVertexAttrib4s];
  TRACE("(%d, %d, %d, %d, %d)\n", index, x, y, z, w );
  ENTER_GL();
  func_glVertexAttrib4s( index, x, y, z, w );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4sARB( GLuint index, GLshort x, GLshort y, GLshort z, GLshort w ) {
  void (*func_glVertexAttrib4sARB)( GLuint, GLshort, GLshort, GLshort, GLshort ) = extension_funcs[EXT_glVertexAttrib4sARB];
  TRACE("(%d, %d, %d, %d, %d)\n", index, x, y, z, w );
  ENTER_GL();
  func_glVertexAttrib4sARB( index, x, y, z, w );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4sNV( GLuint index, GLshort x, GLshort y, GLshort z, GLshort w ) {
  void (*func_glVertexAttrib4sNV)( GLuint, GLshort, GLshort, GLshort, GLshort ) = extension_funcs[EXT_glVertexAttrib4sNV];
  TRACE("(%d, %d, %d, %d, %d)\n", index, x, y, z, w );
  ENTER_GL();
  func_glVertexAttrib4sNV( index, x, y, z, w );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4sv( GLuint index, GLshort* v ) {
  void (*func_glVertexAttrib4sv)( GLuint, GLshort* ) = extension_funcs[EXT_glVertexAttrib4sv];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib4sv( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4svARB( GLuint index, GLshort* v ) {
  void (*func_glVertexAttrib4svARB)( GLuint, GLshort* ) = extension_funcs[EXT_glVertexAttrib4svARB];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib4svARB( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4svNV( GLuint index, GLshort* v ) {
  void (*func_glVertexAttrib4svNV)( GLuint, GLshort* ) = extension_funcs[EXT_glVertexAttrib4svNV];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib4svNV( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4ubNV( GLuint index, GLubyte x, GLubyte y, GLubyte z, GLubyte w ) {
  void (*func_glVertexAttrib4ubNV)( GLuint, GLubyte, GLubyte, GLubyte, GLubyte ) = extension_funcs[EXT_glVertexAttrib4ubNV];
  TRACE("(%d, %d, %d, %d, %d)\n", index, x, y, z, w );
  ENTER_GL();
  func_glVertexAttrib4ubNV( index, x, y, z, w );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4ubv( GLuint index, GLubyte* v ) {
  void (*func_glVertexAttrib4ubv)( GLuint, GLubyte* ) = extension_funcs[EXT_glVertexAttrib4ubv];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib4ubv( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4ubvARB( GLuint index, GLubyte* v ) {
  void (*func_glVertexAttrib4ubvARB)( GLuint, GLubyte* ) = extension_funcs[EXT_glVertexAttrib4ubvARB];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib4ubvARB( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4ubvNV( GLuint index, GLubyte* v ) {
  void (*func_glVertexAttrib4ubvNV)( GLuint, GLubyte* ) = extension_funcs[EXT_glVertexAttrib4ubvNV];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib4ubvNV( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4uiv( GLuint index, GLuint* v ) {
  void (*func_glVertexAttrib4uiv)( GLuint, GLuint* ) = extension_funcs[EXT_glVertexAttrib4uiv];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib4uiv( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4uivARB( GLuint index, GLuint* v ) {
  void (*func_glVertexAttrib4uivARB)( GLuint, GLuint* ) = extension_funcs[EXT_glVertexAttrib4uivARB];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib4uivARB( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4usv( GLuint index, GLushort* v ) {
  void (*func_glVertexAttrib4usv)( GLuint, GLushort* ) = extension_funcs[EXT_glVertexAttrib4usv];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib4usv( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttrib4usvARB( GLuint index, GLushort* v ) {
  void (*func_glVertexAttrib4usvARB)( GLuint, GLushort* ) = extension_funcs[EXT_glVertexAttrib4usvARB];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttrib4usvARB( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribArrayObjectATI( GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, GLuint buffer, GLuint offset ) {
  void (*func_glVertexAttribArrayObjectATI)( GLuint, GLint, GLenum, GLboolean, GLsizei, GLuint, GLuint ) = extension_funcs[EXT_glVertexAttribArrayObjectATI];
  TRACE("(%d, %d, %d, %d, %d, %d, %d)\n", index, size, type, normalized, stride, buffer, offset );
  ENTER_GL();
  func_glVertexAttribArrayObjectATI( index, size, type, normalized, stride, buffer, offset );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribDivisor( GLuint index, GLuint divisor ) {
  void (*func_glVertexAttribDivisor)( GLuint, GLuint ) = extension_funcs[EXT_glVertexAttribDivisor];
  TRACE("(%d, %d)\n", index, divisor );
  ENTER_GL();
  func_glVertexAttribDivisor( index, divisor );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribDivisorARB( GLuint index, GLuint divisor ) {
  void (*func_glVertexAttribDivisorARB)( GLuint, GLuint ) = extension_funcs[EXT_glVertexAttribDivisorARB];
  TRACE("(%d, %d)\n", index, divisor );
  ENTER_GL();
  func_glVertexAttribDivisorARB( index, divisor );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribFormatNV( GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride ) {
  void (*func_glVertexAttribFormatNV)( GLuint, GLint, GLenum, GLboolean, GLsizei ) = extension_funcs[EXT_glVertexAttribFormatNV];
  TRACE("(%d, %d, %d, %d, %d)\n", index, size, type, normalized, stride );
  ENTER_GL();
  func_glVertexAttribFormatNV( index, size, type, normalized, stride );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribI1i( GLuint index, GLint x ) {
  void (*func_glVertexAttribI1i)( GLuint, GLint ) = extension_funcs[EXT_glVertexAttribI1i];
  TRACE("(%d, %d)\n", index, x );
  ENTER_GL();
  func_glVertexAttribI1i( index, x );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribI1iEXT( GLuint index, GLint x ) {
  void (*func_glVertexAttribI1iEXT)( GLuint, GLint ) = extension_funcs[EXT_glVertexAttribI1iEXT];
  TRACE("(%d, %d)\n", index, x );
  ENTER_GL();
  func_glVertexAttribI1iEXT( index, x );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribI1iv( GLuint index, GLint* v ) {
  void (*func_glVertexAttribI1iv)( GLuint, GLint* ) = extension_funcs[EXT_glVertexAttribI1iv];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttribI1iv( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribI1ivEXT( GLuint index, GLint* v ) {
  void (*func_glVertexAttribI1ivEXT)( GLuint, GLint* ) = extension_funcs[EXT_glVertexAttribI1ivEXT];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttribI1ivEXT( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribI1ui( GLuint index, GLuint x ) {
  void (*func_glVertexAttribI1ui)( GLuint, GLuint ) = extension_funcs[EXT_glVertexAttribI1ui];
  TRACE("(%d, %d)\n", index, x );
  ENTER_GL();
  func_glVertexAttribI1ui( index, x );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribI1uiEXT( GLuint index, GLuint x ) {
  void (*func_glVertexAttribI1uiEXT)( GLuint, GLuint ) = extension_funcs[EXT_glVertexAttribI1uiEXT];
  TRACE("(%d, %d)\n", index, x );
  ENTER_GL();
  func_glVertexAttribI1uiEXT( index, x );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribI1uiv( GLuint index, GLuint* v ) {
  void (*func_glVertexAttribI1uiv)( GLuint, GLuint* ) = extension_funcs[EXT_glVertexAttribI1uiv];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttribI1uiv( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribI1uivEXT( GLuint index, GLuint* v ) {
  void (*func_glVertexAttribI1uivEXT)( GLuint, GLuint* ) = extension_funcs[EXT_glVertexAttribI1uivEXT];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttribI1uivEXT( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribI2i( GLuint index, GLint x, GLint y ) {
  void (*func_glVertexAttribI2i)( GLuint, GLint, GLint ) = extension_funcs[EXT_glVertexAttribI2i];
  TRACE("(%d, %d, %d)\n", index, x, y );
  ENTER_GL();
  func_glVertexAttribI2i( index, x, y );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribI2iEXT( GLuint index, GLint x, GLint y ) {
  void (*func_glVertexAttribI2iEXT)( GLuint, GLint, GLint ) = extension_funcs[EXT_glVertexAttribI2iEXT];
  TRACE("(%d, %d, %d)\n", index, x, y );
  ENTER_GL();
  func_glVertexAttribI2iEXT( index, x, y );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribI2iv( GLuint index, GLint* v ) {
  void (*func_glVertexAttribI2iv)( GLuint, GLint* ) = extension_funcs[EXT_glVertexAttribI2iv];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttribI2iv( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribI2ivEXT( GLuint index, GLint* v ) {
  void (*func_glVertexAttribI2ivEXT)( GLuint, GLint* ) = extension_funcs[EXT_glVertexAttribI2ivEXT];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttribI2ivEXT( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribI2ui( GLuint index, GLuint x, GLuint y ) {
  void (*func_glVertexAttribI2ui)( GLuint, GLuint, GLuint ) = extension_funcs[EXT_glVertexAttribI2ui];
  TRACE("(%d, %d, %d)\n", index, x, y );
  ENTER_GL();
  func_glVertexAttribI2ui( index, x, y );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribI2uiEXT( GLuint index, GLuint x, GLuint y ) {
  void (*func_glVertexAttribI2uiEXT)( GLuint, GLuint, GLuint ) = extension_funcs[EXT_glVertexAttribI2uiEXT];
  TRACE("(%d, %d, %d)\n", index, x, y );
  ENTER_GL();
  func_glVertexAttribI2uiEXT( index, x, y );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribI2uiv( GLuint index, GLuint* v ) {
  void (*func_glVertexAttribI2uiv)( GLuint, GLuint* ) = extension_funcs[EXT_glVertexAttribI2uiv];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttribI2uiv( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribI2uivEXT( GLuint index, GLuint* v ) {
  void (*func_glVertexAttribI2uivEXT)( GLuint, GLuint* ) = extension_funcs[EXT_glVertexAttribI2uivEXT];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttribI2uivEXT( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribI3i( GLuint index, GLint x, GLint y, GLint z ) {
  void (*func_glVertexAttribI3i)( GLuint, GLint, GLint, GLint ) = extension_funcs[EXT_glVertexAttribI3i];
  TRACE("(%d, %d, %d, %d)\n", index, x, y, z );
  ENTER_GL();
  func_glVertexAttribI3i( index, x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribI3iEXT( GLuint index, GLint x, GLint y, GLint z ) {
  void (*func_glVertexAttribI3iEXT)( GLuint, GLint, GLint, GLint ) = extension_funcs[EXT_glVertexAttribI3iEXT];
  TRACE("(%d, %d, %d, %d)\n", index, x, y, z );
  ENTER_GL();
  func_glVertexAttribI3iEXT( index, x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribI3iv( GLuint index, GLint* v ) {
  void (*func_glVertexAttribI3iv)( GLuint, GLint* ) = extension_funcs[EXT_glVertexAttribI3iv];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttribI3iv( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribI3ivEXT( GLuint index, GLint* v ) {
  void (*func_glVertexAttribI3ivEXT)( GLuint, GLint* ) = extension_funcs[EXT_glVertexAttribI3ivEXT];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttribI3ivEXT( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribI3ui( GLuint index, GLuint x, GLuint y, GLuint z ) {
  void (*func_glVertexAttribI3ui)( GLuint, GLuint, GLuint, GLuint ) = extension_funcs[EXT_glVertexAttribI3ui];
  TRACE("(%d, %d, %d, %d)\n", index, x, y, z );
  ENTER_GL();
  func_glVertexAttribI3ui( index, x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribI3uiEXT( GLuint index, GLuint x, GLuint y, GLuint z ) {
  void (*func_glVertexAttribI3uiEXT)( GLuint, GLuint, GLuint, GLuint ) = extension_funcs[EXT_glVertexAttribI3uiEXT];
  TRACE("(%d, %d, %d, %d)\n", index, x, y, z );
  ENTER_GL();
  func_glVertexAttribI3uiEXT( index, x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribI3uiv( GLuint index, GLuint* v ) {
  void (*func_glVertexAttribI3uiv)( GLuint, GLuint* ) = extension_funcs[EXT_glVertexAttribI3uiv];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttribI3uiv( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribI3uivEXT( GLuint index, GLuint* v ) {
  void (*func_glVertexAttribI3uivEXT)( GLuint, GLuint* ) = extension_funcs[EXT_glVertexAttribI3uivEXT];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttribI3uivEXT( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribI4bv( GLuint index, GLbyte* v ) {
  void (*func_glVertexAttribI4bv)( GLuint, GLbyte* ) = extension_funcs[EXT_glVertexAttribI4bv];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttribI4bv( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribI4bvEXT( GLuint index, GLbyte* v ) {
  void (*func_glVertexAttribI4bvEXT)( GLuint, GLbyte* ) = extension_funcs[EXT_glVertexAttribI4bvEXT];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttribI4bvEXT( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribI4i( GLuint index, GLint x, GLint y, GLint z, GLint w ) {
  void (*func_glVertexAttribI4i)( GLuint, GLint, GLint, GLint, GLint ) = extension_funcs[EXT_glVertexAttribI4i];
  TRACE("(%d, %d, %d, %d, %d)\n", index, x, y, z, w );
  ENTER_GL();
  func_glVertexAttribI4i( index, x, y, z, w );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribI4iEXT( GLuint index, GLint x, GLint y, GLint z, GLint w ) {
  void (*func_glVertexAttribI4iEXT)( GLuint, GLint, GLint, GLint, GLint ) = extension_funcs[EXT_glVertexAttribI4iEXT];
  TRACE("(%d, %d, %d, %d, %d)\n", index, x, y, z, w );
  ENTER_GL();
  func_glVertexAttribI4iEXT( index, x, y, z, w );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribI4iv( GLuint index, GLint* v ) {
  void (*func_glVertexAttribI4iv)( GLuint, GLint* ) = extension_funcs[EXT_glVertexAttribI4iv];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttribI4iv( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribI4ivEXT( GLuint index, GLint* v ) {
  void (*func_glVertexAttribI4ivEXT)( GLuint, GLint* ) = extension_funcs[EXT_glVertexAttribI4ivEXT];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttribI4ivEXT( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribI4sv( GLuint index, GLshort* v ) {
  void (*func_glVertexAttribI4sv)( GLuint, GLshort* ) = extension_funcs[EXT_glVertexAttribI4sv];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttribI4sv( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribI4svEXT( GLuint index, GLshort* v ) {
  void (*func_glVertexAttribI4svEXT)( GLuint, GLshort* ) = extension_funcs[EXT_glVertexAttribI4svEXT];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttribI4svEXT( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribI4ubv( GLuint index, GLubyte* v ) {
  void (*func_glVertexAttribI4ubv)( GLuint, GLubyte* ) = extension_funcs[EXT_glVertexAttribI4ubv];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttribI4ubv( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribI4ubvEXT( GLuint index, GLubyte* v ) {
  void (*func_glVertexAttribI4ubvEXT)( GLuint, GLubyte* ) = extension_funcs[EXT_glVertexAttribI4ubvEXT];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttribI4ubvEXT( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribI4ui( GLuint index, GLuint x, GLuint y, GLuint z, GLuint w ) {
  void (*func_glVertexAttribI4ui)( GLuint, GLuint, GLuint, GLuint, GLuint ) = extension_funcs[EXT_glVertexAttribI4ui];
  TRACE("(%d, %d, %d, %d, %d)\n", index, x, y, z, w );
  ENTER_GL();
  func_glVertexAttribI4ui( index, x, y, z, w );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribI4uiEXT( GLuint index, GLuint x, GLuint y, GLuint z, GLuint w ) {
  void (*func_glVertexAttribI4uiEXT)( GLuint, GLuint, GLuint, GLuint, GLuint ) = extension_funcs[EXT_glVertexAttribI4uiEXT];
  TRACE("(%d, %d, %d, %d, %d)\n", index, x, y, z, w );
  ENTER_GL();
  func_glVertexAttribI4uiEXT( index, x, y, z, w );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribI4uiv( GLuint index, GLuint* v ) {
  void (*func_glVertexAttribI4uiv)( GLuint, GLuint* ) = extension_funcs[EXT_glVertexAttribI4uiv];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttribI4uiv( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribI4uivEXT( GLuint index, GLuint* v ) {
  void (*func_glVertexAttribI4uivEXT)( GLuint, GLuint* ) = extension_funcs[EXT_glVertexAttribI4uivEXT];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttribI4uivEXT( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribI4usv( GLuint index, GLushort* v ) {
  void (*func_glVertexAttribI4usv)( GLuint, GLushort* ) = extension_funcs[EXT_glVertexAttribI4usv];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttribI4usv( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribI4usvEXT( GLuint index, GLushort* v ) {
  void (*func_glVertexAttribI4usvEXT)( GLuint, GLushort* ) = extension_funcs[EXT_glVertexAttribI4usvEXT];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttribI4usvEXT( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribIFormatNV( GLuint index, GLint size, GLenum type, GLsizei stride ) {
  void (*func_glVertexAttribIFormatNV)( GLuint, GLint, GLenum, GLsizei ) = extension_funcs[EXT_glVertexAttribIFormatNV];
  TRACE("(%d, %d, %d, %d)\n", index, size, type, stride );
  ENTER_GL();
  func_glVertexAttribIFormatNV( index, size, type, stride );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribIPointer( GLuint index, GLint size, GLenum type, GLsizei stride, GLvoid* pointer ) {
  void (*func_glVertexAttribIPointer)( GLuint, GLint, GLenum, GLsizei, GLvoid* ) = extension_funcs[EXT_glVertexAttribIPointer];
  TRACE("(%d, %d, %d, %d, %p)\n", index, size, type, stride, pointer );
  ENTER_GL();
  func_glVertexAttribIPointer( index, size, type, stride, pointer );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribIPointerEXT( GLuint index, GLint size, GLenum type, GLsizei stride, GLvoid* pointer ) {
  void (*func_glVertexAttribIPointerEXT)( GLuint, GLint, GLenum, GLsizei, GLvoid* ) = extension_funcs[EXT_glVertexAttribIPointerEXT];
  TRACE("(%d, %d, %d, %d, %p)\n", index, size, type, stride, pointer );
  ENTER_GL();
  func_glVertexAttribIPointerEXT( index, size, type, stride, pointer );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribL1d( GLuint index, GLdouble x ) {
  void (*func_glVertexAttribL1d)( GLuint, GLdouble ) = extension_funcs[EXT_glVertexAttribL1d];
  TRACE("(%d, %f)\n", index, x );
  ENTER_GL();
  func_glVertexAttribL1d( index, x );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribL1dEXT( GLuint index, GLdouble x ) {
  void (*func_glVertexAttribL1dEXT)( GLuint, GLdouble ) = extension_funcs[EXT_glVertexAttribL1dEXT];
  TRACE("(%d, %f)\n", index, x );
  ENTER_GL();
  func_glVertexAttribL1dEXT( index, x );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribL1dv( GLuint index, GLdouble* v ) {
  void (*func_glVertexAttribL1dv)( GLuint, GLdouble* ) = extension_funcs[EXT_glVertexAttribL1dv];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttribL1dv( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribL1dvEXT( GLuint index, GLdouble* v ) {
  void (*func_glVertexAttribL1dvEXT)( GLuint, GLdouble* ) = extension_funcs[EXT_glVertexAttribL1dvEXT];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttribL1dvEXT( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribL1i64NV( GLuint index, INT64 x ) {
  void (*func_glVertexAttribL1i64NV)( GLuint, INT64 ) = extension_funcs[EXT_glVertexAttribL1i64NV];
  TRACE("(%d, %s)\n", index, wine_dbgstr_longlong(x) );
  ENTER_GL();
  func_glVertexAttribL1i64NV( index, x );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribL1i64vNV( GLuint index, INT64* v ) {
  void (*func_glVertexAttribL1i64vNV)( GLuint, INT64* ) = extension_funcs[EXT_glVertexAttribL1i64vNV];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttribL1i64vNV( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribL1ui64NV( GLuint index, UINT64 x ) {
  void (*func_glVertexAttribL1ui64NV)( GLuint, UINT64 ) = extension_funcs[EXT_glVertexAttribL1ui64NV];
  TRACE("(%d, %s)\n", index, wine_dbgstr_longlong(x) );
  ENTER_GL();
  func_glVertexAttribL1ui64NV( index, x );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribL1ui64vNV( GLuint index, UINT64* v ) {
  void (*func_glVertexAttribL1ui64vNV)( GLuint, UINT64* ) = extension_funcs[EXT_glVertexAttribL1ui64vNV];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttribL1ui64vNV( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribL2d( GLuint index, GLdouble x, GLdouble y ) {
  void (*func_glVertexAttribL2d)( GLuint, GLdouble, GLdouble ) = extension_funcs[EXT_glVertexAttribL2d];
  TRACE("(%d, %f, %f)\n", index, x, y );
  ENTER_GL();
  func_glVertexAttribL2d( index, x, y );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribL2dEXT( GLuint index, GLdouble x, GLdouble y ) {
  void (*func_glVertexAttribL2dEXT)( GLuint, GLdouble, GLdouble ) = extension_funcs[EXT_glVertexAttribL2dEXT];
  TRACE("(%d, %f, %f)\n", index, x, y );
  ENTER_GL();
  func_glVertexAttribL2dEXT( index, x, y );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribL2dv( GLuint index, GLdouble* v ) {
  void (*func_glVertexAttribL2dv)( GLuint, GLdouble* ) = extension_funcs[EXT_glVertexAttribL2dv];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttribL2dv( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribL2dvEXT( GLuint index, GLdouble* v ) {
  void (*func_glVertexAttribL2dvEXT)( GLuint, GLdouble* ) = extension_funcs[EXT_glVertexAttribL2dvEXT];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttribL2dvEXT( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribL2i64NV( GLuint index, INT64 x, INT64 y ) {
  void (*func_glVertexAttribL2i64NV)( GLuint, INT64, INT64 ) = extension_funcs[EXT_glVertexAttribL2i64NV];
  TRACE("(%d, %s, %s)\n", index, wine_dbgstr_longlong(x), wine_dbgstr_longlong(y) );
  ENTER_GL();
  func_glVertexAttribL2i64NV( index, x, y );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribL2i64vNV( GLuint index, INT64* v ) {
  void (*func_glVertexAttribL2i64vNV)( GLuint, INT64* ) = extension_funcs[EXT_glVertexAttribL2i64vNV];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttribL2i64vNV( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribL2ui64NV( GLuint index, UINT64 x, UINT64 y ) {
  void (*func_glVertexAttribL2ui64NV)( GLuint, UINT64, UINT64 ) = extension_funcs[EXT_glVertexAttribL2ui64NV];
  TRACE("(%d, %s, %s)\n", index, wine_dbgstr_longlong(x), wine_dbgstr_longlong(y) );
  ENTER_GL();
  func_glVertexAttribL2ui64NV( index, x, y );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribL2ui64vNV( GLuint index, UINT64* v ) {
  void (*func_glVertexAttribL2ui64vNV)( GLuint, UINT64* ) = extension_funcs[EXT_glVertexAttribL2ui64vNV];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttribL2ui64vNV( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribL3d( GLuint index, GLdouble x, GLdouble y, GLdouble z ) {
  void (*func_glVertexAttribL3d)( GLuint, GLdouble, GLdouble, GLdouble ) = extension_funcs[EXT_glVertexAttribL3d];
  TRACE("(%d, %f, %f, %f)\n", index, x, y, z );
  ENTER_GL();
  func_glVertexAttribL3d( index, x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribL3dEXT( GLuint index, GLdouble x, GLdouble y, GLdouble z ) {
  void (*func_glVertexAttribL3dEXT)( GLuint, GLdouble, GLdouble, GLdouble ) = extension_funcs[EXT_glVertexAttribL3dEXT];
  TRACE("(%d, %f, %f, %f)\n", index, x, y, z );
  ENTER_GL();
  func_glVertexAttribL3dEXT( index, x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribL3dv( GLuint index, GLdouble* v ) {
  void (*func_glVertexAttribL3dv)( GLuint, GLdouble* ) = extension_funcs[EXT_glVertexAttribL3dv];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttribL3dv( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribL3dvEXT( GLuint index, GLdouble* v ) {
  void (*func_glVertexAttribL3dvEXT)( GLuint, GLdouble* ) = extension_funcs[EXT_glVertexAttribL3dvEXT];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttribL3dvEXT( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribL3i64NV( GLuint index, INT64 x, INT64 y, INT64 z ) {
  void (*func_glVertexAttribL3i64NV)( GLuint, INT64, INT64, INT64 ) = extension_funcs[EXT_glVertexAttribL3i64NV];
  TRACE("(%d, %s, %s, %s)\n", index, wine_dbgstr_longlong(x), wine_dbgstr_longlong(y), wine_dbgstr_longlong(z) );
  ENTER_GL();
  func_glVertexAttribL3i64NV( index, x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribL3i64vNV( GLuint index, INT64* v ) {
  void (*func_glVertexAttribL3i64vNV)( GLuint, INT64* ) = extension_funcs[EXT_glVertexAttribL3i64vNV];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttribL3i64vNV( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribL3ui64NV( GLuint index, UINT64 x, UINT64 y, UINT64 z ) {
  void (*func_glVertexAttribL3ui64NV)( GLuint, UINT64, UINT64, UINT64 ) = extension_funcs[EXT_glVertexAttribL3ui64NV];
  TRACE("(%d, %s, %s, %s)\n", index, wine_dbgstr_longlong(x), wine_dbgstr_longlong(y), wine_dbgstr_longlong(z) );
  ENTER_GL();
  func_glVertexAttribL3ui64NV( index, x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribL3ui64vNV( GLuint index, UINT64* v ) {
  void (*func_glVertexAttribL3ui64vNV)( GLuint, UINT64* ) = extension_funcs[EXT_glVertexAttribL3ui64vNV];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttribL3ui64vNV( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribL4d( GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w ) {
  void (*func_glVertexAttribL4d)( GLuint, GLdouble, GLdouble, GLdouble, GLdouble ) = extension_funcs[EXT_glVertexAttribL4d];
  TRACE("(%d, %f, %f, %f, %f)\n", index, x, y, z, w );
  ENTER_GL();
  func_glVertexAttribL4d( index, x, y, z, w );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribL4dEXT( GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w ) {
  void (*func_glVertexAttribL4dEXT)( GLuint, GLdouble, GLdouble, GLdouble, GLdouble ) = extension_funcs[EXT_glVertexAttribL4dEXT];
  TRACE("(%d, %f, %f, %f, %f)\n", index, x, y, z, w );
  ENTER_GL();
  func_glVertexAttribL4dEXT( index, x, y, z, w );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribL4dv( GLuint index, GLdouble* v ) {
  void (*func_glVertexAttribL4dv)( GLuint, GLdouble* ) = extension_funcs[EXT_glVertexAttribL4dv];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttribL4dv( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribL4dvEXT( GLuint index, GLdouble* v ) {
  void (*func_glVertexAttribL4dvEXT)( GLuint, GLdouble* ) = extension_funcs[EXT_glVertexAttribL4dvEXT];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttribL4dvEXT( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribL4i64NV( GLuint index, INT64 x, INT64 y, INT64 z, INT64 w ) {
  void (*func_glVertexAttribL4i64NV)( GLuint, INT64, INT64, INT64, INT64 ) = extension_funcs[EXT_glVertexAttribL4i64NV];
  TRACE("(%d, %s, %s, %s, %s)\n", index, wine_dbgstr_longlong(x), wine_dbgstr_longlong(y), wine_dbgstr_longlong(z), wine_dbgstr_longlong(w) );
  ENTER_GL();
  func_glVertexAttribL4i64NV( index, x, y, z, w );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribL4i64vNV( GLuint index, INT64* v ) {
  void (*func_glVertexAttribL4i64vNV)( GLuint, INT64* ) = extension_funcs[EXT_glVertexAttribL4i64vNV];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttribL4i64vNV( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribL4ui64NV( GLuint index, UINT64 x, UINT64 y, UINT64 z, UINT64 w ) {
  void (*func_glVertexAttribL4ui64NV)( GLuint, UINT64, UINT64, UINT64, UINT64 ) = extension_funcs[EXT_glVertexAttribL4ui64NV];
  TRACE("(%d, %s, %s, %s, %s)\n", index, wine_dbgstr_longlong(x), wine_dbgstr_longlong(y), wine_dbgstr_longlong(z), wine_dbgstr_longlong(w) );
  ENTER_GL();
  func_glVertexAttribL4ui64NV( index, x, y, z, w );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribL4ui64vNV( GLuint index, UINT64* v ) {
  void (*func_glVertexAttribL4ui64vNV)( GLuint, UINT64* ) = extension_funcs[EXT_glVertexAttribL4ui64vNV];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glVertexAttribL4ui64vNV( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribLFormatNV( GLuint index, GLint size, GLenum type, GLsizei stride ) {
  void (*func_glVertexAttribLFormatNV)( GLuint, GLint, GLenum, GLsizei ) = extension_funcs[EXT_glVertexAttribLFormatNV];
  TRACE("(%d, %d, %d, %d)\n", index, size, type, stride );
  ENTER_GL();
  func_glVertexAttribLFormatNV( index, size, type, stride );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribLPointer( GLuint index, GLint size, GLenum type, GLsizei stride, GLvoid* pointer ) {
  void (*func_glVertexAttribLPointer)( GLuint, GLint, GLenum, GLsizei, GLvoid* ) = extension_funcs[EXT_glVertexAttribLPointer];
  TRACE("(%d, %d, %d, %d, %p)\n", index, size, type, stride, pointer );
  ENTER_GL();
  func_glVertexAttribLPointer( index, size, type, stride, pointer );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribLPointerEXT( GLuint index, GLint size, GLenum type, GLsizei stride, GLvoid* pointer ) {
  void (*func_glVertexAttribLPointerEXT)( GLuint, GLint, GLenum, GLsizei, GLvoid* ) = extension_funcs[EXT_glVertexAttribLPointerEXT];
  TRACE("(%d, %d, %d, %d, %p)\n", index, size, type, stride, pointer );
  ENTER_GL();
  func_glVertexAttribLPointerEXT( index, size, type, stride, pointer );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribP1ui( GLuint index, GLenum type, GLboolean normalized, GLuint value ) {
  void (*func_glVertexAttribP1ui)( GLuint, GLenum, GLboolean, GLuint ) = extension_funcs[EXT_glVertexAttribP1ui];
  TRACE("(%d, %d, %d, %d)\n", index, type, normalized, value );
  ENTER_GL();
  func_glVertexAttribP1ui( index, type, normalized, value );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribP1uiv( GLuint index, GLenum type, GLboolean normalized, GLuint* value ) {
  void (*func_glVertexAttribP1uiv)( GLuint, GLenum, GLboolean, GLuint* ) = extension_funcs[EXT_glVertexAttribP1uiv];
  TRACE("(%d, %d, %d, %p)\n", index, type, normalized, value );
  ENTER_GL();
  func_glVertexAttribP1uiv( index, type, normalized, value );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribP2ui( GLuint index, GLenum type, GLboolean normalized, GLuint value ) {
  void (*func_glVertexAttribP2ui)( GLuint, GLenum, GLboolean, GLuint ) = extension_funcs[EXT_glVertexAttribP2ui];
  TRACE("(%d, %d, %d, %d)\n", index, type, normalized, value );
  ENTER_GL();
  func_glVertexAttribP2ui( index, type, normalized, value );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribP2uiv( GLuint index, GLenum type, GLboolean normalized, GLuint* value ) {
  void (*func_glVertexAttribP2uiv)( GLuint, GLenum, GLboolean, GLuint* ) = extension_funcs[EXT_glVertexAttribP2uiv];
  TRACE("(%d, %d, %d, %p)\n", index, type, normalized, value );
  ENTER_GL();
  func_glVertexAttribP2uiv( index, type, normalized, value );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribP3ui( GLuint index, GLenum type, GLboolean normalized, GLuint value ) {
  void (*func_glVertexAttribP3ui)( GLuint, GLenum, GLboolean, GLuint ) = extension_funcs[EXT_glVertexAttribP3ui];
  TRACE("(%d, %d, %d, %d)\n", index, type, normalized, value );
  ENTER_GL();
  func_glVertexAttribP3ui( index, type, normalized, value );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribP3uiv( GLuint index, GLenum type, GLboolean normalized, GLuint* value ) {
  void (*func_glVertexAttribP3uiv)( GLuint, GLenum, GLboolean, GLuint* ) = extension_funcs[EXT_glVertexAttribP3uiv];
  TRACE("(%d, %d, %d, %p)\n", index, type, normalized, value );
  ENTER_GL();
  func_glVertexAttribP3uiv( index, type, normalized, value );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribP4ui( GLuint index, GLenum type, GLboolean normalized, GLuint value ) {
  void (*func_glVertexAttribP4ui)( GLuint, GLenum, GLboolean, GLuint ) = extension_funcs[EXT_glVertexAttribP4ui];
  TRACE("(%d, %d, %d, %d)\n", index, type, normalized, value );
  ENTER_GL();
  func_glVertexAttribP4ui( index, type, normalized, value );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribP4uiv( GLuint index, GLenum type, GLboolean normalized, GLuint* value ) {
  void (*func_glVertexAttribP4uiv)( GLuint, GLenum, GLboolean, GLuint* ) = extension_funcs[EXT_glVertexAttribP4uiv];
  TRACE("(%d, %d, %d, %p)\n", index, type, normalized, value );
  ENTER_GL();
  func_glVertexAttribP4uiv( index, type, normalized, value );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribPointer( GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, GLvoid* pointer ) {
  void (*func_glVertexAttribPointer)( GLuint, GLint, GLenum, GLboolean, GLsizei, GLvoid* ) = extension_funcs[EXT_glVertexAttribPointer];
  TRACE("(%d, %d, %d, %d, %d, %p)\n", index, size, type, normalized, stride, pointer );
  ENTER_GL();
  func_glVertexAttribPointer( index, size, type, normalized, stride, pointer );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribPointerARB( GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, GLvoid* pointer ) {
  void (*func_glVertexAttribPointerARB)( GLuint, GLint, GLenum, GLboolean, GLsizei, GLvoid* ) = extension_funcs[EXT_glVertexAttribPointerARB];
  TRACE("(%d, %d, %d, %d, %d, %p)\n", index, size, type, normalized, stride, pointer );
  ENTER_GL();
  func_glVertexAttribPointerARB( index, size, type, normalized, stride, pointer );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribPointerNV( GLuint index, GLint fsize, GLenum type, GLsizei stride, GLvoid* pointer ) {
  void (*func_glVertexAttribPointerNV)( GLuint, GLint, GLenum, GLsizei, GLvoid* ) = extension_funcs[EXT_glVertexAttribPointerNV];
  TRACE("(%d, %d, %d, %d, %p)\n", index, fsize, type, stride, pointer );
  ENTER_GL();
  func_glVertexAttribPointerNV( index, fsize, type, stride, pointer );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribs1dvNV( GLuint index, GLsizei count, GLdouble* v ) {
  void (*func_glVertexAttribs1dvNV)( GLuint, GLsizei, GLdouble* ) = extension_funcs[EXT_glVertexAttribs1dvNV];
  TRACE("(%d, %d, %p)\n", index, count, v );
  ENTER_GL();
  func_glVertexAttribs1dvNV( index, count, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribs1fvNV( GLuint index, GLsizei count, GLfloat* v ) {
  void (*func_glVertexAttribs1fvNV)( GLuint, GLsizei, GLfloat* ) = extension_funcs[EXT_glVertexAttribs1fvNV];
  TRACE("(%d, %d, %p)\n", index, count, v );
  ENTER_GL();
  func_glVertexAttribs1fvNV( index, count, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribs1hvNV( GLuint index, GLsizei n, unsigned short* v ) {
  void (*func_glVertexAttribs1hvNV)( GLuint, GLsizei, unsigned short* ) = extension_funcs[EXT_glVertexAttribs1hvNV];
  TRACE("(%d, %d, %p)\n", index, n, v );
  ENTER_GL();
  func_glVertexAttribs1hvNV( index, n, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribs1svNV( GLuint index, GLsizei count, GLshort* v ) {
  void (*func_glVertexAttribs1svNV)( GLuint, GLsizei, GLshort* ) = extension_funcs[EXT_glVertexAttribs1svNV];
  TRACE("(%d, %d, %p)\n", index, count, v );
  ENTER_GL();
  func_glVertexAttribs1svNV( index, count, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribs2dvNV( GLuint index, GLsizei count, GLdouble* v ) {
  void (*func_glVertexAttribs2dvNV)( GLuint, GLsizei, GLdouble* ) = extension_funcs[EXT_glVertexAttribs2dvNV];
  TRACE("(%d, %d, %p)\n", index, count, v );
  ENTER_GL();
  func_glVertexAttribs2dvNV( index, count, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribs2fvNV( GLuint index, GLsizei count, GLfloat* v ) {
  void (*func_glVertexAttribs2fvNV)( GLuint, GLsizei, GLfloat* ) = extension_funcs[EXT_glVertexAttribs2fvNV];
  TRACE("(%d, %d, %p)\n", index, count, v );
  ENTER_GL();
  func_glVertexAttribs2fvNV( index, count, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribs2hvNV( GLuint index, GLsizei n, unsigned short* v ) {
  void (*func_glVertexAttribs2hvNV)( GLuint, GLsizei, unsigned short* ) = extension_funcs[EXT_glVertexAttribs2hvNV];
  TRACE("(%d, %d, %p)\n", index, n, v );
  ENTER_GL();
  func_glVertexAttribs2hvNV( index, n, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribs2svNV( GLuint index, GLsizei count, GLshort* v ) {
  void (*func_glVertexAttribs2svNV)( GLuint, GLsizei, GLshort* ) = extension_funcs[EXT_glVertexAttribs2svNV];
  TRACE("(%d, %d, %p)\n", index, count, v );
  ENTER_GL();
  func_glVertexAttribs2svNV( index, count, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribs3dvNV( GLuint index, GLsizei count, GLdouble* v ) {
  void (*func_glVertexAttribs3dvNV)( GLuint, GLsizei, GLdouble* ) = extension_funcs[EXT_glVertexAttribs3dvNV];
  TRACE("(%d, %d, %p)\n", index, count, v );
  ENTER_GL();
  func_glVertexAttribs3dvNV( index, count, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribs3fvNV( GLuint index, GLsizei count, GLfloat* v ) {
  void (*func_glVertexAttribs3fvNV)( GLuint, GLsizei, GLfloat* ) = extension_funcs[EXT_glVertexAttribs3fvNV];
  TRACE("(%d, %d, %p)\n", index, count, v );
  ENTER_GL();
  func_glVertexAttribs3fvNV( index, count, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribs3hvNV( GLuint index, GLsizei n, unsigned short* v ) {
  void (*func_glVertexAttribs3hvNV)( GLuint, GLsizei, unsigned short* ) = extension_funcs[EXT_glVertexAttribs3hvNV];
  TRACE("(%d, %d, %p)\n", index, n, v );
  ENTER_GL();
  func_glVertexAttribs3hvNV( index, n, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribs3svNV( GLuint index, GLsizei count, GLshort* v ) {
  void (*func_glVertexAttribs3svNV)( GLuint, GLsizei, GLshort* ) = extension_funcs[EXT_glVertexAttribs3svNV];
  TRACE("(%d, %d, %p)\n", index, count, v );
  ENTER_GL();
  func_glVertexAttribs3svNV( index, count, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribs4dvNV( GLuint index, GLsizei count, GLdouble* v ) {
  void (*func_glVertexAttribs4dvNV)( GLuint, GLsizei, GLdouble* ) = extension_funcs[EXT_glVertexAttribs4dvNV];
  TRACE("(%d, %d, %p)\n", index, count, v );
  ENTER_GL();
  func_glVertexAttribs4dvNV( index, count, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribs4fvNV( GLuint index, GLsizei count, GLfloat* v ) {
  void (*func_glVertexAttribs4fvNV)( GLuint, GLsizei, GLfloat* ) = extension_funcs[EXT_glVertexAttribs4fvNV];
  TRACE("(%d, %d, %p)\n", index, count, v );
  ENTER_GL();
  func_glVertexAttribs4fvNV( index, count, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribs4hvNV( GLuint index, GLsizei n, unsigned short* v ) {
  void (*func_glVertexAttribs4hvNV)( GLuint, GLsizei, unsigned short* ) = extension_funcs[EXT_glVertexAttribs4hvNV];
  TRACE("(%d, %d, %p)\n", index, n, v );
  ENTER_GL();
  func_glVertexAttribs4hvNV( index, n, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribs4svNV( GLuint index, GLsizei count, GLshort* v ) {
  void (*func_glVertexAttribs4svNV)( GLuint, GLsizei, GLshort* ) = extension_funcs[EXT_glVertexAttribs4svNV];
  TRACE("(%d, %d, %p)\n", index, count, v );
  ENTER_GL();
  func_glVertexAttribs4svNV( index, count, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexAttribs4ubvNV( GLuint index, GLsizei count, GLubyte* v ) {
  void (*func_glVertexAttribs4ubvNV)( GLuint, GLsizei, GLubyte* ) = extension_funcs[EXT_glVertexAttribs4ubvNV];
  TRACE("(%d, %d, %p)\n", index, count, v );
  ENTER_GL();
  func_glVertexAttribs4ubvNV( index, count, v );
  LEAVE_GL();
}

static void WINAPI wine_glVertexBlendARB( GLint count ) {
  void (*func_glVertexBlendARB)( GLint ) = extension_funcs[EXT_glVertexBlendARB];
  TRACE("(%d)\n", count );
  ENTER_GL();
  func_glVertexBlendARB( count );
  LEAVE_GL();
}

static void WINAPI wine_glVertexBlendEnvfATI( GLenum pname, GLfloat param ) {
  void (*func_glVertexBlendEnvfATI)( GLenum, GLfloat ) = extension_funcs[EXT_glVertexBlendEnvfATI];
  TRACE("(%d, %f)\n", pname, param );
  ENTER_GL();
  func_glVertexBlendEnvfATI( pname, param );
  LEAVE_GL();
}

static void WINAPI wine_glVertexBlendEnviATI( GLenum pname, GLint param ) {
  void (*func_glVertexBlendEnviATI)( GLenum, GLint ) = extension_funcs[EXT_glVertexBlendEnviATI];
  TRACE("(%d, %d)\n", pname, param );
  ENTER_GL();
  func_glVertexBlendEnviATI( pname, param );
  LEAVE_GL();
}

static void WINAPI wine_glVertexFormatNV( GLint size, GLenum type, GLsizei stride ) {
  void (*func_glVertexFormatNV)( GLint, GLenum, GLsizei ) = extension_funcs[EXT_glVertexFormatNV];
  TRACE("(%d, %d, %d)\n", size, type, stride );
  ENTER_GL();
  func_glVertexFormatNV( size, type, stride );
  LEAVE_GL();
}

static void WINAPI wine_glVertexP2ui( GLenum type, GLuint value ) {
  void (*func_glVertexP2ui)( GLenum, GLuint ) = extension_funcs[EXT_glVertexP2ui];
  TRACE("(%d, %d)\n", type, value );
  ENTER_GL();
  func_glVertexP2ui( type, value );
  LEAVE_GL();
}

static void WINAPI wine_glVertexP2uiv( GLenum type, GLuint* value ) {
  void (*func_glVertexP2uiv)( GLenum, GLuint* ) = extension_funcs[EXT_glVertexP2uiv];
  TRACE("(%d, %p)\n", type, value );
  ENTER_GL();
  func_glVertexP2uiv( type, value );
  LEAVE_GL();
}

static void WINAPI wine_glVertexP3ui( GLenum type, GLuint value ) {
  void (*func_glVertexP3ui)( GLenum, GLuint ) = extension_funcs[EXT_glVertexP3ui];
  TRACE("(%d, %d)\n", type, value );
  ENTER_GL();
  func_glVertexP3ui( type, value );
  LEAVE_GL();
}

static void WINAPI wine_glVertexP3uiv( GLenum type, GLuint* value ) {
  void (*func_glVertexP3uiv)( GLenum, GLuint* ) = extension_funcs[EXT_glVertexP3uiv];
  TRACE("(%d, %p)\n", type, value );
  ENTER_GL();
  func_glVertexP3uiv( type, value );
  LEAVE_GL();
}

static void WINAPI wine_glVertexP4ui( GLenum type, GLuint value ) {
  void (*func_glVertexP4ui)( GLenum, GLuint ) = extension_funcs[EXT_glVertexP4ui];
  TRACE("(%d, %d)\n", type, value );
  ENTER_GL();
  func_glVertexP4ui( type, value );
  LEAVE_GL();
}

static void WINAPI wine_glVertexP4uiv( GLenum type, GLuint* value ) {
  void (*func_glVertexP4uiv)( GLenum, GLuint* ) = extension_funcs[EXT_glVertexP4uiv];
  TRACE("(%d, %p)\n", type, value );
  ENTER_GL();
  func_glVertexP4uiv( type, value );
  LEAVE_GL();
}

static void WINAPI wine_glVertexPointerEXT( GLint size, GLenum type, GLsizei stride, GLsizei count, GLvoid* pointer ) {
  void (*func_glVertexPointerEXT)( GLint, GLenum, GLsizei, GLsizei, GLvoid* ) = extension_funcs[EXT_glVertexPointerEXT];
  TRACE("(%d, %d, %d, %d, %p)\n", size, type, stride, count, pointer );
  ENTER_GL();
  func_glVertexPointerEXT( size, type, stride, count, pointer );
  LEAVE_GL();
}

static void WINAPI wine_glVertexPointerListIBM( GLint size, GLenum type, GLint stride, GLvoid** pointer, GLint ptrstride ) {
  void (*func_glVertexPointerListIBM)( GLint, GLenum, GLint, GLvoid**, GLint ) = extension_funcs[EXT_glVertexPointerListIBM];
  TRACE("(%d, %d, %d, %p, %d)\n", size, type, stride, pointer, ptrstride );
  ENTER_GL();
  func_glVertexPointerListIBM( size, type, stride, pointer, ptrstride );
  LEAVE_GL();
}

static void WINAPI wine_glVertexPointervINTEL( GLint size, GLenum type, GLvoid** pointer ) {
  void (*func_glVertexPointervINTEL)( GLint, GLenum, GLvoid** ) = extension_funcs[EXT_glVertexPointervINTEL];
  TRACE("(%d, %d, %p)\n", size, type, pointer );
  ENTER_GL();
  func_glVertexPointervINTEL( size, type, pointer );
  LEAVE_GL();
}

static void WINAPI wine_glVertexStream1dATI( GLenum stream, GLdouble x ) {
  void (*func_glVertexStream1dATI)( GLenum, GLdouble ) = extension_funcs[EXT_glVertexStream1dATI];
  TRACE("(%d, %f)\n", stream, x );
  ENTER_GL();
  func_glVertexStream1dATI( stream, x );
  LEAVE_GL();
}

static void WINAPI wine_glVertexStream1dvATI( GLenum stream, GLdouble* coords ) {
  void (*func_glVertexStream1dvATI)( GLenum, GLdouble* ) = extension_funcs[EXT_glVertexStream1dvATI];
  TRACE("(%d, %p)\n", stream, coords );
  ENTER_GL();
  func_glVertexStream1dvATI( stream, coords );
  LEAVE_GL();
}

static void WINAPI wine_glVertexStream1fATI( GLenum stream, GLfloat x ) {
  void (*func_glVertexStream1fATI)( GLenum, GLfloat ) = extension_funcs[EXT_glVertexStream1fATI];
  TRACE("(%d, %f)\n", stream, x );
  ENTER_GL();
  func_glVertexStream1fATI( stream, x );
  LEAVE_GL();
}

static void WINAPI wine_glVertexStream1fvATI( GLenum stream, GLfloat* coords ) {
  void (*func_glVertexStream1fvATI)( GLenum, GLfloat* ) = extension_funcs[EXT_glVertexStream1fvATI];
  TRACE("(%d, %p)\n", stream, coords );
  ENTER_GL();
  func_glVertexStream1fvATI( stream, coords );
  LEAVE_GL();
}

static void WINAPI wine_glVertexStream1iATI( GLenum stream, GLint x ) {
  void (*func_glVertexStream1iATI)( GLenum, GLint ) = extension_funcs[EXT_glVertexStream1iATI];
  TRACE("(%d, %d)\n", stream, x );
  ENTER_GL();
  func_glVertexStream1iATI( stream, x );
  LEAVE_GL();
}

static void WINAPI wine_glVertexStream1ivATI( GLenum stream, GLint* coords ) {
  void (*func_glVertexStream1ivATI)( GLenum, GLint* ) = extension_funcs[EXT_glVertexStream1ivATI];
  TRACE("(%d, %p)\n", stream, coords );
  ENTER_GL();
  func_glVertexStream1ivATI( stream, coords );
  LEAVE_GL();
}

static void WINAPI wine_glVertexStream1sATI( GLenum stream, GLshort x ) {
  void (*func_glVertexStream1sATI)( GLenum, GLshort ) = extension_funcs[EXT_glVertexStream1sATI];
  TRACE("(%d, %d)\n", stream, x );
  ENTER_GL();
  func_glVertexStream1sATI( stream, x );
  LEAVE_GL();
}

static void WINAPI wine_glVertexStream1svATI( GLenum stream, GLshort* coords ) {
  void (*func_glVertexStream1svATI)( GLenum, GLshort* ) = extension_funcs[EXT_glVertexStream1svATI];
  TRACE("(%d, %p)\n", stream, coords );
  ENTER_GL();
  func_glVertexStream1svATI( stream, coords );
  LEAVE_GL();
}

static void WINAPI wine_glVertexStream2dATI( GLenum stream, GLdouble x, GLdouble y ) {
  void (*func_glVertexStream2dATI)( GLenum, GLdouble, GLdouble ) = extension_funcs[EXT_glVertexStream2dATI];
  TRACE("(%d, %f, %f)\n", stream, x, y );
  ENTER_GL();
  func_glVertexStream2dATI( stream, x, y );
  LEAVE_GL();
}

static void WINAPI wine_glVertexStream2dvATI( GLenum stream, GLdouble* coords ) {
  void (*func_glVertexStream2dvATI)( GLenum, GLdouble* ) = extension_funcs[EXT_glVertexStream2dvATI];
  TRACE("(%d, %p)\n", stream, coords );
  ENTER_GL();
  func_glVertexStream2dvATI( stream, coords );
  LEAVE_GL();
}

static void WINAPI wine_glVertexStream2fATI( GLenum stream, GLfloat x, GLfloat y ) {
  void (*func_glVertexStream2fATI)( GLenum, GLfloat, GLfloat ) = extension_funcs[EXT_glVertexStream2fATI];
  TRACE("(%d, %f, %f)\n", stream, x, y );
  ENTER_GL();
  func_glVertexStream2fATI( stream, x, y );
  LEAVE_GL();
}

static void WINAPI wine_glVertexStream2fvATI( GLenum stream, GLfloat* coords ) {
  void (*func_glVertexStream2fvATI)( GLenum, GLfloat* ) = extension_funcs[EXT_glVertexStream2fvATI];
  TRACE("(%d, %p)\n", stream, coords );
  ENTER_GL();
  func_glVertexStream2fvATI( stream, coords );
  LEAVE_GL();
}

static void WINAPI wine_glVertexStream2iATI( GLenum stream, GLint x, GLint y ) {
  void (*func_glVertexStream2iATI)( GLenum, GLint, GLint ) = extension_funcs[EXT_glVertexStream2iATI];
  TRACE("(%d, %d, %d)\n", stream, x, y );
  ENTER_GL();
  func_glVertexStream2iATI( stream, x, y );
  LEAVE_GL();
}

static void WINAPI wine_glVertexStream2ivATI( GLenum stream, GLint* coords ) {
  void (*func_glVertexStream2ivATI)( GLenum, GLint* ) = extension_funcs[EXT_glVertexStream2ivATI];
  TRACE("(%d, %p)\n", stream, coords );
  ENTER_GL();
  func_glVertexStream2ivATI( stream, coords );
  LEAVE_GL();
}

static void WINAPI wine_glVertexStream2sATI( GLenum stream, GLshort x, GLshort y ) {
  void (*func_glVertexStream2sATI)( GLenum, GLshort, GLshort ) = extension_funcs[EXT_glVertexStream2sATI];
  TRACE("(%d, %d, %d)\n", stream, x, y );
  ENTER_GL();
  func_glVertexStream2sATI( stream, x, y );
  LEAVE_GL();
}

static void WINAPI wine_glVertexStream2svATI( GLenum stream, GLshort* coords ) {
  void (*func_glVertexStream2svATI)( GLenum, GLshort* ) = extension_funcs[EXT_glVertexStream2svATI];
  TRACE("(%d, %p)\n", stream, coords );
  ENTER_GL();
  func_glVertexStream2svATI( stream, coords );
  LEAVE_GL();
}

static void WINAPI wine_glVertexStream3dATI( GLenum stream, GLdouble x, GLdouble y, GLdouble z ) {
  void (*func_glVertexStream3dATI)( GLenum, GLdouble, GLdouble, GLdouble ) = extension_funcs[EXT_glVertexStream3dATI];
  TRACE("(%d, %f, %f, %f)\n", stream, x, y, z );
  ENTER_GL();
  func_glVertexStream3dATI( stream, x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glVertexStream3dvATI( GLenum stream, GLdouble* coords ) {
  void (*func_glVertexStream3dvATI)( GLenum, GLdouble* ) = extension_funcs[EXT_glVertexStream3dvATI];
  TRACE("(%d, %p)\n", stream, coords );
  ENTER_GL();
  func_glVertexStream3dvATI( stream, coords );
  LEAVE_GL();
}

static void WINAPI wine_glVertexStream3fATI( GLenum stream, GLfloat x, GLfloat y, GLfloat z ) {
  void (*func_glVertexStream3fATI)( GLenum, GLfloat, GLfloat, GLfloat ) = extension_funcs[EXT_glVertexStream3fATI];
  TRACE("(%d, %f, %f, %f)\n", stream, x, y, z );
  ENTER_GL();
  func_glVertexStream3fATI( stream, x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glVertexStream3fvATI( GLenum stream, GLfloat* coords ) {
  void (*func_glVertexStream3fvATI)( GLenum, GLfloat* ) = extension_funcs[EXT_glVertexStream3fvATI];
  TRACE("(%d, %p)\n", stream, coords );
  ENTER_GL();
  func_glVertexStream3fvATI( stream, coords );
  LEAVE_GL();
}

static void WINAPI wine_glVertexStream3iATI( GLenum stream, GLint x, GLint y, GLint z ) {
  void (*func_glVertexStream3iATI)( GLenum, GLint, GLint, GLint ) = extension_funcs[EXT_glVertexStream3iATI];
  TRACE("(%d, %d, %d, %d)\n", stream, x, y, z );
  ENTER_GL();
  func_glVertexStream3iATI( stream, x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glVertexStream3ivATI( GLenum stream, GLint* coords ) {
  void (*func_glVertexStream3ivATI)( GLenum, GLint* ) = extension_funcs[EXT_glVertexStream3ivATI];
  TRACE("(%d, %p)\n", stream, coords );
  ENTER_GL();
  func_glVertexStream3ivATI( stream, coords );
  LEAVE_GL();
}

static void WINAPI wine_glVertexStream3sATI( GLenum stream, GLshort x, GLshort y, GLshort z ) {
  void (*func_glVertexStream3sATI)( GLenum, GLshort, GLshort, GLshort ) = extension_funcs[EXT_glVertexStream3sATI];
  TRACE("(%d, %d, %d, %d)\n", stream, x, y, z );
  ENTER_GL();
  func_glVertexStream3sATI( stream, x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glVertexStream3svATI( GLenum stream, GLshort* coords ) {
  void (*func_glVertexStream3svATI)( GLenum, GLshort* ) = extension_funcs[EXT_glVertexStream3svATI];
  TRACE("(%d, %p)\n", stream, coords );
  ENTER_GL();
  func_glVertexStream3svATI( stream, coords );
  LEAVE_GL();
}

static void WINAPI wine_glVertexStream4dATI( GLenum stream, GLdouble x, GLdouble y, GLdouble z, GLdouble w ) {
  void (*func_glVertexStream4dATI)( GLenum, GLdouble, GLdouble, GLdouble, GLdouble ) = extension_funcs[EXT_glVertexStream4dATI];
  TRACE("(%d, %f, %f, %f, %f)\n", stream, x, y, z, w );
  ENTER_GL();
  func_glVertexStream4dATI( stream, x, y, z, w );
  LEAVE_GL();
}

static void WINAPI wine_glVertexStream4dvATI( GLenum stream, GLdouble* coords ) {
  void (*func_glVertexStream4dvATI)( GLenum, GLdouble* ) = extension_funcs[EXT_glVertexStream4dvATI];
  TRACE("(%d, %p)\n", stream, coords );
  ENTER_GL();
  func_glVertexStream4dvATI( stream, coords );
  LEAVE_GL();
}

static void WINAPI wine_glVertexStream4fATI( GLenum stream, GLfloat x, GLfloat y, GLfloat z, GLfloat w ) {
  void (*func_glVertexStream4fATI)( GLenum, GLfloat, GLfloat, GLfloat, GLfloat ) = extension_funcs[EXT_glVertexStream4fATI];
  TRACE("(%d, %f, %f, %f, %f)\n", stream, x, y, z, w );
  ENTER_GL();
  func_glVertexStream4fATI( stream, x, y, z, w );
  LEAVE_GL();
}

static void WINAPI wine_glVertexStream4fvATI( GLenum stream, GLfloat* coords ) {
  void (*func_glVertexStream4fvATI)( GLenum, GLfloat* ) = extension_funcs[EXT_glVertexStream4fvATI];
  TRACE("(%d, %p)\n", stream, coords );
  ENTER_GL();
  func_glVertexStream4fvATI( stream, coords );
  LEAVE_GL();
}

static void WINAPI wine_glVertexStream4iATI( GLenum stream, GLint x, GLint y, GLint z, GLint w ) {
  void (*func_glVertexStream4iATI)( GLenum, GLint, GLint, GLint, GLint ) = extension_funcs[EXT_glVertexStream4iATI];
  TRACE("(%d, %d, %d, %d, %d)\n", stream, x, y, z, w );
  ENTER_GL();
  func_glVertexStream4iATI( stream, x, y, z, w );
  LEAVE_GL();
}

static void WINAPI wine_glVertexStream4ivATI( GLenum stream, GLint* coords ) {
  void (*func_glVertexStream4ivATI)( GLenum, GLint* ) = extension_funcs[EXT_glVertexStream4ivATI];
  TRACE("(%d, %p)\n", stream, coords );
  ENTER_GL();
  func_glVertexStream4ivATI( stream, coords );
  LEAVE_GL();
}

static void WINAPI wine_glVertexStream4sATI( GLenum stream, GLshort x, GLshort y, GLshort z, GLshort w ) {
  void (*func_glVertexStream4sATI)( GLenum, GLshort, GLshort, GLshort, GLshort ) = extension_funcs[EXT_glVertexStream4sATI];
  TRACE("(%d, %d, %d, %d, %d)\n", stream, x, y, z, w );
  ENTER_GL();
  func_glVertexStream4sATI( stream, x, y, z, w );
  LEAVE_GL();
}

static void WINAPI wine_glVertexStream4svATI( GLenum stream, GLshort* coords ) {
  void (*func_glVertexStream4svATI)( GLenum, GLshort* ) = extension_funcs[EXT_glVertexStream4svATI];
  TRACE("(%d, %p)\n", stream, coords );
  ENTER_GL();
  func_glVertexStream4svATI( stream, coords );
  LEAVE_GL();
}

static void WINAPI wine_glVertexWeightPointerEXT( GLsizei size, GLenum type, GLsizei stride, GLvoid* pointer ) {
  void (*func_glVertexWeightPointerEXT)( GLsizei, GLenum, GLsizei, GLvoid* ) = extension_funcs[EXT_glVertexWeightPointerEXT];
  TRACE("(%d, %d, %d, %p)\n", size, type, stride, pointer );
  ENTER_GL();
  func_glVertexWeightPointerEXT( size, type, stride, pointer );
  LEAVE_GL();
}

static void WINAPI wine_glVertexWeightfEXT( GLfloat weight ) {
  void (*func_glVertexWeightfEXT)( GLfloat ) = extension_funcs[EXT_glVertexWeightfEXT];
  TRACE("(%f)\n", weight );
  ENTER_GL();
  func_glVertexWeightfEXT( weight );
  LEAVE_GL();
}

static void WINAPI wine_glVertexWeightfvEXT( GLfloat* weight ) {
  void (*func_glVertexWeightfvEXT)( GLfloat* ) = extension_funcs[EXT_glVertexWeightfvEXT];
  TRACE("(%p)\n", weight );
  ENTER_GL();
  func_glVertexWeightfvEXT( weight );
  LEAVE_GL();
}

static void WINAPI wine_glVertexWeighthNV( unsigned short weight ) {
  void (*func_glVertexWeighthNV)( unsigned short ) = extension_funcs[EXT_glVertexWeighthNV];
  TRACE("(%d)\n", weight );
  ENTER_GL();
  func_glVertexWeighthNV( weight );
  LEAVE_GL();
}

static void WINAPI wine_glVertexWeighthvNV( unsigned short* weight ) {
  void (*func_glVertexWeighthvNV)( unsigned short* ) = extension_funcs[EXT_glVertexWeighthvNV];
  TRACE("(%p)\n", weight );
  ENTER_GL();
  func_glVertexWeighthvNV( weight );
  LEAVE_GL();
}

static GLenum WINAPI wine_glVideoCaptureNV( GLuint video_capture_slot, GLuint* sequence_num, UINT64* capture_time ) {
  GLenum ret_value;
  GLenum (*func_glVideoCaptureNV)( GLuint, GLuint*, UINT64* ) = extension_funcs[EXT_glVideoCaptureNV];
  TRACE("(%d, %p, %p)\n", video_capture_slot, sequence_num, capture_time );
  ENTER_GL();
  ret_value = func_glVideoCaptureNV( video_capture_slot, sequence_num, capture_time );
  LEAVE_GL();
  return ret_value;
}

static void WINAPI wine_glVideoCaptureStreamParameterdvNV( GLuint video_capture_slot, GLuint stream, GLenum pname, GLdouble* params ) {
  void (*func_glVideoCaptureStreamParameterdvNV)( GLuint, GLuint, GLenum, GLdouble* ) = extension_funcs[EXT_glVideoCaptureStreamParameterdvNV];
  TRACE("(%d, %d, %d, %p)\n", video_capture_slot, stream, pname, params );
  ENTER_GL();
  func_glVideoCaptureStreamParameterdvNV( video_capture_slot, stream, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glVideoCaptureStreamParameterfvNV( GLuint video_capture_slot, GLuint stream, GLenum pname, GLfloat* params ) {
  void (*func_glVideoCaptureStreamParameterfvNV)( GLuint, GLuint, GLenum, GLfloat* ) = extension_funcs[EXT_glVideoCaptureStreamParameterfvNV];
  TRACE("(%d, %d, %d, %p)\n", video_capture_slot, stream, pname, params );
  ENTER_GL();
  func_glVideoCaptureStreamParameterfvNV( video_capture_slot, stream, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glVideoCaptureStreamParameterivNV( GLuint video_capture_slot, GLuint stream, GLenum pname, GLint* params ) {
  void (*func_glVideoCaptureStreamParameterivNV)( GLuint, GLuint, GLenum, GLint* ) = extension_funcs[EXT_glVideoCaptureStreamParameterivNV];
  TRACE("(%d, %d, %d, %p)\n", video_capture_slot, stream, pname, params );
  ENTER_GL();
  func_glVideoCaptureStreamParameterivNV( video_capture_slot, stream, pname, params );
  LEAVE_GL();
}

static void WINAPI wine_glViewportArrayv( GLuint first, GLsizei count, GLfloat* v ) {
  void (*func_glViewportArrayv)( GLuint, GLsizei, GLfloat* ) = extension_funcs[EXT_glViewportArrayv];
  TRACE("(%d, %d, %p)\n", first, count, v );
  ENTER_GL();
  func_glViewportArrayv( first, count, v );
  LEAVE_GL();
}

static void WINAPI wine_glViewportIndexedf( GLuint index, GLfloat x, GLfloat y, GLfloat w, GLfloat h ) {
  void (*func_glViewportIndexedf)( GLuint, GLfloat, GLfloat, GLfloat, GLfloat ) = extension_funcs[EXT_glViewportIndexedf];
  TRACE("(%d, %f, %f, %f, %f)\n", index, x, y, w, h );
  ENTER_GL();
  func_glViewportIndexedf( index, x, y, w, h );
  LEAVE_GL();
}

static void WINAPI wine_glViewportIndexedfv( GLuint index, GLfloat* v ) {
  void (*func_glViewportIndexedfv)( GLuint, GLfloat* ) = extension_funcs[EXT_glViewportIndexedfv];
  TRACE("(%d, %p)\n", index, v );
  ENTER_GL();
  func_glViewportIndexedfv( index, v );
  LEAVE_GL();
}

static void WINAPI wine_glWaitSync( GLvoid* sync, GLbitfield flags, UINT64 timeout ) {
  void (*func_glWaitSync)( GLvoid*, GLbitfield, UINT64 ) = extension_funcs[EXT_glWaitSync];
  TRACE("(%p, %d, %s)\n", sync, flags, wine_dbgstr_longlong(timeout) );
  ENTER_GL();
  func_glWaitSync( sync, flags, timeout );
  LEAVE_GL();
}

static void WINAPI wine_glWeightPointerARB( GLint size, GLenum type, GLsizei stride, GLvoid* pointer ) {
  void (*func_glWeightPointerARB)( GLint, GLenum, GLsizei, GLvoid* ) = extension_funcs[EXT_glWeightPointerARB];
  TRACE("(%d, %d, %d, %p)\n", size, type, stride, pointer );
  ENTER_GL();
  func_glWeightPointerARB( size, type, stride, pointer );
  LEAVE_GL();
}

static void WINAPI wine_glWeightbvARB( GLint size, GLbyte* weights ) {
  void (*func_glWeightbvARB)( GLint, GLbyte* ) = extension_funcs[EXT_glWeightbvARB];
  TRACE("(%d, %p)\n", size, weights );
  ENTER_GL();
  func_glWeightbvARB( size, weights );
  LEAVE_GL();
}

static void WINAPI wine_glWeightdvARB( GLint size, GLdouble* weights ) {
  void (*func_glWeightdvARB)( GLint, GLdouble* ) = extension_funcs[EXT_glWeightdvARB];
  TRACE("(%d, %p)\n", size, weights );
  ENTER_GL();
  func_glWeightdvARB( size, weights );
  LEAVE_GL();
}

static void WINAPI wine_glWeightfvARB( GLint size, GLfloat* weights ) {
  void (*func_glWeightfvARB)( GLint, GLfloat* ) = extension_funcs[EXT_glWeightfvARB];
  TRACE("(%d, %p)\n", size, weights );
  ENTER_GL();
  func_glWeightfvARB( size, weights );
  LEAVE_GL();
}

static void WINAPI wine_glWeightivARB( GLint size, GLint* weights ) {
  void (*func_glWeightivARB)( GLint, GLint* ) = extension_funcs[EXT_glWeightivARB];
  TRACE("(%d, %p)\n", size, weights );
  ENTER_GL();
  func_glWeightivARB( size, weights );
  LEAVE_GL();
}

static void WINAPI wine_glWeightsvARB( GLint size, GLshort* weights ) {
  void (*func_glWeightsvARB)( GLint, GLshort* ) = extension_funcs[EXT_glWeightsvARB];
  TRACE("(%d, %p)\n", size, weights );
  ENTER_GL();
  func_glWeightsvARB( size, weights );
  LEAVE_GL();
}

static void WINAPI wine_glWeightubvARB( GLint size, GLubyte* weights ) {
  void (*func_glWeightubvARB)( GLint, GLubyte* ) = extension_funcs[EXT_glWeightubvARB];
  TRACE("(%d, %p)\n", size, weights );
  ENTER_GL();
  func_glWeightubvARB( size, weights );
  LEAVE_GL();
}

static void WINAPI wine_glWeightuivARB( GLint size, GLuint* weights ) {
  void (*func_glWeightuivARB)( GLint, GLuint* ) = extension_funcs[EXT_glWeightuivARB];
  TRACE("(%d, %p)\n", size, weights );
  ENTER_GL();
  func_glWeightuivARB( size, weights );
  LEAVE_GL();
}

static void WINAPI wine_glWeightusvARB( GLint size, GLushort* weights ) {
  void (*func_glWeightusvARB)( GLint, GLushort* ) = extension_funcs[EXT_glWeightusvARB];
  TRACE("(%d, %p)\n", size, weights );
  ENTER_GL();
  func_glWeightusvARB( size, weights );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos2d( GLdouble x, GLdouble y ) {
  void (*func_glWindowPos2d)( GLdouble, GLdouble ) = extension_funcs[EXT_glWindowPos2d];
  TRACE("(%f, %f)\n", x, y );
  ENTER_GL();
  func_glWindowPos2d( x, y );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos2dARB( GLdouble x, GLdouble y ) {
  void (*func_glWindowPos2dARB)( GLdouble, GLdouble ) = extension_funcs[EXT_glWindowPos2dARB];
  TRACE("(%f, %f)\n", x, y );
  ENTER_GL();
  func_glWindowPos2dARB( x, y );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos2dMESA( GLdouble x, GLdouble y ) {
  void (*func_glWindowPos2dMESA)( GLdouble, GLdouble ) = extension_funcs[EXT_glWindowPos2dMESA];
  TRACE("(%f, %f)\n", x, y );
  ENTER_GL();
  func_glWindowPos2dMESA( x, y );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos2dv( GLdouble* v ) {
  void (*func_glWindowPos2dv)( GLdouble* ) = extension_funcs[EXT_glWindowPos2dv];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glWindowPos2dv( v );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos2dvARB( GLdouble* v ) {
  void (*func_glWindowPos2dvARB)( GLdouble* ) = extension_funcs[EXT_glWindowPos2dvARB];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glWindowPos2dvARB( v );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos2dvMESA( GLdouble* v ) {
  void (*func_glWindowPos2dvMESA)( GLdouble* ) = extension_funcs[EXT_glWindowPos2dvMESA];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glWindowPos2dvMESA( v );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos2f( GLfloat x, GLfloat y ) {
  void (*func_glWindowPos2f)( GLfloat, GLfloat ) = extension_funcs[EXT_glWindowPos2f];
  TRACE("(%f, %f)\n", x, y );
  ENTER_GL();
  func_glWindowPos2f( x, y );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos2fARB( GLfloat x, GLfloat y ) {
  void (*func_glWindowPos2fARB)( GLfloat, GLfloat ) = extension_funcs[EXT_glWindowPos2fARB];
  TRACE("(%f, %f)\n", x, y );
  ENTER_GL();
  func_glWindowPos2fARB( x, y );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos2fMESA( GLfloat x, GLfloat y ) {
  void (*func_glWindowPos2fMESA)( GLfloat, GLfloat ) = extension_funcs[EXT_glWindowPos2fMESA];
  TRACE("(%f, %f)\n", x, y );
  ENTER_GL();
  func_glWindowPos2fMESA( x, y );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos2fv( GLfloat* v ) {
  void (*func_glWindowPos2fv)( GLfloat* ) = extension_funcs[EXT_glWindowPos2fv];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glWindowPos2fv( v );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos2fvARB( GLfloat* v ) {
  void (*func_glWindowPos2fvARB)( GLfloat* ) = extension_funcs[EXT_glWindowPos2fvARB];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glWindowPos2fvARB( v );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos2fvMESA( GLfloat* v ) {
  void (*func_glWindowPos2fvMESA)( GLfloat* ) = extension_funcs[EXT_glWindowPos2fvMESA];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glWindowPos2fvMESA( v );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos2i( GLint x, GLint y ) {
  void (*func_glWindowPos2i)( GLint, GLint ) = extension_funcs[EXT_glWindowPos2i];
  TRACE("(%d, %d)\n", x, y );
  ENTER_GL();
  func_glWindowPos2i( x, y );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos2iARB( GLint x, GLint y ) {
  void (*func_glWindowPos2iARB)( GLint, GLint ) = extension_funcs[EXT_glWindowPos2iARB];
  TRACE("(%d, %d)\n", x, y );
  ENTER_GL();
  func_glWindowPos2iARB( x, y );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos2iMESA( GLint x, GLint y ) {
  void (*func_glWindowPos2iMESA)( GLint, GLint ) = extension_funcs[EXT_glWindowPos2iMESA];
  TRACE("(%d, %d)\n", x, y );
  ENTER_GL();
  func_glWindowPos2iMESA( x, y );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos2iv( GLint* v ) {
  void (*func_glWindowPos2iv)( GLint* ) = extension_funcs[EXT_glWindowPos2iv];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glWindowPos2iv( v );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos2ivARB( GLint* v ) {
  void (*func_glWindowPos2ivARB)( GLint* ) = extension_funcs[EXT_glWindowPos2ivARB];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glWindowPos2ivARB( v );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos2ivMESA( GLint* v ) {
  void (*func_glWindowPos2ivMESA)( GLint* ) = extension_funcs[EXT_glWindowPos2ivMESA];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glWindowPos2ivMESA( v );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos2s( GLshort x, GLshort y ) {
  void (*func_glWindowPos2s)( GLshort, GLshort ) = extension_funcs[EXT_glWindowPos2s];
  TRACE("(%d, %d)\n", x, y );
  ENTER_GL();
  func_glWindowPos2s( x, y );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos2sARB( GLshort x, GLshort y ) {
  void (*func_glWindowPos2sARB)( GLshort, GLshort ) = extension_funcs[EXT_glWindowPos2sARB];
  TRACE("(%d, %d)\n", x, y );
  ENTER_GL();
  func_glWindowPos2sARB( x, y );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos2sMESA( GLshort x, GLshort y ) {
  void (*func_glWindowPos2sMESA)( GLshort, GLshort ) = extension_funcs[EXT_glWindowPos2sMESA];
  TRACE("(%d, %d)\n", x, y );
  ENTER_GL();
  func_glWindowPos2sMESA( x, y );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos2sv( GLshort* v ) {
  void (*func_glWindowPos2sv)( GLshort* ) = extension_funcs[EXT_glWindowPos2sv];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glWindowPos2sv( v );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos2svARB( GLshort* v ) {
  void (*func_glWindowPos2svARB)( GLshort* ) = extension_funcs[EXT_glWindowPos2svARB];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glWindowPos2svARB( v );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos2svMESA( GLshort* v ) {
  void (*func_glWindowPos2svMESA)( GLshort* ) = extension_funcs[EXT_glWindowPos2svMESA];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glWindowPos2svMESA( v );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos3d( GLdouble x, GLdouble y, GLdouble z ) {
  void (*func_glWindowPos3d)( GLdouble, GLdouble, GLdouble ) = extension_funcs[EXT_glWindowPos3d];
  TRACE("(%f, %f, %f)\n", x, y, z );
  ENTER_GL();
  func_glWindowPos3d( x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos3dARB( GLdouble x, GLdouble y, GLdouble z ) {
  void (*func_glWindowPos3dARB)( GLdouble, GLdouble, GLdouble ) = extension_funcs[EXT_glWindowPos3dARB];
  TRACE("(%f, %f, %f)\n", x, y, z );
  ENTER_GL();
  func_glWindowPos3dARB( x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos3dMESA( GLdouble x, GLdouble y, GLdouble z ) {
  void (*func_glWindowPos3dMESA)( GLdouble, GLdouble, GLdouble ) = extension_funcs[EXT_glWindowPos3dMESA];
  TRACE("(%f, %f, %f)\n", x, y, z );
  ENTER_GL();
  func_glWindowPos3dMESA( x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos3dv( GLdouble* v ) {
  void (*func_glWindowPos3dv)( GLdouble* ) = extension_funcs[EXT_glWindowPos3dv];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glWindowPos3dv( v );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos3dvARB( GLdouble* v ) {
  void (*func_glWindowPos3dvARB)( GLdouble* ) = extension_funcs[EXT_glWindowPos3dvARB];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glWindowPos3dvARB( v );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos3dvMESA( GLdouble* v ) {
  void (*func_glWindowPos3dvMESA)( GLdouble* ) = extension_funcs[EXT_glWindowPos3dvMESA];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glWindowPos3dvMESA( v );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos3f( GLfloat x, GLfloat y, GLfloat z ) {
  void (*func_glWindowPos3f)( GLfloat, GLfloat, GLfloat ) = extension_funcs[EXT_glWindowPos3f];
  TRACE("(%f, %f, %f)\n", x, y, z );
  ENTER_GL();
  func_glWindowPos3f( x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos3fARB( GLfloat x, GLfloat y, GLfloat z ) {
  void (*func_glWindowPos3fARB)( GLfloat, GLfloat, GLfloat ) = extension_funcs[EXT_glWindowPos3fARB];
  TRACE("(%f, %f, %f)\n", x, y, z );
  ENTER_GL();
  func_glWindowPos3fARB( x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos3fMESA( GLfloat x, GLfloat y, GLfloat z ) {
  void (*func_glWindowPos3fMESA)( GLfloat, GLfloat, GLfloat ) = extension_funcs[EXT_glWindowPos3fMESA];
  TRACE("(%f, %f, %f)\n", x, y, z );
  ENTER_GL();
  func_glWindowPos3fMESA( x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos3fv( GLfloat* v ) {
  void (*func_glWindowPos3fv)( GLfloat* ) = extension_funcs[EXT_glWindowPos3fv];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glWindowPos3fv( v );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos3fvARB( GLfloat* v ) {
  void (*func_glWindowPos3fvARB)( GLfloat* ) = extension_funcs[EXT_glWindowPos3fvARB];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glWindowPos3fvARB( v );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos3fvMESA( GLfloat* v ) {
  void (*func_glWindowPos3fvMESA)( GLfloat* ) = extension_funcs[EXT_glWindowPos3fvMESA];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glWindowPos3fvMESA( v );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos3i( GLint x, GLint y, GLint z ) {
  void (*func_glWindowPos3i)( GLint, GLint, GLint ) = extension_funcs[EXT_glWindowPos3i];
  TRACE("(%d, %d, %d)\n", x, y, z );
  ENTER_GL();
  func_glWindowPos3i( x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos3iARB( GLint x, GLint y, GLint z ) {
  void (*func_glWindowPos3iARB)( GLint, GLint, GLint ) = extension_funcs[EXT_glWindowPos3iARB];
  TRACE("(%d, %d, %d)\n", x, y, z );
  ENTER_GL();
  func_glWindowPos3iARB( x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos3iMESA( GLint x, GLint y, GLint z ) {
  void (*func_glWindowPos3iMESA)( GLint, GLint, GLint ) = extension_funcs[EXT_glWindowPos3iMESA];
  TRACE("(%d, %d, %d)\n", x, y, z );
  ENTER_GL();
  func_glWindowPos3iMESA( x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos3iv( GLint* v ) {
  void (*func_glWindowPos3iv)( GLint* ) = extension_funcs[EXT_glWindowPos3iv];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glWindowPos3iv( v );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos3ivARB( GLint* v ) {
  void (*func_glWindowPos3ivARB)( GLint* ) = extension_funcs[EXT_glWindowPos3ivARB];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glWindowPos3ivARB( v );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos3ivMESA( GLint* v ) {
  void (*func_glWindowPos3ivMESA)( GLint* ) = extension_funcs[EXT_glWindowPos3ivMESA];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glWindowPos3ivMESA( v );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos3s( GLshort x, GLshort y, GLshort z ) {
  void (*func_glWindowPos3s)( GLshort, GLshort, GLshort ) = extension_funcs[EXT_glWindowPos3s];
  TRACE("(%d, %d, %d)\n", x, y, z );
  ENTER_GL();
  func_glWindowPos3s( x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos3sARB( GLshort x, GLshort y, GLshort z ) {
  void (*func_glWindowPos3sARB)( GLshort, GLshort, GLshort ) = extension_funcs[EXT_glWindowPos3sARB];
  TRACE("(%d, %d, %d)\n", x, y, z );
  ENTER_GL();
  func_glWindowPos3sARB( x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos3sMESA( GLshort x, GLshort y, GLshort z ) {
  void (*func_glWindowPos3sMESA)( GLshort, GLshort, GLshort ) = extension_funcs[EXT_glWindowPos3sMESA];
  TRACE("(%d, %d, %d)\n", x, y, z );
  ENTER_GL();
  func_glWindowPos3sMESA( x, y, z );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos3sv( GLshort* v ) {
  void (*func_glWindowPos3sv)( GLshort* ) = extension_funcs[EXT_glWindowPos3sv];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glWindowPos3sv( v );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos3svARB( GLshort* v ) {
  void (*func_glWindowPos3svARB)( GLshort* ) = extension_funcs[EXT_glWindowPos3svARB];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glWindowPos3svARB( v );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos3svMESA( GLshort* v ) {
  void (*func_glWindowPos3svMESA)( GLshort* ) = extension_funcs[EXT_glWindowPos3svMESA];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glWindowPos3svMESA( v );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos4dMESA( GLdouble x, GLdouble y, GLdouble z, GLdouble w ) {
  void (*func_glWindowPos4dMESA)( GLdouble, GLdouble, GLdouble, GLdouble ) = extension_funcs[EXT_glWindowPos4dMESA];
  TRACE("(%f, %f, %f, %f)\n", x, y, z, w );
  ENTER_GL();
  func_glWindowPos4dMESA( x, y, z, w );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos4dvMESA( GLdouble* v ) {
  void (*func_glWindowPos4dvMESA)( GLdouble* ) = extension_funcs[EXT_glWindowPos4dvMESA];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glWindowPos4dvMESA( v );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos4fMESA( GLfloat x, GLfloat y, GLfloat z, GLfloat w ) {
  void (*func_glWindowPos4fMESA)( GLfloat, GLfloat, GLfloat, GLfloat ) = extension_funcs[EXT_glWindowPos4fMESA];
  TRACE("(%f, %f, %f, %f)\n", x, y, z, w );
  ENTER_GL();
  func_glWindowPos4fMESA( x, y, z, w );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos4fvMESA( GLfloat* v ) {
  void (*func_glWindowPos4fvMESA)( GLfloat* ) = extension_funcs[EXT_glWindowPos4fvMESA];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glWindowPos4fvMESA( v );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos4iMESA( GLint x, GLint y, GLint z, GLint w ) {
  void (*func_glWindowPos4iMESA)( GLint, GLint, GLint, GLint ) = extension_funcs[EXT_glWindowPos4iMESA];
  TRACE("(%d, %d, %d, %d)\n", x, y, z, w );
  ENTER_GL();
  func_glWindowPos4iMESA( x, y, z, w );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos4ivMESA( GLint* v ) {
  void (*func_glWindowPos4ivMESA)( GLint* ) = extension_funcs[EXT_glWindowPos4ivMESA];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glWindowPos4ivMESA( v );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos4sMESA( GLshort x, GLshort y, GLshort z, GLshort w ) {
  void (*func_glWindowPos4sMESA)( GLshort, GLshort, GLshort, GLshort ) = extension_funcs[EXT_glWindowPos4sMESA];
  TRACE("(%d, %d, %d, %d)\n", x, y, z, w );
  ENTER_GL();
  func_glWindowPos4sMESA( x, y, z, w );
  LEAVE_GL();
}

static void WINAPI wine_glWindowPos4svMESA( GLshort* v ) {
  void (*func_glWindowPos4svMESA)( GLshort* ) = extension_funcs[EXT_glWindowPos4svMESA];
  TRACE("(%p)\n", v );
  ENTER_GL();
  func_glWindowPos4svMESA( v );
  LEAVE_GL();
}

static void WINAPI wine_glWriteMaskEXT( GLuint res, GLuint in, GLenum outX, GLenum outY, GLenum outZ, GLenum outW ) {
  void (*func_glWriteMaskEXT)( GLuint, GLuint, GLenum, GLenum, GLenum, GLenum ) = extension_funcs[EXT_glWriteMaskEXT];
  TRACE("(%d, %d, %d, %d, %d, %d)\n", res, in, outX, outY, outZ, outW );
  ENTER_GL();
  func_glWriteMaskEXT( res, in, outX, outY, outZ, outW );
  LEAVE_GL();
}


/* The table giving the correspondence between names and functions */
const OpenGL_extension extension_registry[NB_EXTENSIONS] = {
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
  { "glDrawElementsInstancedBaseVertex", "GL_ARB_draw_elements_base_vertex", wine_glDrawElementsInstancedBaseVertex },
  { "glDrawElementsInstancedEXT", "GL_EXT_draw_instanced", wine_glDrawElementsInstancedEXT },
  { "glDrawMeshArraysSUN", "GL_SUN_mesh_array", wine_glDrawMeshArraysSUN },
  { "glDrawRangeElementArrayAPPLE", "GL_APPLE_element_array", wine_glDrawRangeElementArrayAPPLE },
  { "glDrawRangeElementArrayATI", "GL_ATI_element_array", wine_glDrawRangeElementArrayATI },
  { "glDrawRangeElements", "GL_VERSION_1_2", wine_glDrawRangeElements },
  { "glDrawRangeElementsBaseVertex", "GL_ARB_draw_elements_base_vertex", wine_glDrawRangeElementsBaseVertex },
  { "glDrawRangeElementsEXT", "GL_EXT_draw_range_elements", wine_glDrawRangeElementsEXT },
  { "glDrawTransformFeedback", "GL_ARB_transform_feedback2", wine_glDrawTransformFeedback },
  { "glDrawTransformFeedbackNV", "GL_NV_transform_feedback2", wine_glDrawTransformFeedbackNV },
  { "glDrawTransformFeedbackStream", "GL_ARB_transform_feedback3", wine_glDrawTransformFeedbackStream },
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
  { "glGetTextureImageEXT", "GL_EXT_direct_state_access", wine_glGetTextureImageEXT },
  { "glGetTextureLevelParameterfvEXT", "GL_EXT_direct_state_access", wine_glGetTextureLevelParameterfvEXT },
  { "glGetTextureLevelParameterivEXT", "GL_EXT_direct_state_access", wine_glGetTextureLevelParameterivEXT },
  { "glGetTextureParameterIivEXT", "GL_EXT_direct_state_access", wine_glGetTextureParameterIivEXT },
  { "glGetTextureParameterIuivEXT", "GL_EXT_direct_state_access", wine_glGetTextureParameterIuivEXT },
  { "glGetTextureParameterfvEXT", "GL_EXT_direct_state_access", wine_glGetTextureParameterfvEXT },
  { "glGetTextureParameterivEXT", "GL_EXT_direct_state_access", wine_glGetTextureParameterivEXT },
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
  { "glIndexFormatNV", "GL_NV_vertex_buffer_unified_memory", wine_glIndexFormatNV },
  { "glIndexFuncEXT", "GL_EXT_index_func", wine_glIndexFuncEXT },
  { "glIndexMaterialEXT", "GL_EXT_index_material", wine_glIndexMaterialEXT },
  { "glIndexPointerEXT", "GL_EXT_vertex_array", wine_glIndexPointerEXT },
  { "glIndexPointerListIBM", "GL_IBM_vertex_array_lists", wine_glIndexPointerListIBM },
  { "glInsertComponentEXT", "GL_EXT_vertex_shader", wine_glInsertComponentEXT },
  { "glInstrumentsBufferSGIX", "GL_SGIX_instruments", wine_glInstrumentsBufferSGIX },
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
  { "glIsNameAMD", "GL_AMD_name_gen_delete", wine_glIsNameAMD },
  { "glIsNamedBufferResidentNV", "GL_NV_shader_buffer_load", wine_glIsNamedBufferResidentNV },
  { "glIsNamedStringARB", "GL_ARB_shading_language_include", wine_glIsNamedStringARB },
  { "glIsObjectBufferATI", "GL_ATI_vertex_array_object", wine_glIsObjectBufferATI },
  { "glIsOcclusionQueryNV", "GL_NV_occlusion_query", wine_glIsOcclusionQueryNV },
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
  { "glMakeNamedBufferNonResidentNV", "GL_NV_shader_buffer_load", wine_glMakeNamedBufferNonResidentNV },
  { "glMakeNamedBufferResidentNV", "GL_NV_shader_buffer_load", wine_glMakeNamedBufferResidentNV },
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
  { "glMultiDrawElementArrayAPPLE", "GL_APPLE_element_array", wine_glMultiDrawElementArrayAPPLE },
  { "glMultiDrawElements", "GL_VERSION_1_4", wine_glMultiDrawElements },
  { "glMultiDrawElementsBaseVertex", "GL_ARB_draw_elements_base_vertex", wine_glMultiDrawElementsBaseVertex },
  { "glMultiDrawElementsEXT", "GL_EXT_multi_draw_arrays", wine_glMultiDrawElementsEXT },
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
  { "glStencilFuncSeparate", "GL_VERSION_2_0", wine_glStencilFuncSeparate },
  { "glStencilFuncSeparateATI", "GL_ATI_separate_stencil", wine_glStencilFuncSeparateATI },
  { "glStencilMaskSeparate", "GL_VERSION_2_0", wine_glStencilMaskSeparate },
  { "glStencilOpSeparate", "GL_VERSION_2_0", wine_glStencilOpSeparate },
  { "glStencilOpSeparateATI", "GL_ATI_separate_stencil", wine_glStencilOpSeparateATI },
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
  { "glTexImage3D", "GL_VERSION_1_2", wine_glTexImage3D },
  { "glTexImage3DEXT", "GL_EXT_texture3D", wine_glTexImage3DEXT },
  { "glTexImage3DMultisample", "GL_ARB_texture_multisample", wine_glTexImage3DMultisample },
  { "glTexImage4DSGIS", "GL_SGIS_texture4D", wine_glTexImage4DSGIS },
  { "glTexParameterIiv", "GL_VERSION_3_0", wine_glTexParameterIiv },
  { "glTexParameterIivEXT", "GL_EXT_texture_integer", wine_glTexParameterIivEXT },
  { "glTexParameterIuiv", "GL_VERSION_3_0", wine_glTexParameterIuiv },
  { "glTexParameterIuivEXT", "GL_EXT_texture_integer", wine_glTexParameterIuivEXT },
  { "glTexRenderbufferNV", "GL_NV_explicit_multisample", wine_glTexRenderbufferNV },
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
  { "glTextureImage3DEXT", "GL_EXT_direct_state_access", wine_glTextureImage3DEXT },
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
  { "glTextureSubImage1DEXT", "GL_EXT_direct_state_access", wine_glTextureSubImage1DEXT },
  { "glTextureSubImage2DEXT", "GL_EXT_direct_state_access", wine_glTextureSubImage2DEXT },
  { "glTextureSubImage3DEXT", "GL_EXT_direct_state_access", wine_glTextureSubImage3DEXT },
  { "glTrackMatrixNV", "GL_NV_vertex_program", wine_glTrackMatrixNV },
  { "glTransformFeedbackAttribsNV", "GL_NV_transform_feedback", wine_glTransformFeedbackAttribsNV },
  { "glTransformFeedbackStreamAttribsNV", "GL_NV_transform_feedback", wine_glTransformFeedbackStreamAttribsNV },
  { "glTransformFeedbackVaryings", "GL_VERSION_3_0", wine_glTransformFeedbackVaryings },
  { "glTransformFeedbackVaryingsEXT", "GL_EXT_transform_feedback", wine_glTransformFeedbackVaryingsEXT },
  { "glTransformFeedbackVaryingsNV", "GL_NV_transform_feedback", wine_glTransformFeedbackVaryingsNV },
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
  { "glWriteMaskEXT", "GL_EXT_vertex_shader", wine_glWriteMaskEXT }
};
