/* Automatically generated from http://www.opengl.org/registry files; DO NOT EDIT! */

#include <stdarg.h>
#include <stddef.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"

#include "unixlib.h"

#include "opengl_ext.h"

static NTSTATUS gl_glAccum( void *args )
{
    struct glAccum_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glAccum( params->op, params->value );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glAlphaFunc( void *args )
{
    struct glAlphaFunc_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glAlphaFunc( params->func, params->ref );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glAreTexturesResident( void *args )
{
    struct glAreTexturesResident_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    params->ret = funcs->gl.p_glAreTexturesResident( params->n, params->textures, params->residences );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glArrayElement( void *args )
{
    struct glArrayElement_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glArrayElement( params->i );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glBegin( void *args )
{
    struct glBegin_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glBegin( params->mode );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glBindTexture( void *args )
{
    struct glBindTexture_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glBindTexture( params->target, params->texture );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glBitmap( void *args )
{
    struct glBitmap_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glBitmap( params->width, params->height, params->xorig, params->yorig, params->xmove, params->ymove, params->bitmap );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glBlendFunc( void *args )
{
    struct glBlendFunc_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glBlendFunc( params->sfactor, params->dfactor );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glCallList( void *args )
{
    struct glCallList_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glCallList( params->list );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glCallLists( void *args )
{
    struct glCallLists_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glCallLists( params->n, params->type, params->lists );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glClear( void *args )
{
    struct glClear_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glClear( params->mask );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glClearAccum( void *args )
{
    struct glClearAccum_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glClearAccum( params->red, params->green, params->blue, params->alpha );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glClearColor( void *args )
{
    struct glClearColor_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glClearColor( params->red, params->green, params->blue, params->alpha );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glClearDepth( void *args )
{
    struct glClearDepth_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glClearDepth( params->depth );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glClearIndex( void *args )
{
    struct glClearIndex_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glClearIndex( params->c );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glClearStencil( void *args )
{
    struct glClearStencil_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glClearStencil( params->s );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glClipPlane( void *args )
{
    struct glClipPlane_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glClipPlane( params->plane, params->equation );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glColor3b( void *args )
{
    struct glColor3b_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glColor3b( params->red, params->green, params->blue );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glColor3bv( void *args )
{
    struct glColor3bv_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glColor3bv( params->v );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glColor3d( void *args )
{
    struct glColor3d_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glColor3d( params->red, params->green, params->blue );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glColor3dv( void *args )
{
    struct glColor3dv_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glColor3dv( params->v );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glColor3f( void *args )
{
    struct glColor3f_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glColor3f( params->red, params->green, params->blue );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glColor3fv( void *args )
{
    struct glColor3fv_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glColor3fv( params->v );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glColor3i( void *args )
{
    struct glColor3i_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glColor3i( params->red, params->green, params->blue );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glColor3iv( void *args )
{
    struct glColor3iv_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glColor3iv( params->v );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glColor3s( void *args )
{
    struct glColor3s_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glColor3s( params->red, params->green, params->blue );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glColor3sv( void *args )
{
    struct glColor3sv_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glColor3sv( params->v );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glColor3ub( void *args )
{
    struct glColor3ub_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glColor3ub( params->red, params->green, params->blue );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glColor3ubv( void *args )
{
    struct glColor3ubv_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glColor3ubv( params->v );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glColor3ui( void *args )
{
    struct glColor3ui_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glColor3ui( params->red, params->green, params->blue );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glColor3uiv( void *args )
{
    struct glColor3uiv_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glColor3uiv( params->v );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glColor3us( void *args )
{
    struct glColor3us_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glColor3us( params->red, params->green, params->blue );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glColor3usv( void *args )
{
    struct glColor3usv_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glColor3usv( params->v );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glColor4b( void *args )
{
    struct glColor4b_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glColor4b( params->red, params->green, params->blue, params->alpha );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glColor4bv( void *args )
{
    struct glColor4bv_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glColor4bv( params->v );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glColor4d( void *args )
{
    struct glColor4d_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glColor4d( params->red, params->green, params->blue, params->alpha );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glColor4dv( void *args )
{
    struct glColor4dv_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glColor4dv( params->v );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glColor4f( void *args )
{
    struct glColor4f_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glColor4f( params->red, params->green, params->blue, params->alpha );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glColor4fv( void *args )
{
    struct glColor4fv_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glColor4fv( params->v );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glColor4i( void *args )
{
    struct glColor4i_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glColor4i( params->red, params->green, params->blue, params->alpha );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glColor4iv( void *args )
{
    struct glColor4iv_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glColor4iv( params->v );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glColor4s( void *args )
{
    struct glColor4s_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glColor4s( params->red, params->green, params->blue, params->alpha );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glColor4sv( void *args )
{
    struct glColor4sv_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glColor4sv( params->v );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glColor4ub( void *args )
{
    struct glColor4ub_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glColor4ub( params->red, params->green, params->blue, params->alpha );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glColor4ubv( void *args )
{
    struct glColor4ubv_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glColor4ubv( params->v );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glColor4ui( void *args )
{
    struct glColor4ui_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glColor4ui( params->red, params->green, params->blue, params->alpha );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glColor4uiv( void *args )
{
    struct glColor4uiv_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glColor4uiv( params->v );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glColor4us( void *args )
{
    struct glColor4us_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glColor4us( params->red, params->green, params->blue, params->alpha );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glColor4usv( void *args )
{
    struct glColor4usv_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glColor4usv( params->v );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glColorMask( void *args )
{
    struct glColorMask_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glColorMask( params->red, params->green, params->blue, params->alpha );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glColorMaterial( void *args )
{
    struct glColorMaterial_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glColorMaterial( params->face, params->mode );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glColorPointer( void *args )
{
    struct glColorPointer_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glColorPointer( params->size, params->type, params->stride, params->pointer );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glCopyPixels( void *args )
{
    struct glCopyPixels_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glCopyPixels( params->x, params->y, params->width, params->height, params->type );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glCopyTexImage1D( void *args )
{
    struct glCopyTexImage1D_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glCopyTexImage1D( params->target, params->level, params->internalformat, params->x, params->y, params->width, params->border );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glCopyTexImage2D( void *args )
{
    struct glCopyTexImage2D_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glCopyTexImage2D( params->target, params->level, params->internalformat, params->x, params->y, params->width, params->height, params->border );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glCopyTexSubImage1D( void *args )
{
    struct glCopyTexSubImage1D_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glCopyTexSubImage1D( params->target, params->level, params->xoffset, params->x, params->y, params->width );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glCopyTexSubImage2D( void *args )
{
    struct glCopyTexSubImage2D_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glCopyTexSubImage2D( params->target, params->level, params->xoffset, params->yoffset, params->x, params->y, params->width, params->height );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glCullFace( void *args )
{
    struct glCullFace_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glCullFace( params->mode );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glDeleteLists( void *args )
{
    struct glDeleteLists_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glDeleteLists( params->list, params->range );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glDeleteTextures( void *args )
{
    struct glDeleteTextures_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glDeleteTextures( params->n, params->textures );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glDepthFunc( void *args )
{
    struct glDepthFunc_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glDepthFunc( params->func );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glDepthMask( void *args )
{
    struct glDepthMask_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glDepthMask( params->flag );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glDepthRange( void *args )
{
    struct glDepthRange_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glDepthRange( params->n, params->f );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glDisable( void *args )
{
    struct glDisable_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glDisable( params->cap );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glDisableClientState( void *args )
{
    struct glDisableClientState_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glDisableClientState( params->array );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glDrawArrays( void *args )
{
    struct glDrawArrays_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glDrawArrays( params->mode, params->first, params->count );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glDrawBuffer( void *args )
{
    struct glDrawBuffer_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glDrawBuffer( params->buf );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glDrawElements( void *args )
{
    struct glDrawElements_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glDrawElements( params->mode, params->count, params->type, params->indices );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glDrawPixels( void *args )
{
    struct glDrawPixels_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glDrawPixels( params->width, params->height, params->format, params->type, params->pixels );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glEdgeFlag( void *args )
{
    struct glEdgeFlag_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glEdgeFlag( params->flag );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glEdgeFlagPointer( void *args )
{
    struct glEdgeFlagPointer_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glEdgeFlagPointer( params->stride, params->pointer );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glEdgeFlagv( void *args )
{
    struct glEdgeFlagv_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glEdgeFlagv( params->flag );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glEnable( void *args )
{
    struct glEnable_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glEnable( params->cap );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glEnableClientState( void *args )
{
    struct glEnableClientState_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glEnableClientState( params->array );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glEnd( void *args )
{
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glEnd();
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glEndList( void *args )
{
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glEndList();
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glEvalCoord1d( void *args )
{
    struct glEvalCoord1d_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glEvalCoord1d( params->u );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glEvalCoord1dv( void *args )
{
    struct glEvalCoord1dv_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glEvalCoord1dv( params->u );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glEvalCoord1f( void *args )
{
    struct glEvalCoord1f_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glEvalCoord1f( params->u );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glEvalCoord1fv( void *args )
{
    struct glEvalCoord1fv_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glEvalCoord1fv( params->u );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glEvalCoord2d( void *args )
{
    struct glEvalCoord2d_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glEvalCoord2d( params->u, params->v );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glEvalCoord2dv( void *args )
{
    struct glEvalCoord2dv_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glEvalCoord2dv( params->u );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glEvalCoord2f( void *args )
{
    struct glEvalCoord2f_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glEvalCoord2f( params->u, params->v );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glEvalCoord2fv( void *args )
{
    struct glEvalCoord2fv_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glEvalCoord2fv( params->u );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glEvalMesh1( void *args )
{
    struct glEvalMesh1_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glEvalMesh1( params->mode, params->i1, params->i2 );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glEvalMesh2( void *args )
{
    struct glEvalMesh2_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glEvalMesh2( params->mode, params->i1, params->i2, params->j1, params->j2 );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glEvalPoint1( void *args )
{
    struct glEvalPoint1_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glEvalPoint1( params->i );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glEvalPoint2( void *args )
{
    struct glEvalPoint2_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glEvalPoint2( params->i, params->j );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glFeedbackBuffer( void *args )
{
    struct glFeedbackBuffer_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glFeedbackBuffer( params->size, params->type, params->buffer );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glFinish( void *args )
{
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glFinish();
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glFlush( void *args )
{
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glFlush();
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glFogf( void *args )
{
    struct glFogf_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glFogf( params->pname, params->param );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glFogfv( void *args )
{
    struct glFogfv_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glFogfv( params->pname, params->params );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glFogi( void *args )
{
    struct glFogi_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glFogi( params->pname, params->param );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glFogiv( void *args )
{
    struct glFogiv_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glFogiv( params->pname, params->params );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glFrontFace( void *args )
{
    struct glFrontFace_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glFrontFace( params->mode );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glFrustum( void *args )
{
    struct glFrustum_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glFrustum( params->left, params->right, params->bottom, params->top, params->zNear, params->zFar );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glGenLists( void *args )
{
    struct glGenLists_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    params->ret = funcs->gl.p_glGenLists( params->range );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glGenTextures( void *args )
{
    struct glGenTextures_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glGenTextures( params->n, params->textures );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glGetBooleanv( void *args )
{
    struct glGetBooleanv_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glGetBooleanv( params->pname, params->data );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glGetClipPlane( void *args )
{
    struct glGetClipPlane_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glGetClipPlane( params->plane, params->equation );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glGetDoublev( void *args )
{
    struct glGetDoublev_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glGetDoublev( params->pname, params->data );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glGetError( void *args )
{
    struct glGetError_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    params->ret = funcs->gl.p_glGetError();
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glGetFloatv( void *args )
{
    struct glGetFloatv_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glGetFloatv( params->pname, params->data );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glGetLightfv( void *args )
{
    struct glGetLightfv_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glGetLightfv( params->light, params->pname, params->params );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glGetLightiv( void *args )
{
    struct glGetLightiv_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glGetLightiv( params->light, params->pname, params->params );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glGetMapdv( void *args )
{
    struct glGetMapdv_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glGetMapdv( params->target, params->query, params->v );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glGetMapfv( void *args )
{
    struct glGetMapfv_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glGetMapfv( params->target, params->query, params->v );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glGetMapiv( void *args )
{
    struct glGetMapiv_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glGetMapiv( params->target, params->query, params->v );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glGetMaterialfv( void *args )
{
    struct glGetMaterialfv_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glGetMaterialfv( params->face, params->pname, params->params );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glGetMaterialiv( void *args )
{
    struct glGetMaterialiv_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glGetMaterialiv( params->face, params->pname, params->params );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glGetPixelMapfv( void *args )
{
    struct glGetPixelMapfv_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glGetPixelMapfv( params->map, params->values );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glGetPixelMapuiv( void *args )
{
    struct glGetPixelMapuiv_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glGetPixelMapuiv( params->map, params->values );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glGetPixelMapusv( void *args )
{
    struct glGetPixelMapusv_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glGetPixelMapusv( params->map, params->values );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glGetPointerv( void *args )
{
    struct glGetPointerv_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glGetPointerv( params->pname, params->params );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glGetPolygonStipple( void *args )
{
    struct glGetPolygonStipple_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glGetPolygonStipple( params->mask );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glGetTexEnvfv( void *args )
{
    struct glGetTexEnvfv_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glGetTexEnvfv( params->target, params->pname, params->params );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glGetTexEnviv( void *args )
{
    struct glGetTexEnviv_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glGetTexEnviv( params->target, params->pname, params->params );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glGetTexGendv( void *args )
{
    struct glGetTexGendv_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glGetTexGendv( params->coord, params->pname, params->params );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glGetTexGenfv( void *args )
{
    struct glGetTexGenfv_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glGetTexGenfv( params->coord, params->pname, params->params );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glGetTexGeniv( void *args )
{
    struct glGetTexGeniv_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glGetTexGeniv( params->coord, params->pname, params->params );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glGetTexImage( void *args )
{
    struct glGetTexImage_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glGetTexImage( params->target, params->level, params->format, params->type, params->pixels );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glGetTexLevelParameterfv( void *args )
{
    struct glGetTexLevelParameterfv_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glGetTexLevelParameterfv( params->target, params->level, params->pname, params->params );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glGetTexLevelParameteriv( void *args )
{
    struct glGetTexLevelParameteriv_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glGetTexLevelParameteriv( params->target, params->level, params->pname, params->params );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glGetTexParameterfv( void *args )
{
    struct glGetTexParameterfv_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glGetTexParameterfv( params->target, params->pname, params->params );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glGetTexParameteriv( void *args )
{
    struct glGetTexParameteriv_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glGetTexParameteriv( params->target, params->pname, params->params );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glHint( void *args )
{
    struct glHint_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glHint( params->target, params->mode );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glIndexMask( void *args )
{
    struct glIndexMask_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glIndexMask( params->mask );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glIndexPointer( void *args )
{
    struct glIndexPointer_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glIndexPointer( params->type, params->stride, params->pointer );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glIndexd( void *args )
{
    struct glIndexd_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glIndexd( params->c );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glIndexdv( void *args )
{
    struct glIndexdv_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glIndexdv( params->c );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glIndexf( void *args )
{
    struct glIndexf_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glIndexf( params->c );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glIndexfv( void *args )
{
    struct glIndexfv_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glIndexfv( params->c );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glIndexi( void *args )
{
    struct glIndexi_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glIndexi( params->c );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glIndexiv( void *args )
{
    struct glIndexiv_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glIndexiv( params->c );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glIndexs( void *args )
{
    struct glIndexs_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glIndexs( params->c );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glIndexsv( void *args )
{
    struct glIndexsv_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glIndexsv( params->c );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glIndexub( void *args )
{
    struct glIndexub_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glIndexub( params->c );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glIndexubv( void *args )
{
    struct glIndexubv_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glIndexubv( params->c );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glInitNames( void *args )
{
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glInitNames();
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glInterleavedArrays( void *args )
{
    struct glInterleavedArrays_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glInterleavedArrays( params->format, params->stride, params->pointer );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glIsEnabled( void *args )
{
    struct glIsEnabled_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    params->ret = funcs->gl.p_glIsEnabled( params->cap );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glIsList( void *args )
{
    struct glIsList_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    params->ret = funcs->gl.p_glIsList( params->list );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glIsTexture( void *args )
{
    struct glIsTexture_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    params->ret = funcs->gl.p_glIsTexture( params->texture );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glLightModelf( void *args )
{
    struct glLightModelf_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glLightModelf( params->pname, params->param );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glLightModelfv( void *args )
{
    struct glLightModelfv_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glLightModelfv( params->pname, params->params );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glLightModeli( void *args )
{
    struct glLightModeli_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glLightModeli( params->pname, params->param );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glLightModeliv( void *args )
{
    struct glLightModeliv_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glLightModeliv( params->pname, params->params );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glLightf( void *args )
{
    struct glLightf_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glLightf( params->light, params->pname, params->param );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glLightfv( void *args )
{
    struct glLightfv_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glLightfv( params->light, params->pname, params->params );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glLighti( void *args )
{
    struct glLighti_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glLighti( params->light, params->pname, params->param );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glLightiv( void *args )
{
    struct glLightiv_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glLightiv( params->light, params->pname, params->params );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glLineStipple( void *args )
{
    struct glLineStipple_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glLineStipple( params->factor, params->pattern );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glLineWidth( void *args )
{
    struct glLineWidth_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glLineWidth( params->width );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glListBase( void *args )
{
    struct glListBase_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glListBase( params->base );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glLoadIdentity( void *args )
{
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glLoadIdentity();
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glLoadMatrixd( void *args )
{
    struct glLoadMatrixd_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glLoadMatrixd( params->m );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glLoadMatrixf( void *args )
{
    struct glLoadMatrixf_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glLoadMatrixf( params->m );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glLoadName( void *args )
{
    struct glLoadName_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glLoadName( params->name );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glLogicOp( void *args )
{
    struct glLogicOp_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glLogicOp( params->opcode );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glMap1d( void *args )
{
    struct glMap1d_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glMap1d( params->target, params->u1, params->u2, params->stride, params->order, params->points );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glMap1f( void *args )
{
    struct glMap1f_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glMap1f( params->target, params->u1, params->u2, params->stride, params->order, params->points );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glMap2d( void *args )
{
    struct glMap2d_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glMap2d( params->target, params->u1, params->u2, params->ustride, params->uorder, params->v1, params->v2, params->vstride, params->vorder, params->points );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glMap2f( void *args )
{
    struct glMap2f_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glMap2f( params->target, params->u1, params->u2, params->ustride, params->uorder, params->v1, params->v2, params->vstride, params->vorder, params->points );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glMapGrid1d( void *args )
{
    struct glMapGrid1d_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glMapGrid1d( params->un, params->u1, params->u2 );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glMapGrid1f( void *args )
{
    struct glMapGrid1f_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glMapGrid1f( params->un, params->u1, params->u2 );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glMapGrid2d( void *args )
{
    struct glMapGrid2d_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glMapGrid2d( params->un, params->u1, params->u2, params->vn, params->v1, params->v2 );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glMapGrid2f( void *args )
{
    struct glMapGrid2f_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glMapGrid2f( params->un, params->u1, params->u2, params->vn, params->v1, params->v2 );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glMaterialf( void *args )
{
    struct glMaterialf_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glMaterialf( params->face, params->pname, params->param );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glMaterialfv( void *args )
{
    struct glMaterialfv_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glMaterialfv( params->face, params->pname, params->params );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glMateriali( void *args )
{
    struct glMateriali_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glMateriali( params->face, params->pname, params->param );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glMaterialiv( void *args )
{
    struct glMaterialiv_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glMaterialiv( params->face, params->pname, params->params );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glMatrixMode( void *args )
{
    struct glMatrixMode_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glMatrixMode( params->mode );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glMultMatrixd( void *args )
{
    struct glMultMatrixd_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glMultMatrixd( params->m );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glMultMatrixf( void *args )
{
    struct glMultMatrixf_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glMultMatrixf( params->m );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glNewList( void *args )
{
    struct glNewList_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glNewList( params->list, params->mode );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glNormal3b( void *args )
{
    struct glNormal3b_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glNormal3b( params->nx, params->ny, params->nz );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glNormal3bv( void *args )
{
    struct glNormal3bv_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glNormal3bv( params->v );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glNormal3d( void *args )
{
    struct glNormal3d_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glNormal3d( params->nx, params->ny, params->nz );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glNormal3dv( void *args )
{
    struct glNormal3dv_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glNormal3dv( params->v );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glNormal3f( void *args )
{
    struct glNormal3f_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glNormal3f( params->nx, params->ny, params->nz );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glNormal3fv( void *args )
{
    struct glNormal3fv_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glNormal3fv( params->v );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glNormal3i( void *args )
{
    struct glNormal3i_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glNormal3i( params->nx, params->ny, params->nz );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glNormal3iv( void *args )
{
    struct glNormal3iv_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glNormal3iv( params->v );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glNormal3s( void *args )
{
    struct glNormal3s_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glNormal3s( params->nx, params->ny, params->nz );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glNormal3sv( void *args )
{
    struct glNormal3sv_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glNormal3sv( params->v );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glNormalPointer( void *args )
{
    struct glNormalPointer_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glNormalPointer( params->type, params->stride, params->pointer );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glOrtho( void *args )
{
    struct glOrtho_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glOrtho( params->left, params->right, params->bottom, params->top, params->zNear, params->zFar );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glPassThrough( void *args )
{
    struct glPassThrough_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glPassThrough( params->token );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glPixelMapfv( void *args )
{
    struct glPixelMapfv_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glPixelMapfv( params->map, params->mapsize, params->values );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glPixelMapuiv( void *args )
{
    struct glPixelMapuiv_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glPixelMapuiv( params->map, params->mapsize, params->values );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glPixelMapusv( void *args )
{
    struct glPixelMapusv_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glPixelMapusv( params->map, params->mapsize, params->values );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glPixelStoref( void *args )
{
    struct glPixelStoref_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glPixelStoref( params->pname, params->param );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glPixelStorei( void *args )
{
    struct glPixelStorei_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glPixelStorei( params->pname, params->param );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glPixelTransferf( void *args )
{
    struct glPixelTransferf_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glPixelTransferf( params->pname, params->param );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glPixelTransferi( void *args )
{
    struct glPixelTransferi_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glPixelTransferi( params->pname, params->param );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glPixelZoom( void *args )
{
    struct glPixelZoom_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glPixelZoom( params->xfactor, params->yfactor );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glPointSize( void *args )
{
    struct glPointSize_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glPointSize( params->size );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glPolygonMode( void *args )
{
    struct glPolygonMode_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glPolygonMode( params->face, params->mode );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glPolygonOffset( void *args )
{
    struct glPolygonOffset_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glPolygonOffset( params->factor, params->units );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glPolygonStipple( void *args )
{
    struct glPolygonStipple_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glPolygonStipple( params->mask );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glPopAttrib( void *args )
{
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glPopAttrib();
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glPopClientAttrib( void *args )
{
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glPopClientAttrib();
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glPopMatrix( void *args )
{
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glPopMatrix();
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glPopName( void *args )
{
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glPopName();
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glPrioritizeTextures( void *args )
{
    struct glPrioritizeTextures_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glPrioritizeTextures( params->n, params->textures, params->priorities );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glPushAttrib( void *args )
{
    struct glPushAttrib_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glPushAttrib( params->mask );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glPushClientAttrib( void *args )
{
    struct glPushClientAttrib_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glPushClientAttrib( params->mask );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glPushMatrix( void *args )
{
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glPushMatrix();
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glPushName( void *args )
{
    struct glPushName_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glPushName( params->name );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glRasterPos2d( void *args )
{
    struct glRasterPos2d_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glRasterPos2d( params->x, params->y );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glRasterPos2dv( void *args )
{
    struct glRasterPos2dv_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glRasterPos2dv( params->v );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glRasterPos2f( void *args )
{
    struct glRasterPos2f_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glRasterPos2f( params->x, params->y );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glRasterPos2fv( void *args )
{
    struct glRasterPos2fv_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glRasterPos2fv( params->v );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glRasterPos2i( void *args )
{
    struct glRasterPos2i_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glRasterPos2i( params->x, params->y );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glRasterPos2iv( void *args )
{
    struct glRasterPos2iv_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glRasterPos2iv( params->v );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glRasterPos2s( void *args )
{
    struct glRasterPos2s_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glRasterPos2s( params->x, params->y );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glRasterPos2sv( void *args )
{
    struct glRasterPos2sv_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glRasterPos2sv( params->v );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glRasterPos3d( void *args )
{
    struct glRasterPos3d_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glRasterPos3d( params->x, params->y, params->z );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glRasterPos3dv( void *args )
{
    struct glRasterPos3dv_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glRasterPos3dv( params->v );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glRasterPos3f( void *args )
{
    struct glRasterPos3f_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glRasterPos3f( params->x, params->y, params->z );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glRasterPos3fv( void *args )
{
    struct glRasterPos3fv_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glRasterPos3fv( params->v );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glRasterPos3i( void *args )
{
    struct glRasterPos3i_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glRasterPos3i( params->x, params->y, params->z );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glRasterPos3iv( void *args )
{
    struct glRasterPos3iv_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glRasterPos3iv( params->v );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glRasterPos3s( void *args )
{
    struct glRasterPos3s_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glRasterPos3s( params->x, params->y, params->z );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glRasterPos3sv( void *args )
{
    struct glRasterPos3sv_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glRasterPos3sv( params->v );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glRasterPos4d( void *args )
{
    struct glRasterPos4d_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glRasterPos4d( params->x, params->y, params->z, params->w );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glRasterPos4dv( void *args )
{
    struct glRasterPos4dv_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glRasterPos4dv( params->v );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glRasterPos4f( void *args )
{
    struct glRasterPos4f_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glRasterPos4f( params->x, params->y, params->z, params->w );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glRasterPos4fv( void *args )
{
    struct glRasterPos4fv_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glRasterPos4fv( params->v );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glRasterPos4i( void *args )
{
    struct glRasterPos4i_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glRasterPos4i( params->x, params->y, params->z, params->w );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glRasterPos4iv( void *args )
{
    struct glRasterPos4iv_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glRasterPos4iv( params->v );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glRasterPos4s( void *args )
{
    struct glRasterPos4s_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glRasterPos4s( params->x, params->y, params->z, params->w );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glRasterPos4sv( void *args )
{
    struct glRasterPos4sv_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glRasterPos4sv( params->v );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glReadBuffer( void *args )
{
    struct glReadBuffer_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glReadBuffer( params->src );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glReadPixels( void *args )
{
    struct glReadPixels_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glReadPixels( params->x, params->y, params->width, params->height, params->format, params->type, params->pixels );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glRectd( void *args )
{
    struct glRectd_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glRectd( params->x1, params->y1, params->x2, params->y2 );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glRectdv( void *args )
{
    struct glRectdv_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glRectdv( params->v1, params->v2 );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glRectf( void *args )
{
    struct glRectf_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glRectf( params->x1, params->y1, params->x2, params->y2 );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glRectfv( void *args )
{
    struct glRectfv_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glRectfv( params->v1, params->v2 );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glRecti( void *args )
{
    struct glRecti_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glRecti( params->x1, params->y1, params->x2, params->y2 );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glRectiv( void *args )
{
    struct glRectiv_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glRectiv( params->v1, params->v2 );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glRects( void *args )
{
    struct glRects_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glRects( params->x1, params->y1, params->x2, params->y2 );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glRectsv( void *args )
{
    struct glRectsv_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glRectsv( params->v1, params->v2 );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glRenderMode( void *args )
{
    struct glRenderMode_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    params->ret = funcs->gl.p_glRenderMode( params->mode );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glRotated( void *args )
{
    struct glRotated_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glRotated( params->angle, params->x, params->y, params->z );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glRotatef( void *args )
{
    struct glRotatef_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glRotatef( params->angle, params->x, params->y, params->z );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glScaled( void *args )
{
    struct glScaled_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glScaled( params->x, params->y, params->z );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glScalef( void *args )
{
    struct glScalef_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glScalef( params->x, params->y, params->z );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glScissor( void *args )
{
    struct glScissor_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glScissor( params->x, params->y, params->width, params->height );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glSelectBuffer( void *args )
{
    struct glSelectBuffer_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glSelectBuffer( params->size, params->buffer );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glShadeModel( void *args )
{
    struct glShadeModel_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glShadeModel( params->mode );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glStencilFunc( void *args )
{
    struct glStencilFunc_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glStencilFunc( params->func, params->ref, params->mask );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glStencilMask( void *args )
{
    struct glStencilMask_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glStencilMask( params->mask );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glStencilOp( void *args )
{
    struct glStencilOp_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glStencilOp( params->fail, params->zfail, params->zpass );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glTexCoord1d( void *args )
{
    struct glTexCoord1d_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glTexCoord1d( params->s );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glTexCoord1dv( void *args )
{
    struct glTexCoord1dv_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glTexCoord1dv( params->v );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glTexCoord1f( void *args )
{
    struct glTexCoord1f_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glTexCoord1f( params->s );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glTexCoord1fv( void *args )
{
    struct glTexCoord1fv_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glTexCoord1fv( params->v );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glTexCoord1i( void *args )
{
    struct glTexCoord1i_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glTexCoord1i( params->s );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glTexCoord1iv( void *args )
{
    struct glTexCoord1iv_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glTexCoord1iv( params->v );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glTexCoord1s( void *args )
{
    struct glTexCoord1s_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glTexCoord1s( params->s );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glTexCoord1sv( void *args )
{
    struct glTexCoord1sv_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glTexCoord1sv( params->v );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glTexCoord2d( void *args )
{
    struct glTexCoord2d_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glTexCoord2d( params->s, params->t );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glTexCoord2dv( void *args )
{
    struct glTexCoord2dv_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glTexCoord2dv( params->v );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glTexCoord2f( void *args )
{
    struct glTexCoord2f_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glTexCoord2f( params->s, params->t );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glTexCoord2fv( void *args )
{
    struct glTexCoord2fv_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glTexCoord2fv( params->v );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glTexCoord2i( void *args )
{
    struct glTexCoord2i_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glTexCoord2i( params->s, params->t );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glTexCoord2iv( void *args )
{
    struct glTexCoord2iv_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glTexCoord2iv( params->v );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glTexCoord2s( void *args )
{
    struct glTexCoord2s_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glTexCoord2s( params->s, params->t );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glTexCoord2sv( void *args )
{
    struct glTexCoord2sv_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glTexCoord2sv( params->v );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glTexCoord3d( void *args )
{
    struct glTexCoord3d_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glTexCoord3d( params->s, params->t, params->r );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glTexCoord3dv( void *args )
{
    struct glTexCoord3dv_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glTexCoord3dv( params->v );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glTexCoord3f( void *args )
{
    struct glTexCoord3f_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glTexCoord3f( params->s, params->t, params->r );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glTexCoord3fv( void *args )
{
    struct glTexCoord3fv_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glTexCoord3fv( params->v );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glTexCoord3i( void *args )
{
    struct glTexCoord3i_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glTexCoord3i( params->s, params->t, params->r );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glTexCoord3iv( void *args )
{
    struct glTexCoord3iv_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glTexCoord3iv( params->v );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glTexCoord3s( void *args )
{
    struct glTexCoord3s_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glTexCoord3s( params->s, params->t, params->r );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glTexCoord3sv( void *args )
{
    struct glTexCoord3sv_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glTexCoord3sv( params->v );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glTexCoord4d( void *args )
{
    struct glTexCoord4d_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glTexCoord4d( params->s, params->t, params->r, params->q );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glTexCoord4dv( void *args )
{
    struct glTexCoord4dv_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glTexCoord4dv( params->v );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glTexCoord4f( void *args )
{
    struct glTexCoord4f_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glTexCoord4f( params->s, params->t, params->r, params->q );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glTexCoord4fv( void *args )
{
    struct glTexCoord4fv_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glTexCoord4fv( params->v );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glTexCoord4i( void *args )
{
    struct glTexCoord4i_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glTexCoord4i( params->s, params->t, params->r, params->q );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glTexCoord4iv( void *args )
{
    struct glTexCoord4iv_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glTexCoord4iv( params->v );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glTexCoord4s( void *args )
{
    struct glTexCoord4s_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glTexCoord4s( params->s, params->t, params->r, params->q );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glTexCoord4sv( void *args )
{
    struct glTexCoord4sv_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glTexCoord4sv( params->v );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glTexCoordPointer( void *args )
{
    struct glTexCoordPointer_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glTexCoordPointer( params->size, params->type, params->stride, params->pointer );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glTexEnvf( void *args )
{
    struct glTexEnvf_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glTexEnvf( params->target, params->pname, params->param );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glTexEnvfv( void *args )
{
    struct glTexEnvfv_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glTexEnvfv( params->target, params->pname, params->params );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glTexEnvi( void *args )
{
    struct glTexEnvi_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glTexEnvi( params->target, params->pname, params->param );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glTexEnviv( void *args )
{
    struct glTexEnviv_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glTexEnviv( params->target, params->pname, params->params );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glTexGend( void *args )
{
    struct glTexGend_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glTexGend( params->coord, params->pname, params->param );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glTexGendv( void *args )
{
    struct glTexGendv_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glTexGendv( params->coord, params->pname, params->params );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glTexGenf( void *args )
{
    struct glTexGenf_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glTexGenf( params->coord, params->pname, params->param );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glTexGenfv( void *args )
{
    struct glTexGenfv_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glTexGenfv( params->coord, params->pname, params->params );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glTexGeni( void *args )
{
    struct glTexGeni_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glTexGeni( params->coord, params->pname, params->param );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glTexGeniv( void *args )
{
    struct glTexGeniv_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glTexGeniv( params->coord, params->pname, params->params );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glTexImage1D( void *args )
{
    struct glTexImage1D_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glTexImage1D( params->target, params->level, params->internalformat, params->width, params->border, params->format, params->type, params->pixels );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glTexImage2D( void *args )
{
    struct glTexImage2D_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glTexImage2D( params->target, params->level, params->internalformat, params->width, params->height, params->border, params->format, params->type, params->pixels );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glTexParameterf( void *args )
{
    struct glTexParameterf_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glTexParameterf( params->target, params->pname, params->param );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glTexParameterfv( void *args )
{
    struct glTexParameterfv_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glTexParameterfv( params->target, params->pname, params->params );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glTexParameteri( void *args )
{
    struct glTexParameteri_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glTexParameteri( params->target, params->pname, params->param );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glTexParameteriv( void *args )
{
    struct glTexParameteriv_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glTexParameteriv( params->target, params->pname, params->params );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glTexSubImage1D( void *args )
{
    struct glTexSubImage1D_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glTexSubImage1D( params->target, params->level, params->xoffset, params->width, params->format, params->type, params->pixels );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glTexSubImage2D( void *args )
{
    struct glTexSubImage2D_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glTexSubImage2D( params->target, params->level, params->xoffset, params->yoffset, params->width, params->height, params->format, params->type, params->pixels );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glTranslated( void *args )
{
    struct glTranslated_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glTranslated( params->x, params->y, params->z );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glTranslatef( void *args )
{
    struct glTranslatef_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glTranslatef( params->x, params->y, params->z );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glVertex2d( void *args )
{
    struct glVertex2d_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glVertex2d( params->x, params->y );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glVertex2dv( void *args )
{
    struct glVertex2dv_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glVertex2dv( params->v );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glVertex2f( void *args )
{
    struct glVertex2f_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glVertex2f( params->x, params->y );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glVertex2fv( void *args )
{
    struct glVertex2fv_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glVertex2fv( params->v );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glVertex2i( void *args )
{
    struct glVertex2i_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glVertex2i( params->x, params->y );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glVertex2iv( void *args )
{
    struct glVertex2iv_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glVertex2iv( params->v );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glVertex2s( void *args )
{
    struct glVertex2s_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glVertex2s( params->x, params->y );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glVertex2sv( void *args )
{
    struct glVertex2sv_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glVertex2sv( params->v );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glVertex3d( void *args )
{
    struct glVertex3d_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glVertex3d( params->x, params->y, params->z );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glVertex3dv( void *args )
{
    struct glVertex3dv_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glVertex3dv( params->v );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glVertex3f( void *args )
{
    struct glVertex3f_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glVertex3f( params->x, params->y, params->z );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glVertex3fv( void *args )
{
    struct glVertex3fv_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glVertex3fv( params->v );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glVertex3i( void *args )
{
    struct glVertex3i_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glVertex3i( params->x, params->y, params->z );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glVertex3iv( void *args )
{
    struct glVertex3iv_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glVertex3iv( params->v );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glVertex3s( void *args )
{
    struct glVertex3s_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glVertex3s( params->x, params->y, params->z );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glVertex3sv( void *args )
{
    struct glVertex3sv_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glVertex3sv( params->v );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glVertex4d( void *args )
{
    struct glVertex4d_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glVertex4d( params->x, params->y, params->z, params->w );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glVertex4dv( void *args )
{
    struct glVertex4dv_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glVertex4dv( params->v );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glVertex4f( void *args )
{
    struct glVertex4f_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glVertex4f( params->x, params->y, params->z, params->w );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glVertex4fv( void *args )
{
    struct glVertex4fv_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glVertex4fv( params->v );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glVertex4i( void *args )
{
    struct glVertex4i_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glVertex4i( params->x, params->y, params->z, params->w );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glVertex4iv( void *args )
{
    struct glVertex4iv_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glVertex4iv( params->v );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glVertex4s( void *args )
{
    struct glVertex4s_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glVertex4s( params->x, params->y, params->z, params->w );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glVertex4sv( void *args )
{
    struct glVertex4sv_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glVertex4sv( params->v );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glVertexPointer( void *args )
{
    struct glVertexPointer_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glVertexPointer( params->size, params->type, params->stride, params->pointer );
    return STATUS_SUCCESS;
}

static NTSTATUS gl_glViewport( void *args )
{
    struct glViewport_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glViewport( params->x, params->y, params->width, params->height );
    return STATUS_SUCCESS;
}

const unixlib_function_t __wine_unix_call_funcs[] =
{
    &gl_glAccum,
    &gl_glAlphaFunc,
    &gl_glAreTexturesResident,
    &gl_glArrayElement,
    &gl_glBegin,
    &gl_glBindTexture,
    &gl_glBitmap,
    &gl_glBlendFunc,
    &gl_glCallList,
    &gl_glCallLists,
    &gl_glClear,
    &gl_glClearAccum,
    &gl_glClearColor,
    &gl_glClearDepth,
    &gl_glClearIndex,
    &gl_glClearStencil,
    &gl_glClipPlane,
    &gl_glColor3b,
    &gl_glColor3bv,
    &gl_glColor3d,
    &gl_glColor3dv,
    &gl_glColor3f,
    &gl_glColor3fv,
    &gl_glColor3i,
    &gl_glColor3iv,
    &gl_glColor3s,
    &gl_glColor3sv,
    &gl_glColor3ub,
    &gl_glColor3ubv,
    &gl_glColor3ui,
    &gl_glColor3uiv,
    &gl_glColor3us,
    &gl_glColor3usv,
    &gl_glColor4b,
    &gl_glColor4bv,
    &gl_glColor4d,
    &gl_glColor4dv,
    &gl_glColor4f,
    &gl_glColor4fv,
    &gl_glColor4i,
    &gl_glColor4iv,
    &gl_glColor4s,
    &gl_glColor4sv,
    &gl_glColor4ub,
    &gl_glColor4ubv,
    &gl_glColor4ui,
    &gl_glColor4uiv,
    &gl_glColor4us,
    &gl_glColor4usv,
    &gl_glColorMask,
    &gl_glColorMaterial,
    &gl_glColorPointer,
    &gl_glCopyPixels,
    &gl_glCopyTexImage1D,
    &gl_glCopyTexImage2D,
    &gl_glCopyTexSubImage1D,
    &gl_glCopyTexSubImage2D,
    &gl_glCullFace,
    &gl_glDeleteLists,
    &gl_glDeleteTextures,
    &gl_glDepthFunc,
    &gl_glDepthMask,
    &gl_glDepthRange,
    &gl_glDisable,
    &gl_glDisableClientState,
    &gl_glDrawArrays,
    &gl_glDrawBuffer,
    &gl_glDrawElements,
    &gl_glDrawPixels,
    &gl_glEdgeFlag,
    &gl_glEdgeFlagPointer,
    &gl_glEdgeFlagv,
    &gl_glEnable,
    &gl_glEnableClientState,
    &gl_glEnd,
    &gl_glEndList,
    &gl_glEvalCoord1d,
    &gl_glEvalCoord1dv,
    &gl_glEvalCoord1f,
    &gl_glEvalCoord1fv,
    &gl_glEvalCoord2d,
    &gl_glEvalCoord2dv,
    &gl_glEvalCoord2f,
    &gl_glEvalCoord2fv,
    &gl_glEvalMesh1,
    &gl_glEvalMesh2,
    &gl_glEvalPoint1,
    &gl_glEvalPoint2,
    &gl_glFeedbackBuffer,
    &gl_glFinish,
    &gl_glFlush,
    &gl_glFogf,
    &gl_glFogfv,
    &gl_glFogi,
    &gl_glFogiv,
    &gl_glFrontFace,
    &gl_glFrustum,
    &gl_glGenLists,
    &gl_glGenTextures,
    &gl_glGetBooleanv,
    &gl_glGetClipPlane,
    &gl_glGetDoublev,
    &gl_glGetError,
    &gl_glGetFloatv,
    &gl_glGetLightfv,
    &gl_glGetLightiv,
    &gl_glGetMapdv,
    &gl_glGetMapfv,
    &gl_glGetMapiv,
    &gl_glGetMaterialfv,
    &gl_glGetMaterialiv,
    &gl_glGetPixelMapfv,
    &gl_glGetPixelMapuiv,
    &gl_glGetPixelMapusv,
    &gl_glGetPointerv,
    &gl_glGetPolygonStipple,
    &gl_glGetTexEnvfv,
    &gl_glGetTexEnviv,
    &gl_glGetTexGendv,
    &gl_glGetTexGenfv,
    &gl_glGetTexGeniv,
    &gl_glGetTexImage,
    &gl_glGetTexLevelParameterfv,
    &gl_glGetTexLevelParameteriv,
    &gl_glGetTexParameterfv,
    &gl_glGetTexParameteriv,
    &gl_glHint,
    &gl_glIndexMask,
    &gl_glIndexPointer,
    &gl_glIndexd,
    &gl_glIndexdv,
    &gl_glIndexf,
    &gl_glIndexfv,
    &gl_glIndexi,
    &gl_glIndexiv,
    &gl_glIndexs,
    &gl_glIndexsv,
    &gl_glIndexub,
    &gl_glIndexubv,
    &gl_glInitNames,
    &gl_glInterleavedArrays,
    &gl_glIsEnabled,
    &gl_glIsList,
    &gl_glIsTexture,
    &gl_glLightModelf,
    &gl_glLightModelfv,
    &gl_glLightModeli,
    &gl_glLightModeliv,
    &gl_glLightf,
    &gl_glLightfv,
    &gl_glLighti,
    &gl_glLightiv,
    &gl_glLineStipple,
    &gl_glLineWidth,
    &gl_glListBase,
    &gl_glLoadIdentity,
    &gl_glLoadMatrixd,
    &gl_glLoadMatrixf,
    &gl_glLoadName,
    &gl_glLogicOp,
    &gl_glMap1d,
    &gl_glMap1f,
    &gl_glMap2d,
    &gl_glMap2f,
    &gl_glMapGrid1d,
    &gl_glMapGrid1f,
    &gl_glMapGrid2d,
    &gl_glMapGrid2f,
    &gl_glMaterialf,
    &gl_glMaterialfv,
    &gl_glMateriali,
    &gl_glMaterialiv,
    &gl_glMatrixMode,
    &gl_glMultMatrixd,
    &gl_glMultMatrixf,
    &gl_glNewList,
    &gl_glNormal3b,
    &gl_glNormal3bv,
    &gl_glNormal3d,
    &gl_glNormal3dv,
    &gl_glNormal3f,
    &gl_glNormal3fv,
    &gl_glNormal3i,
    &gl_glNormal3iv,
    &gl_glNormal3s,
    &gl_glNormal3sv,
    &gl_glNormalPointer,
    &gl_glOrtho,
    &gl_glPassThrough,
    &gl_glPixelMapfv,
    &gl_glPixelMapuiv,
    &gl_glPixelMapusv,
    &gl_glPixelStoref,
    &gl_glPixelStorei,
    &gl_glPixelTransferf,
    &gl_glPixelTransferi,
    &gl_glPixelZoom,
    &gl_glPointSize,
    &gl_glPolygonMode,
    &gl_glPolygonOffset,
    &gl_glPolygonStipple,
    &gl_glPopAttrib,
    &gl_glPopClientAttrib,
    &gl_glPopMatrix,
    &gl_glPopName,
    &gl_glPrioritizeTextures,
    &gl_glPushAttrib,
    &gl_glPushClientAttrib,
    &gl_glPushMatrix,
    &gl_glPushName,
    &gl_glRasterPos2d,
    &gl_glRasterPos2dv,
    &gl_glRasterPos2f,
    &gl_glRasterPos2fv,
    &gl_glRasterPos2i,
    &gl_glRasterPos2iv,
    &gl_glRasterPos2s,
    &gl_glRasterPos2sv,
    &gl_glRasterPos3d,
    &gl_glRasterPos3dv,
    &gl_glRasterPos3f,
    &gl_glRasterPos3fv,
    &gl_glRasterPos3i,
    &gl_glRasterPos3iv,
    &gl_glRasterPos3s,
    &gl_glRasterPos3sv,
    &gl_glRasterPos4d,
    &gl_glRasterPos4dv,
    &gl_glRasterPos4f,
    &gl_glRasterPos4fv,
    &gl_glRasterPos4i,
    &gl_glRasterPos4iv,
    &gl_glRasterPos4s,
    &gl_glRasterPos4sv,
    &gl_glReadBuffer,
    &gl_glReadPixels,
    &gl_glRectd,
    &gl_glRectdv,
    &gl_glRectf,
    &gl_glRectfv,
    &gl_glRecti,
    &gl_glRectiv,
    &gl_glRects,
    &gl_glRectsv,
    &gl_glRenderMode,
    &gl_glRotated,
    &gl_glRotatef,
    &gl_glScaled,
    &gl_glScalef,
    &gl_glScissor,
    &gl_glSelectBuffer,
    &gl_glShadeModel,
    &gl_glStencilFunc,
    &gl_glStencilMask,
    &gl_glStencilOp,
    &gl_glTexCoord1d,
    &gl_glTexCoord1dv,
    &gl_glTexCoord1f,
    &gl_glTexCoord1fv,
    &gl_glTexCoord1i,
    &gl_glTexCoord1iv,
    &gl_glTexCoord1s,
    &gl_glTexCoord1sv,
    &gl_glTexCoord2d,
    &gl_glTexCoord2dv,
    &gl_glTexCoord2f,
    &gl_glTexCoord2fv,
    &gl_glTexCoord2i,
    &gl_glTexCoord2iv,
    &gl_glTexCoord2s,
    &gl_glTexCoord2sv,
    &gl_glTexCoord3d,
    &gl_glTexCoord3dv,
    &gl_glTexCoord3f,
    &gl_glTexCoord3fv,
    &gl_glTexCoord3i,
    &gl_glTexCoord3iv,
    &gl_glTexCoord3s,
    &gl_glTexCoord3sv,
    &gl_glTexCoord4d,
    &gl_glTexCoord4dv,
    &gl_glTexCoord4f,
    &gl_glTexCoord4fv,
    &gl_glTexCoord4i,
    &gl_glTexCoord4iv,
    &gl_glTexCoord4s,
    &gl_glTexCoord4sv,
    &gl_glTexCoordPointer,
    &gl_glTexEnvf,
    &gl_glTexEnvfv,
    &gl_glTexEnvi,
    &gl_glTexEnviv,
    &gl_glTexGend,
    &gl_glTexGendv,
    &gl_glTexGenf,
    &gl_glTexGenfv,
    &gl_glTexGeni,
    &gl_glTexGeniv,
    &gl_glTexImage1D,
    &gl_glTexImage2D,
    &gl_glTexParameterf,
    &gl_glTexParameterfv,
    &gl_glTexParameteri,
    &gl_glTexParameteriv,
    &gl_glTexSubImage1D,
    &gl_glTexSubImage2D,
    &gl_glTranslated,
    &gl_glTranslatef,
    &gl_glVertex2d,
    &gl_glVertex2dv,
    &gl_glVertex2f,
    &gl_glVertex2fv,
    &gl_glVertex2i,
    &gl_glVertex2iv,
    &gl_glVertex2s,
    &gl_glVertex2sv,
    &gl_glVertex3d,
    &gl_glVertex3dv,
    &gl_glVertex3f,
    &gl_glVertex3fv,
    &gl_glVertex3i,
    &gl_glVertex3iv,
    &gl_glVertex3s,
    &gl_glVertex3sv,
    &gl_glVertex4d,
    &gl_glVertex4dv,
    &gl_glVertex4f,
    &gl_glVertex4fv,
    &gl_glVertex4i,
    &gl_glVertex4iv,
    &gl_glVertex4s,
    &gl_glVertex4sv,
    &gl_glVertexPointer,
    &gl_glViewport,
};
