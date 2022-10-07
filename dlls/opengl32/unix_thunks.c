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

static NTSTATUS gl_glGetIntegerv( void *args )
{
    struct glGetIntegerv_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    funcs->gl.p_glGetIntegerv( params->pname, params->data );
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

static NTSTATUS gl_glGetString( void *args )
{
    struct glGetString_params *params = args;
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    params->ret = funcs->gl.p_glGetString( params->name );
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
    &gl_glGetIntegerv,
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
    &gl_glGetString,
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

static BOOL WINAPI null_wglCopyContext( struct wgl_context * hglrcSrc, struct wgl_context * hglrcDst, UINT mask ) { return 0; }
static struct wgl_context * WINAPI null_wglCreateContext( HDC hDc ) { return 0; }
static BOOL WINAPI null_wglDeleteContext( struct wgl_context * oldContext ) { return 0; }
static int WINAPI null_wglDescribePixelFormat( HDC hdc, int ipfd, UINT cjpfd, PIXELFORMATDESCRIPTOR *ppfd ) { return 0; }
static int WINAPI null_wglGetPixelFormat( HDC hdc ) { return 0; }
static PROC WINAPI null_wglGetProcAddress( LPCSTR lpszProc ) { return 0; }
static BOOL WINAPI null_wglMakeCurrent( HDC hDc, struct wgl_context * newContext ) { return 0; }
static BOOL WINAPI null_wglSetPixelFormat( HDC hdc, int ipfd, const PIXELFORMATDESCRIPTOR *ppfd ) { return 0; }
static BOOL WINAPI null_wglShareLists( struct wgl_context * hrcSrvShare, struct wgl_context * hrcSrvSource ) { return 0; }
static BOOL WINAPI null_wglSwapBuffers( HDC hdc ) { return 0; }
static void null_glAccum( GLenum op, GLfloat value ) { }
static void null_glAlphaFunc( GLenum func, GLfloat ref ) { }
static GLboolean null_glAreTexturesResident( GLsizei n, const GLuint *textures, GLboolean *residences ) { return 0; }
static void null_glArrayElement( GLint i ) { }
static void null_glBegin( GLenum mode ) { }
static void null_glBindTexture( GLenum target, GLuint texture ) { }
static void null_glBitmap( GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove, const GLubyte *bitmap ) { }
static void null_glBlendFunc( GLenum sfactor, GLenum dfactor ) { }
static void null_glCallList( GLuint list ) { }
static void null_glCallLists( GLsizei n, GLenum type, const void *lists ) { }
static void null_glClear( GLbitfield mask ) { }
static void null_glClearAccum( GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha ) { }
static void null_glClearColor( GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha ) { }
static void null_glClearDepth( GLdouble depth ) { }
static void null_glClearIndex( GLfloat c ) { }
static void null_glClearStencil( GLint s ) { }
static void null_glClipPlane( GLenum plane, const GLdouble *equation ) { }
static void null_glColor3b( GLbyte red, GLbyte green, GLbyte blue ) { }
static void null_glColor3bv( const GLbyte *v ) { }
static void null_glColor3d( GLdouble red, GLdouble green, GLdouble blue ) { }
static void null_glColor3dv( const GLdouble *v ) { }
static void null_glColor3f( GLfloat red, GLfloat green, GLfloat blue ) { }
static void null_glColor3fv( const GLfloat *v ) { }
static void null_glColor3i( GLint red, GLint green, GLint blue ) { }
static void null_glColor3iv( const GLint *v ) { }
static void null_glColor3s( GLshort red, GLshort green, GLshort blue ) { }
static void null_glColor3sv( const GLshort *v ) { }
static void null_glColor3ub( GLubyte red, GLubyte green, GLubyte blue ) { }
static void null_glColor3ubv( const GLubyte *v ) { }
static void null_glColor3ui( GLuint red, GLuint green, GLuint blue ) { }
static void null_glColor3uiv( const GLuint *v ) { }
static void null_glColor3us( GLushort red, GLushort green, GLushort blue ) { }
static void null_glColor3usv( const GLushort *v ) { }
static void null_glColor4b( GLbyte red, GLbyte green, GLbyte blue, GLbyte alpha ) { }
static void null_glColor4bv( const GLbyte *v ) { }
static void null_glColor4d( GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha ) { }
static void null_glColor4dv( const GLdouble *v ) { }
static void null_glColor4f( GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha ) { }
static void null_glColor4fv( const GLfloat *v ) { }
static void null_glColor4i( GLint red, GLint green, GLint blue, GLint alpha ) { }
static void null_glColor4iv( const GLint *v ) { }
static void null_glColor4s( GLshort red, GLshort green, GLshort blue, GLshort alpha ) { }
static void null_glColor4sv( const GLshort *v ) { }
static void null_glColor4ub( GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha ) { }
static void null_glColor4ubv( const GLubyte *v ) { }
static void null_glColor4ui( GLuint red, GLuint green, GLuint blue, GLuint alpha ) { }
static void null_glColor4uiv( const GLuint *v ) { }
static void null_glColor4us( GLushort red, GLushort green, GLushort blue, GLushort alpha ) { }
static void null_glColor4usv( const GLushort *v ) { }
static void null_glColorMask( GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha ) { }
static void null_glColorMaterial( GLenum face, GLenum mode ) { }
static void null_glColorPointer( GLint size, GLenum type, GLsizei stride, const void *pointer ) { }
static void null_glCopyPixels( GLint x, GLint y, GLsizei width, GLsizei height, GLenum type ) { }
static void null_glCopyTexImage1D( GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLint border ) { }
static void null_glCopyTexImage2D( GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border ) { }
static void null_glCopyTexSubImage1D( GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width ) { }
static void null_glCopyTexSubImage2D( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height ) { }
static void null_glCullFace( GLenum mode ) { }
static void null_glDeleteLists( GLuint list, GLsizei range ) { }
static void null_glDeleteTextures( GLsizei n, const GLuint *textures ) { }
static void null_glDepthFunc( GLenum func ) { }
static void null_glDepthMask( GLboolean flag ) { }
static void null_glDepthRange( GLdouble n, GLdouble f ) { }
static void null_glDisable( GLenum cap ) { }
static void null_glDisableClientState( GLenum array ) { }
static void null_glDrawArrays( GLenum mode, GLint first, GLsizei count ) { }
static void null_glDrawBuffer( GLenum buf ) { }
static void null_glDrawElements( GLenum mode, GLsizei count, GLenum type, const void *indices ) { }
static void null_glDrawPixels( GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels ) { }
static void null_glEdgeFlag( GLboolean flag ) { }
static void null_glEdgeFlagPointer( GLsizei stride, const void *pointer ) { }
static void null_glEdgeFlagv( const GLboolean *flag ) { }
static void null_glEnable( GLenum cap ) { }
static void null_glEnableClientState( GLenum array ) { }
static void null_glEnd(void) { }
static void null_glEndList(void) { }
static void null_glEvalCoord1d( GLdouble u ) { }
static void null_glEvalCoord1dv( const GLdouble *u ) { }
static void null_glEvalCoord1f( GLfloat u ) { }
static void null_glEvalCoord1fv( const GLfloat *u ) { }
static void null_glEvalCoord2d( GLdouble u, GLdouble v ) { }
static void null_glEvalCoord2dv( const GLdouble *u ) { }
static void null_glEvalCoord2f( GLfloat u, GLfloat v ) { }
static void null_glEvalCoord2fv( const GLfloat *u ) { }
static void null_glEvalMesh1( GLenum mode, GLint i1, GLint i2 ) { }
static void null_glEvalMesh2( GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2 ) { }
static void null_glEvalPoint1( GLint i ) { }
static void null_glEvalPoint2( GLint i, GLint j ) { }
static void null_glFeedbackBuffer( GLsizei size, GLenum type, GLfloat *buffer ) { }
static void null_glFinish(void) { }
static void null_glFlush(void) { }
static void null_glFogf( GLenum pname, GLfloat param ) { }
static void null_glFogfv( GLenum pname, const GLfloat *params ) { }
static void null_glFogi( GLenum pname, GLint param ) { }
static void null_glFogiv( GLenum pname, const GLint *params ) { }
static void null_glFrontFace( GLenum mode ) { }
static void null_glFrustum( GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar ) { }
static GLuint null_glGenLists( GLsizei range ) { return 0; }
static void null_glGenTextures( GLsizei n, GLuint *textures ) { }
static void null_glGetBooleanv( GLenum pname, GLboolean *data ) { }
static void null_glGetClipPlane( GLenum plane, GLdouble *equation ) { }
static void null_glGetDoublev( GLenum pname, GLdouble *data ) { }
static GLenum null_glGetError(void) { return GL_INVALID_OPERATION; }
static void null_glGetFloatv( GLenum pname, GLfloat *data ) { }
static void null_glGetIntegerv( GLenum pname, GLint *data ) { }
static void null_glGetLightfv( GLenum light, GLenum pname, GLfloat *params ) { }
static void null_glGetLightiv( GLenum light, GLenum pname, GLint *params ) { }
static void null_glGetMapdv( GLenum target, GLenum query, GLdouble *v ) { }
static void null_glGetMapfv( GLenum target, GLenum query, GLfloat *v ) { }
static void null_glGetMapiv( GLenum target, GLenum query, GLint *v ) { }
static void null_glGetMaterialfv( GLenum face, GLenum pname, GLfloat *params ) { }
static void null_glGetMaterialiv( GLenum face, GLenum pname, GLint *params ) { }
static void null_glGetPixelMapfv( GLenum map, GLfloat *values ) { }
static void null_glGetPixelMapuiv( GLenum map, GLuint *values ) { }
static void null_glGetPixelMapusv( GLenum map, GLushort *values ) { }
static void null_glGetPointerv( GLenum pname, void **params ) { }
static void null_glGetPolygonStipple( GLubyte *mask ) { }
static const GLubyte * null_glGetString( GLenum name ) { return 0; }
static void null_glGetTexEnvfv( GLenum target, GLenum pname, GLfloat *params ) { }
static void null_glGetTexEnviv( GLenum target, GLenum pname, GLint *params ) { }
static void null_glGetTexGendv( GLenum coord, GLenum pname, GLdouble *params ) { }
static void null_glGetTexGenfv( GLenum coord, GLenum pname, GLfloat *params ) { }
static void null_glGetTexGeniv( GLenum coord, GLenum pname, GLint *params ) { }
static void null_glGetTexImage( GLenum target, GLint level, GLenum format, GLenum type, void *pixels ) { }
static void null_glGetTexLevelParameterfv( GLenum target, GLint level, GLenum pname, GLfloat *params ) { }
static void null_glGetTexLevelParameteriv( GLenum target, GLint level, GLenum pname, GLint *params ) { }
static void null_glGetTexParameterfv( GLenum target, GLenum pname, GLfloat *params ) { }
static void null_glGetTexParameteriv( GLenum target, GLenum pname, GLint *params ) { }
static void null_glHint( GLenum target, GLenum mode ) { }
static void null_glIndexMask( GLuint mask ) { }
static void null_glIndexPointer( GLenum type, GLsizei stride, const void *pointer ) { }
static void null_glIndexd( GLdouble c ) { }
static void null_glIndexdv( const GLdouble *c ) { }
static void null_glIndexf( GLfloat c ) { }
static void null_glIndexfv( const GLfloat *c ) { }
static void null_glIndexi( GLint c ) { }
static void null_glIndexiv( const GLint *c ) { }
static void null_glIndexs( GLshort c ) { }
static void null_glIndexsv( const GLshort *c ) { }
static void null_glIndexub( GLubyte c ) { }
static void null_glIndexubv( const GLubyte *c ) { }
static void null_glInitNames(void) { }
static void null_glInterleavedArrays( GLenum format, GLsizei stride, const void *pointer ) { }
static GLboolean null_glIsEnabled( GLenum cap ) { return 0; }
static GLboolean null_glIsList( GLuint list ) { return 0; }
static GLboolean null_glIsTexture( GLuint texture ) { return 0; }
static void null_glLightModelf( GLenum pname, GLfloat param ) { }
static void null_glLightModelfv( GLenum pname, const GLfloat *params ) { }
static void null_glLightModeli( GLenum pname, GLint param ) { }
static void null_glLightModeliv( GLenum pname, const GLint *params ) { }
static void null_glLightf( GLenum light, GLenum pname, GLfloat param ) { }
static void null_glLightfv( GLenum light, GLenum pname, const GLfloat *params ) { }
static void null_glLighti( GLenum light, GLenum pname, GLint param ) { }
static void null_glLightiv( GLenum light, GLenum pname, const GLint *params ) { }
static void null_glLineStipple( GLint factor, GLushort pattern ) { }
static void null_glLineWidth( GLfloat width ) { }
static void null_glListBase( GLuint base ) { }
static void null_glLoadIdentity(void) { }
static void null_glLoadMatrixd( const GLdouble *m ) { }
static void null_glLoadMatrixf( const GLfloat *m ) { }
static void null_glLoadName( GLuint name ) { }
static void null_glLogicOp( GLenum opcode ) { }
static void null_glMap1d( GLenum target, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble *points ) { }
static void null_glMap1f( GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat *points ) { }
static void null_glMap2d( GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, const GLdouble *points ) { }
static void null_glMap2f( GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat *points ) { }
static void null_glMapGrid1d( GLint un, GLdouble u1, GLdouble u2 ) { }
static void null_glMapGrid1f( GLint un, GLfloat u1, GLfloat u2 ) { }
static void null_glMapGrid2d( GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2 ) { }
static void null_glMapGrid2f( GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2 ) { }
static void null_glMaterialf( GLenum face, GLenum pname, GLfloat param ) { }
static void null_glMaterialfv( GLenum face, GLenum pname, const GLfloat *params ) { }
static void null_glMateriali( GLenum face, GLenum pname, GLint param ) { }
static void null_glMaterialiv( GLenum face, GLenum pname, const GLint *params ) { }
static void null_glMatrixMode( GLenum mode ) { }
static void null_glMultMatrixd( const GLdouble *m ) { }
static void null_glMultMatrixf( const GLfloat *m ) { }
static void null_glNewList( GLuint list, GLenum mode ) { }
static void null_glNormal3b( GLbyte nx, GLbyte ny, GLbyte nz ) { }
static void null_glNormal3bv( const GLbyte *v ) { }
static void null_glNormal3d( GLdouble nx, GLdouble ny, GLdouble nz ) { }
static void null_glNormal3dv( const GLdouble *v ) { }
static void null_glNormal3f( GLfloat nx, GLfloat ny, GLfloat nz ) { }
static void null_glNormal3fv( const GLfloat *v ) { }
static void null_glNormal3i( GLint nx, GLint ny, GLint nz ) { }
static void null_glNormal3iv( const GLint *v ) { }
static void null_glNormal3s( GLshort nx, GLshort ny, GLshort nz ) { }
static void null_glNormal3sv( const GLshort *v ) { }
static void null_glNormalPointer( GLenum type, GLsizei stride, const void *pointer ) { }
static void null_glOrtho( GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar ) { }
static void null_glPassThrough( GLfloat token ) { }
static void null_glPixelMapfv( GLenum map, GLsizei mapsize, const GLfloat *values ) { }
static void null_glPixelMapuiv( GLenum map, GLsizei mapsize, const GLuint *values ) { }
static void null_glPixelMapusv( GLenum map, GLsizei mapsize, const GLushort *values ) { }
static void null_glPixelStoref( GLenum pname, GLfloat param ) { }
static void null_glPixelStorei( GLenum pname, GLint param ) { }
static void null_glPixelTransferf( GLenum pname, GLfloat param ) { }
static void null_glPixelTransferi( GLenum pname, GLint param ) { }
static void null_glPixelZoom( GLfloat xfactor, GLfloat yfactor ) { }
static void null_glPointSize( GLfloat size ) { }
static void null_glPolygonMode( GLenum face, GLenum mode ) { }
static void null_glPolygonOffset( GLfloat factor, GLfloat units ) { }
static void null_glPolygonStipple( const GLubyte *mask ) { }
static void null_glPopAttrib(void) { }
static void null_glPopClientAttrib(void) { }
static void null_glPopMatrix(void) { }
static void null_glPopName(void) { }
static void null_glPrioritizeTextures( GLsizei n, const GLuint *textures, const GLfloat *priorities ) { }
static void null_glPushAttrib( GLbitfield mask ) { }
static void null_glPushClientAttrib( GLbitfield mask ) { }
static void null_glPushMatrix(void) { }
static void null_glPushName( GLuint name ) { }
static void null_glRasterPos2d( GLdouble x, GLdouble y ) { }
static void null_glRasterPos2dv( const GLdouble *v ) { }
static void null_glRasterPos2f( GLfloat x, GLfloat y ) { }
static void null_glRasterPos2fv( const GLfloat *v ) { }
static void null_glRasterPos2i( GLint x, GLint y ) { }
static void null_glRasterPos2iv( const GLint *v ) { }
static void null_glRasterPos2s( GLshort x, GLshort y ) { }
static void null_glRasterPos2sv( const GLshort *v ) { }
static void null_glRasterPos3d( GLdouble x, GLdouble y, GLdouble z ) { }
static void null_glRasterPos3dv( const GLdouble *v ) { }
static void null_glRasterPos3f( GLfloat x, GLfloat y, GLfloat z ) { }
static void null_glRasterPos3fv( const GLfloat *v ) { }
static void null_glRasterPos3i( GLint x, GLint y, GLint z ) { }
static void null_glRasterPos3iv( const GLint *v ) { }
static void null_glRasterPos3s( GLshort x, GLshort y, GLshort z ) { }
static void null_glRasterPos3sv( const GLshort *v ) { }
static void null_glRasterPos4d( GLdouble x, GLdouble y, GLdouble z, GLdouble w ) { }
static void null_glRasterPos4dv( const GLdouble *v ) { }
static void null_glRasterPos4f( GLfloat x, GLfloat y, GLfloat z, GLfloat w ) { }
static void null_glRasterPos4fv( const GLfloat *v ) { }
static void null_glRasterPos4i( GLint x, GLint y, GLint z, GLint w ) { }
static void null_glRasterPos4iv( const GLint *v ) { }
static void null_glRasterPos4s( GLshort x, GLshort y, GLshort z, GLshort w ) { }
static void null_glRasterPos4sv( const GLshort *v ) { }
static void null_glReadBuffer( GLenum src ) { }
static void null_glReadPixels( GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, void *pixels ) { }
static void null_glRectd( GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2 ) { }
static void null_glRectdv( const GLdouble *v1, const GLdouble *v2 ) { }
static void null_glRectf( GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2 ) { }
static void null_glRectfv( const GLfloat *v1, const GLfloat *v2 ) { }
static void null_glRecti( GLint x1, GLint y1, GLint x2, GLint y2 ) { }
static void null_glRectiv( const GLint *v1, const GLint *v2 ) { }
static void null_glRects( GLshort x1, GLshort y1, GLshort x2, GLshort y2 ) { }
static void null_glRectsv( const GLshort *v1, const GLshort *v2 ) { }
static GLint null_glRenderMode( GLenum mode ) { return 0; }
static void null_glRotated( GLdouble angle, GLdouble x, GLdouble y, GLdouble z ) { }
static void null_glRotatef( GLfloat angle, GLfloat x, GLfloat y, GLfloat z ) { }
static void null_glScaled( GLdouble x, GLdouble y, GLdouble z ) { }
static void null_glScalef( GLfloat x, GLfloat y, GLfloat z ) { }
static void null_glScissor( GLint x, GLint y, GLsizei width, GLsizei height ) { }
static void null_glSelectBuffer( GLsizei size, GLuint *buffer ) { }
static void null_glShadeModel( GLenum mode ) { }
static void null_glStencilFunc( GLenum func, GLint ref, GLuint mask ) { }
static void null_glStencilMask( GLuint mask ) { }
static void null_glStencilOp( GLenum fail, GLenum zfail, GLenum zpass ) { }
static void null_glTexCoord1d( GLdouble s ) { }
static void null_glTexCoord1dv( const GLdouble *v ) { }
static void null_glTexCoord1f( GLfloat s ) { }
static void null_glTexCoord1fv( const GLfloat *v ) { }
static void null_glTexCoord1i( GLint s ) { }
static void null_glTexCoord1iv( const GLint *v ) { }
static void null_glTexCoord1s( GLshort s ) { }
static void null_glTexCoord1sv( const GLshort *v ) { }
static void null_glTexCoord2d( GLdouble s, GLdouble t ) { }
static void null_glTexCoord2dv( const GLdouble *v ) { }
static void null_glTexCoord2f( GLfloat s, GLfloat t ) { }
static void null_glTexCoord2fv( const GLfloat *v ) { }
static void null_glTexCoord2i( GLint s, GLint t ) { }
static void null_glTexCoord2iv( const GLint *v ) { }
static void null_glTexCoord2s( GLshort s, GLshort t ) { }
static void null_glTexCoord2sv( const GLshort *v ) { }
static void null_glTexCoord3d( GLdouble s, GLdouble t, GLdouble r ) { }
static void null_glTexCoord3dv( const GLdouble *v ) { }
static void null_glTexCoord3f( GLfloat s, GLfloat t, GLfloat r ) { }
static void null_glTexCoord3fv( const GLfloat *v ) { }
static void null_glTexCoord3i( GLint s, GLint t, GLint r ) { }
static void null_glTexCoord3iv( const GLint *v ) { }
static void null_glTexCoord3s( GLshort s, GLshort t, GLshort r ) { }
static void null_glTexCoord3sv( const GLshort *v ) { }
static void null_glTexCoord4d( GLdouble s, GLdouble t, GLdouble r, GLdouble q ) { }
static void null_glTexCoord4dv( const GLdouble *v ) { }
static void null_glTexCoord4f( GLfloat s, GLfloat t, GLfloat r, GLfloat q ) { }
static void null_glTexCoord4fv( const GLfloat *v ) { }
static void null_glTexCoord4i( GLint s, GLint t, GLint r, GLint q ) { }
static void null_glTexCoord4iv( const GLint *v ) { }
static void null_glTexCoord4s( GLshort s, GLshort t, GLshort r, GLshort q ) { }
static void null_glTexCoord4sv( const GLshort *v ) { }
static void null_glTexCoordPointer( GLint size, GLenum type, GLsizei stride, const void *pointer ) { }
static void null_glTexEnvf( GLenum target, GLenum pname, GLfloat param ) { }
static void null_glTexEnvfv( GLenum target, GLenum pname, const GLfloat *params ) { }
static void null_glTexEnvi( GLenum target, GLenum pname, GLint param ) { }
static void null_glTexEnviv( GLenum target, GLenum pname, const GLint *params ) { }
static void null_glTexGend( GLenum coord, GLenum pname, GLdouble param ) { }
static void null_glTexGendv( GLenum coord, GLenum pname, const GLdouble *params ) { }
static void null_glTexGenf( GLenum coord, GLenum pname, GLfloat param ) { }
static void null_glTexGenfv( GLenum coord, GLenum pname, const GLfloat *params ) { }
static void null_glTexGeni( GLenum coord, GLenum pname, GLint param ) { }
static void null_glTexGeniv( GLenum coord, GLenum pname, const GLint *params ) { }
static void null_glTexImage1D( GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const void *pixels ) { }
static void null_glTexImage2D( GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void *pixels ) { }
static void null_glTexParameterf( GLenum target, GLenum pname, GLfloat param ) { }
static void null_glTexParameterfv( GLenum target, GLenum pname, const GLfloat *params ) { }
static void null_glTexParameteri( GLenum target, GLenum pname, GLint param ) { }
static void null_glTexParameteriv( GLenum target, GLenum pname, const GLint *params ) { }
static void null_glTexSubImage1D( GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const void *pixels ) { }
static void null_glTexSubImage2D( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels ) { }
static void null_glTranslated( GLdouble x, GLdouble y, GLdouble z ) { }
static void null_glTranslatef( GLfloat x, GLfloat y, GLfloat z ) { }
static void null_glVertex2d( GLdouble x, GLdouble y ) { }
static void null_glVertex2dv( const GLdouble *v ) { }
static void null_glVertex2f( GLfloat x, GLfloat y ) { }
static void null_glVertex2fv( const GLfloat *v ) { }
static void null_glVertex2i( GLint x, GLint y ) { }
static void null_glVertex2iv( const GLint *v ) { }
static void null_glVertex2s( GLshort x, GLshort y ) { }
static void null_glVertex2sv( const GLshort *v ) { }
static void null_glVertex3d( GLdouble x, GLdouble y, GLdouble z ) { }
static void null_glVertex3dv( const GLdouble *v ) { }
static void null_glVertex3f( GLfloat x, GLfloat y, GLfloat z ) { }
static void null_glVertex3fv( const GLfloat *v ) { }
static void null_glVertex3i( GLint x, GLint y, GLint z ) { }
static void null_glVertex3iv( const GLint *v ) { }
static void null_glVertex3s( GLshort x, GLshort y, GLshort z ) { }
static void null_glVertex3sv( const GLshort *v ) { }
static void null_glVertex4d( GLdouble x, GLdouble y, GLdouble z, GLdouble w ) { }
static void null_glVertex4dv( const GLdouble *v ) { }
static void null_glVertex4f( GLfloat x, GLfloat y, GLfloat z, GLfloat w ) { }
static void null_glVertex4fv( const GLfloat *v ) { }
static void null_glVertex4i( GLint x, GLint y, GLint z, GLint w ) { }
static void null_glVertex4iv( const GLint *v ) { }
static void null_glVertex4s( GLshort x, GLshort y, GLshort z, GLshort w ) { }
static void null_glVertex4sv( const GLshort *v ) { }
static void null_glVertexPointer( GLint size, GLenum type, GLsizei stride, const void *pointer ) { }
static void null_glViewport( GLint x, GLint y, GLsizei width, GLsizei height ) { }
static void null_glAccumxOES( GLenum op, GLfixed value ) { }
static GLboolean null_glAcquireKeyedMutexWin32EXT( GLuint memory, GLuint64 key, GLuint timeout ) { return 0; }
static void null_glActiveProgramEXT( GLuint program ) { }
static void null_glActiveShaderProgram( GLuint pipeline, GLuint program ) { }
static void null_glActiveStencilFaceEXT( GLenum face ) { }
static void null_glActiveTexture( GLenum texture ) { }
static void null_glActiveTextureARB( GLenum texture ) { }
static void null_glActiveVaryingNV( GLuint program, const GLchar *name ) { }
static void null_glAlphaFragmentOp1ATI( GLenum op, GLuint dst, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod ) { }
static void null_glAlphaFragmentOp2ATI( GLenum op, GLuint dst, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod, GLuint arg2, GLuint arg2Rep, GLuint arg2Mod ) { }
static void null_glAlphaFragmentOp3ATI( GLenum op, GLuint dst, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod, GLuint arg2, GLuint arg2Rep, GLuint arg2Mod, GLuint arg3, GLuint arg3Rep, GLuint arg3Mod ) { }
static void null_glAlphaFuncxOES( GLenum func, GLfixed ref ) { }
static void null_glAlphaToCoverageDitherControlNV( GLenum mode ) { }
static void null_glApplyFramebufferAttachmentCMAAINTEL(void) { }
static void null_glApplyTextureEXT( GLenum mode ) { }
static GLboolean null_glAreProgramsResidentNV( GLsizei n, const GLuint *programs, GLboolean *residences ) { return 0; }
static GLboolean null_glAreTexturesResidentEXT( GLsizei n, const GLuint *textures, GLboolean *residences ) { return 0; }
static void null_glArrayElementEXT( GLint i ) { }
static void null_glArrayObjectATI( GLenum array, GLint size, GLenum type, GLsizei stride, GLuint buffer, GLuint offset ) { }
static GLuint null_glAsyncCopyBufferSubDataNVX( GLsizei waitSemaphoreCount, const GLuint *waitSemaphoreArray, const GLuint64 *fenceValueArray, GLuint readGpu, GLbitfield writeGpuMask, GLuint readBuffer, GLuint writeBuffer, GLintptr readOffset, GLintptr writeOffset, GLsizeiptr size, GLsizei signalSemaphoreCount, const GLuint *signalSemaphoreArray, const GLuint64 *signalValueArray ) { return 0; }
static GLuint null_glAsyncCopyImageSubDataNVX( GLsizei waitSemaphoreCount, const GLuint *waitSemaphoreArray, const GLuint64 *waitValueArray, GLuint srcGpu, GLbitfield dstGpuMask, GLuint srcName, GLenum srcTarget, GLint srcLevel, GLint srcX, GLint srcY, GLint srcZ, GLuint dstName, GLenum dstTarget, GLint dstLevel, GLint dstX, GLint dstY, GLint dstZ, GLsizei srcWidth, GLsizei srcHeight, GLsizei srcDepth, GLsizei signalSemaphoreCount, const GLuint *signalSemaphoreArray, const GLuint64 *signalValueArray ) { return 0; }
static void null_glAsyncMarkerSGIX( GLuint marker ) { }
static void null_glAttachObjectARB( GLhandleARB containerObj, GLhandleARB obj ) { }
static void null_glAttachShader( GLuint program, GLuint shader ) { }
static void null_glBeginConditionalRender( GLuint id, GLenum mode ) { }
static void null_glBeginConditionalRenderNV( GLuint id, GLenum mode ) { }
static void null_glBeginConditionalRenderNVX( GLuint id ) { }
static void null_glBeginFragmentShaderATI(void) { }
static void null_glBeginOcclusionQueryNV( GLuint id ) { }
static void null_glBeginPerfMonitorAMD( GLuint monitor ) { }
static void null_glBeginPerfQueryINTEL( GLuint queryHandle ) { }
static void null_glBeginQuery( GLenum target, GLuint id ) { }
static void null_glBeginQueryARB( GLenum target, GLuint id ) { }
static void null_glBeginQueryIndexed( GLenum target, GLuint index, GLuint id ) { }
static void null_glBeginTransformFeedback( GLenum primitiveMode ) { }
static void null_glBeginTransformFeedbackEXT( GLenum primitiveMode ) { }
static void null_glBeginTransformFeedbackNV( GLenum primitiveMode ) { }
static void null_glBeginVertexShaderEXT(void) { }
static void null_glBeginVideoCaptureNV( GLuint video_capture_slot ) { }
static void null_glBindAttribLocation( GLuint program, GLuint index, const GLchar *name ) { }
static void null_glBindAttribLocationARB( GLhandleARB programObj, GLuint index, const GLcharARB *name ) { }
static void null_glBindBuffer( GLenum target, GLuint buffer ) { }
static void null_glBindBufferARB( GLenum target, GLuint buffer ) { }
static void null_glBindBufferBase( GLenum target, GLuint index, GLuint buffer ) { }
static void null_glBindBufferBaseEXT( GLenum target, GLuint index, GLuint buffer ) { }
static void null_glBindBufferBaseNV( GLenum target, GLuint index, GLuint buffer ) { }
static void null_glBindBufferOffsetEXT( GLenum target, GLuint index, GLuint buffer, GLintptr offset ) { }
static void null_glBindBufferOffsetNV( GLenum target, GLuint index, GLuint buffer, GLintptr offset ) { }
static void null_glBindBufferRange( GLenum target, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size ) { }
static void null_glBindBufferRangeEXT( GLenum target, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size ) { }
static void null_glBindBufferRangeNV( GLenum target, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size ) { }
static void null_glBindBuffersBase( GLenum target, GLuint first, GLsizei count, const GLuint *buffers ) { }
static void null_glBindBuffersRange( GLenum target, GLuint first, GLsizei count, const GLuint *buffers, const GLintptr *offsets, const GLsizeiptr *sizes ) { }
static void null_glBindFragDataLocation( GLuint program, GLuint color, const GLchar *name ) { }
static void null_glBindFragDataLocationEXT( GLuint program, GLuint color, const GLchar *name ) { }
static void null_glBindFragDataLocationIndexed( GLuint program, GLuint colorNumber, GLuint index, const GLchar *name ) { }
static void null_glBindFragmentShaderATI( GLuint id ) { }
static void null_glBindFramebuffer( GLenum target, GLuint framebuffer ) { }
static void null_glBindFramebufferEXT( GLenum target, GLuint framebuffer ) { }
static void null_glBindImageTexture( GLuint unit, GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum access, GLenum format ) { }
static void null_glBindImageTextureEXT( GLuint index, GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum access, GLint format ) { }
static void null_glBindImageTextures( GLuint first, GLsizei count, const GLuint *textures ) { }
static GLuint null_glBindLightParameterEXT( GLenum light, GLenum value ) { return 0; }
static GLuint null_glBindMaterialParameterEXT( GLenum face, GLenum value ) { return 0; }
static void null_glBindMultiTextureEXT( GLenum texunit, GLenum target, GLuint texture ) { }
static GLuint null_glBindParameterEXT( GLenum value ) { return 0; }
static void null_glBindProgramARB( GLenum target, GLuint program ) { }
static void null_glBindProgramNV( GLenum target, GLuint id ) { }
static void null_glBindProgramPipeline( GLuint pipeline ) { }
static void null_glBindRenderbuffer( GLenum target, GLuint renderbuffer ) { }
static void null_glBindRenderbufferEXT( GLenum target, GLuint renderbuffer ) { }
static void null_glBindSampler( GLuint unit, GLuint sampler ) { }
static void null_glBindSamplers( GLuint first, GLsizei count, const GLuint *samplers ) { }
static void null_glBindShadingRateImageNV( GLuint texture ) { }
static GLuint null_glBindTexGenParameterEXT( GLenum unit, GLenum coord, GLenum value ) { return 0; }
static void null_glBindTextureEXT( GLenum target, GLuint texture ) { }
static void null_glBindTextureUnit( GLuint unit, GLuint texture ) { }
static GLuint null_glBindTextureUnitParameterEXT( GLenum unit, GLenum value ) { return 0; }
static void null_glBindTextures( GLuint first, GLsizei count, const GLuint *textures ) { }
static void null_glBindTransformFeedback( GLenum target, GLuint id ) { }
static void null_glBindTransformFeedbackNV( GLenum target, GLuint id ) { }
static void null_glBindVertexArray( GLuint array ) { }
static void null_glBindVertexArrayAPPLE( GLuint array ) { }
static void null_glBindVertexBuffer( GLuint bindingindex, GLuint buffer, GLintptr offset, GLsizei stride ) { }
static void null_glBindVertexBuffers( GLuint first, GLsizei count, const GLuint *buffers, const GLintptr *offsets, const GLsizei *strides ) { }
static void null_glBindVertexShaderEXT( GLuint id ) { }
static void null_glBindVideoCaptureStreamBufferNV( GLuint video_capture_slot, GLuint stream, GLenum frame_region, GLintptrARB offset ) { }
static void null_glBindVideoCaptureStreamTextureNV( GLuint video_capture_slot, GLuint stream, GLenum frame_region, GLenum target, GLuint texture ) { }
static void null_glBinormal3bEXT( GLbyte bx, GLbyte by, GLbyte bz ) { }
static void null_glBinormal3bvEXT( const GLbyte *v ) { }
static void null_glBinormal3dEXT( GLdouble bx, GLdouble by, GLdouble bz ) { }
static void null_glBinormal3dvEXT( const GLdouble *v ) { }
static void null_glBinormal3fEXT( GLfloat bx, GLfloat by, GLfloat bz ) { }
static void null_glBinormal3fvEXT( const GLfloat *v ) { }
static void null_glBinormal3iEXT( GLint bx, GLint by, GLint bz ) { }
static void null_glBinormal3ivEXT( const GLint *v ) { }
static void null_glBinormal3sEXT( GLshort bx, GLshort by, GLshort bz ) { }
static void null_glBinormal3svEXT( const GLshort *v ) { }
static void null_glBinormalPointerEXT( GLenum type, GLsizei stride, const void *pointer ) { }
static void null_glBitmapxOES( GLsizei width, GLsizei height, GLfixed xorig, GLfixed yorig, GLfixed xmove, GLfixed ymove, const GLubyte *bitmap ) { }
static void null_glBlendBarrierKHR(void) { }
static void null_glBlendBarrierNV(void) { }
static void null_glBlendColor( GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha ) { }
static void null_glBlendColorEXT( GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha ) { }
static void null_glBlendColorxOES( GLfixed red, GLfixed green, GLfixed blue, GLfixed alpha ) { }
static void null_glBlendEquation( GLenum mode ) { }
static void null_glBlendEquationEXT( GLenum mode ) { }
static void null_glBlendEquationIndexedAMD( GLuint buf, GLenum mode ) { }
static void null_glBlendEquationSeparate( GLenum modeRGB, GLenum modeAlpha ) { }
static void null_glBlendEquationSeparateEXT( GLenum modeRGB, GLenum modeAlpha ) { }
static void null_glBlendEquationSeparateIndexedAMD( GLuint buf, GLenum modeRGB, GLenum modeAlpha ) { }
static void null_glBlendEquationSeparatei( GLuint buf, GLenum modeRGB, GLenum modeAlpha ) { }
static void null_glBlendEquationSeparateiARB( GLuint buf, GLenum modeRGB, GLenum modeAlpha ) { }
static void null_glBlendEquationi( GLuint buf, GLenum mode ) { }
static void null_glBlendEquationiARB( GLuint buf, GLenum mode ) { }
static void null_glBlendFuncIndexedAMD( GLuint buf, GLenum src, GLenum dst ) { }
static void null_glBlendFuncSeparate( GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha ) { }
static void null_glBlendFuncSeparateEXT( GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha ) { }
static void null_glBlendFuncSeparateINGR( GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha ) { }
static void null_glBlendFuncSeparateIndexedAMD( GLuint buf, GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha ) { }
static void null_glBlendFuncSeparatei( GLuint buf, GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha ) { }
static void null_glBlendFuncSeparateiARB( GLuint buf, GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha ) { }
static void null_glBlendFunci( GLuint buf, GLenum src, GLenum dst ) { }
static void null_glBlendFunciARB( GLuint buf, GLenum src, GLenum dst ) { }
static void null_glBlendParameteriNV( GLenum pname, GLint value ) { }
static void null_glBlitFramebuffer( GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter ) { }
static void null_glBlitFramebufferEXT( GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter ) { }
static void null_glBlitNamedFramebuffer( GLuint readFramebuffer, GLuint drawFramebuffer, GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter ) { }
static void null_glBufferAddressRangeNV( GLenum pname, GLuint index, GLuint64EXT address, GLsizeiptr length ) { }
static void null_glBufferAttachMemoryNV( GLenum target, GLuint memory, GLuint64 offset ) { }
static void null_glBufferData( GLenum target, GLsizeiptr size, const void *data, GLenum usage ) { }
static void null_glBufferDataARB( GLenum target, GLsizeiptrARB size, const void *data, GLenum usage ) { }
static void null_glBufferPageCommitmentARB( GLenum target, GLintptr offset, GLsizeiptr size, GLboolean commit ) { }
static void null_glBufferParameteriAPPLE( GLenum target, GLenum pname, GLint param ) { }
static GLuint null_glBufferRegionEnabled(void) { return 0; }
static void null_glBufferStorage( GLenum target, GLsizeiptr size, const void *data, GLbitfield flags ) { }
static void null_glBufferStorageExternalEXT( GLenum target, GLintptr offset, GLsizeiptr size, GLeglClientBufferEXT clientBuffer, GLbitfield flags ) { }
static void null_glBufferStorageMemEXT( GLenum target, GLsizeiptr size, GLuint memory, GLuint64 offset ) { }
static void null_glBufferSubData( GLenum target, GLintptr offset, GLsizeiptr size, const void *data ) { }
static void null_glBufferSubDataARB( GLenum target, GLintptrARB offset, GLsizeiptrARB size, const void *data ) { }
static void null_glCallCommandListNV( GLuint list ) { }
static GLenum null_glCheckFramebufferStatus( GLenum target ) { return 0; }
static GLenum null_glCheckFramebufferStatusEXT( GLenum target ) { return 0; }
static GLenum null_glCheckNamedFramebufferStatus( GLuint framebuffer, GLenum target ) { return 0; }
static GLenum null_glCheckNamedFramebufferStatusEXT( GLuint framebuffer, GLenum target ) { return 0; }
static void null_glClampColor( GLenum target, GLenum clamp ) { }
static void null_glClampColorARB( GLenum target, GLenum clamp ) { }
static void null_glClearAccumxOES( GLfixed red, GLfixed green, GLfixed blue, GLfixed alpha ) { }
static void null_glClearBufferData( GLenum target, GLenum internalformat, GLenum format, GLenum type, const void *data ) { }
static void null_glClearBufferSubData( GLenum target, GLenum internalformat, GLintptr offset, GLsizeiptr size, GLenum format, GLenum type, const void *data ) { }
static void null_glClearBufferfi( GLenum buffer, GLint drawbuffer, GLfloat depth, GLint stencil ) { }
static void null_glClearBufferfv( GLenum buffer, GLint drawbuffer, const GLfloat *value ) { }
static void null_glClearBufferiv( GLenum buffer, GLint drawbuffer, const GLint *value ) { }
static void null_glClearBufferuiv( GLenum buffer, GLint drawbuffer, const GLuint *value ) { }
static void null_glClearColorIiEXT( GLint red, GLint green, GLint blue, GLint alpha ) { }
static void null_glClearColorIuiEXT( GLuint red, GLuint green, GLuint blue, GLuint alpha ) { }
static void null_glClearColorxOES( GLfixed red, GLfixed green, GLfixed blue, GLfixed alpha ) { }
static void null_glClearDepthdNV( GLdouble depth ) { }
static void null_glClearDepthf( GLfloat d ) { }
static void null_glClearDepthfOES( GLclampf depth ) { }
static void null_glClearDepthxOES( GLfixed depth ) { }
static void null_glClearNamedBufferData( GLuint buffer, GLenum internalformat, GLenum format, GLenum type, const void *data ) { }
static void null_glClearNamedBufferDataEXT( GLuint buffer, GLenum internalformat, GLenum format, GLenum type, const void *data ) { }
static void null_glClearNamedBufferSubData( GLuint buffer, GLenum internalformat, GLintptr offset, GLsizeiptr size, GLenum format, GLenum type, const void *data ) { }
static void null_glClearNamedBufferSubDataEXT( GLuint buffer, GLenum internalformat, GLsizeiptr offset, GLsizeiptr size, GLenum format, GLenum type, const void *data ) { }
static void null_glClearNamedFramebufferfi( GLuint framebuffer, GLenum buffer, GLint drawbuffer, GLfloat depth, GLint stencil ) { }
static void null_glClearNamedFramebufferfv( GLuint framebuffer, GLenum buffer, GLint drawbuffer, const GLfloat *value ) { }
static void null_glClearNamedFramebufferiv( GLuint framebuffer, GLenum buffer, GLint drawbuffer, const GLint *value ) { }
static void null_glClearNamedFramebufferuiv( GLuint framebuffer, GLenum buffer, GLint drawbuffer, const GLuint *value ) { }
static void null_glClearTexImage( GLuint texture, GLint level, GLenum format, GLenum type, const void *data ) { }
static void null_glClearTexSubImage( GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void *data ) { }
static void null_glClientActiveTexture( GLenum texture ) { }
static void null_glClientActiveTextureARB( GLenum texture ) { }
static void null_glClientActiveVertexStreamATI( GLenum stream ) { }
static void null_glClientAttribDefaultEXT( GLbitfield mask ) { }
static void null_glClientWaitSemaphoreui64NVX( GLsizei fenceObjectCount, const GLuint *semaphoreArray, const GLuint64 *fenceValueArray ) { }
static GLenum null_glClientWaitSync( GLsync sync, GLbitfield flags, GLuint64 timeout ) { return 0; }
static void null_glClipControl( GLenum origin, GLenum depth ) { }
static void null_glClipPlanefOES( GLenum plane, const GLfloat *equation ) { }
static void null_glClipPlanexOES( GLenum plane, const GLfixed *equation ) { }
static void null_glColor3fVertex3fSUN( GLfloat r, GLfloat g, GLfloat b, GLfloat x, GLfloat y, GLfloat z ) { }
static void null_glColor3fVertex3fvSUN( const GLfloat *c, const GLfloat *v ) { }
static void null_glColor3hNV( GLhalfNV red, GLhalfNV green, GLhalfNV blue ) { }
static void null_glColor3hvNV( const GLhalfNV *v ) { }
static void null_glColor3xOES( GLfixed red, GLfixed green, GLfixed blue ) { }
static void null_glColor3xvOES( const GLfixed *components ) { }
static void null_glColor4fNormal3fVertex3fSUN( GLfloat r, GLfloat g, GLfloat b, GLfloat a, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z ) { }
static void null_glColor4fNormal3fVertex3fvSUN( const GLfloat *c, const GLfloat *n, const GLfloat *v ) { }
static void null_glColor4hNV( GLhalfNV red, GLhalfNV green, GLhalfNV blue, GLhalfNV alpha ) { }
static void null_glColor4hvNV( const GLhalfNV *v ) { }
static void null_glColor4ubVertex2fSUN( GLubyte r, GLubyte g, GLubyte b, GLubyte a, GLfloat x, GLfloat y ) { }
static void null_glColor4ubVertex2fvSUN( const GLubyte *c, const GLfloat *v ) { }
static void null_glColor4ubVertex3fSUN( GLubyte r, GLubyte g, GLubyte b, GLubyte a, GLfloat x, GLfloat y, GLfloat z ) { }
static void null_glColor4ubVertex3fvSUN( const GLubyte *c, const GLfloat *v ) { }
static void null_glColor4xOES( GLfixed red, GLfixed green, GLfixed blue, GLfixed alpha ) { }
static void null_glColor4xvOES( const GLfixed *components ) { }
static void null_glColorFormatNV( GLint size, GLenum type, GLsizei stride ) { }
static void null_glColorFragmentOp1ATI( GLenum op, GLuint dst, GLuint dstMask, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod ) { }
static void null_glColorFragmentOp2ATI( GLenum op, GLuint dst, GLuint dstMask, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod, GLuint arg2, GLuint arg2Rep, GLuint arg2Mod ) { }
static void null_glColorFragmentOp3ATI( GLenum op, GLuint dst, GLuint dstMask, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod, GLuint arg2, GLuint arg2Rep, GLuint arg2Mod, GLuint arg3, GLuint arg3Rep, GLuint arg3Mod ) { }
static void null_glColorMaskIndexedEXT( GLuint index, GLboolean r, GLboolean g, GLboolean b, GLboolean a ) { }
static void null_glColorMaski( GLuint index, GLboolean r, GLboolean g, GLboolean b, GLboolean a ) { }
static void null_glColorP3ui( GLenum type, GLuint color ) { }
static void null_glColorP3uiv( GLenum type, const GLuint *color ) { }
static void null_glColorP4ui( GLenum type, GLuint color ) { }
static void null_glColorP4uiv( GLenum type, const GLuint *color ) { }
static void null_glColorPointerEXT( GLint size, GLenum type, GLsizei stride, GLsizei count, const void *pointer ) { }
static void null_glColorPointerListIBM( GLint size, GLenum type, GLint stride, const void **pointer, GLint ptrstride ) { }
static void null_glColorPointervINTEL( GLint size, GLenum type, const void **pointer ) { }
static void null_glColorSubTable( GLenum target, GLsizei start, GLsizei count, GLenum format, GLenum type, const void *data ) { }
static void null_glColorSubTableEXT( GLenum target, GLsizei start, GLsizei count, GLenum format, GLenum type, const void *data ) { }
static void null_glColorTable( GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const void *table ) { }
static void null_glColorTableEXT( GLenum target, GLenum internalFormat, GLsizei width, GLenum format, GLenum type, const void *table ) { }
static void null_glColorTableParameterfv( GLenum target, GLenum pname, const GLfloat *params ) { }
static void null_glColorTableParameterfvSGI( GLenum target, GLenum pname, const GLfloat *params ) { }
static void null_glColorTableParameteriv( GLenum target, GLenum pname, const GLint *params ) { }
static void null_glColorTableParameterivSGI( GLenum target, GLenum pname, const GLint *params ) { }
static void null_glColorTableSGI( GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const void *table ) { }
static void null_glCombinerInputNV( GLenum stage, GLenum portion, GLenum variable, GLenum input, GLenum mapping, GLenum componentUsage ) { }
static void null_glCombinerOutputNV( GLenum stage, GLenum portion, GLenum abOutput, GLenum cdOutput, GLenum sumOutput, GLenum scale, GLenum bias, GLboolean abDotProduct, GLboolean cdDotProduct, GLboolean muxSum ) { }
static void null_glCombinerParameterfNV( GLenum pname, GLfloat param ) { }
static void null_glCombinerParameterfvNV( GLenum pname, const GLfloat *params ) { }
static void null_glCombinerParameteriNV( GLenum pname, GLint param ) { }
static void null_glCombinerParameterivNV( GLenum pname, const GLint *params ) { }
static void null_glCombinerStageParameterfvNV( GLenum stage, GLenum pname, const GLfloat *params ) { }
static void null_glCommandListSegmentsNV( GLuint list, GLuint segments ) { }
static void null_glCompileCommandListNV( GLuint list ) { }
static void null_glCompileShader( GLuint shader ) { }
static void null_glCompileShaderARB( GLhandleARB shaderObj ) { }
static void null_glCompileShaderIncludeARB( GLuint shader, GLsizei count, const GLchar *const*path, const GLint *length ) { }
static void null_glCompressedMultiTexImage1DEXT( GLenum texunit, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imageSize, const void *bits ) { }
static void null_glCompressedMultiTexImage2DEXT( GLenum texunit, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void *bits ) { }
static void null_glCompressedMultiTexImage3DEXT( GLenum texunit, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const void *bits ) { }
static void null_glCompressedMultiTexSubImage1DEXT( GLenum texunit, GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const void *bits ) { }
static void null_glCompressedMultiTexSubImage2DEXT( GLenum texunit, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *bits ) { }
static void null_glCompressedMultiTexSubImage3DEXT( GLenum texunit, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void *bits ) { }
static void null_glCompressedTexImage1D( GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imageSize, const void *data ) { }
static void null_glCompressedTexImage1DARB( GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imageSize, const void *data ) { }
static void null_glCompressedTexImage2D( GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void *data ) { }
static void null_glCompressedTexImage2DARB( GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void *data ) { }
static void null_glCompressedTexImage3D( GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const void *data ) { }
static void null_glCompressedTexImage3DARB( GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const void *data ) { }
static void null_glCompressedTexSubImage1D( GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const void *data ) { }
static void null_glCompressedTexSubImage1DARB( GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const void *data ) { }
static void null_glCompressedTexSubImage2D( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *data ) { }
static void null_glCompressedTexSubImage2DARB( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *data ) { }
static void null_glCompressedTexSubImage3D( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void *data ) { }
static void null_glCompressedTexSubImage3DARB( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void *data ) { }
static void null_glCompressedTextureImage1DEXT( GLuint texture, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imageSize, const void *bits ) { }
static void null_glCompressedTextureImage2DEXT( GLuint texture, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void *bits ) { }
static void null_glCompressedTextureImage3DEXT( GLuint texture, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const void *bits ) { }
static void null_glCompressedTextureSubImage1D( GLuint texture, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const void *data ) { }
static void null_glCompressedTextureSubImage1DEXT( GLuint texture, GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const void *bits ) { }
static void null_glCompressedTextureSubImage2D( GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *data ) { }
static void null_glCompressedTextureSubImage2DEXT( GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *bits ) { }
static void null_glCompressedTextureSubImage3D( GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void *data ) { }
static void null_glCompressedTextureSubImage3DEXT( GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void *bits ) { }
static void null_glConservativeRasterParameterfNV( GLenum pname, GLfloat value ) { }
static void null_glConservativeRasterParameteriNV( GLenum pname, GLint param ) { }
static void null_glConvolutionFilter1D( GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const void *image ) { }
static void null_glConvolutionFilter1DEXT( GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const void *image ) { }
static void null_glConvolutionFilter2D( GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *image ) { }
static void null_glConvolutionFilter2DEXT( GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *image ) { }
static void null_glConvolutionParameterf( GLenum target, GLenum pname, GLfloat params ) { }
static void null_glConvolutionParameterfEXT( GLenum target, GLenum pname, GLfloat params ) { }
static void null_glConvolutionParameterfv( GLenum target, GLenum pname, const GLfloat *params ) { }
static void null_glConvolutionParameterfvEXT( GLenum target, GLenum pname, const GLfloat *params ) { }
static void null_glConvolutionParameteri( GLenum target, GLenum pname, GLint params ) { }
static void null_glConvolutionParameteriEXT( GLenum target, GLenum pname, GLint params ) { }
static void null_glConvolutionParameteriv( GLenum target, GLenum pname, const GLint *params ) { }
static void null_glConvolutionParameterivEXT( GLenum target, GLenum pname, const GLint *params ) { }
static void null_glConvolutionParameterxOES( GLenum target, GLenum pname, GLfixed param ) { }
static void null_glConvolutionParameterxvOES( GLenum target, GLenum pname, const GLfixed *params ) { }
static void null_glCopyBufferSubData( GLenum readTarget, GLenum writeTarget, GLintptr readOffset, GLintptr writeOffset, GLsizeiptr size ) { }
static void null_glCopyColorSubTable( GLenum target, GLsizei start, GLint x, GLint y, GLsizei width ) { }
static void null_glCopyColorSubTableEXT( GLenum target, GLsizei start, GLint x, GLint y, GLsizei width ) { }
static void null_glCopyColorTable( GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width ) { }
static void null_glCopyColorTableSGI( GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width ) { }
static void null_glCopyConvolutionFilter1D( GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width ) { }
static void null_glCopyConvolutionFilter1DEXT( GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width ) { }
static void null_glCopyConvolutionFilter2D( GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height ) { }
static void null_glCopyConvolutionFilter2DEXT( GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height ) { }
static void null_glCopyImageSubData( GLuint srcName, GLenum srcTarget, GLint srcLevel, GLint srcX, GLint srcY, GLint srcZ, GLuint dstName, GLenum dstTarget, GLint dstLevel, GLint dstX, GLint dstY, GLint dstZ, GLsizei srcWidth, GLsizei srcHeight, GLsizei srcDepth ) { }
static void null_glCopyImageSubDataNV( GLuint srcName, GLenum srcTarget, GLint srcLevel, GLint srcX, GLint srcY, GLint srcZ, GLuint dstName, GLenum dstTarget, GLint dstLevel, GLint dstX, GLint dstY, GLint dstZ, GLsizei width, GLsizei height, GLsizei depth ) { }
static void null_glCopyMultiTexImage1DEXT( GLenum texunit, GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLint border ) { }
static void null_glCopyMultiTexImage2DEXT( GLenum texunit, GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border ) { }
static void null_glCopyMultiTexSubImage1DEXT( GLenum texunit, GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width ) { }
static void null_glCopyMultiTexSubImage2DEXT( GLenum texunit, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height ) { }
static void null_glCopyMultiTexSubImage3DEXT( GLenum texunit, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height ) { }
static void null_glCopyNamedBufferSubData( GLuint readBuffer, GLuint writeBuffer, GLintptr readOffset, GLintptr writeOffset, GLsizeiptr size ) { }
static void null_glCopyPathNV( GLuint resultPath, GLuint srcPath ) { }
static void null_glCopyTexImage1DEXT( GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLint border ) { }
static void null_glCopyTexImage2DEXT( GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border ) { }
static void null_glCopyTexSubImage1DEXT( GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width ) { }
static void null_glCopyTexSubImage2DEXT( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height ) { }
static void null_glCopyTexSubImage3D( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height ) { }
static void null_glCopyTexSubImage3DEXT( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height ) { }
static void null_glCopyTextureImage1DEXT( GLuint texture, GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLint border ) { }
static void null_glCopyTextureImage2DEXT( GLuint texture, GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border ) { }
static void null_glCopyTextureSubImage1D( GLuint texture, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width ) { }
static void null_glCopyTextureSubImage1DEXT( GLuint texture, GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width ) { }
static void null_glCopyTextureSubImage2D( GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height ) { }
static void null_glCopyTextureSubImage2DEXT( GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height ) { }
static void null_glCopyTextureSubImage3D( GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height ) { }
static void null_glCopyTextureSubImage3DEXT( GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height ) { }
static void null_glCoverFillPathInstancedNV( GLsizei numPaths, GLenum pathNameType, const void *paths, GLuint pathBase, GLenum coverMode, GLenum transformType, const GLfloat *transformValues ) { }
static void null_glCoverFillPathNV( GLuint path, GLenum coverMode ) { }
static void null_glCoverStrokePathInstancedNV( GLsizei numPaths, GLenum pathNameType, const void *paths, GLuint pathBase, GLenum coverMode, GLenum transformType, const GLfloat *transformValues ) { }
static void null_glCoverStrokePathNV( GLuint path, GLenum coverMode ) { }
static void null_glCoverageModulationNV( GLenum components ) { }
static void null_glCoverageModulationTableNV( GLsizei n, const GLfloat *v ) { }
static void null_glCreateBuffers( GLsizei n, GLuint *buffers ) { }
static void null_glCreateCommandListsNV( GLsizei n, GLuint *lists ) { }
static void null_glCreateFramebuffers( GLsizei n, GLuint *framebuffers ) { }
static void null_glCreateMemoryObjectsEXT( GLsizei n, GLuint *memoryObjects ) { }
static void null_glCreatePerfQueryINTEL( GLuint queryId, GLuint *queryHandle ) { }
static GLuint null_glCreateProgram(void) { return 0; }
static GLhandleARB null_glCreateProgramObjectARB(void) { return 0; }
static void null_glCreateProgramPipelines( GLsizei n, GLuint *pipelines ) { }
static GLuint null_glCreateProgressFenceNVX(void) { return 0; }
static void null_glCreateQueries( GLenum target, GLsizei n, GLuint *ids ) { }
static void null_glCreateRenderbuffers( GLsizei n, GLuint *renderbuffers ) { }
static void null_glCreateSamplers( GLsizei n, GLuint *samplers ) { }
static GLuint null_glCreateShader( GLenum type ) { return 0; }
static GLhandleARB null_glCreateShaderObjectARB( GLenum shaderType ) { return 0; }
static GLuint null_glCreateShaderProgramEXT( GLenum type, const GLchar *string ) { return 0; }
static GLuint null_glCreateShaderProgramv( GLenum type, GLsizei count, const GLchar *const*strings ) { return 0; }
static void null_glCreateStatesNV( GLsizei n, GLuint *states ) { }
static GLsync null_glCreateSyncFromCLeventARB( struct _cl_context *context, struct _cl_event *event, GLbitfield flags ) { return 0; }
static void null_glCreateTextures( GLenum target, GLsizei n, GLuint *textures ) { }
static void null_glCreateTransformFeedbacks( GLsizei n, GLuint *ids ) { }
static void null_glCreateVertexArrays( GLsizei n, GLuint *arrays ) { }
static void null_glCullParameterdvEXT( GLenum pname, GLdouble *params ) { }
static void null_glCullParameterfvEXT( GLenum pname, GLfloat *params ) { }
static void null_glCurrentPaletteMatrixARB( GLint index ) { }
static void null_glDebugMessageCallback( GLDEBUGPROC callback, const void *userParam ) { }
static void null_glDebugMessageCallbackAMD( GLDEBUGPROCAMD callback, void *userParam ) { }
static void null_glDebugMessageCallbackARB( GLDEBUGPROCARB callback, const void *userParam ) { }
static void null_glDebugMessageControl( GLenum source, GLenum type, GLenum severity, GLsizei count, const GLuint *ids, GLboolean enabled ) { }
static void null_glDebugMessageControlARB( GLenum source, GLenum type, GLenum severity, GLsizei count, const GLuint *ids, GLboolean enabled ) { }
static void null_glDebugMessageEnableAMD( GLenum category, GLenum severity, GLsizei count, const GLuint *ids, GLboolean enabled ) { }
static void null_glDebugMessageInsert( GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *buf ) { }
static void null_glDebugMessageInsertAMD( GLenum category, GLenum severity, GLuint id, GLsizei length, const GLchar *buf ) { }
static void null_glDebugMessageInsertARB( GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *buf ) { }
static void null_glDeformSGIX( GLbitfield mask ) { }
static void null_glDeformationMap3dSGIX( GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, GLdouble w1, GLdouble w2, GLint wstride, GLint worder, const GLdouble *points ) { }
static void null_glDeformationMap3fSGIX( GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, GLfloat w1, GLfloat w2, GLint wstride, GLint worder, const GLfloat *points ) { }
static void null_glDeleteAsyncMarkersSGIX( GLuint marker, GLsizei range ) { }
static void null_glDeleteBufferRegion( GLenum region ) { }
static void null_glDeleteBuffers( GLsizei n, const GLuint *buffers ) { }
static void null_glDeleteBuffersARB( GLsizei n, const GLuint *buffers ) { }
static void null_glDeleteCommandListsNV( GLsizei n, const GLuint *lists ) { }
static void null_glDeleteFencesAPPLE( GLsizei n, const GLuint *fences ) { }
static void null_glDeleteFencesNV( GLsizei n, const GLuint *fences ) { }
static void null_glDeleteFragmentShaderATI( GLuint id ) { }
static void null_glDeleteFramebuffers( GLsizei n, const GLuint *framebuffers ) { }
static void null_glDeleteFramebuffersEXT( GLsizei n, const GLuint *framebuffers ) { }
static void null_glDeleteMemoryObjectsEXT( GLsizei n, const GLuint *memoryObjects ) { }
static void null_glDeleteNamedStringARB( GLint namelen, const GLchar *name ) { }
static void null_glDeleteNamesAMD( GLenum identifier, GLuint num, const GLuint *names ) { }
static void null_glDeleteObjectARB( GLhandleARB obj ) { }
static void null_glDeleteObjectBufferATI( GLuint buffer ) { }
static void null_glDeleteOcclusionQueriesNV( GLsizei n, const GLuint *ids ) { }
static void null_glDeletePathsNV( GLuint path, GLsizei range ) { }
static void null_glDeletePerfMonitorsAMD( GLsizei n, GLuint *monitors ) { }
static void null_glDeletePerfQueryINTEL( GLuint queryHandle ) { }
static void null_glDeleteProgram( GLuint program ) { }
static void null_glDeleteProgramPipelines( GLsizei n, const GLuint *pipelines ) { }
static void null_glDeleteProgramsARB( GLsizei n, const GLuint *programs ) { }
static void null_glDeleteProgramsNV( GLsizei n, const GLuint *programs ) { }
static void null_glDeleteQueries( GLsizei n, const GLuint *ids ) { }
static void null_glDeleteQueriesARB( GLsizei n, const GLuint *ids ) { }
static void null_glDeleteQueryResourceTagNV( GLsizei n, const GLint *tagIds ) { }
static void null_glDeleteRenderbuffers( GLsizei n, const GLuint *renderbuffers ) { }
static void null_glDeleteRenderbuffersEXT( GLsizei n, const GLuint *renderbuffers ) { }
static void null_glDeleteSamplers( GLsizei count, const GLuint *samplers ) { }
static void null_glDeleteSemaphoresEXT( GLsizei n, const GLuint *semaphores ) { }
static void null_glDeleteShader( GLuint shader ) { }
static void null_glDeleteStatesNV( GLsizei n, const GLuint *states ) { }
static void null_glDeleteSync( GLsync sync ) { }
static void null_glDeleteTexturesEXT( GLsizei n, const GLuint *textures ) { }
static void null_glDeleteTransformFeedbacks( GLsizei n, const GLuint *ids ) { }
static void null_glDeleteTransformFeedbacksNV( GLsizei n, const GLuint *ids ) { }
static void null_glDeleteVertexArrays( GLsizei n, const GLuint *arrays ) { }
static void null_glDeleteVertexArraysAPPLE( GLsizei n, const GLuint *arrays ) { }
static void null_glDeleteVertexShaderEXT( GLuint id ) { }
static void null_glDepthBoundsEXT( GLclampd zmin, GLclampd zmax ) { }
static void null_glDepthBoundsdNV( GLdouble zmin, GLdouble zmax ) { }
static void null_glDepthRangeArraydvNV( GLuint first, GLsizei count, const GLdouble *v ) { }
static void null_glDepthRangeArrayv( GLuint first, GLsizei count, const GLdouble *v ) { }
static void null_glDepthRangeIndexed( GLuint index, GLdouble n, GLdouble f ) { }
static void null_glDepthRangeIndexeddNV( GLuint index, GLdouble n, GLdouble f ) { }
static void null_glDepthRangedNV( GLdouble zNear, GLdouble zFar ) { }
static void null_glDepthRangef( GLfloat n, GLfloat f ) { }
static void null_glDepthRangefOES( GLclampf n, GLclampf f ) { }
static void null_glDepthRangexOES( GLfixed n, GLfixed f ) { }
static void null_glDetachObjectARB( GLhandleARB containerObj, GLhandleARB attachedObj ) { }
static void null_glDetachShader( GLuint program, GLuint shader ) { }
static void null_glDetailTexFuncSGIS( GLenum target, GLsizei n, const GLfloat *points ) { }
static void null_glDisableClientStateIndexedEXT( GLenum array, GLuint index ) { }
static void null_glDisableClientStateiEXT( GLenum array, GLuint index ) { }
static void null_glDisableIndexedEXT( GLenum target, GLuint index ) { }
static void null_glDisableVariantClientStateEXT( GLuint id ) { }
static void null_glDisableVertexArrayAttrib( GLuint vaobj, GLuint index ) { }
static void null_glDisableVertexArrayAttribEXT( GLuint vaobj, GLuint index ) { }
static void null_glDisableVertexArrayEXT( GLuint vaobj, GLenum array ) { }
static void null_glDisableVertexAttribAPPLE( GLuint index, GLenum pname ) { }
static void null_glDisableVertexAttribArray( GLuint index ) { }
static void null_glDisableVertexAttribArrayARB( GLuint index ) { }
static void null_glDisablei( GLenum target, GLuint index ) { }
static void null_glDispatchCompute( GLuint num_groups_x, GLuint num_groups_y, GLuint num_groups_z ) { }
static void null_glDispatchComputeGroupSizeARB( GLuint num_groups_x, GLuint num_groups_y, GLuint num_groups_z, GLuint group_size_x, GLuint group_size_y, GLuint group_size_z ) { }
static void null_glDispatchComputeIndirect( GLintptr indirect ) { }
static void null_glDrawArraysEXT( GLenum mode, GLint first, GLsizei count ) { }
static void null_glDrawArraysIndirect( GLenum mode, const void *indirect ) { }
static void null_glDrawArraysInstanced( GLenum mode, GLint first, GLsizei count, GLsizei instancecount ) { }
static void null_glDrawArraysInstancedARB( GLenum mode, GLint first, GLsizei count, GLsizei primcount ) { }
static void null_glDrawArraysInstancedBaseInstance( GLenum mode, GLint first, GLsizei count, GLsizei instancecount, GLuint baseinstance ) { }
static void null_glDrawArraysInstancedEXT( GLenum mode, GLint start, GLsizei count, GLsizei primcount ) { }
static void null_glDrawBufferRegion( GLenum region, GLint x, GLint y, GLsizei width, GLsizei height, GLint xDest, GLint yDest ) { }
static void null_glDrawBuffers( GLsizei n, const GLenum *bufs ) { }
static void null_glDrawBuffersARB( GLsizei n, const GLenum *bufs ) { }
static void null_glDrawBuffersATI( GLsizei n, const GLenum *bufs ) { }
static void null_glDrawCommandsAddressNV( GLenum primitiveMode, const GLuint64 *indirects, const GLsizei *sizes, GLuint count ) { }
static void null_glDrawCommandsNV( GLenum primitiveMode, GLuint buffer, const GLintptr *indirects, const GLsizei *sizes, GLuint count ) { }
static void null_glDrawCommandsStatesAddressNV( const GLuint64 *indirects, const GLsizei *sizes, const GLuint *states, const GLuint *fbos, GLuint count ) { }
static void null_glDrawCommandsStatesNV( GLuint buffer, const GLintptr *indirects, const GLsizei *sizes, const GLuint *states, const GLuint *fbos, GLuint count ) { }
static void null_glDrawElementArrayAPPLE( GLenum mode, GLint first, GLsizei count ) { }
static void null_glDrawElementArrayATI( GLenum mode, GLsizei count ) { }
static void null_glDrawElementsBaseVertex( GLenum mode, GLsizei count, GLenum type, const void *indices, GLint basevertex ) { }
static void null_glDrawElementsIndirect( GLenum mode, GLenum type, const void *indirect ) { }
static void null_glDrawElementsInstanced( GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount ) { }
static void null_glDrawElementsInstancedARB( GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei primcount ) { }
static void null_glDrawElementsInstancedBaseInstance( GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount, GLuint baseinstance ) { }
static void null_glDrawElementsInstancedBaseVertex( GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount, GLint basevertex ) { }
static void null_glDrawElementsInstancedBaseVertexBaseInstance( GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount, GLint basevertex, GLuint baseinstance ) { }
static void null_glDrawElementsInstancedEXT( GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei primcount ) { }
static void null_glDrawMeshArraysSUN( GLenum mode, GLint first, GLsizei count, GLsizei width ) { }
static void null_glDrawMeshTasksIndirectNV( GLintptr indirect ) { }
static void null_glDrawMeshTasksNV( GLuint first, GLuint count ) { }
static void null_glDrawRangeElementArrayAPPLE( GLenum mode, GLuint start, GLuint end, GLint first, GLsizei count ) { }
static void null_glDrawRangeElementArrayATI( GLenum mode, GLuint start, GLuint end, GLsizei count ) { }
static void null_glDrawRangeElements( GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const void *indices ) { }
static void null_glDrawRangeElementsBaseVertex( GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const void *indices, GLint basevertex ) { }
static void null_glDrawRangeElementsEXT( GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const void *indices ) { }
static void null_glDrawTextureNV( GLuint texture, GLuint sampler, GLfloat x0, GLfloat y0, GLfloat x1, GLfloat y1, GLfloat z, GLfloat s0, GLfloat t0, GLfloat s1, GLfloat t1 ) { }
static void null_glDrawTransformFeedback( GLenum mode, GLuint id ) { }
static void null_glDrawTransformFeedbackInstanced( GLenum mode, GLuint id, GLsizei instancecount ) { }
static void null_glDrawTransformFeedbackNV( GLenum mode, GLuint id ) { }
static void null_glDrawTransformFeedbackStream( GLenum mode, GLuint id, GLuint stream ) { }
static void null_glDrawTransformFeedbackStreamInstanced( GLenum mode, GLuint id, GLuint stream, GLsizei instancecount ) { }
static void null_glDrawVkImageNV( GLuint64 vkImage, GLuint sampler, GLfloat x0, GLfloat y0, GLfloat x1, GLfloat y1, GLfloat z, GLfloat s0, GLfloat t0, GLfloat s1, GLfloat t1 ) { }
static void null_glEGLImageTargetTexStorageEXT( GLenum target, GLeglImageOES image, const GLint* attrib_list ) { }
static void null_glEGLImageTargetTextureStorageEXT( GLuint texture, GLeglImageOES image, const GLint* attrib_list ) { }
static void null_glEdgeFlagFormatNV( GLsizei stride ) { }
static void null_glEdgeFlagPointerEXT( GLsizei stride, GLsizei count, const GLboolean *pointer ) { }
static void null_glEdgeFlagPointerListIBM( GLint stride, const GLboolean **pointer, GLint ptrstride ) { }
static void null_glElementPointerAPPLE( GLenum type, const void *pointer ) { }
static void null_glElementPointerATI( GLenum type, const void *pointer ) { }
static void null_glEnableClientStateIndexedEXT( GLenum array, GLuint index ) { }
static void null_glEnableClientStateiEXT( GLenum array, GLuint index ) { }
static void null_glEnableIndexedEXT( GLenum target, GLuint index ) { }
static void null_glEnableVariantClientStateEXT( GLuint id ) { }
static void null_glEnableVertexArrayAttrib( GLuint vaobj, GLuint index ) { }
static void null_glEnableVertexArrayAttribEXT( GLuint vaobj, GLuint index ) { }
static void null_glEnableVertexArrayEXT( GLuint vaobj, GLenum array ) { }
static void null_glEnableVertexAttribAPPLE( GLuint index, GLenum pname ) { }
static void null_glEnableVertexAttribArray( GLuint index ) { }
static void null_glEnableVertexAttribArrayARB( GLuint index ) { }
static void null_glEnablei( GLenum target, GLuint index ) { }
static void null_glEndConditionalRender(void) { }
static void null_glEndConditionalRenderNV(void) { }
static void null_glEndConditionalRenderNVX(void) { }
static void null_glEndFragmentShaderATI(void) { }
static void null_glEndOcclusionQueryNV(void) { }
static void null_glEndPerfMonitorAMD( GLuint monitor ) { }
static void null_glEndPerfQueryINTEL( GLuint queryHandle ) { }
static void null_glEndQuery( GLenum target ) { }
static void null_glEndQueryARB( GLenum target ) { }
static void null_glEndQueryIndexed( GLenum target, GLuint index ) { }
static void null_glEndTransformFeedback(void) { }
static void null_glEndTransformFeedbackEXT(void) { }
static void null_glEndTransformFeedbackNV(void) { }
static void null_glEndVertexShaderEXT(void) { }
static void null_glEndVideoCaptureNV( GLuint video_capture_slot ) { }
static void null_glEvalCoord1xOES( GLfixed u ) { }
static void null_glEvalCoord1xvOES( const GLfixed *coords ) { }
static void null_glEvalCoord2xOES( GLfixed u, GLfixed v ) { }
static void null_glEvalCoord2xvOES( const GLfixed *coords ) { }
static void null_glEvalMapsNV( GLenum target, GLenum mode ) { }
static void null_glEvaluateDepthValuesARB(void) { }
static void null_glExecuteProgramNV( GLenum target, GLuint id, const GLfloat *params ) { }
static void null_glExtractComponentEXT( GLuint res, GLuint src, GLuint num ) { }
static void null_glFeedbackBufferxOES( GLsizei n, GLenum type, const GLfixed *buffer ) { }
static GLsync null_glFenceSync( GLenum condition, GLbitfield flags ) { return 0; }
static void null_glFinalCombinerInputNV( GLenum variable, GLenum input, GLenum mapping, GLenum componentUsage ) { }
static GLint null_glFinishAsyncSGIX( GLuint *markerp ) { return 0; }
static void null_glFinishFenceAPPLE( GLuint fence ) { }
static void null_glFinishFenceNV( GLuint fence ) { }
static void null_glFinishObjectAPPLE( GLenum object, GLint name ) { }
static void null_glFinishTextureSUNX(void) { }
static void null_glFlushMappedBufferRange( GLenum target, GLintptr offset, GLsizeiptr length ) { }
static void null_glFlushMappedBufferRangeAPPLE( GLenum target, GLintptr offset, GLsizeiptr size ) { }
static void null_glFlushMappedNamedBufferRange( GLuint buffer, GLintptr offset, GLsizeiptr length ) { }
static void null_glFlushMappedNamedBufferRangeEXT( GLuint buffer, GLintptr offset, GLsizeiptr length ) { }
static void null_glFlushPixelDataRangeNV( GLenum target ) { }
static void null_glFlushRasterSGIX(void) { }
static void null_glFlushStaticDataIBM( GLenum target ) { }
static void null_glFlushVertexArrayRangeAPPLE( GLsizei length, void *pointer ) { }
static void null_glFlushVertexArrayRangeNV(void) { }
static void null_glFogCoordFormatNV( GLenum type, GLsizei stride ) { }
static void null_glFogCoordPointer( GLenum type, GLsizei stride, const void *pointer ) { }
static void null_glFogCoordPointerEXT( GLenum type, GLsizei stride, const void *pointer ) { }
static void null_glFogCoordPointerListIBM( GLenum type, GLint stride, const void **pointer, GLint ptrstride ) { }
static void null_glFogCoordd( GLdouble coord ) { }
static void null_glFogCoorddEXT( GLdouble coord ) { }
static void null_glFogCoorddv( const GLdouble *coord ) { }
static void null_glFogCoorddvEXT( const GLdouble *coord ) { }
static void null_glFogCoordf( GLfloat coord ) { }
static void null_glFogCoordfEXT( GLfloat coord ) { }
static void null_glFogCoordfv( const GLfloat *coord ) { }
static void null_glFogCoordfvEXT( const GLfloat *coord ) { }
static void null_glFogCoordhNV( GLhalfNV fog ) { }
static void null_glFogCoordhvNV( const GLhalfNV *fog ) { }
static void null_glFogFuncSGIS( GLsizei n, const GLfloat *points ) { }
static void null_glFogxOES( GLenum pname, GLfixed param ) { }
static void null_glFogxvOES( GLenum pname, const GLfixed *param ) { }
static void null_glFragmentColorMaterialSGIX( GLenum face, GLenum mode ) { }
static void null_glFragmentCoverageColorNV( GLuint color ) { }
static void null_glFragmentLightModelfSGIX( GLenum pname, GLfloat param ) { }
static void null_glFragmentLightModelfvSGIX( GLenum pname, const GLfloat *params ) { }
static void null_glFragmentLightModeliSGIX( GLenum pname, GLint param ) { }
static void null_glFragmentLightModelivSGIX( GLenum pname, const GLint *params ) { }
static void null_glFragmentLightfSGIX( GLenum light, GLenum pname, GLfloat param ) { }
static void null_glFragmentLightfvSGIX( GLenum light, GLenum pname, const GLfloat *params ) { }
static void null_glFragmentLightiSGIX( GLenum light, GLenum pname, GLint param ) { }
static void null_glFragmentLightivSGIX( GLenum light, GLenum pname, const GLint *params ) { }
static void null_glFragmentMaterialfSGIX( GLenum face, GLenum pname, GLfloat param ) { }
static void null_glFragmentMaterialfvSGIX( GLenum face, GLenum pname, const GLfloat *params ) { }
static void null_glFragmentMaterialiSGIX( GLenum face, GLenum pname, GLint param ) { }
static void null_glFragmentMaterialivSGIX( GLenum face, GLenum pname, const GLint *params ) { }
static void null_glFrameTerminatorGREMEDY(void) { }
static void null_glFrameZoomSGIX( GLint factor ) { }
static void null_glFramebufferDrawBufferEXT( GLuint framebuffer, GLenum mode ) { }
static void null_glFramebufferDrawBuffersEXT( GLuint framebuffer, GLsizei n, const GLenum *bufs ) { }
static void null_glFramebufferFetchBarrierEXT(void) { }
static void null_glFramebufferParameteri( GLenum target, GLenum pname, GLint param ) { }
static void null_glFramebufferParameteriMESA( GLenum target, GLenum pname, GLint param ) { }
static void null_glFramebufferReadBufferEXT( GLuint framebuffer, GLenum mode ) { }
static void null_glFramebufferRenderbuffer( GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer ) { }
static void null_glFramebufferRenderbufferEXT( GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer ) { }
static void null_glFramebufferSampleLocationsfvARB( GLenum target, GLuint start, GLsizei count, const GLfloat *v ) { }
static void null_glFramebufferSampleLocationsfvNV( GLenum target, GLuint start, GLsizei count, const GLfloat *v ) { }
static void null_glFramebufferSamplePositionsfvAMD( GLenum target, GLuint numsamples, GLuint pixelindex, const GLfloat *values ) { }
static void null_glFramebufferTexture( GLenum target, GLenum attachment, GLuint texture, GLint level ) { }
static void null_glFramebufferTexture1D( GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level ) { }
static void null_glFramebufferTexture1DEXT( GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level ) { }
static void null_glFramebufferTexture2D( GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level ) { }
static void null_glFramebufferTexture2DEXT( GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level ) { }
static void null_glFramebufferTexture3D( GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint zoffset ) { }
static void null_glFramebufferTexture3DEXT( GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint zoffset ) { }
static void null_glFramebufferTextureARB( GLenum target, GLenum attachment, GLuint texture, GLint level ) { }
static void null_glFramebufferTextureEXT( GLenum target, GLenum attachment, GLuint texture, GLint level ) { }
static void null_glFramebufferTextureFaceARB( GLenum target, GLenum attachment, GLuint texture, GLint level, GLenum face ) { }
static void null_glFramebufferTextureFaceEXT( GLenum target, GLenum attachment, GLuint texture, GLint level, GLenum face ) { }
static void null_glFramebufferTextureLayer( GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer ) { }
static void null_glFramebufferTextureLayerARB( GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer ) { }
static void null_glFramebufferTextureLayerEXT( GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer ) { }
static void null_glFramebufferTextureMultiviewOVR( GLenum target, GLenum attachment, GLuint texture, GLint level, GLint baseViewIndex, GLsizei numViews ) { }
static void null_glFreeObjectBufferATI( GLuint buffer ) { }
static void null_glFrustumfOES( GLfloat l, GLfloat r, GLfloat b, GLfloat t, GLfloat n, GLfloat f ) { }
static void null_glFrustumxOES( GLfixed l, GLfixed r, GLfixed b, GLfixed t, GLfixed n, GLfixed f ) { }
static GLuint null_glGenAsyncMarkersSGIX( GLsizei range ) { return 0; }
static void null_glGenBuffers( GLsizei n, GLuint *buffers ) { }
static void null_glGenBuffersARB( GLsizei n, GLuint *buffers ) { }
static void null_glGenFencesAPPLE( GLsizei n, GLuint *fences ) { }
static void null_glGenFencesNV( GLsizei n, GLuint *fences ) { }
static GLuint null_glGenFragmentShadersATI( GLuint range ) { return 0; }
static void null_glGenFramebuffers( GLsizei n, GLuint *framebuffers ) { }
static void null_glGenFramebuffersEXT( GLsizei n, GLuint *framebuffers ) { }
static void null_glGenNamesAMD( GLenum identifier, GLuint num, GLuint *names ) { }
static void null_glGenOcclusionQueriesNV( GLsizei n, GLuint *ids ) { }
static GLuint null_glGenPathsNV( GLsizei range ) { return 0; }
static void null_glGenPerfMonitorsAMD( GLsizei n, GLuint *monitors ) { }
static void null_glGenProgramPipelines( GLsizei n, GLuint *pipelines ) { }
static void null_glGenProgramsARB( GLsizei n, GLuint *programs ) { }
static void null_glGenProgramsNV( GLsizei n, GLuint *programs ) { }
static void null_glGenQueries( GLsizei n, GLuint *ids ) { }
static void null_glGenQueriesARB( GLsizei n, GLuint *ids ) { }
static void null_glGenQueryResourceTagNV( GLsizei n, GLint *tagIds ) { }
static void null_glGenRenderbuffers( GLsizei n, GLuint *renderbuffers ) { }
static void null_glGenRenderbuffersEXT( GLsizei n, GLuint *renderbuffers ) { }
static void null_glGenSamplers( GLsizei count, GLuint *samplers ) { }
static void null_glGenSemaphoresEXT( GLsizei n, GLuint *semaphores ) { }
static GLuint null_glGenSymbolsEXT( GLenum datatype, GLenum storagetype, GLenum range, GLuint components ) { return 0; }
static void null_glGenTexturesEXT( GLsizei n, GLuint *textures ) { }
static void null_glGenTransformFeedbacks( GLsizei n, GLuint *ids ) { }
static void null_glGenTransformFeedbacksNV( GLsizei n, GLuint *ids ) { }
static void null_glGenVertexArrays( GLsizei n, GLuint *arrays ) { }
static void null_glGenVertexArraysAPPLE( GLsizei n, GLuint *arrays ) { }
static GLuint null_glGenVertexShadersEXT( GLuint range ) { return 0; }
static void null_glGenerateMipmap( GLenum target ) { }
static void null_glGenerateMipmapEXT( GLenum target ) { }
static void null_glGenerateMultiTexMipmapEXT( GLenum texunit, GLenum target ) { }
static void null_glGenerateTextureMipmap( GLuint texture ) { }
static void null_glGenerateTextureMipmapEXT( GLuint texture, GLenum target ) { }
static void null_glGetActiveAtomicCounterBufferiv( GLuint program, GLuint bufferIndex, GLenum pname, GLint *params ) { }
static void null_glGetActiveAttrib( GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLint *size, GLenum *type, GLchar *name ) { }
static void null_glGetActiveAttribARB( GLhandleARB programObj, GLuint index, GLsizei maxLength, GLsizei *length, GLint *size, GLenum *type, GLcharARB *name ) { }
static void null_glGetActiveSubroutineName( GLuint program, GLenum shadertype, GLuint index, GLsizei bufSize, GLsizei *length, GLchar *name ) { }
static void null_glGetActiveSubroutineUniformName( GLuint program, GLenum shadertype, GLuint index, GLsizei bufSize, GLsizei *length, GLchar *name ) { }
static void null_glGetActiveSubroutineUniformiv( GLuint program, GLenum shadertype, GLuint index, GLenum pname, GLint *values ) { }
static void null_glGetActiveUniform( GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLint *size, GLenum *type, GLchar *name ) { }
static void null_glGetActiveUniformARB( GLhandleARB programObj, GLuint index, GLsizei maxLength, GLsizei *length, GLint *size, GLenum *type, GLcharARB *name ) { }
static void null_glGetActiveUniformBlockName( GLuint program, GLuint uniformBlockIndex, GLsizei bufSize, GLsizei *length, GLchar *uniformBlockName ) { }
static void null_glGetActiveUniformBlockiv( GLuint program, GLuint uniformBlockIndex, GLenum pname, GLint *params ) { }
static void null_glGetActiveUniformName( GLuint program, GLuint uniformIndex, GLsizei bufSize, GLsizei *length, GLchar *uniformName ) { }
static void null_glGetActiveUniformsiv( GLuint program, GLsizei uniformCount, const GLuint *uniformIndices, GLenum pname, GLint *params ) { }
static void null_glGetActiveVaryingNV( GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLsizei *size, GLenum *type, GLchar *name ) { }
static void null_glGetArrayObjectfvATI( GLenum array, GLenum pname, GLfloat *params ) { }
static void null_glGetArrayObjectivATI( GLenum array, GLenum pname, GLint *params ) { }
static void null_glGetAttachedObjectsARB( GLhandleARB containerObj, GLsizei maxCount, GLsizei *count, GLhandleARB *obj ) { }
static void null_glGetAttachedShaders( GLuint program, GLsizei maxCount, GLsizei *count, GLuint *shaders ) { }
static GLint null_glGetAttribLocation( GLuint program, const GLchar *name ) { return 0; }
static GLint null_glGetAttribLocationARB( GLhandleARB programObj, const GLcharARB *name ) { return 0; }
static void null_glGetBooleanIndexedvEXT( GLenum target, GLuint index, GLboolean *data ) { }
static void null_glGetBooleani_v( GLenum target, GLuint index, GLboolean *data ) { }
static void null_glGetBufferParameteri64v( GLenum target, GLenum pname, GLint64 *params ) { }
static void null_glGetBufferParameteriv( GLenum target, GLenum pname, GLint *params ) { }
static void null_glGetBufferParameterivARB( GLenum target, GLenum pname, GLint *params ) { }
static void null_glGetBufferParameterui64vNV( GLenum target, GLenum pname, GLuint64EXT *params ) { }
static void null_glGetBufferPointerv( GLenum target, GLenum pname, void **params ) { }
static void null_glGetBufferPointervARB( GLenum target, GLenum pname, void **params ) { }
static void null_glGetBufferSubData( GLenum target, GLintptr offset, GLsizeiptr size, void *data ) { }
static void null_glGetBufferSubDataARB( GLenum target, GLintptrARB offset, GLsizeiptrARB size, void *data ) { }
static void null_glGetClipPlanefOES( GLenum plane, GLfloat *equation ) { }
static void null_glGetClipPlanexOES( GLenum plane, GLfixed *equation ) { }
static void null_glGetColorTable( GLenum target, GLenum format, GLenum type, void *table ) { }
static void null_glGetColorTableEXT( GLenum target, GLenum format, GLenum type, void *data ) { }
static void null_glGetColorTableParameterfv( GLenum target, GLenum pname, GLfloat *params ) { }
static void null_glGetColorTableParameterfvEXT( GLenum target, GLenum pname, GLfloat *params ) { }
static void null_glGetColorTableParameterfvSGI( GLenum target, GLenum pname, GLfloat *params ) { }
static void null_glGetColorTableParameteriv( GLenum target, GLenum pname, GLint *params ) { }
static void null_glGetColorTableParameterivEXT( GLenum target, GLenum pname, GLint *params ) { }
static void null_glGetColorTableParameterivSGI( GLenum target, GLenum pname, GLint *params ) { }
static void null_glGetColorTableSGI( GLenum target, GLenum format, GLenum type, void *table ) { }
static void null_glGetCombinerInputParameterfvNV( GLenum stage, GLenum portion, GLenum variable, GLenum pname, GLfloat *params ) { }
static void null_glGetCombinerInputParameterivNV( GLenum stage, GLenum portion, GLenum variable, GLenum pname, GLint *params ) { }
static void null_glGetCombinerOutputParameterfvNV( GLenum stage, GLenum portion, GLenum pname, GLfloat *params ) { }
static void null_glGetCombinerOutputParameterivNV( GLenum stage, GLenum portion, GLenum pname, GLint *params ) { }
static void null_glGetCombinerStageParameterfvNV( GLenum stage, GLenum pname, GLfloat *params ) { }
static GLuint null_glGetCommandHeaderNV( GLenum tokenID, GLuint size ) { return 0; }
static void null_glGetCompressedMultiTexImageEXT( GLenum texunit, GLenum target, GLint lod, void *img ) { }
static void null_glGetCompressedTexImage( GLenum target, GLint level, void *img ) { }
static void null_glGetCompressedTexImageARB( GLenum target, GLint level, void *img ) { }
static void null_glGetCompressedTextureImage( GLuint texture, GLint level, GLsizei bufSize, void *pixels ) { }
static void null_glGetCompressedTextureImageEXT( GLuint texture, GLenum target, GLint lod, void *img ) { }
static void null_glGetCompressedTextureSubImage( GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLsizei bufSize, void *pixels ) { }
static void null_glGetConvolutionFilter( GLenum target, GLenum format, GLenum type, void *image ) { }
static void null_glGetConvolutionFilterEXT( GLenum target, GLenum format, GLenum type, void *image ) { }
static void null_glGetConvolutionParameterfv( GLenum target, GLenum pname, GLfloat *params ) { }
static void null_glGetConvolutionParameterfvEXT( GLenum target, GLenum pname, GLfloat *params ) { }
static void null_glGetConvolutionParameteriv( GLenum target, GLenum pname, GLint *params ) { }
static void null_glGetConvolutionParameterivEXT( GLenum target, GLenum pname, GLint *params ) { }
static void null_glGetConvolutionParameterxvOES( GLenum target, GLenum pname, GLfixed *params ) { }
static void null_glGetCoverageModulationTableNV( GLsizei bufSize, GLfloat *v ) { }
static GLuint null_glGetDebugMessageLog( GLuint count, GLsizei bufSize, GLenum *sources, GLenum *types, GLuint *ids, GLenum *severities, GLsizei *lengths, GLchar *messageLog ) { return 0; }
static GLuint null_glGetDebugMessageLogAMD( GLuint count, GLsizei bufSize, GLenum *categories, GLuint *severities, GLuint *ids, GLsizei *lengths, GLchar *message ) { return 0; }
static GLuint null_glGetDebugMessageLogARB( GLuint count, GLsizei bufSize, GLenum *sources, GLenum *types, GLuint *ids, GLenum *severities, GLsizei *lengths, GLchar *messageLog ) { return 0; }
static void null_glGetDetailTexFuncSGIS( GLenum target, GLfloat *points ) { }
static void null_glGetDoubleIndexedvEXT( GLenum target, GLuint index, GLdouble *data ) { }
static void null_glGetDoublei_v( GLenum target, GLuint index, GLdouble *data ) { }
static void null_glGetDoublei_vEXT( GLenum pname, GLuint index, GLdouble *params ) { }
static void null_glGetFenceivNV( GLuint fence, GLenum pname, GLint *params ) { }
static void null_glGetFinalCombinerInputParameterfvNV( GLenum variable, GLenum pname, GLfloat *params ) { }
static void null_glGetFinalCombinerInputParameterivNV( GLenum variable, GLenum pname, GLint *params ) { }
static void null_glGetFirstPerfQueryIdINTEL( GLuint *queryId ) { }
static void null_glGetFixedvOES( GLenum pname, GLfixed *params ) { }
static void null_glGetFloatIndexedvEXT( GLenum target, GLuint index, GLfloat *data ) { }
static void null_glGetFloati_v( GLenum target, GLuint index, GLfloat *data ) { }
static void null_glGetFloati_vEXT( GLenum pname, GLuint index, GLfloat *params ) { }
static void null_glGetFogFuncSGIS( GLfloat *points ) { }
static GLint null_glGetFragDataIndex( GLuint program, const GLchar *name ) { return 0; }
static GLint null_glGetFragDataLocation( GLuint program, const GLchar *name ) { return 0; }
static GLint null_glGetFragDataLocationEXT( GLuint program, const GLchar *name ) { return 0; }
static void null_glGetFragmentLightfvSGIX( GLenum light, GLenum pname, GLfloat *params ) { }
static void null_glGetFragmentLightivSGIX( GLenum light, GLenum pname, GLint *params ) { }
static void null_glGetFragmentMaterialfvSGIX( GLenum face, GLenum pname, GLfloat *params ) { }
static void null_glGetFragmentMaterialivSGIX( GLenum face, GLenum pname, GLint *params ) { }
static void null_glGetFramebufferAttachmentParameteriv( GLenum target, GLenum attachment, GLenum pname, GLint *params ) { }
static void null_glGetFramebufferAttachmentParameterivEXT( GLenum target, GLenum attachment, GLenum pname, GLint *params ) { }
static void null_glGetFramebufferParameterfvAMD( GLenum target, GLenum pname, GLuint numsamples, GLuint pixelindex, GLsizei size, GLfloat *values ) { }
static void null_glGetFramebufferParameteriv( GLenum target, GLenum pname, GLint *params ) { }
static void null_glGetFramebufferParameterivEXT( GLuint framebuffer, GLenum pname, GLint *params ) { }
static void null_glGetFramebufferParameterivMESA( GLenum target, GLenum pname, GLint *params ) { }
static GLenum null_glGetGraphicsResetStatus(void) { return 0; }
static GLenum null_glGetGraphicsResetStatusARB(void) { return 0; }
static GLhandleARB null_glGetHandleARB( GLenum pname ) { return 0; }
static void null_glGetHistogram( GLenum target, GLboolean reset, GLenum format, GLenum type, void *values ) { }
static void null_glGetHistogramEXT( GLenum target, GLboolean reset, GLenum format, GLenum type, void *values ) { }
static void null_glGetHistogramParameterfv( GLenum target, GLenum pname, GLfloat *params ) { }
static void null_glGetHistogramParameterfvEXT( GLenum target, GLenum pname, GLfloat *params ) { }
static void null_glGetHistogramParameteriv( GLenum target, GLenum pname, GLint *params ) { }
static void null_glGetHistogramParameterivEXT( GLenum target, GLenum pname, GLint *params ) { }
static void null_glGetHistogramParameterxvOES( GLenum target, GLenum pname, GLfixed *params ) { }
static GLuint64 null_glGetImageHandleARB( GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum format ) { return 0; }
static GLuint64 null_glGetImageHandleNV( GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum format ) { return 0; }
static void null_glGetImageTransformParameterfvHP( GLenum target, GLenum pname, GLfloat *params ) { }
static void null_glGetImageTransformParameterivHP( GLenum target, GLenum pname, GLint *params ) { }
static void null_glGetInfoLogARB( GLhandleARB obj, GLsizei maxLength, GLsizei *length, GLcharARB *infoLog ) { }
static GLint null_glGetInstrumentsSGIX(void) { return 0; }
static void null_glGetInteger64i_v( GLenum target, GLuint index, GLint64 *data ) { }
static void null_glGetInteger64v( GLenum pname, GLint64 *data ) { }
static void null_glGetIntegerIndexedvEXT( GLenum target, GLuint index, GLint *data ) { }
static void null_glGetIntegeri_v( GLenum target, GLuint index, GLint *data ) { }
static void null_glGetIntegerui64i_vNV( GLenum value, GLuint index, GLuint64EXT *result ) { }
static void null_glGetIntegerui64vNV( GLenum value, GLuint64EXT *result ) { }
static void null_glGetInternalformatSampleivNV( GLenum target, GLenum internalformat, GLsizei samples, GLenum pname, GLsizei count, GLint *params ) { }
static void null_glGetInternalformati64v( GLenum target, GLenum internalformat, GLenum pname, GLsizei count, GLint64 *params ) { }
static void null_glGetInternalformativ( GLenum target, GLenum internalformat, GLenum pname, GLsizei count, GLint *params ) { }
static void null_glGetInvariantBooleanvEXT( GLuint id, GLenum value, GLboolean *data ) { }
static void null_glGetInvariantFloatvEXT( GLuint id, GLenum value, GLfloat *data ) { }
static void null_glGetInvariantIntegervEXT( GLuint id, GLenum value, GLint *data ) { }
static void null_glGetLightxOES( GLenum light, GLenum pname, GLfixed *params ) { }
static void null_glGetListParameterfvSGIX( GLuint list, GLenum pname, GLfloat *params ) { }
static void null_glGetListParameterivSGIX( GLuint list, GLenum pname, GLint *params ) { }
static void null_glGetLocalConstantBooleanvEXT( GLuint id, GLenum value, GLboolean *data ) { }
static void null_glGetLocalConstantFloatvEXT( GLuint id, GLenum value, GLfloat *data ) { }
static void null_glGetLocalConstantIntegervEXT( GLuint id, GLenum value, GLint *data ) { }
static void null_glGetMapAttribParameterfvNV( GLenum target, GLuint index, GLenum pname, GLfloat *params ) { }
static void null_glGetMapAttribParameterivNV( GLenum target, GLuint index, GLenum pname, GLint *params ) { }
static void null_glGetMapControlPointsNV( GLenum target, GLuint index, GLenum type, GLsizei ustride, GLsizei vstride, GLboolean packed, void *points ) { }
static void null_glGetMapParameterfvNV( GLenum target, GLenum pname, GLfloat *params ) { }
static void null_glGetMapParameterivNV( GLenum target, GLenum pname, GLint *params ) { }
static void null_glGetMapxvOES( GLenum target, GLenum query, GLfixed *v ) { }
static void null_glGetMaterialxOES( GLenum face, GLenum pname, GLfixed param ) { }
static void null_glGetMemoryObjectDetachedResourcesuivNV( GLuint memory, GLenum pname, GLint first, GLsizei count, GLuint *params ) { }
static void null_glGetMemoryObjectParameterivEXT( GLuint memoryObject, GLenum pname, GLint *params ) { }
static void null_glGetMinmax( GLenum target, GLboolean reset, GLenum format, GLenum type, void *values ) { }
static void null_glGetMinmaxEXT( GLenum target, GLboolean reset, GLenum format, GLenum type, void *values ) { }
static void null_glGetMinmaxParameterfv( GLenum target, GLenum pname, GLfloat *params ) { }
static void null_glGetMinmaxParameterfvEXT( GLenum target, GLenum pname, GLfloat *params ) { }
static void null_glGetMinmaxParameteriv( GLenum target, GLenum pname, GLint *params ) { }
static void null_glGetMinmaxParameterivEXT( GLenum target, GLenum pname, GLint *params ) { }
static void null_glGetMultiTexEnvfvEXT( GLenum texunit, GLenum target, GLenum pname, GLfloat *params ) { }
static void null_glGetMultiTexEnvivEXT( GLenum texunit, GLenum target, GLenum pname, GLint *params ) { }
static void null_glGetMultiTexGendvEXT( GLenum texunit, GLenum coord, GLenum pname, GLdouble *params ) { }
static void null_glGetMultiTexGenfvEXT( GLenum texunit, GLenum coord, GLenum pname, GLfloat *params ) { }
static void null_glGetMultiTexGenivEXT( GLenum texunit, GLenum coord, GLenum pname, GLint *params ) { }
static void null_glGetMultiTexImageEXT( GLenum texunit, GLenum target, GLint level, GLenum format, GLenum type, void *pixels ) { }
static void null_glGetMultiTexLevelParameterfvEXT( GLenum texunit, GLenum target, GLint level, GLenum pname, GLfloat *params ) { }
static void null_glGetMultiTexLevelParameterivEXT( GLenum texunit, GLenum target, GLint level, GLenum pname, GLint *params ) { }
static void null_glGetMultiTexParameterIivEXT( GLenum texunit, GLenum target, GLenum pname, GLint *params ) { }
static void null_glGetMultiTexParameterIuivEXT( GLenum texunit, GLenum target, GLenum pname, GLuint *params ) { }
static void null_glGetMultiTexParameterfvEXT( GLenum texunit, GLenum target, GLenum pname, GLfloat *params ) { }
static void null_glGetMultiTexParameterivEXT( GLenum texunit, GLenum target, GLenum pname, GLint *params ) { }
static void null_glGetMultisamplefv( GLenum pname, GLuint index, GLfloat *val ) { }
static void null_glGetMultisamplefvNV( GLenum pname, GLuint index, GLfloat *val ) { }
static void null_glGetNamedBufferParameteri64v( GLuint buffer, GLenum pname, GLint64 *params ) { }
static void null_glGetNamedBufferParameteriv( GLuint buffer, GLenum pname, GLint *params ) { }
static void null_glGetNamedBufferParameterivEXT( GLuint buffer, GLenum pname, GLint *params ) { }
static void null_glGetNamedBufferParameterui64vNV( GLuint buffer, GLenum pname, GLuint64EXT *params ) { }
static void null_glGetNamedBufferPointerv( GLuint buffer, GLenum pname, void **params ) { }
static void null_glGetNamedBufferPointervEXT( GLuint buffer, GLenum pname, void **params ) { }
static void null_glGetNamedBufferSubData( GLuint buffer, GLintptr offset, GLsizeiptr size, void *data ) { }
static void null_glGetNamedBufferSubDataEXT( GLuint buffer, GLintptr offset, GLsizeiptr size, void *data ) { }
static void null_glGetNamedFramebufferAttachmentParameteriv( GLuint framebuffer, GLenum attachment, GLenum pname, GLint *params ) { }
static void null_glGetNamedFramebufferAttachmentParameterivEXT( GLuint framebuffer, GLenum attachment, GLenum pname, GLint *params ) { }
static void null_glGetNamedFramebufferParameterfvAMD( GLuint framebuffer, GLenum pname, GLuint numsamples, GLuint pixelindex, GLsizei size, GLfloat *values ) { }
static void null_glGetNamedFramebufferParameteriv( GLuint framebuffer, GLenum pname, GLint *param ) { }
static void null_glGetNamedFramebufferParameterivEXT( GLuint framebuffer, GLenum pname, GLint *params ) { }
static void null_glGetNamedProgramLocalParameterIivEXT( GLuint program, GLenum target, GLuint index, GLint *params ) { }
static void null_glGetNamedProgramLocalParameterIuivEXT( GLuint program, GLenum target, GLuint index, GLuint *params ) { }
static void null_glGetNamedProgramLocalParameterdvEXT( GLuint program, GLenum target, GLuint index, GLdouble *params ) { }
static void null_glGetNamedProgramLocalParameterfvEXT( GLuint program, GLenum target, GLuint index, GLfloat *params ) { }
static void null_glGetNamedProgramStringEXT( GLuint program, GLenum target, GLenum pname, void *string ) { }
static void null_glGetNamedProgramivEXT( GLuint program, GLenum target, GLenum pname, GLint *params ) { }
static void null_glGetNamedRenderbufferParameteriv( GLuint renderbuffer, GLenum pname, GLint *params ) { }
static void null_glGetNamedRenderbufferParameterivEXT( GLuint renderbuffer, GLenum pname, GLint *params ) { }
static void null_glGetNamedStringARB( GLint namelen, const GLchar *name, GLsizei bufSize, GLint *stringlen, GLchar *string ) { }
static void null_glGetNamedStringivARB( GLint namelen, const GLchar *name, GLenum pname, GLint *params ) { }
static void null_glGetNextPerfQueryIdINTEL( GLuint queryId, GLuint *nextQueryId ) { }
static void null_glGetObjectBufferfvATI( GLuint buffer, GLenum pname, GLfloat *params ) { }
static void null_glGetObjectBufferivATI( GLuint buffer, GLenum pname, GLint *params ) { }
static void null_glGetObjectLabel( GLenum identifier, GLuint name, GLsizei bufSize, GLsizei *length, GLchar *label ) { }
static void null_glGetObjectLabelEXT( GLenum type, GLuint object, GLsizei bufSize, GLsizei *length, GLchar *label ) { }
static void null_glGetObjectParameterfvARB( GLhandleARB obj, GLenum pname, GLfloat *params ) { }
static void null_glGetObjectParameterivAPPLE( GLenum objectType, GLuint name, GLenum pname, GLint *params ) { }
static void null_glGetObjectParameterivARB( GLhandleARB obj, GLenum pname, GLint *params ) { }
static void null_glGetObjectPtrLabel( const void *ptr, GLsizei bufSize, GLsizei *length, GLchar *label ) { }
static void null_glGetOcclusionQueryivNV( GLuint id, GLenum pname, GLint *params ) { }
static void null_glGetOcclusionQueryuivNV( GLuint id, GLenum pname, GLuint *params ) { }
static void null_glGetPathColorGenfvNV( GLenum color, GLenum pname, GLfloat *value ) { }
static void null_glGetPathColorGenivNV( GLenum color, GLenum pname, GLint *value ) { }
static void null_glGetPathCommandsNV( GLuint path, GLubyte *commands ) { }
static void null_glGetPathCoordsNV( GLuint path, GLfloat *coords ) { }
static void null_glGetPathDashArrayNV( GLuint path, GLfloat *dashArray ) { }
static GLfloat null_glGetPathLengthNV( GLuint path, GLsizei startSegment, GLsizei numSegments ) { return 0; }
static void null_glGetPathMetricRangeNV( GLbitfield metricQueryMask, GLuint firstPathName, GLsizei numPaths, GLsizei stride, GLfloat *metrics ) { }
static void null_glGetPathMetricsNV( GLbitfield metricQueryMask, GLsizei numPaths, GLenum pathNameType, const void *paths, GLuint pathBase, GLsizei stride, GLfloat *metrics ) { }
static void null_glGetPathParameterfvNV( GLuint path, GLenum pname, GLfloat *value ) { }
static void null_glGetPathParameterivNV( GLuint path, GLenum pname, GLint *value ) { }
static void null_glGetPathSpacingNV( GLenum pathListMode, GLsizei numPaths, GLenum pathNameType, const void *paths, GLuint pathBase, GLfloat advanceScale, GLfloat kerningScale, GLenum transformType, GLfloat *returnedSpacing ) { }
static void null_glGetPathTexGenfvNV( GLenum texCoordSet, GLenum pname, GLfloat *value ) { }
static void null_glGetPathTexGenivNV( GLenum texCoordSet, GLenum pname, GLint *value ) { }
static void null_glGetPerfCounterInfoINTEL( GLuint queryId, GLuint counterId, GLuint counterNameLength, GLchar *counterName, GLuint counterDescLength, GLchar *counterDesc, GLuint *counterOffset, GLuint *counterDataSize, GLuint *counterTypeEnum, GLuint *counterDataTypeEnum, GLuint64 *rawCounterMaxValue ) { }
static void null_glGetPerfMonitorCounterDataAMD( GLuint monitor, GLenum pname, GLsizei dataSize, GLuint *data, GLint *bytesWritten ) { }
static void null_glGetPerfMonitorCounterInfoAMD( GLuint group, GLuint counter, GLenum pname, void *data ) { }
static void null_glGetPerfMonitorCounterStringAMD( GLuint group, GLuint counter, GLsizei bufSize, GLsizei *length, GLchar *counterString ) { }
static void null_glGetPerfMonitorCountersAMD( GLuint group, GLint *numCounters, GLint *maxActiveCounters, GLsizei counterSize, GLuint *counters ) { }
static void null_glGetPerfMonitorGroupStringAMD( GLuint group, GLsizei bufSize, GLsizei *length, GLchar *groupString ) { }
static void null_glGetPerfMonitorGroupsAMD( GLint *numGroups, GLsizei groupsSize, GLuint *groups ) { }
static void null_glGetPerfQueryDataINTEL( GLuint queryHandle, GLuint flags, GLsizei dataSize, void *data, GLuint *bytesWritten ) { }
static void null_glGetPerfQueryIdByNameINTEL( GLchar *queryName, GLuint *queryId ) { }
static void null_glGetPerfQueryInfoINTEL( GLuint queryId, GLuint queryNameLength, GLchar *queryName, GLuint *dataSize, GLuint *noCounters, GLuint *noInstances, GLuint *capsMask ) { }
static void null_glGetPixelMapxv( GLenum map, GLint size, GLfixed *values ) { }
static void null_glGetPixelTexGenParameterfvSGIS( GLenum pname, GLfloat *params ) { }
static void null_glGetPixelTexGenParameterivSGIS( GLenum pname, GLint *params ) { }
static void null_glGetPixelTransformParameterfvEXT( GLenum target, GLenum pname, GLfloat *params ) { }
static void null_glGetPixelTransformParameterivEXT( GLenum target, GLenum pname, GLint *params ) { }
static void null_glGetPointerIndexedvEXT( GLenum target, GLuint index, void **data ) { }
static void null_glGetPointeri_vEXT( GLenum pname, GLuint index, void **params ) { }
static void null_glGetPointervEXT( GLenum pname, void **params ) { }
static void null_glGetProgramBinary( GLuint program, GLsizei bufSize, GLsizei *length, GLenum *binaryFormat, void *binary ) { }
static void null_glGetProgramEnvParameterIivNV( GLenum target, GLuint index, GLint *params ) { }
static void null_glGetProgramEnvParameterIuivNV( GLenum target, GLuint index, GLuint *params ) { }
static void null_glGetProgramEnvParameterdvARB( GLenum target, GLuint index, GLdouble *params ) { }
static void null_glGetProgramEnvParameterfvARB( GLenum target, GLuint index, GLfloat *params ) { }
static void null_glGetProgramInfoLog( GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog ) { }
static void null_glGetProgramInterfaceiv( GLuint program, GLenum programInterface, GLenum pname, GLint *params ) { }
static void null_glGetProgramLocalParameterIivNV( GLenum target, GLuint index, GLint *params ) { }
static void null_glGetProgramLocalParameterIuivNV( GLenum target, GLuint index, GLuint *params ) { }
static void null_glGetProgramLocalParameterdvARB( GLenum target, GLuint index, GLdouble *params ) { }
static void null_glGetProgramLocalParameterfvARB( GLenum target, GLuint index, GLfloat *params ) { }
static void null_glGetProgramNamedParameterdvNV( GLuint id, GLsizei len, const GLubyte *name, GLdouble *params ) { }
static void null_glGetProgramNamedParameterfvNV( GLuint id, GLsizei len, const GLubyte *name, GLfloat *params ) { }
static void null_glGetProgramParameterdvNV( GLenum target, GLuint index, GLenum pname, GLdouble *params ) { }
static void null_glGetProgramParameterfvNV( GLenum target, GLuint index, GLenum pname, GLfloat *params ) { }
static void null_glGetProgramPipelineInfoLog( GLuint pipeline, GLsizei bufSize, GLsizei *length, GLchar *infoLog ) { }
static void null_glGetProgramPipelineiv( GLuint pipeline, GLenum pname, GLint *params ) { }
static GLuint null_glGetProgramResourceIndex( GLuint program, GLenum programInterface, const GLchar *name ) { return 0; }
static GLint null_glGetProgramResourceLocation( GLuint program, GLenum programInterface, const GLchar *name ) { return 0; }
static GLint null_glGetProgramResourceLocationIndex( GLuint program, GLenum programInterface, const GLchar *name ) { return 0; }
static void null_glGetProgramResourceName( GLuint program, GLenum programInterface, GLuint index, GLsizei bufSize, GLsizei *length, GLchar *name ) { }
static void null_glGetProgramResourcefvNV( GLuint program, GLenum programInterface, GLuint index, GLsizei propCount, const GLenum *props, GLsizei count, GLsizei *length, GLfloat *params ) { }
static void null_glGetProgramResourceiv( GLuint program, GLenum programInterface, GLuint index, GLsizei propCount, const GLenum *props, GLsizei count, GLsizei *length, GLint *params ) { }
static void null_glGetProgramStageiv( GLuint program, GLenum shadertype, GLenum pname, GLint *values ) { }
static void null_glGetProgramStringARB( GLenum target, GLenum pname, void *string ) { }
static void null_glGetProgramStringNV( GLuint id, GLenum pname, GLubyte *program ) { }
static void null_glGetProgramSubroutineParameteruivNV( GLenum target, GLuint index, GLuint *param ) { }
static void null_glGetProgramiv( GLuint program, GLenum pname, GLint *params ) { }
static void null_glGetProgramivARB( GLenum target, GLenum pname, GLint *params ) { }
static void null_glGetProgramivNV( GLuint id, GLenum pname, GLint *params ) { }
static void null_glGetQueryBufferObjecti64v( GLuint id, GLuint buffer, GLenum pname, GLintptr offset ) { }
static void null_glGetQueryBufferObjectiv( GLuint id, GLuint buffer, GLenum pname, GLintptr offset ) { }
static void null_glGetQueryBufferObjectui64v( GLuint id, GLuint buffer, GLenum pname, GLintptr offset ) { }
static void null_glGetQueryBufferObjectuiv( GLuint id, GLuint buffer, GLenum pname, GLintptr offset ) { }
static void null_glGetQueryIndexediv( GLenum target, GLuint index, GLenum pname, GLint *params ) { }
static void null_glGetQueryObjecti64v( GLuint id, GLenum pname, GLint64 *params ) { }
static void null_glGetQueryObjecti64vEXT( GLuint id, GLenum pname, GLint64 *params ) { }
static void null_glGetQueryObjectiv( GLuint id, GLenum pname, GLint *params ) { }
static void null_glGetQueryObjectivARB( GLuint id, GLenum pname, GLint *params ) { }
static void null_glGetQueryObjectui64v( GLuint id, GLenum pname, GLuint64 *params ) { }
static void null_glGetQueryObjectui64vEXT( GLuint id, GLenum pname, GLuint64 *params ) { }
static void null_glGetQueryObjectuiv( GLuint id, GLenum pname, GLuint *params ) { }
static void null_glGetQueryObjectuivARB( GLuint id, GLenum pname, GLuint *params ) { }
static void null_glGetQueryiv( GLenum target, GLenum pname, GLint *params ) { }
static void null_glGetQueryivARB( GLenum target, GLenum pname, GLint *params ) { }
static void null_glGetRenderbufferParameteriv( GLenum target, GLenum pname, GLint *params ) { }
static void null_glGetRenderbufferParameterivEXT( GLenum target, GLenum pname, GLint *params ) { }
static void null_glGetSamplerParameterIiv( GLuint sampler, GLenum pname, GLint *params ) { }
static void null_glGetSamplerParameterIuiv( GLuint sampler, GLenum pname, GLuint *params ) { }
static void null_glGetSamplerParameterfv( GLuint sampler, GLenum pname, GLfloat *params ) { }
static void null_glGetSamplerParameteriv( GLuint sampler, GLenum pname, GLint *params ) { }
static void null_glGetSemaphoreParameterui64vEXT( GLuint semaphore, GLenum pname, GLuint64 *params ) { }
static void null_glGetSeparableFilter( GLenum target, GLenum format, GLenum type, void *row, void *column, void *span ) { }
static void null_glGetSeparableFilterEXT( GLenum target, GLenum format, GLenum type, void *row, void *column, void *span ) { }
static void null_glGetShaderInfoLog( GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog ) { }
static void null_glGetShaderPrecisionFormat( GLenum shadertype, GLenum precisiontype, GLint *range, GLint *precision ) { }
static void null_glGetShaderSource( GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *source ) { }
static void null_glGetShaderSourceARB( GLhandleARB obj, GLsizei maxLength, GLsizei *length, GLcharARB *source ) { }
static void null_glGetShaderiv( GLuint shader, GLenum pname, GLint *params ) { }
static void null_glGetShadingRateImagePaletteNV( GLuint viewport, GLuint entry, GLenum *rate ) { }
static void null_glGetShadingRateSampleLocationivNV( GLenum rate, GLuint samples, GLuint index, GLint *location ) { }
static void null_glGetSharpenTexFuncSGIS( GLenum target, GLfloat *points ) { }
static GLushort null_glGetStageIndexNV( GLenum shadertype ) { return 0; }
static const GLubyte * null_glGetStringi( GLenum name, GLuint index ) { return 0; }
static GLuint null_glGetSubroutineIndex( GLuint program, GLenum shadertype, const GLchar *name ) { return 0; }
static GLint null_glGetSubroutineUniformLocation( GLuint program, GLenum shadertype, const GLchar *name ) { return 0; }
static void null_glGetSynciv( GLsync sync, GLenum pname, GLsizei count, GLsizei *length, GLint *values ) { }
static void null_glGetTexBumpParameterfvATI( GLenum pname, GLfloat *param ) { }
static void null_glGetTexBumpParameterivATI( GLenum pname, GLint *param ) { }
static void null_glGetTexEnvxvOES( GLenum target, GLenum pname, GLfixed *params ) { }
static void null_glGetTexFilterFuncSGIS( GLenum target, GLenum filter, GLfloat *weights ) { }
static void null_glGetTexGenxvOES( GLenum coord, GLenum pname, GLfixed *params ) { }
static void null_glGetTexLevelParameterxvOES( GLenum target, GLint level, GLenum pname, GLfixed *params ) { }
static void null_glGetTexParameterIiv( GLenum target, GLenum pname, GLint *params ) { }
static void null_glGetTexParameterIivEXT( GLenum target, GLenum pname, GLint *params ) { }
static void null_glGetTexParameterIuiv( GLenum target, GLenum pname, GLuint *params ) { }
static void null_glGetTexParameterIuivEXT( GLenum target, GLenum pname, GLuint *params ) { }
static void null_glGetTexParameterPointervAPPLE( GLenum target, GLenum pname, void **params ) { }
static void null_glGetTexParameterxvOES( GLenum target, GLenum pname, GLfixed *params ) { }
static GLuint64 null_glGetTextureHandleARB( GLuint texture ) { return 0; }
static GLuint64 null_glGetTextureHandleNV( GLuint texture ) { return 0; }
static void null_glGetTextureImage( GLuint texture, GLint level, GLenum format, GLenum type, GLsizei bufSize, void *pixels ) { }
static void null_glGetTextureImageEXT( GLuint texture, GLenum target, GLint level, GLenum format, GLenum type, void *pixels ) { }
static void null_glGetTextureLevelParameterfv( GLuint texture, GLint level, GLenum pname, GLfloat *params ) { }
static void null_glGetTextureLevelParameterfvEXT( GLuint texture, GLenum target, GLint level, GLenum pname, GLfloat *params ) { }
static void null_glGetTextureLevelParameteriv( GLuint texture, GLint level, GLenum pname, GLint *params ) { }
static void null_glGetTextureLevelParameterivEXT( GLuint texture, GLenum target, GLint level, GLenum pname, GLint *params ) { }
static void null_glGetTextureParameterIiv( GLuint texture, GLenum pname, GLint *params ) { }
static void null_glGetTextureParameterIivEXT( GLuint texture, GLenum target, GLenum pname, GLint *params ) { }
static void null_glGetTextureParameterIuiv( GLuint texture, GLenum pname, GLuint *params ) { }
static void null_glGetTextureParameterIuivEXT( GLuint texture, GLenum target, GLenum pname, GLuint *params ) { }
static void null_glGetTextureParameterfv( GLuint texture, GLenum pname, GLfloat *params ) { }
static void null_glGetTextureParameterfvEXT( GLuint texture, GLenum target, GLenum pname, GLfloat *params ) { }
static void null_glGetTextureParameteriv( GLuint texture, GLenum pname, GLint *params ) { }
static void null_glGetTextureParameterivEXT( GLuint texture, GLenum target, GLenum pname, GLint *params ) { }
static GLuint64 null_glGetTextureSamplerHandleARB( GLuint texture, GLuint sampler ) { return 0; }
static GLuint64 null_glGetTextureSamplerHandleNV( GLuint texture, GLuint sampler ) { return 0; }
static void null_glGetTextureSubImage( GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, GLsizei bufSize, void *pixels ) { }
static void null_glGetTrackMatrixivNV( GLenum target, GLuint address, GLenum pname, GLint *params ) { }
static void null_glGetTransformFeedbackVarying( GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLsizei *size, GLenum *type, GLchar *name ) { }
static void null_glGetTransformFeedbackVaryingEXT( GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLsizei *size, GLenum *type, GLchar *name ) { }
static void null_glGetTransformFeedbackVaryingNV( GLuint program, GLuint index, GLint *location ) { }
static void null_glGetTransformFeedbacki64_v( GLuint xfb, GLenum pname, GLuint index, GLint64 *param ) { }
static void null_glGetTransformFeedbacki_v( GLuint xfb, GLenum pname, GLuint index, GLint *param ) { }
static void null_glGetTransformFeedbackiv( GLuint xfb, GLenum pname, GLint *param ) { }
static GLuint null_glGetUniformBlockIndex( GLuint program, const GLchar *uniformBlockName ) { return 0; }
static GLint null_glGetUniformBufferSizeEXT( GLuint program, GLint location ) { return 0; }
static void null_glGetUniformIndices( GLuint program, GLsizei uniformCount, const GLchar *const*uniformNames, GLuint *uniformIndices ) { }
static GLint null_glGetUniformLocation( GLuint program, const GLchar *name ) { return 0; }
static GLint null_glGetUniformLocationARB( GLhandleARB programObj, const GLcharARB *name ) { return 0; }
static GLintptr null_glGetUniformOffsetEXT( GLuint program, GLint location ) { return 0; }
static void null_glGetUniformSubroutineuiv( GLenum shadertype, GLint location, GLuint *params ) { }
static void null_glGetUniformdv( GLuint program, GLint location, GLdouble *params ) { }
static void null_glGetUniformfv( GLuint program, GLint location, GLfloat *params ) { }
static void null_glGetUniformfvARB( GLhandleARB programObj, GLint location, GLfloat *params ) { }
static void null_glGetUniformi64vARB( GLuint program, GLint location, GLint64 *params ) { }
static void null_glGetUniformi64vNV( GLuint program, GLint location, GLint64EXT *params ) { }
static void null_glGetUniformiv( GLuint program, GLint location, GLint *params ) { }
static void null_glGetUniformivARB( GLhandleARB programObj, GLint location, GLint *params ) { }
static void null_glGetUniformui64vARB( GLuint program, GLint location, GLuint64 *params ) { }
static void null_glGetUniformui64vNV( GLuint program, GLint location, GLuint64EXT *params ) { }
static void null_glGetUniformuiv( GLuint program, GLint location, GLuint *params ) { }
static void null_glGetUniformuivEXT( GLuint program, GLint location, GLuint *params ) { }
static void null_glGetUnsignedBytei_vEXT( GLenum target, GLuint index, GLubyte *data ) { }
static void null_glGetUnsignedBytevEXT( GLenum pname, GLubyte *data ) { }
static void null_glGetVariantArrayObjectfvATI( GLuint id, GLenum pname, GLfloat *params ) { }
static void null_glGetVariantArrayObjectivATI( GLuint id, GLenum pname, GLint *params ) { }
static void null_glGetVariantBooleanvEXT( GLuint id, GLenum value, GLboolean *data ) { }
static void null_glGetVariantFloatvEXT( GLuint id, GLenum value, GLfloat *data ) { }
static void null_glGetVariantIntegervEXT( GLuint id, GLenum value, GLint *data ) { }
static void null_glGetVariantPointervEXT( GLuint id, GLenum value, void **data ) { }
static GLint null_glGetVaryingLocationNV( GLuint program, const GLchar *name ) { return 0; }
static void null_glGetVertexArrayIndexed64iv( GLuint vaobj, GLuint index, GLenum pname, GLint64 *param ) { }
static void null_glGetVertexArrayIndexediv( GLuint vaobj, GLuint index, GLenum pname, GLint *param ) { }
static void null_glGetVertexArrayIntegeri_vEXT( GLuint vaobj, GLuint index, GLenum pname, GLint *param ) { }
static void null_glGetVertexArrayIntegervEXT( GLuint vaobj, GLenum pname, GLint *param ) { }
static void null_glGetVertexArrayPointeri_vEXT( GLuint vaobj, GLuint index, GLenum pname, void **param ) { }
static void null_glGetVertexArrayPointervEXT( GLuint vaobj, GLenum pname, void **param ) { }
static void null_glGetVertexArrayiv( GLuint vaobj, GLenum pname, GLint *param ) { }
static void null_glGetVertexAttribArrayObjectfvATI( GLuint index, GLenum pname, GLfloat *params ) { }
static void null_glGetVertexAttribArrayObjectivATI( GLuint index, GLenum pname, GLint *params ) { }
static void null_glGetVertexAttribIiv( GLuint index, GLenum pname, GLint *params ) { }
static void null_glGetVertexAttribIivEXT( GLuint index, GLenum pname, GLint *params ) { }
static void null_glGetVertexAttribIuiv( GLuint index, GLenum pname, GLuint *params ) { }
static void null_glGetVertexAttribIuivEXT( GLuint index, GLenum pname, GLuint *params ) { }
static void null_glGetVertexAttribLdv( GLuint index, GLenum pname, GLdouble *params ) { }
static void null_glGetVertexAttribLdvEXT( GLuint index, GLenum pname, GLdouble *params ) { }
static void null_glGetVertexAttribLi64vNV( GLuint index, GLenum pname, GLint64EXT *params ) { }
static void null_glGetVertexAttribLui64vARB( GLuint index, GLenum pname, GLuint64EXT *params ) { }
static void null_glGetVertexAttribLui64vNV( GLuint index, GLenum pname, GLuint64EXT *params ) { }
static void null_glGetVertexAttribPointerv( GLuint index, GLenum pname, void **pointer ) { }
static void null_glGetVertexAttribPointervARB( GLuint index, GLenum pname, void **pointer ) { }
static void null_glGetVertexAttribPointervNV( GLuint index, GLenum pname, void **pointer ) { }
static void null_glGetVertexAttribdv( GLuint index, GLenum pname, GLdouble *params ) { }
static void null_glGetVertexAttribdvARB( GLuint index, GLenum pname, GLdouble *params ) { }
static void null_glGetVertexAttribdvNV( GLuint index, GLenum pname, GLdouble *params ) { }
static void null_glGetVertexAttribfv( GLuint index, GLenum pname, GLfloat *params ) { }
static void null_glGetVertexAttribfvARB( GLuint index, GLenum pname, GLfloat *params ) { }
static void null_glGetVertexAttribfvNV( GLuint index, GLenum pname, GLfloat *params ) { }
static void null_glGetVertexAttribiv( GLuint index, GLenum pname, GLint *params ) { }
static void null_glGetVertexAttribivARB( GLuint index, GLenum pname, GLint *params ) { }
static void null_glGetVertexAttribivNV( GLuint index, GLenum pname, GLint *params ) { }
static void null_glGetVideoCaptureStreamdvNV( GLuint video_capture_slot, GLuint stream, GLenum pname, GLdouble *params ) { }
static void null_glGetVideoCaptureStreamfvNV( GLuint video_capture_slot, GLuint stream, GLenum pname, GLfloat *params ) { }
static void null_glGetVideoCaptureStreamivNV( GLuint video_capture_slot, GLuint stream, GLenum pname, GLint *params ) { }
static void null_glGetVideoCaptureivNV( GLuint video_capture_slot, GLenum pname, GLint *params ) { }
static void null_glGetVideoi64vNV( GLuint video_slot, GLenum pname, GLint64EXT *params ) { }
static void null_glGetVideoivNV( GLuint video_slot, GLenum pname, GLint *params ) { }
static void null_glGetVideoui64vNV( GLuint video_slot, GLenum pname, GLuint64EXT *params ) { }
static void null_glGetVideouivNV( GLuint video_slot, GLenum pname, GLuint *params ) { }
static GLVULKANPROCNV null_glGetVkProcAddrNV( const GLchar *name ) { return 0; }
static void null_glGetnColorTable( GLenum target, GLenum format, GLenum type, GLsizei bufSize, void *table ) { }
static void null_glGetnColorTableARB( GLenum target, GLenum format, GLenum type, GLsizei bufSize, void *table ) { }
static void null_glGetnCompressedTexImage( GLenum target, GLint lod, GLsizei bufSize, void *pixels ) { }
static void null_glGetnCompressedTexImageARB( GLenum target, GLint lod, GLsizei bufSize, void *img ) { }
static void null_glGetnConvolutionFilter( GLenum target, GLenum format, GLenum type, GLsizei bufSize, void *image ) { }
static void null_glGetnConvolutionFilterARB( GLenum target, GLenum format, GLenum type, GLsizei bufSize, void *image ) { }
static void null_glGetnHistogram( GLenum target, GLboolean reset, GLenum format, GLenum type, GLsizei bufSize, void *values ) { }
static void null_glGetnHistogramARB( GLenum target, GLboolean reset, GLenum format, GLenum type, GLsizei bufSize, void *values ) { }
static void null_glGetnMapdv( GLenum target, GLenum query, GLsizei bufSize, GLdouble *v ) { }
static void null_glGetnMapdvARB( GLenum target, GLenum query, GLsizei bufSize, GLdouble *v ) { }
static void null_glGetnMapfv( GLenum target, GLenum query, GLsizei bufSize, GLfloat *v ) { }
static void null_glGetnMapfvARB( GLenum target, GLenum query, GLsizei bufSize, GLfloat *v ) { }
static void null_glGetnMapiv( GLenum target, GLenum query, GLsizei bufSize, GLint *v ) { }
static void null_glGetnMapivARB( GLenum target, GLenum query, GLsizei bufSize, GLint *v ) { }
static void null_glGetnMinmax( GLenum target, GLboolean reset, GLenum format, GLenum type, GLsizei bufSize, void *values ) { }
static void null_glGetnMinmaxARB( GLenum target, GLboolean reset, GLenum format, GLenum type, GLsizei bufSize, void *values ) { }
static void null_glGetnPixelMapfv( GLenum map, GLsizei bufSize, GLfloat *values ) { }
static void null_glGetnPixelMapfvARB( GLenum map, GLsizei bufSize, GLfloat *values ) { }
static void null_glGetnPixelMapuiv( GLenum map, GLsizei bufSize, GLuint *values ) { }
static void null_glGetnPixelMapuivARB( GLenum map, GLsizei bufSize, GLuint *values ) { }
static void null_glGetnPixelMapusv( GLenum map, GLsizei bufSize, GLushort *values ) { }
static void null_glGetnPixelMapusvARB( GLenum map, GLsizei bufSize, GLushort *values ) { }
static void null_glGetnPolygonStipple( GLsizei bufSize, GLubyte *pattern ) { }
static void null_glGetnPolygonStippleARB( GLsizei bufSize, GLubyte *pattern ) { }
static void null_glGetnSeparableFilter( GLenum target, GLenum format, GLenum type, GLsizei rowBufSize, void *row, GLsizei columnBufSize, void *column, void *span ) { }
static void null_glGetnSeparableFilterARB( GLenum target, GLenum format, GLenum type, GLsizei rowBufSize, void *row, GLsizei columnBufSize, void *column, void *span ) { }
static void null_glGetnTexImage( GLenum target, GLint level, GLenum format, GLenum type, GLsizei bufSize, void *pixels ) { }
static void null_glGetnTexImageARB( GLenum target, GLint level, GLenum format, GLenum type, GLsizei bufSize, void *img ) { }
static void null_glGetnUniformdv( GLuint program, GLint location, GLsizei bufSize, GLdouble *params ) { }
static void null_glGetnUniformdvARB( GLuint program, GLint location, GLsizei bufSize, GLdouble *params ) { }
static void null_glGetnUniformfv( GLuint program, GLint location, GLsizei bufSize, GLfloat *params ) { }
static void null_glGetnUniformfvARB( GLuint program, GLint location, GLsizei bufSize, GLfloat *params ) { }
static void null_glGetnUniformi64vARB( GLuint program, GLint location, GLsizei bufSize, GLint64 *params ) { }
static void null_glGetnUniformiv( GLuint program, GLint location, GLsizei bufSize, GLint *params ) { }
static void null_glGetnUniformivARB( GLuint program, GLint location, GLsizei bufSize, GLint *params ) { }
static void null_glGetnUniformui64vARB( GLuint program, GLint location, GLsizei bufSize, GLuint64 *params ) { }
static void null_glGetnUniformuiv( GLuint program, GLint location, GLsizei bufSize, GLuint *params ) { }
static void null_glGetnUniformuivARB( GLuint program, GLint location, GLsizei bufSize, GLuint *params ) { }
static void null_glGlobalAlphaFactorbSUN( GLbyte factor ) { }
static void null_glGlobalAlphaFactordSUN( GLdouble factor ) { }
static void null_glGlobalAlphaFactorfSUN( GLfloat factor ) { }
static void null_glGlobalAlphaFactoriSUN( GLint factor ) { }
static void null_glGlobalAlphaFactorsSUN( GLshort factor ) { }
static void null_glGlobalAlphaFactorubSUN( GLubyte factor ) { }
static void null_glGlobalAlphaFactoruiSUN( GLuint factor ) { }
static void null_glGlobalAlphaFactorusSUN( GLushort factor ) { }
static void null_glHintPGI( GLenum target, GLint mode ) { }
static void null_glHistogram( GLenum target, GLsizei width, GLenum internalformat, GLboolean sink ) { }
static void null_glHistogramEXT( GLenum target, GLsizei width, GLenum internalformat, GLboolean sink ) { }
static void null_glIglooInterfaceSGIX( GLenum pname, const void *params ) { }
static void null_glImageTransformParameterfHP( GLenum target, GLenum pname, GLfloat param ) { }
static void null_glImageTransformParameterfvHP( GLenum target, GLenum pname, const GLfloat *params ) { }
static void null_glImageTransformParameteriHP( GLenum target, GLenum pname, GLint param ) { }
static void null_glImageTransformParameterivHP( GLenum target, GLenum pname, const GLint *params ) { }
static void null_glImportMemoryFdEXT( GLuint memory, GLuint64 size, GLenum handleType, GLint fd ) { }
static void null_glImportMemoryWin32HandleEXT( GLuint memory, GLuint64 size, GLenum handleType, void *handle ) { }
static void null_glImportMemoryWin32NameEXT( GLuint memory, GLuint64 size, GLenum handleType, const void *name ) { }
static void null_glImportSemaphoreFdEXT( GLuint semaphore, GLenum handleType, GLint fd ) { }
static void null_glImportSemaphoreWin32HandleEXT( GLuint semaphore, GLenum handleType, void *handle ) { }
static void null_glImportSemaphoreWin32NameEXT( GLuint semaphore, GLenum handleType, const void *name ) { }
static GLsync null_glImportSyncEXT( GLenum external_sync_type, GLintptr external_sync, GLbitfield flags ) { return 0; }
static void null_glIndexFormatNV( GLenum type, GLsizei stride ) { }
static void null_glIndexFuncEXT( GLenum func, GLclampf ref ) { }
static void null_glIndexMaterialEXT( GLenum face, GLenum mode ) { }
static void null_glIndexPointerEXT( GLenum type, GLsizei stride, GLsizei count, const void *pointer ) { }
static void null_glIndexPointerListIBM( GLenum type, GLint stride, const void **pointer, GLint ptrstride ) { }
static void null_glIndexxOES( GLfixed component ) { }
static void null_glIndexxvOES( const GLfixed *component ) { }
static void null_glInsertComponentEXT( GLuint res, GLuint src, GLuint num ) { }
static void null_glInsertEventMarkerEXT( GLsizei length, const GLchar *marker ) { }
static void null_glInstrumentsBufferSGIX( GLsizei size, GLint *buffer ) { }
static void null_glInterpolatePathsNV( GLuint resultPath, GLuint pathA, GLuint pathB, GLfloat weight ) { }
static void null_glInvalidateBufferData( GLuint buffer ) { }
static void null_glInvalidateBufferSubData( GLuint buffer, GLintptr offset, GLsizeiptr length ) { }
static void null_glInvalidateFramebuffer( GLenum target, GLsizei numAttachments, const GLenum *attachments ) { }
static void null_glInvalidateNamedFramebufferData( GLuint framebuffer, GLsizei numAttachments, const GLenum *attachments ) { }
static void null_glInvalidateNamedFramebufferSubData( GLuint framebuffer, GLsizei numAttachments, const GLenum *attachments, GLint x, GLint y, GLsizei width, GLsizei height ) { }
static void null_glInvalidateSubFramebuffer( GLenum target, GLsizei numAttachments, const GLenum *attachments, GLint x, GLint y, GLsizei width, GLsizei height ) { }
static void null_glInvalidateTexImage( GLuint texture, GLint level ) { }
static void null_glInvalidateTexSubImage( GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth ) { }
static GLboolean null_glIsAsyncMarkerSGIX( GLuint marker ) { return 0; }
static GLboolean null_glIsBuffer( GLuint buffer ) { return 0; }
static GLboolean null_glIsBufferARB( GLuint buffer ) { return 0; }
static GLboolean null_glIsBufferResidentNV( GLenum target ) { return 0; }
static GLboolean null_glIsCommandListNV( GLuint list ) { return 0; }
static GLboolean null_glIsEnabledIndexedEXT( GLenum target, GLuint index ) { return 0; }
static GLboolean null_glIsEnabledi( GLenum target, GLuint index ) { return 0; }
static GLboolean null_glIsFenceAPPLE( GLuint fence ) { return 0; }
static GLboolean null_glIsFenceNV( GLuint fence ) { return 0; }
static GLboolean null_glIsFramebuffer( GLuint framebuffer ) { return 0; }
static GLboolean null_glIsFramebufferEXT( GLuint framebuffer ) { return 0; }
static GLboolean null_glIsImageHandleResidentARB( GLuint64 handle ) { return 0; }
static GLboolean null_glIsImageHandleResidentNV( GLuint64 handle ) { return 0; }
static GLboolean null_glIsMemoryObjectEXT( GLuint memoryObject ) { return 0; }
static GLboolean null_glIsNameAMD( GLenum identifier, GLuint name ) { return 0; }
static GLboolean null_glIsNamedBufferResidentNV( GLuint buffer ) { return 0; }
static GLboolean null_glIsNamedStringARB( GLint namelen, const GLchar *name ) { return 0; }
static GLboolean null_glIsObjectBufferATI( GLuint buffer ) { return 0; }
static GLboolean null_glIsOcclusionQueryNV( GLuint id ) { return 0; }
static GLboolean null_glIsPathNV( GLuint path ) { return 0; }
static GLboolean null_glIsPointInFillPathNV( GLuint path, GLuint mask, GLfloat x, GLfloat y ) { return 0; }
static GLboolean null_glIsPointInStrokePathNV( GLuint path, GLfloat x, GLfloat y ) { return 0; }
static GLboolean null_glIsProgram( GLuint program ) { return 0; }
static GLboolean null_glIsProgramARB( GLuint program ) { return 0; }
static GLboolean null_glIsProgramNV( GLuint id ) { return 0; }
static GLboolean null_glIsProgramPipeline( GLuint pipeline ) { return 0; }
static GLboolean null_glIsQuery( GLuint id ) { return 0; }
static GLboolean null_glIsQueryARB( GLuint id ) { return 0; }
static GLboolean null_glIsRenderbuffer( GLuint renderbuffer ) { return 0; }
static GLboolean null_glIsRenderbufferEXT( GLuint renderbuffer ) { return 0; }
static GLboolean null_glIsSampler( GLuint sampler ) { return 0; }
static GLboolean null_glIsSemaphoreEXT( GLuint semaphore ) { return 0; }
static GLboolean null_glIsShader( GLuint shader ) { return 0; }
static GLboolean null_glIsStateNV( GLuint state ) { return 0; }
static GLboolean null_glIsSync( GLsync sync ) { return 0; }
static GLboolean null_glIsTextureEXT( GLuint texture ) { return 0; }
static GLboolean null_glIsTextureHandleResidentARB( GLuint64 handle ) { return 0; }
static GLboolean null_glIsTextureHandleResidentNV( GLuint64 handle ) { return 0; }
static GLboolean null_glIsTransformFeedback( GLuint id ) { return 0; }
static GLboolean null_glIsTransformFeedbackNV( GLuint id ) { return 0; }
static GLboolean null_glIsVariantEnabledEXT( GLuint id, GLenum cap ) { return 0; }
static GLboolean null_glIsVertexArray( GLuint array ) { return 0; }
static GLboolean null_glIsVertexArrayAPPLE( GLuint array ) { return 0; }
static GLboolean null_glIsVertexAttribEnabledAPPLE( GLuint index, GLenum pname ) { return 0; }
static void null_glLGPUCopyImageSubDataNVX( GLuint sourceGpu, GLbitfield destinationGpuMask, GLuint srcName, GLenum srcTarget, GLint srcLevel, GLint srcX, GLint srxY, GLint srcZ, GLuint dstName, GLenum dstTarget, GLint dstLevel, GLint dstX, GLint dstY, GLint dstZ, GLsizei width, GLsizei height, GLsizei depth ) { }
static void null_glLGPUInterlockNVX(void) { }
static void null_glLGPUNamedBufferSubDataNVX( GLbitfield gpuMask, GLuint buffer, GLintptr offset, GLsizeiptr size, const void *data ) { }
static void null_glLabelObjectEXT( GLenum type, GLuint object, GLsizei length, const GLchar *label ) { }
static void null_glLightEnviSGIX( GLenum pname, GLint param ) { }
static void null_glLightModelxOES( GLenum pname, GLfixed param ) { }
static void null_glLightModelxvOES( GLenum pname, const GLfixed *param ) { }
static void null_glLightxOES( GLenum light, GLenum pname, GLfixed param ) { }
static void null_glLightxvOES( GLenum light, GLenum pname, const GLfixed *params ) { }
static void null_glLineWidthxOES( GLfixed width ) { }
static void null_glLinkProgram( GLuint program ) { }
static void null_glLinkProgramARB( GLhandleARB programObj ) { }
static void null_glListDrawCommandsStatesClientNV( GLuint list, GLuint segment, const void **indirects, const GLsizei *sizes, const GLuint *states, const GLuint *fbos, GLuint count ) { }
static void null_glListParameterfSGIX( GLuint list, GLenum pname, GLfloat param ) { }
static void null_glListParameterfvSGIX( GLuint list, GLenum pname, const GLfloat *params ) { }
static void null_glListParameteriSGIX( GLuint list, GLenum pname, GLint param ) { }
static void null_glListParameterivSGIX( GLuint list, GLenum pname, const GLint *params ) { }
static void null_glLoadIdentityDeformationMapSGIX( GLbitfield mask ) { }
static void null_glLoadMatrixxOES( const GLfixed *m ) { }
static void null_glLoadProgramNV( GLenum target, GLuint id, GLsizei len, const GLubyte *program ) { }
static void null_glLoadTransposeMatrixd( const GLdouble *m ) { }
static void null_glLoadTransposeMatrixdARB( const GLdouble *m ) { }
static void null_glLoadTransposeMatrixf( const GLfloat *m ) { }
static void null_glLoadTransposeMatrixfARB( const GLfloat *m ) { }
static void null_glLoadTransposeMatrixxOES( const GLfixed *m ) { }
static void null_glLockArraysEXT( GLint first, GLsizei count ) { }
static void null_glMTexCoord2fSGIS( GLenum target, GLfloat s, GLfloat t ) { }
static void null_glMTexCoord2fvSGIS( GLenum target, GLfloat * v ) { }
static void null_glMakeBufferNonResidentNV( GLenum target ) { }
static void null_glMakeBufferResidentNV( GLenum target, GLenum access ) { }
static void null_glMakeImageHandleNonResidentARB( GLuint64 handle ) { }
static void null_glMakeImageHandleNonResidentNV( GLuint64 handle ) { }
static void null_glMakeImageHandleResidentARB( GLuint64 handle, GLenum access ) { }
static void null_glMakeImageHandleResidentNV( GLuint64 handle, GLenum access ) { }
static void null_glMakeNamedBufferNonResidentNV( GLuint buffer ) { }
static void null_glMakeNamedBufferResidentNV( GLuint buffer, GLenum access ) { }
static void null_glMakeTextureHandleNonResidentARB( GLuint64 handle ) { }
static void null_glMakeTextureHandleNonResidentNV( GLuint64 handle ) { }
static void null_glMakeTextureHandleResidentARB( GLuint64 handle ) { }
static void null_glMakeTextureHandleResidentNV( GLuint64 handle ) { }
static void null_glMap1xOES( GLenum target, GLfixed u1, GLfixed u2, GLint stride, GLint order, GLfixed points ) { }
static void null_glMap2xOES( GLenum target, GLfixed u1, GLfixed u2, GLint ustride, GLint uorder, GLfixed v1, GLfixed v2, GLint vstride, GLint vorder, GLfixed points ) { }
static void * null_glMapBuffer( GLenum target, GLenum access ) { return 0; }
static void * null_glMapBufferARB( GLenum target, GLenum access ) { return 0; }
static void * null_glMapBufferRange( GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access ) { return 0; }
static void null_glMapControlPointsNV( GLenum target, GLuint index, GLenum type, GLsizei ustride, GLsizei vstride, GLint uorder, GLint vorder, GLboolean packed, const void *points ) { }
static void null_glMapGrid1xOES( GLint n, GLfixed u1, GLfixed u2 ) { }
static void null_glMapGrid2xOES( GLint n, GLfixed u1, GLfixed u2, GLfixed v1, GLfixed v2 ) { }
static void * null_glMapNamedBuffer( GLuint buffer, GLenum access ) { return 0; }
static void * null_glMapNamedBufferEXT( GLuint buffer, GLenum access ) { return 0; }
static void * null_glMapNamedBufferRange( GLuint buffer, GLintptr offset, GLsizeiptr length, GLbitfield access ) { return 0; }
static void * null_glMapNamedBufferRangeEXT( GLuint buffer, GLintptr offset, GLsizeiptr length, GLbitfield access ) { return 0; }
static void * null_glMapObjectBufferATI( GLuint buffer ) { return 0; }
static void null_glMapParameterfvNV( GLenum target, GLenum pname, const GLfloat *params ) { }
static void null_glMapParameterivNV( GLenum target, GLenum pname, const GLint *params ) { }
static void * null_glMapTexture2DINTEL( GLuint texture, GLint level, GLbitfield access, GLint *stride, GLenum *layout ) { return 0; }
static void null_glMapVertexAttrib1dAPPLE( GLuint index, GLuint size, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble *points ) { }
static void null_glMapVertexAttrib1fAPPLE( GLuint index, GLuint size, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat *points ) { }
static void null_glMapVertexAttrib2dAPPLE( GLuint index, GLuint size, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, const GLdouble *points ) { }
static void null_glMapVertexAttrib2fAPPLE( GLuint index, GLuint size, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat *points ) { }
static void null_glMaterialxOES( GLenum face, GLenum pname, GLfixed param ) { }
static void null_glMaterialxvOES( GLenum face, GLenum pname, const GLfixed *param ) { }
static void null_glMatrixFrustumEXT( GLenum mode, GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar ) { }
static void null_glMatrixIndexPointerARB( GLint size, GLenum type, GLsizei stride, const void *pointer ) { }
static void null_glMatrixIndexubvARB( GLint size, const GLubyte *indices ) { }
static void null_glMatrixIndexuivARB( GLint size, const GLuint *indices ) { }
static void null_glMatrixIndexusvARB( GLint size, const GLushort *indices ) { }
static void null_glMatrixLoad3x2fNV( GLenum matrixMode, const GLfloat *m ) { }
static void null_glMatrixLoad3x3fNV( GLenum matrixMode, const GLfloat *m ) { }
static void null_glMatrixLoadIdentityEXT( GLenum mode ) { }
static void null_glMatrixLoadTranspose3x3fNV( GLenum matrixMode, const GLfloat *m ) { }
static void null_glMatrixLoadTransposedEXT( GLenum mode, const GLdouble *m ) { }
static void null_glMatrixLoadTransposefEXT( GLenum mode, const GLfloat *m ) { }
static void null_glMatrixLoaddEXT( GLenum mode, const GLdouble *m ) { }
static void null_glMatrixLoadfEXT( GLenum mode, const GLfloat *m ) { }
static void null_glMatrixMult3x2fNV( GLenum matrixMode, const GLfloat *m ) { }
static void null_glMatrixMult3x3fNV( GLenum matrixMode, const GLfloat *m ) { }
static void null_glMatrixMultTranspose3x3fNV( GLenum matrixMode, const GLfloat *m ) { }
static void null_glMatrixMultTransposedEXT( GLenum mode, const GLdouble *m ) { }
static void null_glMatrixMultTransposefEXT( GLenum mode, const GLfloat *m ) { }
static void null_glMatrixMultdEXT( GLenum mode, const GLdouble *m ) { }
static void null_glMatrixMultfEXT( GLenum mode, const GLfloat *m ) { }
static void null_glMatrixOrthoEXT( GLenum mode, GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar ) { }
static void null_glMatrixPopEXT( GLenum mode ) { }
static void null_glMatrixPushEXT( GLenum mode ) { }
static void null_glMatrixRotatedEXT( GLenum mode, GLdouble angle, GLdouble x, GLdouble y, GLdouble z ) { }
static void null_glMatrixRotatefEXT( GLenum mode, GLfloat angle, GLfloat x, GLfloat y, GLfloat z ) { }
static void null_glMatrixScaledEXT( GLenum mode, GLdouble x, GLdouble y, GLdouble z ) { }
static void null_glMatrixScalefEXT( GLenum mode, GLfloat x, GLfloat y, GLfloat z ) { }
static void null_glMatrixTranslatedEXT( GLenum mode, GLdouble x, GLdouble y, GLdouble z ) { }
static void null_glMatrixTranslatefEXT( GLenum mode, GLfloat x, GLfloat y, GLfloat z ) { }
static void null_glMaxShaderCompilerThreadsARB( GLuint count ) { }
static void null_glMaxShaderCompilerThreadsKHR( GLuint count ) { }
static void null_glMemoryBarrier( GLbitfield barriers ) { }
static void null_glMemoryBarrierByRegion( GLbitfield barriers ) { }
static void null_glMemoryBarrierEXT( GLbitfield barriers ) { }
static void null_glMemoryObjectParameterivEXT( GLuint memoryObject, GLenum pname, const GLint *params ) { }
static void null_glMinSampleShading( GLfloat value ) { }
static void null_glMinSampleShadingARB( GLfloat value ) { }
static void null_glMinmax( GLenum target, GLenum internalformat, GLboolean sink ) { }
static void null_glMinmaxEXT( GLenum target, GLenum internalformat, GLboolean sink ) { }
static void null_glMultMatrixxOES( const GLfixed *m ) { }
static void null_glMultTransposeMatrixd( const GLdouble *m ) { }
static void null_glMultTransposeMatrixdARB( const GLdouble *m ) { }
static void null_glMultTransposeMatrixf( const GLfloat *m ) { }
static void null_glMultTransposeMatrixfARB( const GLfloat *m ) { }
static void null_glMultTransposeMatrixxOES( const GLfixed *m ) { }
static void null_glMultiDrawArrays( GLenum mode, const GLint *first, const GLsizei *count, GLsizei drawcount ) { }
static void null_glMultiDrawArraysEXT( GLenum mode, const GLint *first, const GLsizei *count, GLsizei primcount ) { }
static void null_glMultiDrawArraysIndirect( GLenum mode, const void *indirect, GLsizei drawcount, GLsizei stride ) { }
static void null_glMultiDrawArraysIndirectAMD( GLenum mode, const void *indirect, GLsizei primcount, GLsizei stride ) { }
static void null_glMultiDrawArraysIndirectBindlessCountNV( GLenum mode, const void *indirect, GLsizei drawCount, GLsizei maxDrawCount, GLsizei stride, GLint vertexBufferCount ) { }
static void null_glMultiDrawArraysIndirectBindlessNV( GLenum mode, const void *indirect, GLsizei drawCount, GLsizei stride, GLint vertexBufferCount ) { }
static void null_glMultiDrawArraysIndirectCount( GLenum mode, const void *indirect, GLintptr drawcount, GLsizei maxdrawcount, GLsizei stride ) { }
static void null_glMultiDrawArraysIndirectCountARB( GLenum mode, const void *indirect, GLintptr drawcount, GLsizei maxdrawcount, GLsizei stride ) { }
static void null_glMultiDrawElementArrayAPPLE( GLenum mode, const GLint *first, const GLsizei *count, GLsizei primcount ) { }
static void null_glMultiDrawElements( GLenum mode, const GLsizei *count, GLenum type, const void *const*indices, GLsizei drawcount ) { }
static void null_glMultiDrawElementsBaseVertex( GLenum mode, const GLsizei *count, GLenum type, const void *const*indices, GLsizei drawcount, const GLint *basevertex ) { }
static void null_glMultiDrawElementsEXT( GLenum mode, const GLsizei *count, GLenum type, const void *const*indices, GLsizei primcount ) { }
static void null_glMultiDrawElementsIndirect( GLenum mode, GLenum type, const void *indirect, GLsizei drawcount, GLsizei stride ) { }
static void null_glMultiDrawElementsIndirectAMD( GLenum mode, GLenum type, const void *indirect, GLsizei primcount, GLsizei stride ) { }
static void null_glMultiDrawElementsIndirectBindlessCountNV( GLenum mode, GLenum type, const void *indirect, GLsizei drawCount, GLsizei maxDrawCount, GLsizei stride, GLint vertexBufferCount ) { }
static void null_glMultiDrawElementsIndirectBindlessNV( GLenum mode, GLenum type, const void *indirect, GLsizei drawCount, GLsizei stride, GLint vertexBufferCount ) { }
static void null_glMultiDrawElementsIndirectCount( GLenum mode, GLenum type, const void *indirect, GLintptr drawcount, GLsizei maxdrawcount, GLsizei stride ) { }
static void null_glMultiDrawElementsIndirectCountARB( GLenum mode, GLenum type, const void *indirect, GLintptr drawcount, GLsizei maxdrawcount, GLsizei stride ) { }
static void null_glMultiDrawMeshTasksIndirectCountNV( GLintptr indirect, GLintptr drawcount, GLsizei maxdrawcount, GLsizei stride ) { }
static void null_glMultiDrawMeshTasksIndirectNV( GLintptr indirect, GLsizei drawcount, GLsizei stride ) { }
static void null_glMultiDrawRangeElementArrayAPPLE( GLenum mode, GLuint start, GLuint end, const GLint *first, const GLsizei *count, GLsizei primcount ) { }
static void null_glMultiModeDrawArraysIBM( const GLenum *mode, const GLint *first, const GLsizei *count, GLsizei primcount, GLint modestride ) { }
static void null_glMultiModeDrawElementsIBM( const GLenum *mode, const GLsizei *count, GLenum type, const void *const*indices, GLsizei primcount, GLint modestride ) { }
static void null_glMultiTexBufferEXT( GLenum texunit, GLenum target, GLenum internalformat, GLuint buffer ) { }
static void null_glMultiTexCoord1bOES( GLenum texture, GLbyte s ) { }
static void null_glMultiTexCoord1bvOES( GLenum texture, const GLbyte *coords ) { }
static void null_glMultiTexCoord1d( GLenum target, GLdouble s ) { }
static void null_glMultiTexCoord1dARB( GLenum target, GLdouble s ) { }
static void null_glMultiTexCoord1dSGIS( GLenum target, GLdouble s ) { }
static void null_glMultiTexCoord1dv( GLenum target, const GLdouble *v ) { }
static void null_glMultiTexCoord1dvARB( GLenum target, const GLdouble *v ) { }
static void null_glMultiTexCoord1dvSGIS( GLenum target, GLdouble * v ) { }
static void null_glMultiTexCoord1f( GLenum target, GLfloat s ) { }
static void null_glMultiTexCoord1fARB( GLenum target, GLfloat s ) { }
static void null_glMultiTexCoord1fSGIS( GLenum target, GLfloat s ) { }
static void null_glMultiTexCoord1fv( GLenum target, const GLfloat *v ) { }
static void null_glMultiTexCoord1fvARB( GLenum target, const GLfloat *v ) { }
static void null_glMultiTexCoord1fvSGIS( GLenum target, const GLfloat * v ) { }
static void null_glMultiTexCoord1hNV( GLenum target, GLhalfNV s ) { }
static void null_glMultiTexCoord1hvNV( GLenum target, const GLhalfNV *v ) { }
static void null_glMultiTexCoord1i( GLenum target, GLint s ) { }
static void null_glMultiTexCoord1iARB( GLenum target, GLint s ) { }
static void null_glMultiTexCoord1iSGIS( GLenum target, GLint s ) { }
static void null_glMultiTexCoord1iv( GLenum target, const GLint *v ) { }
static void null_glMultiTexCoord1ivARB( GLenum target, const GLint *v ) { }
static void null_glMultiTexCoord1ivSGIS( GLenum target, GLint * v ) { }
static void null_glMultiTexCoord1s( GLenum target, GLshort s ) { }
static void null_glMultiTexCoord1sARB( GLenum target, GLshort s ) { }
static void null_glMultiTexCoord1sSGIS( GLenum target, GLshort s ) { }
static void null_glMultiTexCoord1sv( GLenum target, const GLshort *v ) { }
static void null_glMultiTexCoord1svARB( GLenum target, const GLshort *v ) { }
static void null_glMultiTexCoord1svSGIS( GLenum target, GLshort * v ) { }
static void null_glMultiTexCoord1xOES( GLenum texture, GLfixed s ) { }
static void null_glMultiTexCoord1xvOES( GLenum texture, const GLfixed *coords ) { }
static void null_glMultiTexCoord2bOES( GLenum texture, GLbyte s, GLbyte t ) { }
static void null_glMultiTexCoord2bvOES( GLenum texture, const GLbyte *coords ) { }
static void null_glMultiTexCoord2d( GLenum target, GLdouble s, GLdouble t ) { }
static void null_glMultiTexCoord2dARB( GLenum target, GLdouble s, GLdouble t ) { }
static void null_glMultiTexCoord2dSGIS( GLenum target, GLdouble s, GLdouble t ) { }
static void null_glMultiTexCoord2dv( GLenum target, const GLdouble *v ) { }
static void null_glMultiTexCoord2dvARB( GLenum target, const GLdouble *v ) { }
static void null_glMultiTexCoord2dvSGIS( GLenum target, GLdouble * v ) { }
static void null_glMultiTexCoord2f( GLenum target, GLfloat s, GLfloat t ) { }
static void null_glMultiTexCoord2fARB( GLenum target, GLfloat s, GLfloat t ) { }
static void null_glMultiTexCoord2fSGIS( GLenum target, GLfloat s, GLfloat t ) { }
static void null_glMultiTexCoord2fv( GLenum target, const GLfloat *v ) { }
static void null_glMultiTexCoord2fvARB( GLenum target, const GLfloat *v ) { }
static void null_glMultiTexCoord2fvSGIS( GLenum target, GLfloat * v ) { }
static void null_glMultiTexCoord2hNV( GLenum target, GLhalfNV s, GLhalfNV t ) { }
static void null_glMultiTexCoord2hvNV( GLenum target, const GLhalfNV *v ) { }
static void null_glMultiTexCoord2i( GLenum target, GLint s, GLint t ) { }
static void null_glMultiTexCoord2iARB( GLenum target, GLint s, GLint t ) { }
static void null_glMultiTexCoord2iSGIS( GLenum target, GLint s, GLint t ) { }
static void null_glMultiTexCoord2iv( GLenum target, const GLint *v ) { }
static void null_glMultiTexCoord2ivARB( GLenum target, const GLint *v ) { }
static void null_glMultiTexCoord2ivSGIS( GLenum target, GLint * v ) { }
static void null_glMultiTexCoord2s( GLenum target, GLshort s, GLshort t ) { }
static void null_glMultiTexCoord2sARB( GLenum target, GLshort s, GLshort t ) { }
static void null_glMultiTexCoord2sSGIS( GLenum target, GLshort s, GLshort t ) { }
static void null_glMultiTexCoord2sv( GLenum target, const GLshort *v ) { }
static void null_glMultiTexCoord2svARB( GLenum target, const GLshort *v ) { }
static void null_glMultiTexCoord2svSGIS( GLenum target, GLshort * v ) { }
static void null_glMultiTexCoord2xOES( GLenum texture, GLfixed s, GLfixed t ) { }
static void null_glMultiTexCoord2xvOES( GLenum texture, const GLfixed *coords ) { }
static void null_glMultiTexCoord3bOES( GLenum texture, GLbyte s, GLbyte t, GLbyte r ) { }
static void null_glMultiTexCoord3bvOES( GLenum texture, const GLbyte *coords ) { }
static void null_glMultiTexCoord3d( GLenum target, GLdouble s, GLdouble t, GLdouble r ) { }
static void null_glMultiTexCoord3dARB( GLenum target, GLdouble s, GLdouble t, GLdouble r ) { }
static void null_glMultiTexCoord3dSGIS( GLenum target, GLdouble s, GLdouble t, GLdouble r ) { }
static void null_glMultiTexCoord3dv( GLenum target, const GLdouble *v ) { }
static void null_glMultiTexCoord3dvARB( GLenum target, const GLdouble *v ) { }
static void null_glMultiTexCoord3dvSGIS( GLenum target, GLdouble * v ) { }
static void null_glMultiTexCoord3f( GLenum target, GLfloat s, GLfloat t, GLfloat r ) { }
static void null_glMultiTexCoord3fARB( GLenum target, GLfloat s, GLfloat t, GLfloat r ) { }
static void null_glMultiTexCoord3fSGIS( GLenum target, GLfloat s, GLfloat t, GLfloat r ) { }
static void null_glMultiTexCoord3fv( GLenum target, const GLfloat *v ) { }
static void null_glMultiTexCoord3fvARB( GLenum target, const GLfloat *v ) { }
static void null_glMultiTexCoord3fvSGIS( GLenum target, GLfloat * v ) { }
static void null_glMultiTexCoord3hNV( GLenum target, GLhalfNV s, GLhalfNV t, GLhalfNV r ) { }
static void null_glMultiTexCoord3hvNV( GLenum target, const GLhalfNV *v ) { }
static void null_glMultiTexCoord3i( GLenum target, GLint s, GLint t, GLint r ) { }
static void null_glMultiTexCoord3iARB( GLenum target, GLint s, GLint t, GLint r ) { }
static void null_glMultiTexCoord3iSGIS( GLenum target, GLint s, GLint t, GLint r ) { }
static void null_glMultiTexCoord3iv( GLenum target, const GLint *v ) { }
static void null_glMultiTexCoord3ivARB( GLenum target, const GLint *v ) { }
static void null_glMultiTexCoord3ivSGIS( GLenum target, GLint * v ) { }
static void null_glMultiTexCoord3s( GLenum target, GLshort s, GLshort t, GLshort r ) { }
static void null_glMultiTexCoord3sARB( GLenum target, GLshort s, GLshort t, GLshort r ) { }
static void null_glMultiTexCoord3sSGIS( GLenum target, GLshort s, GLshort t, GLshort r ) { }
static void null_glMultiTexCoord3sv( GLenum target, const GLshort *v ) { }
static void null_glMultiTexCoord3svARB( GLenum target, const GLshort *v ) { }
static void null_glMultiTexCoord3svSGIS( GLenum target, GLshort * v ) { }
static void null_glMultiTexCoord3xOES( GLenum texture, GLfixed s, GLfixed t, GLfixed r ) { }
static void null_glMultiTexCoord3xvOES( GLenum texture, const GLfixed *coords ) { }
static void null_glMultiTexCoord4bOES( GLenum texture, GLbyte s, GLbyte t, GLbyte r, GLbyte q ) { }
static void null_glMultiTexCoord4bvOES( GLenum texture, const GLbyte *coords ) { }
static void null_glMultiTexCoord4d( GLenum target, GLdouble s, GLdouble t, GLdouble r, GLdouble q ) { }
static void null_glMultiTexCoord4dARB( GLenum target, GLdouble s, GLdouble t, GLdouble r, GLdouble q ) { }
static void null_glMultiTexCoord4dSGIS( GLenum target, GLdouble s, GLdouble t, GLdouble r, GLdouble q ) { }
static void null_glMultiTexCoord4dv( GLenum target, const GLdouble *v ) { }
static void null_glMultiTexCoord4dvARB( GLenum target, const GLdouble *v ) { }
static void null_glMultiTexCoord4dvSGIS( GLenum target, GLdouble * v ) { }
static void null_glMultiTexCoord4f( GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q ) { }
static void null_glMultiTexCoord4fARB( GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q ) { }
static void null_glMultiTexCoord4fSGIS( GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q ) { }
static void null_glMultiTexCoord4fv( GLenum target, const GLfloat *v ) { }
static void null_glMultiTexCoord4fvARB( GLenum target, const GLfloat *v ) { }
static void null_glMultiTexCoord4fvSGIS( GLenum target, GLfloat * v ) { }
static void null_glMultiTexCoord4hNV( GLenum target, GLhalfNV s, GLhalfNV t, GLhalfNV r, GLhalfNV q ) { }
static void null_glMultiTexCoord4hvNV( GLenum target, const GLhalfNV *v ) { }
static void null_glMultiTexCoord4i( GLenum target, GLint s, GLint t, GLint r, GLint q ) { }
static void null_glMultiTexCoord4iARB( GLenum target, GLint s, GLint t, GLint r, GLint q ) { }
static void null_glMultiTexCoord4iSGIS( GLenum target, GLint s, GLint t, GLint r, GLint q ) { }
static void null_glMultiTexCoord4iv( GLenum target, const GLint *v ) { }
static void null_glMultiTexCoord4ivARB( GLenum target, const GLint *v ) { }
static void null_glMultiTexCoord4ivSGIS( GLenum target, GLint * v ) { }
static void null_glMultiTexCoord4s( GLenum target, GLshort s, GLshort t, GLshort r, GLshort q ) { }
static void null_glMultiTexCoord4sARB( GLenum target, GLshort s, GLshort t, GLshort r, GLshort q ) { }
static void null_glMultiTexCoord4sSGIS( GLenum target, GLshort s, GLshort t, GLshort r, GLshort q ) { }
static void null_glMultiTexCoord4sv( GLenum target, const GLshort *v ) { }
static void null_glMultiTexCoord4svARB( GLenum target, const GLshort *v ) { }
static void null_glMultiTexCoord4svSGIS( GLenum target, GLshort * v ) { }
static void null_glMultiTexCoord4xOES( GLenum texture, GLfixed s, GLfixed t, GLfixed r, GLfixed q ) { }
static void null_glMultiTexCoord4xvOES( GLenum texture, const GLfixed *coords ) { }
static void null_glMultiTexCoordP1ui( GLenum texture, GLenum type, GLuint coords ) { }
static void null_glMultiTexCoordP1uiv( GLenum texture, GLenum type, const GLuint *coords ) { }
static void null_glMultiTexCoordP2ui( GLenum texture, GLenum type, GLuint coords ) { }
static void null_glMultiTexCoordP2uiv( GLenum texture, GLenum type, const GLuint *coords ) { }
static void null_glMultiTexCoordP3ui( GLenum texture, GLenum type, GLuint coords ) { }
static void null_glMultiTexCoordP3uiv( GLenum texture, GLenum type, const GLuint *coords ) { }
static void null_glMultiTexCoordP4ui( GLenum texture, GLenum type, GLuint coords ) { }
static void null_glMultiTexCoordP4uiv( GLenum texture, GLenum type, const GLuint *coords ) { }
static void null_glMultiTexCoordPointerEXT( GLenum texunit, GLint size, GLenum type, GLsizei stride, const void *pointer ) { }
static void null_glMultiTexCoordPointerSGIS( GLenum target, GLint size, GLenum type, GLsizei stride, GLvoid * pointer ) { }
static void null_glMultiTexEnvfEXT( GLenum texunit, GLenum target, GLenum pname, GLfloat param ) { }
static void null_glMultiTexEnvfvEXT( GLenum texunit, GLenum target, GLenum pname, const GLfloat *params ) { }
static void null_glMultiTexEnviEXT( GLenum texunit, GLenum target, GLenum pname, GLint param ) { }
static void null_glMultiTexEnvivEXT( GLenum texunit, GLenum target, GLenum pname, const GLint *params ) { }
static void null_glMultiTexGendEXT( GLenum texunit, GLenum coord, GLenum pname, GLdouble param ) { }
static void null_glMultiTexGendvEXT( GLenum texunit, GLenum coord, GLenum pname, const GLdouble *params ) { }
static void null_glMultiTexGenfEXT( GLenum texunit, GLenum coord, GLenum pname, GLfloat param ) { }
static void null_glMultiTexGenfvEXT( GLenum texunit, GLenum coord, GLenum pname, const GLfloat *params ) { }
static void null_glMultiTexGeniEXT( GLenum texunit, GLenum coord, GLenum pname, GLint param ) { }
static void null_glMultiTexGenivEXT( GLenum texunit, GLenum coord, GLenum pname, const GLint *params ) { }
static void null_glMultiTexImage1DEXT( GLenum texunit, GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const void *pixels ) { }
static void null_glMultiTexImage2DEXT( GLenum texunit, GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void *pixels ) { }
static void null_glMultiTexImage3DEXT( GLenum texunit, GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void *pixels ) { }
static void null_glMultiTexParameterIivEXT( GLenum texunit, GLenum target, GLenum pname, const GLint *params ) { }
static void null_glMultiTexParameterIuivEXT( GLenum texunit, GLenum target, GLenum pname, const GLuint *params ) { }
static void null_glMultiTexParameterfEXT( GLenum texunit, GLenum target, GLenum pname, GLfloat param ) { }
static void null_glMultiTexParameterfvEXT( GLenum texunit, GLenum target, GLenum pname, const GLfloat *params ) { }
static void null_glMultiTexParameteriEXT( GLenum texunit, GLenum target, GLenum pname, GLint param ) { }
static void null_glMultiTexParameterivEXT( GLenum texunit, GLenum target, GLenum pname, const GLint *params ) { }
static void null_glMultiTexRenderbufferEXT( GLenum texunit, GLenum target, GLuint renderbuffer ) { }
static void null_glMultiTexSubImage1DEXT( GLenum texunit, GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const void *pixels ) { }
static void null_glMultiTexSubImage2DEXT( GLenum texunit, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels ) { }
static void null_glMultiTexSubImage3DEXT( GLenum texunit, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void *pixels ) { }
static void null_glMulticastBarrierNV(void) { }
static void null_glMulticastBlitFramebufferNV( GLuint srcGpu, GLuint dstGpu, GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter ) { }
static void null_glMulticastBufferSubDataNV( GLbitfield gpuMask, GLuint buffer, GLintptr offset, GLsizeiptr size, const void *data ) { }
static void null_glMulticastCopyBufferSubDataNV( GLuint readGpu, GLbitfield writeGpuMask, GLuint readBuffer, GLuint writeBuffer, GLintptr readOffset, GLintptr writeOffset, GLsizeiptr size ) { }
static void null_glMulticastCopyImageSubDataNV( GLuint srcGpu, GLbitfield dstGpuMask, GLuint srcName, GLenum srcTarget, GLint srcLevel, GLint srcX, GLint srcY, GLint srcZ, GLuint dstName, GLenum dstTarget, GLint dstLevel, GLint dstX, GLint dstY, GLint dstZ, GLsizei srcWidth, GLsizei srcHeight, GLsizei srcDepth ) { }
static void null_glMulticastFramebufferSampleLocationsfvNV( GLuint gpu, GLuint framebuffer, GLuint start, GLsizei count, const GLfloat *v ) { }
static void null_glMulticastGetQueryObjecti64vNV( GLuint gpu, GLuint id, GLenum pname, GLint64 *params ) { }
static void null_glMulticastGetQueryObjectivNV( GLuint gpu, GLuint id, GLenum pname, GLint *params ) { }
static void null_glMulticastGetQueryObjectui64vNV( GLuint gpu, GLuint id, GLenum pname, GLuint64 *params ) { }
static void null_glMulticastGetQueryObjectuivNV( GLuint gpu, GLuint id, GLenum pname, GLuint *params ) { }
static void null_glMulticastScissorArrayvNVX( GLuint gpu, GLuint first, GLsizei count, const GLint *v ) { }
static void null_glMulticastViewportArrayvNVX( GLuint gpu, GLuint first, GLsizei count, const GLfloat *v ) { }
static void null_glMulticastViewportPositionWScaleNVX( GLuint gpu, GLuint index, GLfloat xcoeff, GLfloat ycoeff ) { }
static void null_glMulticastWaitSyncNV( GLuint signalGpu, GLbitfield waitGpuMask ) { }
static void null_glNamedBufferAttachMemoryNV( GLuint buffer, GLuint memory, GLuint64 offset ) { }
static void null_glNamedBufferData( GLuint buffer, GLsizeiptr size, const void *data, GLenum usage ) { }
static void null_glNamedBufferDataEXT( GLuint buffer, GLsizeiptr size, const void *data, GLenum usage ) { }
static void null_glNamedBufferPageCommitmentARB( GLuint buffer, GLintptr offset, GLsizeiptr size, GLboolean commit ) { }
static void null_glNamedBufferPageCommitmentEXT( GLuint buffer, GLintptr offset, GLsizeiptr size, GLboolean commit ) { }
static void null_glNamedBufferStorage( GLuint buffer, GLsizeiptr size, const void *data, GLbitfield flags ) { }
static void null_glNamedBufferStorageEXT( GLuint buffer, GLsizeiptr size, const void *data, GLbitfield flags ) { }
static void null_glNamedBufferStorageExternalEXT( GLuint buffer, GLintptr offset, GLsizeiptr size, GLeglClientBufferEXT clientBuffer, GLbitfield flags ) { }
static void null_glNamedBufferStorageMemEXT( GLuint buffer, GLsizeiptr size, GLuint memory, GLuint64 offset ) { }
static void null_glNamedBufferSubData( GLuint buffer, GLintptr offset, GLsizeiptr size, const void *data ) { }
static void null_glNamedBufferSubDataEXT( GLuint buffer, GLintptr offset, GLsizeiptr size, const void *data ) { }
static void null_glNamedCopyBufferSubDataEXT( GLuint readBuffer, GLuint writeBuffer, GLintptr readOffset, GLintptr writeOffset, GLsizeiptr size ) { }
static void null_glNamedFramebufferDrawBuffer( GLuint framebuffer, GLenum buf ) { }
static void null_glNamedFramebufferDrawBuffers( GLuint framebuffer, GLsizei n, const GLenum *bufs ) { }
static void null_glNamedFramebufferParameteri( GLuint framebuffer, GLenum pname, GLint param ) { }
static void null_glNamedFramebufferParameteriEXT( GLuint framebuffer, GLenum pname, GLint param ) { }
static void null_glNamedFramebufferReadBuffer( GLuint framebuffer, GLenum src ) { }
static void null_glNamedFramebufferRenderbuffer( GLuint framebuffer, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer ) { }
static void null_glNamedFramebufferRenderbufferEXT( GLuint framebuffer, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer ) { }
static void null_glNamedFramebufferSampleLocationsfvARB( GLuint framebuffer, GLuint start, GLsizei count, const GLfloat *v ) { }
static void null_glNamedFramebufferSampleLocationsfvNV( GLuint framebuffer, GLuint start, GLsizei count, const GLfloat *v ) { }
static void null_glNamedFramebufferSamplePositionsfvAMD( GLuint framebuffer, GLuint numsamples, GLuint pixelindex, const GLfloat *values ) { }
static void null_glNamedFramebufferTexture( GLuint framebuffer, GLenum attachment, GLuint texture, GLint level ) { }
static void null_glNamedFramebufferTexture1DEXT( GLuint framebuffer, GLenum attachment, GLenum textarget, GLuint texture, GLint level ) { }
static void null_glNamedFramebufferTexture2DEXT( GLuint framebuffer, GLenum attachment, GLenum textarget, GLuint texture, GLint level ) { }
static void null_glNamedFramebufferTexture3DEXT( GLuint framebuffer, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint zoffset ) { }
static void null_glNamedFramebufferTextureEXT( GLuint framebuffer, GLenum attachment, GLuint texture, GLint level ) { }
static void null_glNamedFramebufferTextureFaceEXT( GLuint framebuffer, GLenum attachment, GLuint texture, GLint level, GLenum face ) { }
static void null_glNamedFramebufferTextureLayer( GLuint framebuffer, GLenum attachment, GLuint texture, GLint level, GLint layer ) { }
static void null_glNamedFramebufferTextureLayerEXT( GLuint framebuffer, GLenum attachment, GLuint texture, GLint level, GLint layer ) { }
static void null_glNamedProgramLocalParameter4dEXT( GLuint program, GLenum target, GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w ) { }
static void null_glNamedProgramLocalParameter4dvEXT( GLuint program, GLenum target, GLuint index, const GLdouble *params ) { }
static void null_glNamedProgramLocalParameter4fEXT( GLuint program, GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w ) { }
static void null_glNamedProgramLocalParameter4fvEXT( GLuint program, GLenum target, GLuint index, const GLfloat *params ) { }
static void null_glNamedProgramLocalParameterI4iEXT( GLuint program, GLenum target, GLuint index, GLint x, GLint y, GLint z, GLint w ) { }
static void null_glNamedProgramLocalParameterI4ivEXT( GLuint program, GLenum target, GLuint index, const GLint *params ) { }
static void null_glNamedProgramLocalParameterI4uiEXT( GLuint program, GLenum target, GLuint index, GLuint x, GLuint y, GLuint z, GLuint w ) { }
static void null_glNamedProgramLocalParameterI4uivEXT( GLuint program, GLenum target, GLuint index, const GLuint *params ) { }
static void null_glNamedProgramLocalParameters4fvEXT( GLuint program, GLenum target, GLuint index, GLsizei count, const GLfloat *params ) { }
static void null_glNamedProgramLocalParametersI4ivEXT( GLuint program, GLenum target, GLuint index, GLsizei count, const GLint *params ) { }
static void null_glNamedProgramLocalParametersI4uivEXT( GLuint program, GLenum target, GLuint index, GLsizei count, const GLuint *params ) { }
static void null_glNamedProgramStringEXT( GLuint program, GLenum target, GLenum format, GLsizei len, const void *string ) { }
static void null_glNamedRenderbufferStorage( GLuint renderbuffer, GLenum internalformat, GLsizei width, GLsizei height ) { }
static void null_glNamedRenderbufferStorageEXT( GLuint renderbuffer, GLenum internalformat, GLsizei width, GLsizei height ) { }
static void null_glNamedRenderbufferStorageMultisample( GLuint renderbuffer, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height ) { }
static void null_glNamedRenderbufferStorageMultisampleAdvancedAMD( GLuint renderbuffer, GLsizei samples, GLsizei storageSamples, GLenum internalformat, GLsizei width, GLsizei height ) { }
static void null_glNamedRenderbufferStorageMultisampleCoverageEXT( GLuint renderbuffer, GLsizei coverageSamples, GLsizei colorSamples, GLenum internalformat, GLsizei width, GLsizei height ) { }
static void null_glNamedRenderbufferStorageMultisampleEXT( GLuint renderbuffer, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height ) { }
static void null_glNamedStringARB( GLenum type, GLint namelen, const GLchar *name, GLint stringlen, const GLchar *string ) { }
static GLuint null_glNewBufferRegion( GLenum type ) { return 0; }
static GLuint null_glNewObjectBufferATI( GLsizei size, const void *pointer, GLenum usage ) { return 0; }
static void null_glNormal3fVertex3fSUN( GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z ) { }
static void null_glNormal3fVertex3fvSUN( const GLfloat *n, const GLfloat *v ) { }
static void null_glNormal3hNV( GLhalfNV nx, GLhalfNV ny, GLhalfNV nz ) { }
static void null_glNormal3hvNV( const GLhalfNV *v ) { }
static void null_glNormal3xOES( GLfixed nx, GLfixed ny, GLfixed nz ) { }
static void null_glNormal3xvOES( const GLfixed *coords ) { }
static void null_glNormalFormatNV( GLenum type, GLsizei stride ) { }
static void null_glNormalP3ui( GLenum type, GLuint coords ) { }
static void null_glNormalP3uiv( GLenum type, const GLuint *coords ) { }
static void null_glNormalPointerEXT( GLenum type, GLsizei stride, GLsizei count, const void *pointer ) { }
static void null_glNormalPointerListIBM( GLenum type, GLint stride, const void **pointer, GLint ptrstride ) { }
static void null_glNormalPointervINTEL( GLenum type, const void **pointer ) { }
static void null_glNormalStream3bATI( GLenum stream, GLbyte nx, GLbyte ny, GLbyte nz ) { }
static void null_glNormalStream3bvATI( GLenum stream, const GLbyte *coords ) { }
static void null_glNormalStream3dATI( GLenum stream, GLdouble nx, GLdouble ny, GLdouble nz ) { }
static void null_glNormalStream3dvATI( GLenum stream, const GLdouble *coords ) { }
static void null_glNormalStream3fATI( GLenum stream, GLfloat nx, GLfloat ny, GLfloat nz ) { }
static void null_glNormalStream3fvATI( GLenum stream, const GLfloat *coords ) { }
static void null_glNormalStream3iATI( GLenum stream, GLint nx, GLint ny, GLint nz ) { }
static void null_glNormalStream3ivATI( GLenum stream, const GLint *coords ) { }
static void null_glNormalStream3sATI( GLenum stream, GLshort nx, GLshort ny, GLshort nz ) { }
static void null_glNormalStream3svATI( GLenum stream, const GLshort *coords ) { }
static void null_glObjectLabel( GLenum identifier, GLuint name, GLsizei length, const GLchar *label ) { }
static void null_glObjectPtrLabel( const void *ptr, GLsizei length, const GLchar *label ) { }
static GLenum null_glObjectPurgeableAPPLE( GLenum objectType, GLuint name, GLenum option ) { return 0; }
static GLenum null_glObjectUnpurgeableAPPLE( GLenum objectType, GLuint name, GLenum option ) { return 0; }
static void null_glOrthofOES( GLfloat l, GLfloat r, GLfloat b, GLfloat t, GLfloat n, GLfloat f ) { }
static void null_glOrthoxOES( GLfixed l, GLfixed r, GLfixed b, GLfixed t, GLfixed n, GLfixed f ) { }
static void null_glPNTrianglesfATI( GLenum pname, GLfloat param ) { }
static void null_glPNTrianglesiATI( GLenum pname, GLint param ) { }
static void null_glPassTexCoordATI( GLuint dst, GLuint coord, GLenum swizzle ) { }
static void null_glPassThroughxOES( GLfixed token ) { }
static void null_glPatchParameterfv( GLenum pname, const GLfloat *values ) { }
static void null_glPatchParameteri( GLenum pname, GLint value ) { }
static void null_glPathColorGenNV( GLenum color, GLenum genMode, GLenum colorFormat, const GLfloat *coeffs ) { }
static void null_glPathCommandsNV( GLuint path, GLsizei numCommands, const GLubyte *commands, GLsizei numCoords, GLenum coordType, const void *coords ) { }
static void null_glPathCoordsNV( GLuint path, GLsizei numCoords, GLenum coordType, const void *coords ) { }
static void null_glPathCoverDepthFuncNV( GLenum func ) { }
static void null_glPathDashArrayNV( GLuint path, GLsizei dashCount, const GLfloat *dashArray ) { }
static void null_glPathFogGenNV( GLenum genMode ) { }
static GLenum null_glPathGlyphIndexArrayNV( GLuint firstPathName, GLenum fontTarget, const void *fontName, GLbitfield fontStyle, GLuint firstGlyphIndex, GLsizei numGlyphs, GLuint pathParameterTemplate, GLfloat emScale ) { return 0; }
static GLenum null_glPathGlyphIndexRangeNV( GLenum fontTarget, const void *fontName, GLbitfield fontStyle, GLuint pathParameterTemplate, GLfloat emScale, GLuint baseAndCount[2] ) { return 0; }
static void null_glPathGlyphRangeNV( GLuint firstPathName, GLenum fontTarget, const void *fontName, GLbitfield fontStyle, GLuint firstGlyph, GLsizei numGlyphs, GLenum handleMissingGlyphs, GLuint pathParameterTemplate, GLfloat emScale ) { }
static void null_glPathGlyphsNV( GLuint firstPathName, GLenum fontTarget, const void *fontName, GLbitfield fontStyle, GLsizei numGlyphs, GLenum type, const void *charcodes, GLenum handleMissingGlyphs, GLuint pathParameterTemplate, GLfloat emScale ) { }
static GLenum null_glPathMemoryGlyphIndexArrayNV( GLuint firstPathName, GLenum fontTarget, GLsizeiptr fontSize, const void *fontData, GLsizei faceIndex, GLuint firstGlyphIndex, GLsizei numGlyphs, GLuint pathParameterTemplate, GLfloat emScale ) { return 0; }
static void null_glPathParameterfNV( GLuint path, GLenum pname, GLfloat value ) { }
static void null_glPathParameterfvNV( GLuint path, GLenum pname, const GLfloat *value ) { }
static void null_glPathParameteriNV( GLuint path, GLenum pname, GLint value ) { }
static void null_glPathParameterivNV( GLuint path, GLenum pname, const GLint *value ) { }
static void null_glPathStencilDepthOffsetNV( GLfloat factor, GLfloat units ) { }
static void null_glPathStencilFuncNV( GLenum func, GLint ref, GLuint mask ) { }
static void null_glPathStringNV( GLuint path, GLenum format, GLsizei length, const void *pathString ) { }
static void null_glPathSubCommandsNV( GLuint path, GLsizei commandStart, GLsizei commandsToDelete, GLsizei numCommands, const GLubyte *commands, GLsizei numCoords, GLenum coordType, const void *coords ) { }
static void null_glPathSubCoordsNV( GLuint path, GLsizei coordStart, GLsizei numCoords, GLenum coordType, const void *coords ) { }
static void null_glPathTexGenNV( GLenum texCoordSet, GLenum genMode, GLint components, const GLfloat *coeffs ) { }
static void null_glPauseTransformFeedback(void) { }
static void null_glPauseTransformFeedbackNV(void) { }
static void null_glPixelDataRangeNV( GLenum target, GLsizei length, const void *pointer ) { }
static void null_glPixelMapx( GLenum map, GLint size, const GLfixed *values ) { }
static void null_glPixelStorex( GLenum pname, GLfixed param ) { }
static void null_glPixelTexGenParameterfSGIS( GLenum pname, GLfloat param ) { }
static void null_glPixelTexGenParameterfvSGIS( GLenum pname, const GLfloat *params ) { }
static void null_glPixelTexGenParameteriSGIS( GLenum pname, GLint param ) { }
static void null_glPixelTexGenParameterivSGIS( GLenum pname, const GLint *params ) { }
static void null_glPixelTexGenSGIX( GLenum mode ) { }
static void null_glPixelTransferxOES( GLenum pname, GLfixed param ) { }
static void null_glPixelTransformParameterfEXT( GLenum target, GLenum pname, GLfloat param ) { }
static void null_glPixelTransformParameterfvEXT( GLenum target, GLenum pname, const GLfloat *params ) { }
static void null_glPixelTransformParameteriEXT( GLenum target, GLenum pname, GLint param ) { }
static void null_glPixelTransformParameterivEXT( GLenum target, GLenum pname, const GLint *params ) { }
static void null_glPixelZoomxOES( GLfixed xfactor, GLfixed yfactor ) { }
static GLboolean null_glPointAlongPathNV( GLuint path, GLsizei startSegment, GLsizei numSegments, GLfloat distance, GLfloat *x, GLfloat *y, GLfloat *tangentX, GLfloat *tangentY ) { return 0; }
static void null_glPointParameterf( GLenum pname, GLfloat param ) { }
static void null_glPointParameterfARB( GLenum pname, GLfloat param ) { }
static void null_glPointParameterfEXT( GLenum pname, GLfloat param ) { }
static void null_glPointParameterfSGIS( GLenum pname, GLfloat param ) { }
static void null_glPointParameterfv( GLenum pname, const GLfloat *params ) { }
static void null_glPointParameterfvARB( GLenum pname, const GLfloat *params ) { }
static void null_glPointParameterfvEXT( GLenum pname, const GLfloat *params ) { }
static void null_glPointParameterfvSGIS( GLenum pname, const GLfloat *params ) { }
static void null_glPointParameteri( GLenum pname, GLint param ) { }
static void null_glPointParameteriNV( GLenum pname, GLint param ) { }
static void null_glPointParameteriv( GLenum pname, const GLint *params ) { }
static void null_glPointParameterivNV( GLenum pname, const GLint *params ) { }
static void null_glPointParameterxvOES( GLenum pname, const GLfixed *params ) { }
static void null_glPointSizexOES( GLfixed size ) { }
static GLint null_glPollAsyncSGIX( GLuint *markerp ) { return 0; }
static GLint null_glPollInstrumentsSGIX( GLint *marker_p ) { return 0; }
static void null_glPolygonOffsetClamp( GLfloat factor, GLfloat units, GLfloat clamp ) { }
static void null_glPolygonOffsetClampEXT( GLfloat factor, GLfloat units, GLfloat clamp ) { }
static void null_glPolygonOffsetEXT( GLfloat factor, GLfloat bias ) { }
static void null_glPolygonOffsetxOES( GLfixed factor, GLfixed units ) { }
static void null_glPopDebugGroup(void) { }
static void null_glPopGroupMarkerEXT(void) { }
static void null_glPresentFrameDualFillNV( GLuint video_slot, GLuint64EXT minPresentTime, GLuint beginPresentTimeId, GLuint presentDurationId, GLenum type, GLenum target0, GLuint fill0, GLenum target1, GLuint fill1, GLenum target2, GLuint fill2, GLenum target3, GLuint fill3 ) { }
static void null_glPresentFrameKeyedNV( GLuint video_slot, GLuint64EXT minPresentTime, GLuint beginPresentTimeId, GLuint presentDurationId, GLenum type, GLenum target0, GLuint fill0, GLuint key0, GLenum target1, GLuint fill1, GLuint key1 ) { }
static void null_glPrimitiveBoundingBoxARB( GLfloat minX, GLfloat minY, GLfloat minZ, GLfloat minW, GLfloat maxX, GLfloat maxY, GLfloat maxZ, GLfloat maxW ) { }
static void null_glPrimitiveRestartIndex( GLuint index ) { }
static void null_glPrimitiveRestartIndexNV( GLuint index ) { }
static void null_glPrimitiveRestartNV(void) { }
static void null_glPrioritizeTexturesEXT( GLsizei n, const GLuint *textures, const GLclampf *priorities ) { }
static void null_glPrioritizeTexturesxOES( GLsizei n, const GLuint *textures, const GLfixed *priorities ) { }
static void null_glProgramBinary( GLuint program, GLenum binaryFormat, const void *binary, GLsizei length ) { }
static void null_glProgramBufferParametersIivNV( GLenum target, GLuint bindingIndex, GLuint wordIndex, GLsizei count, const GLint *params ) { }
static void null_glProgramBufferParametersIuivNV( GLenum target, GLuint bindingIndex, GLuint wordIndex, GLsizei count, const GLuint *params ) { }
static void null_glProgramBufferParametersfvNV( GLenum target, GLuint bindingIndex, GLuint wordIndex, GLsizei count, const GLfloat *params ) { }
static void null_glProgramEnvParameter4dARB( GLenum target, GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w ) { }
static void null_glProgramEnvParameter4dvARB( GLenum target, GLuint index, const GLdouble *params ) { }
static void null_glProgramEnvParameter4fARB( GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w ) { }
static void null_glProgramEnvParameter4fvARB( GLenum target, GLuint index, const GLfloat *params ) { }
static void null_glProgramEnvParameterI4iNV( GLenum target, GLuint index, GLint x, GLint y, GLint z, GLint w ) { }
static void null_glProgramEnvParameterI4ivNV( GLenum target, GLuint index, const GLint *params ) { }
static void null_glProgramEnvParameterI4uiNV( GLenum target, GLuint index, GLuint x, GLuint y, GLuint z, GLuint w ) { }
static void null_glProgramEnvParameterI4uivNV( GLenum target, GLuint index, const GLuint *params ) { }
static void null_glProgramEnvParameters4fvEXT( GLenum target, GLuint index, GLsizei count, const GLfloat *params ) { }
static void null_glProgramEnvParametersI4ivNV( GLenum target, GLuint index, GLsizei count, const GLint *params ) { }
static void null_glProgramEnvParametersI4uivNV( GLenum target, GLuint index, GLsizei count, const GLuint *params ) { }
static void null_glProgramLocalParameter4dARB( GLenum target, GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w ) { }
static void null_glProgramLocalParameter4dvARB( GLenum target, GLuint index, const GLdouble *params ) { }
static void null_glProgramLocalParameter4fARB( GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w ) { }
static void null_glProgramLocalParameter4fvARB( GLenum target, GLuint index, const GLfloat *params ) { }
static void null_glProgramLocalParameterI4iNV( GLenum target, GLuint index, GLint x, GLint y, GLint z, GLint w ) { }
static void null_glProgramLocalParameterI4ivNV( GLenum target, GLuint index, const GLint *params ) { }
static void null_glProgramLocalParameterI4uiNV( GLenum target, GLuint index, GLuint x, GLuint y, GLuint z, GLuint w ) { }
static void null_glProgramLocalParameterI4uivNV( GLenum target, GLuint index, const GLuint *params ) { }
static void null_glProgramLocalParameters4fvEXT( GLenum target, GLuint index, GLsizei count, const GLfloat *params ) { }
static void null_glProgramLocalParametersI4ivNV( GLenum target, GLuint index, GLsizei count, const GLint *params ) { }
static void null_glProgramLocalParametersI4uivNV( GLenum target, GLuint index, GLsizei count, const GLuint *params ) { }
static void null_glProgramNamedParameter4dNV( GLuint id, GLsizei len, const GLubyte *name, GLdouble x, GLdouble y, GLdouble z, GLdouble w ) { }
static void null_glProgramNamedParameter4dvNV( GLuint id, GLsizei len, const GLubyte *name, const GLdouble *v ) { }
static void null_glProgramNamedParameter4fNV( GLuint id, GLsizei len, const GLubyte *name, GLfloat x, GLfloat y, GLfloat z, GLfloat w ) { }
static void null_glProgramNamedParameter4fvNV( GLuint id, GLsizei len, const GLubyte *name, const GLfloat *v ) { }
static void null_glProgramParameter4dNV( GLenum target, GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w ) { }
static void null_glProgramParameter4dvNV( GLenum target, GLuint index, const GLdouble *v ) { }
static void null_glProgramParameter4fNV( GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w ) { }
static void null_glProgramParameter4fvNV( GLenum target, GLuint index, const GLfloat *v ) { }
static void null_glProgramParameteri( GLuint program, GLenum pname, GLint value ) { }
static void null_glProgramParameteriARB( GLuint program, GLenum pname, GLint value ) { }
static void null_glProgramParameteriEXT( GLuint program, GLenum pname, GLint value ) { }
static void null_glProgramParameters4dvNV( GLenum target, GLuint index, GLsizei count, const GLdouble *v ) { }
static void null_glProgramParameters4fvNV( GLenum target, GLuint index, GLsizei count, const GLfloat *v ) { }
static void null_glProgramPathFragmentInputGenNV( GLuint program, GLint location, GLenum genMode, GLint components, const GLfloat *coeffs ) { }
static void null_glProgramStringARB( GLenum target, GLenum format, GLsizei len, const void *string ) { }
static void null_glProgramSubroutineParametersuivNV( GLenum target, GLsizei count, const GLuint *params ) { }
static void null_glProgramUniform1d( GLuint program, GLint location, GLdouble v0 ) { }
static void null_glProgramUniform1dEXT( GLuint program, GLint location, GLdouble x ) { }
static void null_glProgramUniform1dv( GLuint program, GLint location, GLsizei count, const GLdouble *value ) { }
static void null_glProgramUniform1dvEXT( GLuint program, GLint location, GLsizei count, const GLdouble *value ) { }
static void null_glProgramUniform1f( GLuint program, GLint location, GLfloat v0 ) { }
static void null_glProgramUniform1fEXT( GLuint program, GLint location, GLfloat v0 ) { }
static void null_glProgramUniform1fv( GLuint program, GLint location, GLsizei count, const GLfloat *value ) { }
static void null_glProgramUniform1fvEXT( GLuint program, GLint location, GLsizei count, const GLfloat *value ) { }
static void null_glProgramUniform1i( GLuint program, GLint location, GLint v0 ) { }
static void null_glProgramUniform1i64ARB( GLuint program, GLint location, GLint64 x ) { }
static void null_glProgramUniform1i64NV( GLuint program, GLint location, GLint64EXT x ) { }
static void null_glProgramUniform1i64vARB( GLuint program, GLint location, GLsizei count, const GLint64 *value ) { }
static void null_glProgramUniform1i64vNV( GLuint program, GLint location, GLsizei count, const GLint64EXT *value ) { }
static void null_glProgramUniform1iEXT( GLuint program, GLint location, GLint v0 ) { }
static void null_glProgramUniform1iv( GLuint program, GLint location, GLsizei count, const GLint *value ) { }
static void null_glProgramUniform1ivEXT( GLuint program, GLint location, GLsizei count, const GLint *value ) { }
static void null_glProgramUniform1ui( GLuint program, GLint location, GLuint v0 ) { }
static void null_glProgramUniform1ui64ARB( GLuint program, GLint location, GLuint64 x ) { }
static void null_glProgramUniform1ui64NV( GLuint program, GLint location, GLuint64EXT x ) { }
static void null_glProgramUniform1ui64vARB( GLuint program, GLint location, GLsizei count, const GLuint64 *value ) { }
static void null_glProgramUniform1ui64vNV( GLuint program, GLint location, GLsizei count, const GLuint64EXT *value ) { }
static void null_glProgramUniform1uiEXT( GLuint program, GLint location, GLuint v0 ) { }
static void null_glProgramUniform1uiv( GLuint program, GLint location, GLsizei count, const GLuint *value ) { }
static void null_glProgramUniform1uivEXT( GLuint program, GLint location, GLsizei count, const GLuint *value ) { }
static void null_glProgramUniform2d( GLuint program, GLint location, GLdouble v0, GLdouble v1 ) { }
static void null_glProgramUniform2dEXT( GLuint program, GLint location, GLdouble x, GLdouble y ) { }
static void null_glProgramUniform2dv( GLuint program, GLint location, GLsizei count, const GLdouble *value ) { }
static void null_glProgramUniform2dvEXT( GLuint program, GLint location, GLsizei count, const GLdouble *value ) { }
static void null_glProgramUniform2f( GLuint program, GLint location, GLfloat v0, GLfloat v1 ) { }
static void null_glProgramUniform2fEXT( GLuint program, GLint location, GLfloat v0, GLfloat v1 ) { }
static void null_glProgramUniform2fv( GLuint program, GLint location, GLsizei count, const GLfloat *value ) { }
static void null_glProgramUniform2fvEXT( GLuint program, GLint location, GLsizei count, const GLfloat *value ) { }
static void null_glProgramUniform2i( GLuint program, GLint location, GLint v0, GLint v1 ) { }
static void null_glProgramUniform2i64ARB( GLuint program, GLint location, GLint64 x, GLint64 y ) { }
static void null_glProgramUniform2i64NV( GLuint program, GLint location, GLint64EXT x, GLint64EXT y ) { }
static void null_glProgramUniform2i64vARB( GLuint program, GLint location, GLsizei count, const GLint64 *value ) { }
static void null_glProgramUniform2i64vNV( GLuint program, GLint location, GLsizei count, const GLint64EXT *value ) { }
static void null_glProgramUniform2iEXT( GLuint program, GLint location, GLint v0, GLint v1 ) { }
static void null_glProgramUniform2iv( GLuint program, GLint location, GLsizei count, const GLint *value ) { }
static void null_glProgramUniform2ivEXT( GLuint program, GLint location, GLsizei count, const GLint *value ) { }
static void null_glProgramUniform2ui( GLuint program, GLint location, GLuint v0, GLuint v1 ) { }
static void null_glProgramUniform2ui64ARB( GLuint program, GLint location, GLuint64 x, GLuint64 y ) { }
static void null_glProgramUniform2ui64NV( GLuint program, GLint location, GLuint64EXT x, GLuint64EXT y ) { }
static void null_glProgramUniform2ui64vARB( GLuint program, GLint location, GLsizei count, const GLuint64 *value ) { }
static void null_glProgramUniform2ui64vNV( GLuint program, GLint location, GLsizei count, const GLuint64EXT *value ) { }
static void null_glProgramUniform2uiEXT( GLuint program, GLint location, GLuint v0, GLuint v1 ) { }
static void null_glProgramUniform2uiv( GLuint program, GLint location, GLsizei count, const GLuint *value ) { }
static void null_glProgramUniform2uivEXT( GLuint program, GLint location, GLsizei count, const GLuint *value ) { }
static void null_glProgramUniform3d( GLuint program, GLint location, GLdouble v0, GLdouble v1, GLdouble v2 ) { }
static void null_glProgramUniform3dEXT( GLuint program, GLint location, GLdouble x, GLdouble y, GLdouble z ) { }
static void null_glProgramUniform3dv( GLuint program, GLint location, GLsizei count, const GLdouble *value ) { }
static void null_glProgramUniform3dvEXT( GLuint program, GLint location, GLsizei count, const GLdouble *value ) { }
static void null_glProgramUniform3f( GLuint program, GLint location, GLfloat v0, GLfloat v1, GLfloat v2 ) { }
static void null_glProgramUniform3fEXT( GLuint program, GLint location, GLfloat v0, GLfloat v1, GLfloat v2 ) { }
static void null_glProgramUniform3fv( GLuint program, GLint location, GLsizei count, const GLfloat *value ) { }
static void null_glProgramUniform3fvEXT( GLuint program, GLint location, GLsizei count, const GLfloat *value ) { }
static void null_glProgramUniform3i( GLuint program, GLint location, GLint v0, GLint v1, GLint v2 ) { }
static void null_glProgramUniform3i64ARB( GLuint program, GLint location, GLint64 x, GLint64 y, GLint64 z ) { }
static void null_glProgramUniform3i64NV( GLuint program, GLint location, GLint64EXT x, GLint64EXT y, GLint64EXT z ) { }
static void null_glProgramUniform3i64vARB( GLuint program, GLint location, GLsizei count, const GLint64 *value ) { }
static void null_glProgramUniform3i64vNV( GLuint program, GLint location, GLsizei count, const GLint64EXT *value ) { }
static void null_glProgramUniform3iEXT( GLuint program, GLint location, GLint v0, GLint v1, GLint v2 ) { }
static void null_glProgramUniform3iv( GLuint program, GLint location, GLsizei count, const GLint *value ) { }
static void null_glProgramUniform3ivEXT( GLuint program, GLint location, GLsizei count, const GLint *value ) { }
static void null_glProgramUniform3ui( GLuint program, GLint location, GLuint v0, GLuint v1, GLuint v2 ) { }
static void null_glProgramUniform3ui64ARB( GLuint program, GLint location, GLuint64 x, GLuint64 y, GLuint64 z ) { }
static void null_glProgramUniform3ui64NV( GLuint program, GLint location, GLuint64EXT x, GLuint64EXT y, GLuint64EXT z ) { }
static void null_glProgramUniform3ui64vARB( GLuint program, GLint location, GLsizei count, const GLuint64 *value ) { }
static void null_glProgramUniform3ui64vNV( GLuint program, GLint location, GLsizei count, const GLuint64EXT *value ) { }
static void null_glProgramUniform3uiEXT( GLuint program, GLint location, GLuint v0, GLuint v1, GLuint v2 ) { }
static void null_glProgramUniform3uiv( GLuint program, GLint location, GLsizei count, const GLuint *value ) { }
static void null_glProgramUniform3uivEXT( GLuint program, GLint location, GLsizei count, const GLuint *value ) { }
static void null_glProgramUniform4d( GLuint program, GLint location, GLdouble v0, GLdouble v1, GLdouble v2, GLdouble v3 ) { }
static void null_glProgramUniform4dEXT( GLuint program, GLint location, GLdouble x, GLdouble y, GLdouble z, GLdouble w ) { }
static void null_glProgramUniform4dv( GLuint program, GLint location, GLsizei count, const GLdouble *value ) { }
static void null_glProgramUniform4dvEXT( GLuint program, GLint location, GLsizei count, const GLdouble *value ) { }
static void null_glProgramUniform4f( GLuint program, GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3 ) { }
static void null_glProgramUniform4fEXT( GLuint program, GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3 ) { }
static void null_glProgramUniform4fv( GLuint program, GLint location, GLsizei count, const GLfloat *value ) { }
static void null_glProgramUniform4fvEXT( GLuint program, GLint location, GLsizei count, const GLfloat *value ) { }
static void null_glProgramUniform4i( GLuint program, GLint location, GLint v0, GLint v1, GLint v2, GLint v3 ) { }
static void null_glProgramUniform4i64ARB( GLuint program, GLint location, GLint64 x, GLint64 y, GLint64 z, GLint64 w ) { }
static void null_glProgramUniform4i64NV( GLuint program, GLint location, GLint64EXT x, GLint64EXT y, GLint64EXT z, GLint64EXT w ) { }
static void null_glProgramUniform4i64vARB( GLuint program, GLint location, GLsizei count, const GLint64 *value ) { }
static void null_glProgramUniform4i64vNV( GLuint program, GLint location, GLsizei count, const GLint64EXT *value ) { }
static void null_glProgramUniform4iEXT( GLuint program, GLint location, GLint v0, GLint v1, GLint v2, GLint v3 ) { }
static void null_glProgramUniform4iv( GLuint program, GLint location, GLsizei count, const GLint *value ) { }
static void null_glProgramUniform4ivEXT( GLuint program, GLint location, GLsizei count, const GLint *value ) { }
static void null_glProgramUniform4ui( GLuint program, GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3 ) { }
static void null_glProgramUniform4ui64ARB( GLuint program, GLint location, GLuint64 x, GLuint64 y, GLuint64 z, GLuint64 w ) { }
static void null_glProgramUniform4ui64NV( GLuint program, GLint location, GLuint64EXT x, GLuint64EXT y, GLuint64EXT z, GLuint64EXT w ) { }
static void null_glProgramUniform4ui64vARB( GLuint program, GLint location, GLsizei count, const GLuint64 *value ) { }
static void null_glProgramUniform4ui64vNV( GLuint program, GLint location, GLsizei count, const GLuint64EXT *value ) { }
static void null_glProgramUniform4uiEXT( GLuint program, GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3 ) { }
static void null_glProgramUniform4uiv( GLuint program, GLint location, GLsizei count, const GLuint *value ) { }
static void null_glProgramUniform4uivEXT( GLuint program, GLint location, GLsizei count, const GLuint *value ) { }
static void null_glProgramUniformHandleui64ARB( GLuint program, GLint location, GLuint64 value ) { }
static void null_glProgramUniformHandleui64NV( GLuint program, GLint location, GLuint64 value ) { }
static void null_glProgramUniformHandleui64vARB( GLuint program, GLint location, GLsizei count, const GLuint64 *values ) { }
static void null_glProgramUniformHandleui64vNV( GLuint program, GLint location, GLsizei count, const GLuint64 *values ) { }
static void null_glProgramUniformMatrix2dv( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value ) { }
static void null_glProgramUniformMatrix2dvEXT( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value ) { }
static void null_glProgramUniformMatrix2fv( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value ) { }
static void null_glProgramUniformMatrix2fvEXT( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value ) { }
static void null_glProgramUniformMatrix2x3dv( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value ) { }
static void null_glProgramUniformMatrix2x3dvEXT( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value ) { }
static void null_glProgramUniformMatrix2x3fv( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value ) { }
static void null_glProgramUniformMatrix2x3fvEXT( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value ) { }
static void null_glProgramUniformMatrix2x4dv( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value ) { }
static void null_glProgramUniformMatrix2x4dvEXT( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value ) { }
static void null_glProgramUniformMatrix2x4fv( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value ) { }
static void null_glProgramUniformMatrix2x4fvEXT( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value ) { }
static void null_glProgramUniformMatrix3dv( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value ) { }
static void null_glProgramUniformMatrix3dvEXT( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value ) { }
static void null_glProgramUniformMatrix3fv( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value ) { }
static void null_glProgramUniformMatrix3fvEXT( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value ) { }
static void null_glProgramUniformMatrix3x2dv( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value ) { }
static void null_glProgramUniformMatrix3x2dvEXT( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value ) { }
static void null_glProgramUniformMatrix3x2fv( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value ) { }
static void null_glProgramUniformMatrix3x2fvEXT( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value ) { }
static void null_glProgramUniformMatrix3x4dv( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value ) { }
static void null_glProgramUniformMatrix3x4dvEXT( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value ) { }
static void null_glProgramUniformMatrix3x4fv( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value ) { }
static void null_glProgramUniformMatrix3x4fvEXT( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value ) { }
static void null_glProgramUniformMatrix4dv( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value ) { }
static void null_glProgramUniformMatrix4dvEXT( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value ) { }
static void null_glProgramUniformMatrix4fv( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value ) { }
static void null_glProgramUniformMatrix4fvEXT( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value ) { }
static void null_glProgramUniformMatrix4x2dv( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value ) { }
static void null_glProgramUniformMatrix4x2dvEXT( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value ) { }
static void null_glProgramUniformMatrix4x2fv( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value ) { }
static void null_glProgramUniformMatrix4x2fvEXT( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value ) { }
static void null_glProgramUniformMatrix4x3dv( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value ) { }
static void null_glProgramUniformMatrix4x3dvEXT( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value ) { }
static void null_glProgramUniformMatrix4x3fv( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value ) { }
static void null_glProgramUniformMatrix4x3fvEXT( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value ) { }
static void null_glProgramUniformui64NV( GLuint program, GLint location, GLuint64EXT value ) { }
static void null_glProgramUniformui64vNV( GLuint program, GLint location, GLsizei count, const GLuint64EXT *value ) { }
static void null_glProgramVertexLimitNV( GLenum target, GLint limit ) { }
static void null_glProvokingVertex( GLenum mode ) { }
static void null_glProvokingVertexEXT( GLenum mode ) { }
static void null_glPushClientAttribDefaultEXT( GLbitfield mask ) { }
static void null_glPushDebugGroup( GLenum source, GLuint id, GLsizei length, const GLchar *message ) { }
static void null_glPushGroupMarkerEXT( GLsizei length, const GLchar *marker ) { }
static void null_glQueryCounter( GLuint id, GLenum target ) { }
static GLbitfield null_glQueryMatrixxOES( GLfixed *mantissa, GLint *exponent ) { return 0; }
static void null_glQueryObjectParameteruiAMD( GLenum target, GLuint id, GLenum pname, GLuint param ) { }
static GLint null_glQueryResourceNV( GLenum queryType, GLint tagId, GLuint count, GLint *buffer ) { return 0; }
static void null_glQueryResourceTagNV( GLint tagId, const GLchar *tagString ) { }
static void null_glRasterPos2xOES( GLfixed x, GLfixed y ) { }
static void null_glRasterPos2xvOES( const GLfixed *coords ) { }
static void null_glRasterPos3xOES( GLfixed x, GLfixed y, GLfixed z ) { }
static void null_glRasterPos3xvOES( const GLfixed *coords ) { }
static void null_glRasterPos4xOES( GLfixed x, GLfixed y, GLfixed z, GLfixed w ) { }
static void null_glRasterPos4xvOES( const GLfixed *coords ) { }
static void null_glRasterSamplesEXT( GLuint samples, GLboolean fixedsamplelocations ) { }
static void null_glReadBufferRegion( GLenum region, GLint x, GLint y, GLsizei width, GLsizei height ) { }
static void null_glReadInstrumentsSGIX( GLint marker ) { }
static void null_glReadnPixels( GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLsizei bufSize, void *data ) { }
static void null_glReadnPixelsARB( GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLsizei bufSize, void *data ) { }
static void null_glRectxOES( GLfixed x1, GLfixed y1, GLfixed x2, GLfixed y2 ) { }
static void null_glRectxvOES( const GLfixed *v1, const GLfixed *v2 ) { }
static void null_glReferencePlaneSGIX( const GLdouble *equation ) { }
static GLboolean null_glReleaseKeyedMutexWin32EXT( GLuint memory, GLuint64 key ) { return 0; }
static void null_glReleaseShaderCompiler(void) { }
static void null_glRenderGpuMaskNV( GLbitfield mask ) { }
static void null_glRenderbufferStorage( GLenum target, GLenum internalformat, GLsizei width, GLsizei height ) { }
static void null_glRenderbufferStorageEXT( GLenum target, GLenum internalformat, GLsizei width, GLsizei height ) { }
static void null_glRenderbufferStorageMultisample( GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height ) { }
static void null_glRenderbufferStorageMultisampleAdvancedAMD( GLenum target, GLsizei samples, GLsizei storageSamples, GLenum internalformat, GLsizei width, GLsizei height ) { }
static void null_glRenderbufferStorageMultisampleCoverageNV( GLenum target, GLsizei coverageSamples, GLsizei colorSamples, GLenum internalformat, GLsizei width, GLsizei height ) { }
static void null_glRenderbufferStorageMultisampleEXT( GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height ) { }
static void null_glReplacementCodePointerSUN( GLenum type, GLsizei stride, const void **pointer ) { }
static void null_glReplacementCodeubSUN( GLubyte code ) { }
static void null_glReplacementCodeubvSUN( const GLubyte *code ) { }
static void null_glReplacementCodeuiColor3fVertex3fSUN( GLuint rc, GLfloat r, GLfloat g, GLfloat b, GLfloat x, GLfloat y, GLfloat z ) { }
static void null_glReplacementCodeuiColor3fVertex3fvSUN( const GLuint *rc, const GLfloat *c, const GLfloat *v ) { }
static void null_glReplacementCodeuiColor4fNormal3fVertex3fSUN( GLuint rc, GLfloat r, GLfloat g, GLfloat b, GLfloat a, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z ) { }
static void null_glReplacementCodeuiColor4fNormal3fVertex3fvSUN( const GLuint *rc, const GLfloat *c, const GLfloat *n, const GLfloat *v ) { }
static void null_glReplacementCodeuiColor4ubVertex3fSUN( GLuint rc, GLubyte r, GLubyte g, GLubyte b, GLubyte a, GLfloat x, GLfloat y, GLfloat z ) { }
static void null_glReplacementCodeuiColor4ubVertex3fvSUN( const GLuint *rc, const GLubyte *c, const GLfloat *v ) { }
static void null_glReplacementCodeuiNormal3fVertex3fSUN( GLuint rc, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z ) { }
static void null_glReplacementCodeuiNormal3fVertex3fvSUN( const GLuint *rc, const GLfloat *n, const GLfloat *v ) { }
static void null_glReplacementCodeuiSUN( GLuint code ) { }
static void null_glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fSUN( GLuint rc, GLfloat s, GLfloat t, GLfloat r, GLfloat g, GLfloat b, GLfloat a, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z ) { }
static void null_glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fvSUN( const GLuint *rc, const GLfloat *tc, const GLfloat *c, const GLfloat *n, const GLfloat *v ) { }
static void null_glReplacementCodeuiTexCoord2fNormal3fVertex3fSUN( GLuint rc, GLfloat s, GLfloat t, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z ) { }
static void null_glReplacementCodeuiTexCoord2fNormal3fVertex3fvSUN( const GLuint *rc, const GLfloat *tc, const GLfloat *n, const GLfloat *v ) { }
static void null_glReplacementCodeuiTexCoord2fVertex3fSUN( GLuint rc, GLfloat s, GLfloat t, GLfloat x, GLfloat y, GLfloat z ) { }
static void null_glReplacementCodeuiTexCoord2fVertex3fvSUN( const GLuint *rc, const GLfloat *tc, const GLfloat *v ) { }
static void null_glReplacementCodeuiVertex3fSUN( GLuint rc, GLfloat x, GLfloat y, GLfloat z ) { }
static void null_glReplacementCodeuiVertex3fvSUN( const GLuint *rc, const GLfloat *v ) { }
static void null_glReplacementCodeuivSUN( const GLuint *code ) { }
static void null_glReplacementCodeusSUN( GLushort code ) { }
static void null_glReplacementCodeusvSUN( const GLushort *code ) { }
static void null_glRequestResidentProgramsNV( GLsizei n, const GLuint *programs ) { }
static void null_glResetHistogram( GLenum target ) { }
static void null_glResetHistogramEXT( GLenum target ) { }
static void null_glResetMemoryObjectParameterNV( GLuint memory, GLenum pname ) { }
static void null_glResetMinmax( GLenum target ) { }
static void null_glResetMinmaxEXT( GLenum target ) { }
static void null_glResizeBuffersMESA(void) { }
static void null_glResolveDepthValuesNV(void) { }
static void null_glResumeTransformFeedback(void) { }
static void null_glResumeTransformFeedbackNV(void) { }
static void null_glRotatexOES( GLfixed angle, GLfixed x, GLfixed y, GLfixed z ) { }
static void null_glSampleCoverage( GLfloat value, GLboolean invert ) { }
static void null_glSampleCoverageARB( GLfloat value, GLboolean invert ) { }
static void null_glSampleMapATI( GLuint dst, GLuint interp, GLenum swizzle ) { }
static void null_glSampleMaskEXT( GLclampf value, GLboolean invert ) { }
static void null_glSampleMaskIndexedNV( GLuint index, GLbitfield mask ) { }
static void null_glSampleMaskSGIS( GLclampf value, GLboolean invert ) { }
static void null_glSampleMaski( GLuint maskNumber, GLbitfield mask ) { }
static void null_glSamplePatternEXT( GLenum pattern ) { }
static void null_glSamplePatternSGIS( GLenum pattern ) { }
static void null_glSamplerParameterIiv( GLuint sampler, GLenum pname, const GLint *param ) { }
static void null_glSamplerParameterIuiv( GLuint sampler, GLenum pname, const GLuint *param ) { }
static void null_glSamplerParameterf( GLuint sampler, GLenum pname, GLfloat param ) { }
static void null_glSamplerParameterfv( GLuint sampler, GLenum pname, const GLfloat *param ) { }
static void null_glSamplerParameteri( GLuint sampler, GLenum pname, GLint param ) { }
static void null_glSamplerParameteriv( GLuint sampler, GLenum pname, const GLint *param ) { }
static void null_glScalexOES( GLfixed x, GLfixed y, GLfixed z ) { }
static void null_glScissorArrayv( GLuint first, GLsizei count, const GLint *v ) { }
static void null_glScissorExclusiveArrayvNV( GLuint first, GLsizei count, const GLint *v ) { }
static void null_glScissorExclusiveNV( GLint x, GLint y, GLsizei width, GLsizei height ) { }
static void null_glScissorIndexed( GLuint index, GLint left, GLint bottom, GLsizei width, GLsizei height ) { }
static void null_glScissorIndexedv( GLuint index, const GLint *v ) { }
static void null_glSecondaryColor3b( GLbyte red, GLbyte green, GLbyte blue ) { }
static void null_glSecondaryColor3bEXT( GLbyte red, GLbyte green, GLbyte blue ) { }
static void null_glSecondaryColor3bv( const GLbyte *v ) { }
static void null_glSecondaryColor3bvEXT( const GLbyte *v ) { }
static void null_glSecondaryColor3d( GLdouble red, GLdouble green, GLdouble blue ) { }
static void null_glSecondaryColor3dEXT( GLdouble red, GLdouble green, GLdouble blue ) { }
static void null_glSecondaryColor3dv( const GLdouble *v ) { }
static void null_glSecondaryColor3dvEXT( const GLdouble *v ) { }
static void null_glSecondaryColor3f( GLfloat red, GLfloat green, GLfloat blue ) { }
static void null_glSecondaryColor3fEXT( GLfloat red, GLfloat green, GLfloat blue ) { }
static void null_glSecondaryColor3fv( const GLfloat *v ) { }
static void null_glSecondaryColor3fvEXT( const GLfloat *v ) { }
static void null_glSecondaryColor3hNV( GLhalfNV red, GLhalfNV green, GLhalfNV blue ) { }
static void null_glSecondaryColor3hvNV( const GLhalfNV *v ) { }
static void null_glSecondaryColor3i( GLint red, GLint green, GLint blue ) { }
static void null_glSecondaryColor3iEXT( GLint red, GLint green, GLint blue ) { }
static void null_glSecondaryColor3iv( const GLint *v ) { }
static void null_glSecondaryColor3ivEXT( const GLint *v ) { }
static void null_glSecondaryColor3s( GLshort red, GLshort green, GLshort blue ) { }
static void null_glSecondaryColor3sEXT( GLshort red, GLshort green, GLshort blue ) { }
static void null_glSecondaryColor3sv( const GLshort *v ) { }
static void null_glSecondaryColor3svEXT( const GLshort *v ) { }
static void null_glSecondaryColor3ub( GLubyte red, GLubyte green, GLubyte blue ) { }
static void null_glSecondaryColor3ubEXT( GLubyte red, GLubyte green, GLubyte blue ) { }
static void null_glSecondaryColor3ubv( const GLubyte *v ) { }
static void null_glSecondaryColor3ubvEXT( const GLubyte *v ) { }
static void null_glSecondaryColor3ui( GLuint red, GLuint green, GLuint blue ) { }
static void null_glSecondaryColor3uiEXT( GLuint red, GLuint green, GLuint blue ) { }
static void null_glSecondaryColor3uiv( const GLuint *v ) { }
static void null_glSecondaryColor3uivEXT( const GLuint *v ) { }
static void null_glSecondaryColor3us( GLushort red, GLushort green, GLushort blue ) { }
static void null_glSecondaryColor3usEXT( GLushort red, GLushort green, GLushort blue ) { }
static void null_glSecondaryColor3usv( const GLushort *v ) { }
static void null_glSecondaryColor3usvEXT( const GLushort *v ) { }
static void null_glSecondaryColorFormatNV( GLint size, GLenum type, GLsizei stride ) { }
static void null_glSecondaryColorP3ui( GLenum type, GLuint color ) { }
static void null_glSecondaryColorP3uiv( GLenum type, const GLuint *color ) { }
static void null_glSecondaryColorPointer( GLint size, GLenum type, GLsizei stride, const void *pointer ) { }
static void null_glSecondaryColorPointerEXT( GLint size, GLenum type, GLsizei stride, const void *pointer ) { }
static void null_glSecondaryColorPointerListIBM( GLint size, GLenum type, GLint stride, const void **pointer, GLint ptrstride ) { }
static void null_glSelectPerfMonitorCountersAMD( GLuint monitor, GLboolean enable, GLuint group, GLint numCounters, GLuint *counterList ) { }
static void null_glSelectTextureCoordSetSGIS( GLenum target ) { }
static void null_glSelectTextureSGIS( GLenum target ) { }
static void null_glSemaphoreParameterui64vEXT( GLuint semaphore, GLenum pname, const GLuint64 *params ) { }
static void null_glSeparableFilter2D( GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *row, const void *column ) { }
static void null_glSeparableFilter2DEXT( GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *row, const void *column ) { }
static void null_glSetFenceAPPLE( GLuint fence ) { }
static void null_glSetFenceNV( GLuint fence, GLenum condition ) { }
static void null_glSetFragmentShaderConstantATI( GLuint dst, const GLfloat *value ) { }
static void null_glSetInvariantEXT( GLuint id, GLenum type, const void *addr ) { }
static void null_glSetLocalConstantEXT( GLuint id, GLenum type, const void *addr ) { }
static void null_glSetMultisamplefvAMD( GLenum pname, GLuint index, const GLfloat *val ) { }
static void null_glShaderBinary( GLsizei count, const GLuint *shaders, GLenum binaryformat, const void *binary, GLsizei length ) { }
static void null_glShaderOp1EXT( GLenum op, GLuint res, GLuint arg1 ) { }
static void null_glShaderOp2EXT( GLenum op, GLuint res, GLuint arg1, GLuint arg2 ) { }
static void null_glShaderOp3EXT( GLenum op, GLuint res, GLuint arg1, GLuint arg2, GLuint arg3 ) { }
static void null_glShaderSource( GLuint shader, GLsizei count, const GLchar *const*string, const GLint *length ) { }
static void null_glShaderSourceARB( GLhandleARB shaderObj, GLsizei count, const GLcharARB **string, const GLint *length ) { }
static void null_glShaderStorageBlockBinding( GLuint program, GLuint storageBlockIndex, GLuint storageBlockBinding ) { }
static void null_glShadingRateImageBarrierNV( GLboolean synchronize ) { }
static void null_glShadingRateImagePaletteNV( GLuint viewport, GLuint first, GLsizei count, const GLenum *rates ) { }
static void null_glShadingRateSampleOrderCustomNV( GLenum rate, GLuint samples, const GLint *locations ) { }
static void null_glShadingRateSampleOrderNV( GLenum order ) { }
static void null_glSharpenTexFuncSGIS( GLenum target, GLsizei n, const GLfloat *points ) { }
static void null_glSignalSemaphoreEXT( GLuint semaphore, GLuint numBufferBarriers, const GLuint *buffers, GLuint numTextureBarriers, const GLuint *textures, const GLenum *dstLayouts ) { }
static void null_glSignalSemaphoreui64NVX( GLuint signalGpu, GLsizei fenceObjectCount, const GLuint *semaphoreArray, const GLuint64 *fenceValueArray ) { }
static void null_glSignalVkFenceNV( GLuint64 vkFence ) { }
static void null_glSignalVkSemaphoreNV( GLuint64 vkSemaphore ) { }
static void null_glSpecializeShader( GLuint shader, const GLchar *pEntryPoint, GLuint numSpecializationConstants, const GLuint *pConstantIndex, const GLuint *pConstantValue ) { }
static void null_glSpecializeShaderARB( GLuint shader, const GLchar *pEntryPoint, GLuint numSpecializationConstants, const GLuint *pConstantIndex, const GLuint *pConstantValue ) { }
static void null_glSpriteParameterfSGIX( GLenum pname, GLfloat param ) { }
static void null_glSpriteParameterfvSGIX( GLenum pname, const GLfloat *params ) { }
static void null_glSpriteParameteriSGIX( GLenum pname, GLint param ) { }
static void null_glSpriteParameterivSGIX( GLenum pname, const GLint *params ) { }
static void null_glStartInstrumentsSGIX(void) { }
static void null_glStateCaptureNV( GLuint state, GLenum mode ) { }
static void null_glStencilClearTagEXT( GLsizei stencilTagBits, GLuint stencilClearTag ) { }
static void null_glStencilFillPathInstancedNV( GLsizei numPaths, GLenum pathNameType, const void *paths, GLuint pathBase, GLenum fillMode, GLuint mask, GLenum transformType, const GLfloat *transformValues ) { }
static void null_glStencilFillPathNV( GLuint path, GLenum fillMode, GLuint mask ) { }
static void null_glStencilFuncSeparate( GLenum face, GLenum func, GLint ref, GLuint mask ) { }
static void null_glStencilFuncSeparateATI( GLenum frontfunc, GLenum backfunc, GLint ref, GLuint mask ) { }
static void null_glStencilMaskSeparate( GLenum face, GLuint mask ) { }
static void null_glStencilOpSeparate( GLenum face, GLenum sfail, GLenum dpfail, GLenum dppass ) { }
static void null_glStencilOpSeparateATI( GLenum face, GLenum sfail, GLenum dpfail, GLenum dppass ) { }
static void null_glStencilOpValueAMD( GLenum face, GLuint value ) { }
static void null_glStencilStrokePathInstancedNV( GLsizei numPaths, GLenum pathNameType, const void *paths, GLuint pathBase, GLint reference, GLuint mask, GLenum transformType, const GLfloat *transformValues ) { }
static void null_glStencilStrokePathNV( GLuint path, GLint reference, GLuint mask ) { }
static void null_glStencilThenCoverFillPathInstancedNV( GLsizei numPaths, GLenum pathNameType, const void *paths, GLuint pathBase, GLenum fillMode, GLuint mask, GLenum coverMode, GLenum transformType, const GLfloat *transformValues ) { }
static void null_glStencilThenCoverFillPathNV( GLuint path, GLenum fillMode, GLuint mask, GLenum coverMode ) { }
static void null_glStencilThenCoverStrokePathInstancedNV( GLsizei numPaths, GLenum pathNameType, const void *paths, GLuint pathBase, GLint reference, GLuint mask, GLenum coverMode, GLenum transformType, const GLfloat *transformValues ) { }
static void null_glStencilThenCoverStrokePathNV( GLuint path, GLint reference, GLuint mask, GLenum coverMode ) { }
static void null_glStopInstrumentsSGIX( GLint marker ) { }
static void null_glStringMarkerGREMEDY( GLsizei len, const void *string ) { }
static void null_glSubpixelPrecisionBiasNV( GLuint xbits, GLuint ybits ) { }
static void null_glSwizzleEXT( GLuint res, GLuint in, GLenum outX, GLenum outY, GLenum outZ, GLenum outW ) { }
static void null_glSyncTextureINTEL( GLuint texture ) { }
static void null_glTagSampleBufferSGIX(void) { }
static void null_glTangent3bEXT( GLbyte tx, GLbyte ty, GLbyte tz ) { }
static void null_glTangent3bvEXT( const GLbyte *v ) { }
static void null_glTangent3dEXT( GLdouble tx, GLdouble ty, GLdouble tz ) { }
static void null_glTangent3dvEXT( const GLdouble *v ) { }
static void null_glTangent3fEXT( GLfloat tx, GLfloat ty, GLfloat tz ) { }
static void null_glTangent3fvEXT( const GLfloat *v ) { }
static void null_glTangent3iEXT( GLint tx, GLint ty, GLint tz ) { }
static void null_glTangent3ivEXT( const GLint *v ) { }
static void null_glTangent3sEXT( GLshort tx, GLshort ty, GLshort tz ) { }
static void null_glTangent3svEXT( const GLshort *v ) { }
static void null_glTangentPointerEXT( GLenum type, GLsizei stride, const void *pointer ) { }
static void null_glTbufferMask3DFX( GLuint mask ) { }
static void null_glTessellationFactorAMD( GLfloat factor ) { }
static void null_glTessellationModeAMD( GLenum mode ) { }
static GLboolean null_glTestFenceAPPLE( GLuint fence ) { return 0; }
static GLboolean null_glTestFenceNV( GLuint fence ) { return 0; }
static GLboolean null_glTestObjectAPPLE( GLenum object, GLuint name ) { return 0; }
static void null_glTexAttachMemoryNV( GLenum target, GLuint memory, GLuint64 offset ) { }
static void null_glTexBuffer( GLenum target, GLenum internalformat, GLuint buffer ) { }
static void null_glTexBufferARB( GLenum target, GLenum internalformat, GLuint buffer ) { }
static void null_glTexBufferEXT( GLenum target, GLenum internalformat, GLuint buffer ) { }
static void null_glTexBufferRange( GLenum target, GLenum internalformat, GLuint buffer, GLintptr offset, GLsizeiptr size ) { }
static void null_glTexBumpParameterfvATI( GLenum pname, const GLfloat *param ) { }
static void null_glTexBumpParameterivATI( GLenum pname, const GLint *param ) { }
static void null_glTexCoord1bOES( GLbyte s ) { }
static void null_glTexCoord1bvOES( const GLbyte *coords ) { }
static void null_glTexCoord1hNV( GLhalfNV s ) { }
static void null_glTexCoord1hvNV( const GLhalfNV *v ) { }
static void null_glTexCoord1xOES( GLfixed s ) { }
static void null_glTexCoord1xvOES( const GLfixed *coords ) { }
static void null_glTexCoord2bOES( GLbyte s, GLbyte t ) { }
static void null_glTexCoord2bvOES( const GLbyte *coords ) { }
static void null_glTexCoord2fColor3fVertex3fSUN( GLfloat s, GLfloat t, GLfloat r, GLfloat g, GLfloat b, GLfloat x, GLfloat y, GLfloat z ) { }
static void null_glTexCoord2fColor3fVertex3fvSUN( const GLfloat *tc, const GLfloat *c, const GLfloat *v ) { }
static void null_glTexCoord2fColor4fNormal3fVertex3fSUN( GLfloat s, GLfloat t, GLfloat r, GLfloat g, GLfloat b, GLfloat a, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z ) { }
static void null_glTexCoord2fColor4fNormal3fVertex3fvSUN( const GLfloat *tc, const GLfloat *c, const GLfloat *n, const GLfloat *v ) { }
static void null_glTexCoord2fColor4ubVertex3fSUN( GLfloat s, GLfloat t, GLubyte r, GLubyte g, GLubyte b, GLubyte a, GLfloat x, GLfloat y, GLfloat z ) { }
static void null_glTexCoord2fColor4ubVertex3fvSUN( const GLfloat *tc, const GLubyte *c, const GLfloat *v ) { }
static void null_glTexCoord2fNormal3fVertex3fSUN( GLfloat s, GLfloat t, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z ) { }
static void null_glTexCoord2fNormal3fVertex3fvSUN( const GLfloat *tc, const GLfloat *n, const GLfloat *v ) { }
static void null_glTexCoord2fVertex3fSUN( GLfloat s, GLfloat t, GLfloat x, GLfloat y, GLfloat z ) { }
static void null_glTexCoord2fVertex3fvSUN( const GLfloat *tc, const GLfloat *v ) { }
static void null_glTexCoord2hNV( GLhalfNV s, GLhalfNV t ) { }
static void null_glTexCoord2hvNV( const GLhalfNV *v ) { }
static void null_glTexCoord2xOES( GLfixed s, GLfixed t ) { }
static void null_glTexCoord2xvOES( const GLfixed *coords ) { }
static void null_glTexCoord3bOES( GLbyte s, GLbyte t, GLbyte r ) { }
static void null_glTexCoord3bvOES( const GLbyte *coords ) { }
static void null_glTexCoord3hNV( GLhalfNV s, GLhalfNV t, GLhalfNV r ) { }
static void null_glTexCoord3hvNV( const GLhalfNV *v ) { }
static void null_glTexCoord3xOES( GLfixed s, GLfixed t, GLfixed r ) { }
static void null_glTexCoord3xvOES( const GLfixed *coords ) { }
static void null_glTexCoord4bOES( GLbyte s, GLbyte t, GLbyte r, GLbyte q ) { }
static void null_glTexCoord4bvOES( const GLbyte *coords ) { }
static void null_glTexCoord4fColor4fNormal3fVertex4fSUN( GLfloat s, GLfloat t, GLfloat p, GLfloat q, GLfloat r, GLfloat g, GLfloat b, GLfloat a, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z, GLfloat w ) { }
static void null_glTexCoord4fColor4fNormal3fVertex4fvSUN( const GLfloat *tc, const GLfloat *c, const GLfloat *n, const GLfloat *v ) { }
static void null_glTexCoord4fVertex4fSUN( GLfloat s, GLfloat t, GLfloat p, GLfloat q, GLfloat x, GLfloat y, GLfloat z, GLfloat w ) { }
static void null_glTexCoord4fVertex4fvSUN( const GLfloat *tc, const GLfloat *v ) { }
static void null_glTexCoord4hNV( GLhalfNV s, GLhalfNV t, GLhalfNV r, GLhalfNV q ) { }
static void null_glTexCoord4hvNV( const GLhalfNV *v ) { }
static void null_glTexCoord4xOES( GLfixed s, GLfixed t, GLfixed r, GLfixed q ) { }
static void null_glTexCoord4xvOES( const GLfixed *coords ) { }
static void null_glTexCoordFormatNV( GLint size, GLenum type, GLsizei stride ) { }
static void null_glTexCoordP1ui( GLenum type, GLuint coords ) { }
static void null_glTexCoordP1uiv( GLenum type, const GLuint *coords ) { }
static void null_glTexCoordP2ui( GLenum type, GLuint coords ) { }
static void null_glTexCoordP2uiv( GLenum type, const GLuint *coords ) { }
static void null_glTexCoordP3ui( GLenum type, GLuint coords ) { }
static void null_glTexCoordP3uiv( GLenum type, const GLuint *coords ) { }
static void null_glTexCoordP4ui( GLenum type, GLuint coords ) { }
static void null_glTexCoordP4uiv( GLenum type, const GLuint *coords ) { }
static void null_glTexCoordPointerEXT( GLint size, GLenum type, GLsizei stride, GLsizei count, const void *pointer ) { }
static void null_glTexCoordPointerListIBM( GLint size, GLenum type, GLint stride, const void **pointer, GLint ptrstride ) { }
static void null_glTexCoordPointervINTEL( GLint size, GLenum type, const void **pointer ) { }
static void null_glTexEnvxOES( GLenum target, GLenum pname, GLfixed param ) { }
static void null_glTexEnvxvOES( GLenum target, GLenum pname, const GLfixed *params ) { }
static void null_glTexFilterFuncSGIS( GLenum target, GLenum filter, GLsizei n, const GLfloat *weights ) { }
static void null_glTexGenxOES( GLenum coord, GLenum pname, GLfixed param ) { }
static void null_glTexGenxvOES( GLenum coord, GLenum pname, const GLfixed *params ) { }
static void null_glTexImage2DMultisample( GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations ) { }
static void null_glTexImage2DMultisampleCoverageNV( GLenum target, GLsizei coverageSamples, GLsizei colorSamples, GLint internalFormat, GLsizei width, GLsizei height, GLboolean fixedSampleLocations ) { }
static void null_glTexImage3D( GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void *pixels ) { }
static void null_glTexImage3DEXT( GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void *pixels ) { }
static void null_glTexImage3DMultisample( GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedsamplelocations ) { }
static void null_glTexImage3DMultisampleCoverageNV( GLenum target, GLsizei coverageSamples, GLsizei colorSamples, GLint internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedSampleLocations ) { }
static void null_glTexImage4DSGIS( GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLsizei size4d, GLint border, GLenum format, GLenum type, const void *pixels ) { }
static void null_glTexPageCommitmentARB( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLboolean commit ) { }
static void null_glTexParameterIiv( GLenum target, GLenum pname, const GLint *params ) { }
static void null_glTexParameterIivEXT( GLenum target, GLenum pname, const GLint *params ) { }
static void null_glTexParameterIuiv( GLenum target, GLenum pname, const GLuint *params ) { }
static void null_glTexParameterIuivEXT( GLenum target, GLenum pname, const GLuint *params ) { }
static void null_glTexParameterxOES( GLenum target, GLenum pname, GLfixed param ) { }
static void null_glTexParameterxvOES( GLenum target, GLenum pname, const GLfixed *params ) { }
static void null_glTexRenderbufferNV( GLenum target, GLuint renderbuffer ) { }
static void null_glTexStorage1D( GLenum target, GLsizei levels, GLenum internalformat, GLsizei width ) { }
static void null_glTexStorage2D( GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height ) { }
static void null_glTexStorage2DMultisample( GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations ) { }
static void null_glTexStorage3D( GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth ) { }
static void null_glTexStorage3DMultisample( GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedsamplelocations ) { }
static void null_glTexStorageMem1DEXT( GLenum target, GLsizei levels, GLenum internalFormat, GLsizei width, GLuint memory, GLuint64 offset ) { }
static void null_glTexStorageMem2DEXT( GLenum target, GLsizei levels, GLenum internalFormat, GLsizei width, GLsizei height, GLuint memory, GLuint64 offset ) { }
static void null_glTexStorageMem2DMultisampleEXT( GLenum target, GLsizei samples, GLenum internalFormat, GLsizei width, GLsizei height, GLboolean fixedSampleLocations, GLuint memory, GLuint64 offset ) { }
static void null_glTexStorageMem3DEXT( GLenum target, GLsizei levels, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLuint memory, GLuint64 offset ) { }
static void null_glTexStorageMem3DMultisampleEXT( GLenum target, GLsizei samples, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedSampleLocations, GLuint memory, GLuint64 offset ) { }
static void null_glTexStorageSparseAMD( GLenum target, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLsizei layers, GLbitfield flags ) { }
static void null_glTexSubImage1DEXT( GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const void *pixels ) { }
static void null_glTexSubImage2DEXT( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels ) { }
static void null_glTexSubImage3D( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void *pixels ) { }
static void null_glTexSubImage3DEXT( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void *pixels ) { }
static void null_glTexSubImage4DSGIS( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint woffset, GLsizei width, GLsizei height, GLsizei depth, GLsizei size4d, GLenum format, GLenum type, const void *pixels ) { }
static void null_glTextureAttachMemoryNV( GLuint texture, GLuint memory, GLuint64 offset ) { }
static void null_glTextureBarrier(void) { }
static void null_glTextureBarrierNV(void) { }
static void null_glTextureBuffer( GLuint texture, GLenum internalformat, GLuint buffer ) { }
static void null_glTextureBufferEXT( GLuint texture, GLenum target, GLenum internalformat, GLuint buffer ) { }
static void null_glTextureBufferRange( GLuint texture, GLenum internalformat, GLuint buffer, GLintptr offset, GLsizeiptr size ) { }
static void null_glTextureBufferRangeEXT( GLuint texture, GLenum target, GLenum internalformat, GLuint buffer, GLintptr offset, GLsizeiptr size ) { }
static void null_glTextureColorMaskSGIS( GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha ) { }
static void null_glTextureImage1DEXT( GLuint texture, GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const void *pixels ) { }
static void null_glTextureImage2DEXT( GLuint texture, GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void *pixels ) { }
static void null_glTextureImage2DMultisampleCoverageNV( GLuint texture, GLenum target, GLsizei coverageSamples, GLsizei colorSamples, GLint internalFormat, GLsizei width, GLsizei height, GLboolean fixedSampleLocations ) { }
static void null_glTextureImage2DMultisampleNV( GLuint texture, GLenum target, GLsizei samples, GLint internalFormat, GLsizei width, GLsizei height, GLboolean fixedSampleLocations ) { }
static void null_glTextureImage3DEXT( GLuint texture, GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void *pixels ) { }
static void null_glTextureImage3DMultisampleCoverageNV( GLuint texture, GLenum target, GLsizei coverageSamples, GLsizei colorSamples, GLint internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedSampleLocations ) { }
static void null_glTextureImage3DMultisampleNV( GLuint texture, GLenum target, GLsizei samples, GLint internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedSampleLocations ) { }
static void null_glTextureLightEXT( GLenum pname ) { }
static void null_glTextureMaterialEXT( GLenum face, GLenum mode ) { }
static void null_glTextureNormalEXT( GLenum mode ) { }
static void null_glTexturePageCommitmentEXT( GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLboolean commit ) { }
static void null_glTextureParameterIiv( GLuint texture, GLenum pname, const GLint *params ) { }
static void null_glTextureParameterIivEXT( GLuint texture, GLenum target, GLenum pname, const GLint *params ) { }
static void null_glTextureParameterIuiv( GLuint texture, GLenum pname, const GLuint *params ) { }
static void null_glTextureParameterIuivEXT( GLuint texture, GLenum target, GLenum pname, const GLuint *params ) { }
static void null_glTextureParameterf( GLuint texture, GLenum pname, GLfloat param ) { }
static void null_glTextureParameterfEXT( GLuint texture, GLenum target, GLenum pname, GLfloat param ) { }
static void null_glTextureParameterfv( GLuint texture, GLenum pname, const GLfloat *param ) { }
static void null_glTextureParameterfvEXT( GLuint texture, GLenum target, GLenum pname, const GLfloat *params ) { }
static void null_glTextureParameteri( GLuint texture, GLenum pname, GLint param ) { }
static void null_glTextureParameteriEXT( GLuint texture, GLenum target, GLenum pname, GLint param ) { }
static void null_glTextureParameteriv( GLuint texture, GLenum pname, const GLint *param ) { }
static void null_glTextureParameterivEXT( GLuint texture, GLenum target, GLenum pname, const GLint *params ) { }
static void null_glTextureRangeAPPLE( GLenum target, GLsizei length, const void *pointer ) { }
static void null_glTextureRenderbufferEXT( GLuint texture, GLenum target, GLuint renderbuffer ) { }
static void null_glTextureStorage1D( GLuint texture, GLsizei levels, GLenum internalformat, GLsizei width ) { }
static void null_glTextureStorage1DEXT( GLuint texture, GLenum target, GLsizei levels, GLenum internalformat, GLsizei width ) { }
static void null_glTextureStorage2D( GLuint texture, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height ) { }
static void null_glTextureStorage2DEXT( GLuint texture, GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height ) { }
static void null_glTextureStorage2DMultisample( GLuint texture, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations ) { }
static void null_glTextureStorage2DMultisampleEXT( GLuint texture, GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations ) { }
static void null_glTextureStorage3D( GLuint texture, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth ) { }
static void null_glTextureStorage3DEXT( GLuint texture, GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth ) { }
static void null_glTextureStorage3DMultisample( GLuint texture, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedsamplelocations ) { }
static void null_glTextureStorage3DMultisampleEXT( GLuint texture, GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedsamplelocations ) { }
static void null_glTextureStorageMem1DEXT( GLuint texture, GLsizei levels, GLenum internalFormat, GLsizei width, GLuint memory, GLuint64 offset ) { }
static void null_glTextureStorageMem2DEXT( GLuint texture, GLsizei levels, GLenum internalFormat, GLsizei width, GLsizei height, GLuint memory, GLuint64 offset ) { }
static void null_glTextureStorageMem2DMultisampleEXT( GLuint texture, GLsizei samples, GLenum internalFormat, GLsizei width, GLsizei height, GLboolean fixedSampleLocations, GLuint memory, GLuint64 offset ) { }
static void null_glTextureStorageMem3DEXT( GLuint texture, GLsizei levels, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLuint memory, GLuint64 offset ) { }
static void null_glTextureStorageMem3DMultisampleEXT( GLuint texture, GLsizei samples, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedSampleLocations, GLuint memory, GLuint64 offset ) { }
static void null_glTextureStorageSparseAMD( GLuint texture, GLenum target, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLsizei layers, GLbitfield flags ) { }
static void null_glTextureSubImage1D( GLuint texture, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const void *pixels ) { }
static void null_glTextureSubImage1DEXT( GLuint texture, GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const void *pixels ) { }
static void null_glTextureSubImage2D( GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels ) { }
static void null_glTextureSubImage2DEXT( GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels ) { }
static void null_glTextureSubImage3D( GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void *pixels ) { }
static void null_glTextureSubImage3DEXT( GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void *pixels ) { }
static void null_glTextureView( GLuint texture, GLenum target, GLuint origtexture, GLenum internalformat, GLuint minlevel, GLuint numlevels, GLuint minlayer, GLuint numlayers ) { }
static void null_glTrackMatrixNV( GLenum target, GLuint address, GLenum matrix, GLenum transform ) { }
static void null_glTransformFeedbackAttribsNV( GLsizei count, const GLint *attribs, GLenum bufferMode ) { }
static void null_glTransformFeedbackBufferBase( GLuint xfb, GLuint index, GLuint buffer ) { }
static void null_glTransformFeedbackBufferRange( GLuint xfb, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size ) { }
static void null_glTransformFeedbackStreamAttribsNV( GLsizei count, const GLint *attribs, GLsizei nbuffers, const GLint *bufstreams, GLenum bufferMode ) { }
static void null_glTransformFeedbackVaryings( GLuint program, GLsizei count, const GLchar *const*varyings, GLenum bufferMode ) { }
static void null_glTransformFeedbackVaryingsEXT( GLuint program, GLsizei count, const GLchar *const*varyings, GLenum bufferMode ) { }
static void null_glTransformFeedbackVaryingsNV( GLuint program, GLsizei count, const GLint *locations, GLenum bufferMode ) { }
static void null_glTransformPathNV( GLuint resultPath, GLuint srcPath, GLenum transformType, const GLfloat *transformValues ) { }
static void null_glTranslatexOES( GLfixed x, GLfixed y, GLfixed z ) { }
static void null_glUniform1d( GLint location, GLdouble x ) { }
static void null_glUniform1dv( GLint location, GLsizei count, const GLdouble *value ) { }
static void null_glUniform1f( GLint location, GLfloat v0 ) { }
static void null_glUniform1fARB( GLint location, GLfloat v0 ) { }
static void null_glUniform1fv( GLint location, GLsizei count, const GLfloat *value ) { }
static void null_glUniform1fvARB( GLint location, GLsizei count, const GLfloat *value ) { }
static void null_glUniform1i( GLint location, GLint v0 ) { }
static void null_glUniform1i64ARB( GLint location, GLint64 x ) { }
static void null_glUniform1i64NV( GLint location, GLint64EXT x ) { }
static void null_glUniform1i64vARB( GLint location, GLsizei count, const GLint64 *value ) { }
static void null_glUniform1i64vNV( GLint location, GLsizei count, const GLint64EXT *value ) { }
static void null_glUniform1iARB( GLint location, GLint v0 ) { }
static void null_glUniform1iv( GLint location, GLsizei count, const GLint *value ) { }
static void null_glUniform1ivARB( GLint location, GLsizei count, const GLint *value ) { }
static void null_glUniform1ui( GLint location, GLuint v0 ) { }
static void null_glUniform1ui64ARB( GLint location, GLuint64 x ) { }
static void null_glUniform1ui64NV( GLint location, GLuint64EXT x ) { }
static void null_glUniform1ui64vARB( GLint location, GLsizei count, const GLuint64 *value ) { }
static void null_glUniform1ui64vNV( GLint location, GLsizei count, const GLuint64EXT *value ) { }
static void null_glUniform1uiEXT( GLint location, GLuint v0 ) { }
static void null_glUniform1uiv( GLint location, GLsizei count, const GLuint *value ) { }
static void null_glUniform1uivEXT( GLint location, GLsizei count, const GLuint *value ) { }
static void null_glUniform2d( GLint location, GLdouble x, GLdouble y ) { }
static void null_glUniform2dv( GLint location, GLsizei count, const GLdouble *value ) { }
static void null_glUniform2f( GLint location, GLfloat v0, GLfloat v1 ) { }
static void null_glUniform2fARB( GLint location, GLfloat v0, GLfloat v1 ) { }
static void null_glUniform2fv( GLint location, GLsizei count, const GLfloat *value ) { }
static void null_glUniform2fvARB( GLint location, GLsizei count, const GLfloat *value ) { }
static void null_glUniform2i( GLint location, GLint v0, GLint v1 ) { }
static void null_glUniform2i64ARB( GLint location, GLint64 x, GLint64 y ) { }
static void null_glUniform2i64NV( GLint location, GLint64EXT x, GLint64EXT y ) { }
static void null_glUniform2i64vARB( GLint location, GLsizei count, const GLint64 *value ) { }
static void null_glUniform2i64vNV( GLint location, GLsizei count, const GLint64EXT *value ) { }
static void null_glUniform2iARB( GLint location, GLint v0, GLint v1 ) { }
static void null_glUniform2iv( GLint location, GLsizei count, const GLint *value ) { }
static void null_glUniform2ivARB( GLint location, GLsizei count, const GLint *value ) { }
static void null_glUniform2ui( GLint location, GLuint v0, GLuint v1 ) { }
static void null_glUniform2ui64ARB( GLint location, GLuint64 x, GLuint64 y ) { }
static void null_glUniform2ui64NV( GLint location, GLuint64EXT x, GLuint64EXT y ) { }
static void null_glUniform2ui64vARB( GLint location, GLsizei count, const GLuint64 *value ) { }
static void null_glUniform2ui64vNV( GLint location, GLsizei count, const GLuint64EXT *value ) { }
static void null_glUniform2uiEXT( GLint location, GLuint v0, GLuint v1 ) { }
static void null_glUniform2uiv( GLint location, GLsizei count, const GLuint *value ) { }
static void null_glUniform2uivEXT( GLint location, GLsizei count, const GLuint *value ) { }
static void null_glUniform3d( GLint location, GLdouble x, GLdouble y, GLdouble z ) { }
static void null_glUniform3dv( GLint location, GLsizei count, const GLdouble *value ) { }
static void null_glUniform3f( GLint location, GLfloat v0, GLfloat v1, GLfloat v2 ) { }
static void null_glUniform3fARB( GLint location, GLfloat v0, GLfloat v1, GLfloat v2 ) { }
static void null_glUniform3fv( GLint location, GLsizei count, const GLfloat *value ) { }
static void null_glUniform3fvARB( GLint location, GLsizei count, const GLfloat *value ) { }
static void null_glUniform3i( GLint location, GLint v0, GLint v1, GLint v2 ) { }
static void null_glUniform3i64ARB( GLint location, GLint64 x, GLint64 y, GLint64 z ) { }
static void null_glUniform3i64NV( GLint location, GLint64EXT x, GLint64EXT y, GLint64EXT z ) { }
static void null_glUniform3i64vARB( GLint location, GLsizei count, const GLint64 *value ) { }
static void null_glUniform3i64vNV( GLint location, GLsizei count, const GLint64EXT *value ) { }
static void null_glUniform3iARB( GLint location, GLint v0, GLint v1, GLint v2 ) { }
static void null_glUniform3iv( GLint location, GLsizei count, const GLint *value ) { }
static void null_glUniform3ivARB( GLint location, GLsizei count, const GLint *value ) { }
static void null_glUniform3ui( GLint location, GLuint v0, GLuint v1, GLuint v2 ) { }
static void null_glUniform3ui64ARB( GLint location, GLuint64 x, GLuint64 y, GLuint64 z ) { }
static void null_glUniform3ui64NV( GLint location, GLuint64EXT x, GLuint64EXT y, GLuint64EXT z ) { }
static void null_glUniform3ui64vARB( GLint location, GLsizei count, const GLuint64 *value ) { }
static void null_glUniform3ui64vNV( GLint location, GLsizei count, const GLuint64EXT *value ) { }
static void null_glUniform3uiEXT( GLint location, GLuint v0, GLuint v1, GLuint v2 ) { }
static void null_glUniform3uiv( GLint location, GLsizei count, const GLuint *value ) { }
static void null_glUniform3uivEXT( GLint location, GLsizei count, const GLuint *value ) { }
static void null_glUniform4d( GLint location, GLdouble x, GLdouble y, GLdouble z, GLdouble w ) { }
static void null_glUniform4dv( GLint location, GLsizei count, const GLdouble *value ) { }
static void null_glUniform4f( GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3 ) { }
static void null_glUniform4fARB( GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3 ) { }
static void null_glUniform4fv( GLint location, GLsizei count, const GLfloat *value ) { }
static void null_glUniform4fvARB( GLint location, GLsizei count, const GLfloat *value ) { }
static void null_glUniform4i( GLint location, GLint v0, GLint v1, GLint v2, GLint v3 ) { }
static void null_glUniform4i64ARB( GLint location, GLint64 x, GLint64 y, GLint64 z, GLint64 w ) { }
static void null_glUniform4i64NV( GLint location, GLint64EXT x, GLint64EXT y, GLint64EXT z, GLint64EXT w ) { }
static void null_glUniform4i64vARB( GLint location, GLsizei count, const GLint64 *value ) { }
static void null_glUniform4i64vNV( GLint location, GLsizei count, const GLint64EXT *value ) { }
static void null_glUniform4iARB( GLint location, GLint v0, GLint v1, GLint v2, GLint v3 ) { }
static void null_glUniform4iv( GLint location, GLsizei count, const GLint *value ) { }
static void null_glUniform4ivARB( GLint location, GLsizei count, const GLint *value ) { }
static void null_glUniform4ui( GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3 ) { }
static void null_glUniform4ui64ARB( GLint location, GLuint64 x, GLuint64 y, GLuint64 z, GLuint64 w ) { }
static void null_glUniform4ui64NV( GLint location, GLuint64EXT x, GLuint64EXT y, GLuint64EXT z, GLuint64EXT w ) { }
static void null_glUniform4ui64vARB( GLint location, GLsizei count, const GLuint64 *value ) { }
static void null_glUniform4ui64vNV( GLint location, GLsizei count, const GLuint64EXT *value ) { }
static void null_glUniform4uiEXT( GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3 ) { }
static void null_glUniform4uiv( GLint location, GLsizei count, const GLuint *value ) { }
static void null_glUniform4uivEXT( GLint location, GLsizei count, const GLuint *value ) { }
static void null_glUniformBlockBinding( GLuint program, GLuint uniformBlockIndex, GLuint uniformBlockBinding ) { }
static void null_glUniformBufferEXT( GLuint program, GLint location, GLuint buffer ) { }
static void null_glUniformHandleui64ARB( GLint location, GLuint64 value ) { }
static void null_glUniformHandleui64NV( GLint location, GLuint64 value ) { }
static void null_glUniformHandleui64vARB( GLint location, GLsizei count, const GLuint64 *value ) { }
static void null_glUniformHandleui64vNV( GLint location, GLsizei count, const GLuint64 *value ) { }
static void null_glUniformMatrix2dv( GLint location, GLsizei count, GLboolean transpose, const GLdouble *value ) { }
static void null_glUniformMatrix2fv( GLint location, GLsizei count, GLboolean transpose, const GLfloat *value ) { }
static void null_glUniformMatrix2fvARB( GLint location, GLsizei count, GLboolean transpose, const GLfloat *value ) { }
static void null_glUniformMatrix2x3dv( GLint location, GLsizei count, GLboolean transpose, const GLdouble *value ) { }
static void null_glUniformMatrix2x3fv( GLint location, GLsizei count, GLboolean transpose, const GLfloat *value ) { }
static void null_glUniformMatrix2x4dv( GLint location, GLsizei count, GLboolean transpose, const GLdouble *value ) { }
static void null_glUniformMatrix2x4fv( GLint location, GLsizei count, GLboolean transpose, const GLfloat *value ) { }
static void null_glUniformMatrix3dv( GLint location, GLsizei count, GLboolean transpose, const GLdouble *value ) { }
static void null_glUniformMatrix3fv( GLint location, GLsizei count, GLboolean transpose, const GLfloat *value ) { }
static void null_glUniformMatrix3fvARB( GLint location, GLsizei count, GLboolean transpose, const GLfloat *value ) { }
static void null_glUniformMatrix3x2dv( GLint location, GLsizei count, GLboolean transpose, const GLdouble *value ) { }
static void null_glUniformMatrix3x2fv( GLint location, GLsizei count, GLboolean transpose, const GLfloat *value ) { }
static void null_glUniformMatrix3x4dv( GLint location, GLsizei count, GLboolean transpose, const GLdouble *value ) { }
static void null_glUniformMatrix3x4fv( GLint location, GLsizei count, GLboolean transpose, const GLfloat *value ) { }
static void null_glUniformMatrix4dv( GLint location, GLsizei count, GLboolean transpose, const GLdouble *value ) { }
static void null_glUniformMatrix4fv( GLint location, GLsizei count, GLboolean transpose, const GLfloat *value ) { }
static void null_glUniformMatrix4fvARB( GLint location, GLsizei count, GLboolean transpose, const GLfloat *value ) { }
static void null_glUniformMatrix4x2dv( GLint location, GLsizei count, GLboolean transpose, const GLdouble *value ) { }
static void null_glUniformMatrix4x2fv( GLint location, GLsizei count, GLboolean transpose, const GLfloat *value ) { }
static void null_glUniformMatrix4x3dv( GLint location, GLsizei count, GLboolean transpose, const GLdouble *value ) { }
static void null_glUniformMatrix4x3fv( GLint location, GLsizei count, GLboolean transpose, const GLfloat *value ) { }
static void null_glUniformSubroutinesuiv( GLenum shadertype, GLsizei count, const GLuint *indices ) { }
static void null_glUniformui64NV( GLint location, GLuint64EXT value ) { }
static void null_glUniformui64vNV( GLint location, GLsizei count, const GLuint64EXT *value ) { }
static void null_glUnlockArraysEXT(void) { }
static GLboolean null_glUnmapBuffer( GLenum target ) { return 0; }
static GLboolean null_glUnmapBufferARB( GLenum target ) { return 0; }
static GLboolean null_glUnmapNamedBuffer( GLuint buffer ) { return 0; }
static GLboolean null_glUnmapNamedBufferEXT( GLuint buffer ) { return 0; }
static void null_glUnmapObjectBufferATI( GLuint buffer ) { }
static void null_glUnmapTexture2DINTEL( GLuint texture, GLint level ) { }
static void null_glUpdateObjectBufferATI( GLuint buffer, GLuint offset, GLsizei size, const void *pointer, GLenum preserve ) { }
static void null_glUploadGpuMaskNVX( GLbitfield mask ) { }
static void null_glUseProgram( GLuint program ) { }
static void null_glUseProgramObjectARB( GLhandleARB programObj ) { }
static void null_glUseProgramStages( GLuint pipeline, GLbitfield stages, GLuint program ) { }
static void null_glUseShaderProgramEXT( GLenum type, GLuint program ) { }
static void null_glVDPAUFiniNV(void) { }
static void null_glVDPAUGetSurfaceivNV( GLvdpauSurfaceNV surface, GLenum pname, GLsizei count, GLsizei *length, GLint *values ) { }
static void null_glVDPAUInitNV( const void *vdpDevice, const void *getProcAddress ) { }
static GLboolean null_glVDPAUIsSurfaceNV( GLvdpauSurfaceNV surface ) { return 0; }
static void null_glVDPAUMapSurfacesNV( GLsizei numSurfaces, const GLvdpauSurfaceNV *surfaces ) { }
static GLvdpauSurfaceNV null_glVDPAURegisterOutputSurfaceNV( const void *vdpSurface, GLenum target, GLsizei numTextureNames, const GLuint *textureNames ) { return 0; }
static GLvdpauSurfaceNV null_glVDPAURegisterVideoSurfaceNV( const void *vdpSurface, GLenum target, GLsizei numTextureNames, const GLuint *textureNames ) { return 0; }
static GLvdpauSurfaceNV null_glVDPAURegisterVideoSurfaceWithPictureStructureNV( const void *vdpSurface, GLenum target, GLsizei numTextureNames, const GLuint *textureNames, GLboolean isFrameStructure ) { return 0; }
static void null_glVDPAUSurfaceAccessNV( GLvdpauSurfaceNV surface, GLenum access ) { }
static void null_glVDPAUUnmapSurfacesNV( GLsizei numSurface, const GLvdpauSurfaceNV *surfaces ) { }
static void null_glVDPAUUnregisterSurfaceNV( GLvdpauSurfaceNV surface ) { }
static void null_glValidateProgram( GLuint program ) { }
static void null_glValidateProgramARB( GLhandleARB programObj ) { }
static void null_glValidateProgramPipeline( GLuint pipeline ) { }
static void null_glVariantArrayObjectATI( GLuint id, GLenum type, GLsizei stride, GLuint buffer, GLuint offset ) { }
static void null_glVariantPointerEXT( GLuint id, GLenum type, GLuint stride, const void *addr ) { }
static void null_glVariantbvEXT( GLuint id, const GLbyte *addr ) { }
static void null_glVariantdvEXT( GLuint id, const GLdouble *addr ) { }
static void null_glVariantfvEXT( GLuint id, const GLfloat *addr ) { }
static void null_glVariantivEXT( GLuint id, const GLint *addr ) { }
static void null_glVariantsvEXT( GLuint id, const GLshort *addr ) { }
static void null_glVariantubvEXT( GLuint id, const GLubyte *addr ) { }
static void null_glVariantuivEXT( GLuint id, const GLuint *addr ) { }
static void null_glVariantusvEXT( GLuint id, const GLushort *addr ) { }
static void null_glVertex2bOES( GLbyte x, GLbyte y ) { }
static void null_glVertex2bvOES( const GLbyte *coords ) { }
static void null_glVertex2hNV( GLhalfNV x, GLhalfNV y ) { }
static void null_glVertex2hvNV( const GLhalfNV *v ) { }
static void null_glVertex2xOES( GLfixed x ) { }
static void null_glVertex2xvOES( const GLfixed *coords ) { }
static void null_glVertex3bOES( GLbyte x, GLbyte y, GLbyte z ) { }
static void null_glVertex3bvOES( const GLbyte *coords ) { }
static void null_glVertex3hNV( GLhalfNV x, GLhalfNV y, GLhalfNV z ) { }
static void null_glVertex3hvNV( const GLhalfNV *v ) { }
static void null_glVertex3xOES( GLfixed x, GLfixed y ) { }
static void null_glVertex3xvOES( const GLfixed *coords ) { }
static void null_glVertex4bOES( GLbyte x, GLbyte y, GLbyte z, GLbyte w ) { }
static void null_glVertex4bvOES( const GLbyte *coords ) { }
static void null_glVertex4hNV( GLhalfNV x, GLhalfNV y, GLhalfNV z, GLhalfNV w ) { }
static void null_glVertex4hvNV( const GLhalfNV *v ) { }
static void null_glVertex4xOES( GLfixed x, GLfixed y, GLfixed z ) { }
static void null_glVertex4xvOES( const GLfixed *coords ) { }
static void null_glVertexArrayAttribBinding( GLuint vaobj, GLuint attribindex, GLuint bindingindex ) { }
static void null_glVertexArrayAttribFormat( GLuint vaobj, GLuint attribindex, GLint size, GLenum type, GLboolean normalized, GLuint relativeoffset ) { }
static void null_glVertexArrayAttribIFormat( GLuint vaobj, GLuint attribindex, GLint size, GLenum type, GLuint relativeoffset ) { }
static void null_glVertexArrayAttribLFormat( GLuint vaobj, GLuint attribindex, GLint size, GLenum type, GLuint relativeoffset ) { }
static void null_glVertexArrayBindVertexBufferEXT( GLuint vaobj, GLuint bindingindex, GLuint buffer, GLintptr offset, GLsizei stride ) { }
static void null_glVertexArrayBindingDivisor( GLuint vaobj, GLuint bindingindex, GLuint divisor ) { }
static void null_glVertexArrayColorOffsetEXT( GLuint vaobj, GLuint buffer, GLint size, GLenum type, GLsizei stride, GLintptr offset ) { }
static void null_glVertexArrayEdgeFlagOffsetEXT( GLuint vaobj, GLuint buffer, GLsizei stride, GLintptr offset ) { }
static void null_glVertexArrayElementBuffer( GLuint vaobj, GLuint buffer ) { }
static void null_glVertexArrayFogCoordOffsetEXT( GLuint vaobj, GLuint buffer, GLenum type, GLsizei stride, GLintptr offset ) { }
static void null_glVertexArrayIndexOffsetEXT( GLuint vaobj, GLuint buffer, GLenum type, GLsizei stride, GLintptr offset ) { }
static void null_glVertexArrayMultiTexCoordOffsetEXT( GLuint vaobj, GLuint buffer, GLenum texunit, GLint size, GLenum type, GLsizei stride, GLintptr offset ) { }
static void null_glVertexArrayNormalOffsetEXT( GLuint vaobj, GLuint buffer, GLenum type, GLsizei stride, GLintptr offset ) { }
static void null_glVertexArrayParameteriAPPLE( GLenum pname, GLint param ) { }
static void null_glVertexArrayRangeAPPLE( GLsizei length, void *pointer ) { }
static void null_glVertexArrayRangeNV( GLsizei length, const void *pointer ) { }
static void null_glVertexArraySecondaryColorOffsetEXT( GLuint vaobj, GLuint buffer, GLint size, GLenum type, GLsizei stride, GLintptr offset ) { }
static void null_glVertexArrayTexCoordOffsetEXT( GLuint vaobj, GLuint buffer, GLint size, GLenum type, GLsizei stride, GLintptr offset ) { }
static void null_glVertexArrayVertexAttribBindingEXT( GLuint vaobj, GLuint attribindex, GLuint bindingindex ) { }
static void null_glVertexArrayVertexAttribDivisorEXT( GLuint vaobj, GLuint index, GLuint divisor ) { }
static void null_glVertexArrayVertexAttribFormatEXT( GLuint vaobj, GLuint attribindex, GLint size, GLenum type, GLboolean normalized, GLuint relativeoffset ) { }
static void null_glVertexArrayVertexAttribIFormatEXT( GLuint vaobj, GLuint attribindex, GLint size, GLenum type, GLuint relativeoffset ) { }
static void null_glVertexArrayVertexAttribIOffsetEXT( GLuint vaobj, GLuint buffer, GLuint index, GLint size, GLenum type, GLsizei stride, GLintptr offset ) { }
static void null_glVertexArrayVertexAttribLFormatEXT( GLuint vaobj, GLuint attribindex, GLint size, GLenum type, GLuint relativeoffset ) { }
static void null_glVertexArrayVertexAttribLOffsetEXT( GLuint vaobj, GLuint buffer, GLuint index, GLint size, GLenum type, GLsizei stride, GLintptr offset ) { }
static void null_glVertexArrayVertexAttribOffsetEXT( GLuint vaobj, GLuint buffer, GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, GLintptr offset ) { }
static void null_glVertexArrayVertexBindingDivisorEXT( GLuint vaobj, GLuint bindingindex, GLuint divisor ) { }
static void null_glVertexArrayVertexBuffer( GLuint vaobj, GLuint bindingindex, GLuint buffer, GLintptr offset, GLsizei stride ) { }
static void null_glVertexArrayVertexBuffers( GLuint vaobj, GLuint first, GLsizei count, const GLuint *buffers, const GLintptr *offsets, const GLsizei *strides ) { }
static void null_glVertexArrayVertexOffsetEXT( GLuint vaobj, GLuint buffer, GLint size, GLenum type, GLsizei stride, GLintptr offset ) { }
static void null_glVertexAttrib1d( GLuint index, GLdouble x ) { }
static void null_glVertexAttrib1dARB( GLuint index, GLdouble x ) { }
static void null_glVertexAttrib1dNV( GLuint index, GLdouble x ) { }
static void null_glVertexAttrib1dv( GLuint index, const GLdouble *v ) { }
static void null_glVertexAttrib1dvARB( GLuint index, const GLdouble *v ) { }
static void null_glVertexAttrib1dvNV( GLuint index, const GLdouble *v ) { }
static void null_glVertexAttrib1f( GLuint index, GLfloat x ) { }
static void null_glVertexAttrib1fARB( GLuint index, GLfloat x ) { }
static void null_glVertexAttrib1fNV( GLuint index, GLfloat x ) { }
static void null_glVertexAttrib1fv( GLuint index, const GLfloat *v ) { }
static void null_glVertexAttrib1fvARB( GLuint index, const GLfloat *v ) { }
static void null_glVertexAttrib1fvNV( GLuint index, const GLfloat *v ) { }
static void null_glVertexAttrib1hNV( GLuint index, GLhalfNV x ) { }
static void null_glVertexAttrib1hvNV( GLuint index, const GLhalfNV *v ) { }
static void null_glVertexAttrib1s( GLuint index, GLshort x ) { }
static void null_glVertexAttrib1sARB( GLuint index, GLshort x ) { }
static void null_glVertexAttrib1sNV( GLuint index, GLshort x ) { }
static void null_glVertexAttrib1sv( GLuint index, const GLshort *v ) { }
static void null_glVertexAttrib1svARB( GLuint index, const GLshort *v ) { }
static void null_glVertexAttrib1svNV( GLuint index, const GLshort *v ) { }
static void null_glVertexAttrib2d( GLuint index, GLdouble x, GLdouble y ) { }
static void null_glVertexAttrib2dARB( GLuint index, GLdouble x, GLdouble y ) { }
static void null_glVertexAttrib2dNV( GLuint index, GLdouble x, GLdouble y ) { }
static void null_glVertexAttrib2dv( GLuint index, const GLdouble *v ) { }
static void null_glVertexAttrib2dvARB( GLuint index, const GLdouble *v ) { }
static void null_glVertexAttrib2dvNV( GLuint index, const GLdouble *v ) { }
static void null_glVertexAttrib2f( GLuint index, GLfloat x, GLfloat y ) { }
static void null_glVertexAttrib2fARB( GLuint index, GLfloat x, GLfloat y ) { }
static void null_glVertexAttrib2fNV( GLuint index, GLfloat x, GLfloat y ) { }
static void null_glVertexAttrib2fv( GLuint index, const GLfloat *v ) { }
static void null_glVertexAttrib2fvARB( GLuint index, const GLfloat *v ) { }
static void null_glVertexAttrib2fvNV( GLuint index, const GLfloat *v ) { }
static void null_glVertexAttrib2hNV( GLuint index, GLhalfNV x, GLhalfNV y ) { }
static void null_glVertexAttrib2hvNV( GLuint index, const GLhalfNV *v ) { }
static void null_glVertexAttrib2s( GLuint index, GLshort x, GLshort y ) { }
static void null_glVertexAttrib2sARB( GLuint index, GLshort x, GLshort y ) { }
static void null_glVertexAttrib2sNV( GLuint index, GLshort x, GLshort y ) { }
static void null_glVertexAttrib2sv( GLuint index, const GLshort *v ) { }
static void null_glVertexAttrib2svARB( GLuint index, const GLshort *v ) { }
static void null_glVertexAttrib2svNV( GLuint index, const GLshort *v ) { }
static void null_glVertexAttrib3d( GLuint index, GLdouble x, GLdouble y, GLdouble z ) { }
static void null_glVertexAttrib3dARB( GLuint index, GLdouble x, GLdouble y, GLdouble z ) { }
static void null_glVertexAttrib3dNV( GLuint index, GLdouble x, GLdouble y, GLdouble z ) { }
static void null_glVertexAttrib3dv( GLuint index, const GLdouble *v ) { }
static void null_glVertexAttrib3dvARB( GLuint index, const GLdouble *v ) { }
static void null_glVertexAttrib3dvNV( GLuint index, const GLdouble *v ) { }
static void null_glVertexAttrib3f( GLuint index, GLfloat x, GLfloat y, GLfloat z ) { }
static void null_glVertexAttrib3fARB( GLuint index, GLfloat x, GLfloat y, GLfloat z ) { }
static void null_glVertexAttrib3fNV( GLuint index, GLfloat x, GLfloat y, GLfloat z ) { }
static void null_glVertexAttrib3fv( GLuint index, const GLfloat *v ) { }
static void null_glVertexAttrib3fvARB( GLuint index, const GLfloat *v ) { }
static void null_glVertexAttrib3fvNV( GLuint index, const GLfloat *v ) { }
static void null_glVertexAttrib3hNV( GLuint index, GLhalfNV x, GLhalfNV y, GLhalfNV z ) { }
static void null_glVertexAttrib3hvNV( GLuint index, const GLhalfNV *v ) { }
static void null_glVertexAttrib3s( GLuint index, GLshort x, GLshort y, GLshort z ) { }
static void null_glVertexAttrib3sARB( GLuint index, GLshort x, GLshort y, GLshort z ) { }
static void null_glVertexAttrib3sNV( GLuint index, GLshort x, GLshort y, GLshort z ) { }
static void null_glVertexAttrib3sv( GLuint index, const GLshort *v ) { }
static void null_glVertexAttrib3svARB( GLuint index, const GLshort *v ) { }
static void null_glVertexAttrib3svNV( GLuint index, const GLshort *v ) { }
static void null_glVertexAttrib4Nbv( GLuint index, const GLbyte *v ) { }
static void null_glVertexAttrib4NbvARB( GLuint index, const GLbyte *v ) { }
static void null_glVertexAttrib4Niv( GLuint index, const GLint *v ) { }
static void null_glVertexAttrib4NivARB( GLuint index, const GLint *v ) { }
static void null_glVertexAttrib4Nsv( GLuint index, const GLshort *v ) { }
static void null_glVertexAttrib4NsvARB( GLuint index, const GLshort *v ) { }
static void null_glVertexAttrib4Nub( GLuint index, GLubyte x, GLubyte y, GLubyte z, GLubyte w ) { }
static void null_glVertexAttrib4NubARB( GLuint index, GLubyte x, GLubyte y, GLubyte z, GLubyte w ) { }
static void null_glVertexAttrib4Nubv( GLuint index, const GLubyte *v ) { }
static void null_glVertexAttrib4NubvARB( GLuint index, const GLubyte *v ) { }
static void null_glVertexAttrib4Nuiv( GLuint index, const GLuint *v ) { }
static void null_glVertexAttrib4NuivARB( GLuint index, const GLuint *v ) { }
static void null_glVertexAttrib4Nusv( GLuint index, const GLushort *v ) { }
static void null_glVertexAttrib4NusvARB( GLuint index, const GLushort *v ) { }
static void null_glVertexAttrib4bv( GLuint index, const GLbyte *v ) { }
static void null_glVertexAttrib4bvARB( GLuint index, const GLbyte *v ) { }
static void null_glVertexAttrib4d( GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w ) { }
static void null_glVertexAttrib4dARB( GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w ) { }
static void null_glVertexAttrib4dNV( GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w ) { }
static void null_glVertexAttrib4dv( GLuint index, const GLdouble *v ) { }
static void null_glVertexAttrib4dvARB( GLuint index, const GLdouble *v ) { }
static void null_glVertexAttrib4dvNV( GLuint index, const GLdouble *v ) { }
static void null_glVertexAttrib4f( GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w ) { }
static void null_glVertexAttrib4fARB( GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w ) { }
static void null_glVertexAttrib4fNV( GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w ) { }
static void null_glVertexAttrib4fv( GLuint index, const GLfloat *v ) { }
static void null_glVertexAttrib4fvARB( GLuint index, const GLfloat *v ) { }
static void null_glVertexAttrib4fvNV( GLuint index, const GLfloat *v ) { }
static void null_glVertexAttrib4hNV( GLuint index, GLhalfNV x, GLhalfNV y, GLhalfNV z, GLhalfNV w ) { }
static void null_glVertexAttrib4hvNV( GLuint index, const GLhalfNV *v ) { }
static void null_glVertexAttrib4iv( GLuint index, const GLint *v ) { }
static void null_glVertexAttrib4ivARB( GLuint index, const GLint *v ) { }
static void null_glVertexAttrib4s( GLuint index, GLshort x, GLshort y, GLshort z, GLshort w ) { }
static void null_glVertexAttrib4sARB( GLuint index, GLshort x, GLshort y, GLshort z, GLshort w ) { }
static void null_glVertexAttrib4sNV( GLuint index, GLshort x, GLshort y, GLshort z, GLshort w ) { }
static void null_glVertexAttrib4sv( GLuint index, const GLshort *v ) { }
static void null_glVertexAttrib4svARB( GLuint index, const GLshort *v ) { }
static void null_glVertexAttrib4svNV( GLuint index, const GLshort *v ) { }
static void null_glVertexAttrib4ubNV( GLuint index, GLubyte x, GLubyte y, GLubyte z, GLubyte w ) { }
static void null_glVertexAttrib4ubv( GLuint index, const GLubyte *v ) { }
static void null_glVertexAttrib4ubvARB( GLuint index, const GLubyte *v ) { }
static void null_glVertexAttrib4ubvNV( GLuint index, const GLubyte *v ) { }
static void null_glVertexAttrib4uiv( GLuint index, const GLuint *v ) { }
static void null_glVertexAttrib4uivARB( GLuint index, const GLuint *v ) { }
static void null_glVertexAttrib4usv( GLuint index, const GLushort *v ) { }
static void null_glVertexAttrib4usvARB( GLuint index, const GLushort *v ) { }
static void null_glVertexAttribArrayObjectATI( GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, GLuint buffer, GLuint offset ) { }
static void null_glVertexAttribBinding( GLuint attribindex, GLuint bindingindex ) { }
static void null_glVertexAttribDivisor( GLuint index, GLuint divisor ) { }
static void null_glVertexAttribDivisorARB( GLuint index, GLuint divisor ) { }
static void null_glVertexAttribFormat( GLuint attribindex, GLint size, GLenum type, GLboolean normalized, GLuint relativeoffset ) { }
static void null_glVertexAttribFormatNV( GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride ) { }
static void null_glVertexAttribI1i( GLuint index, GLint x ) { }
static void null_glVertexAttribI1iEXT( GLuint index, GLint x ) { }
static void null_glVertexAttribI1iv( GLuint index, const GLint *v ) { }
static void null_glVertexAttribI1ivEXT( GLuint index, const GLint *v ) { }
static void null_glVertexAttribI1ui( GLuint index, GLuint x ) { }
static void null_glVertexAttribI1uiEXT( GLuint index, GLuint x ) { }
static void null_glVertexAttribI1uiv( GLuint index, const GLuint *v ) { }
static void null_glVertexAttribI1uivEXT( GLuint index, const GLuint *v ) { }
static void null_glVertexAttribI2i( GLuint index, GLint x, GLint y ) { }
static void null_glVertexAttribI2iEXT( GLuint index, GLint x, GLint y ) { }
static void null_glVertexAttribI2iv( GLuint index, const GLint *v ) { }
static void null_glVertexAttribI2ivEXT( GLuint index, const GLint *v ) { }
static void null_glVertexAttribI2ui( GLuint index, GLuint x, GLuint y ) { }
static void null_glVertexAttribI2uiEXT( GLuint index, GLuint x, GLuint y ) { }
static void null_glVertexAttribI2uiv( GLuint index, const GLuint *v ) { }
static void null_glVertexAttribI2uivEXT( GLuint index, const GLuint *v ) { }
static void null_glVertexAttribI3i( GLuint index, GLint x, GLint y, GLint z ) { }
static void null_glVertexAttribI3iEXT( GLuint index, GLint x, GLint y, GLint z ) { }
static void null_glVertexAttribI3iv( GLuint index, const GLint *v ) { }
static void null_glVertexAttribI3ivEXT( GLuint index, const GLint *v ) { }
static void null_glVertexAttribI3ui( GLuint index, GLuint x, GLuint y, GLuint z ) { }
static void null_glVertexAttribI3uiEXT( GLuint index, GLuint x, GLuint y, GLuint z ) { }
static void null_glVertexAttribI3uiv( GLuint index, const GLuint *v ) { }
static void null_glVertexAttribI3uivEXT( GLuint index, const GLuint *v ) { }
static void null_glVertexAttribI4bv( GLuint index, const GLbyte *v ) { }
static void null_glVertexAttribI4bvEXT( GLuint index, const GLbyte *v ) { }
static void null_glVertexAttribI4i( GLuint index, GLint x, GLint y, GLint z, GLint w ) { }
static void null_glVertexAttribI4iEXT( GLuint index, GLint x, GLint y, GLint z, GLint w ) { }
static void null_glVertexAttribI4iv( GLuint index, const GLint *v ) { }
static void null_glVertexAttribI4ivEXT( GLuint index, const GLint *v ) { }
static void null_glVertexAttribI4sv( GLuint index, const GLshort *v ) { }
static void null_glVertexAttribI4svEXT( GLuint index, const GLshort *v ) { }
static void null_glVertexAttribI4ubv( GLuint index, const GLubyte *v ) { }
static void null_glVertexAttribI4ubvEXT( GLuint index, const GLubyte *v ) { }
static void null_glVertexAttribI4ui( GLuint index, GLuint x, GLuint y, GLuint z, GLuint w ) { }
static void null_glVertexAttribI4uiEXT( GLuint index, GLuint x, GLuint y, GLuint z, GLuint w ) { }
static void null_glVertexAttribI4uiv( GLuint index, const GLuint *v ) { }
static void null_glVertexAttribI4uivEXT( GLuint index, const GLuint *v ) { }
static void null_glVertexAttribI4usv( GLuint index, const GLushort *v ) { }
static void null_glVertexAttribI4usvEXT( GLuint index, const GLushort *v ) { }
static void null_glVertexAttribIFormat( GLuint attribindex, GLint size, GLenum type, GLuint relativeoffset ) { }
static void null_glVertexAttribIFormatNV( GLuint index, GLint size, GLenum type, GLsizei stride ) { }
static void null_glVertexAttribIPointer( GLuint index, GLint size, GLenum type, GLsizei stride, const void *pointer ) { }
static void null_glVertexAttribIPointerEXT( GLuint index, GLint size, GLenum type, GLsizei stride, const void *pointer ) { }
static void null_glVertexAttribL1d( GLuint index, GLdouble x ) { }
static void null_glVertexAttribL1dEXT( GLuint index, GLdouble x ) { }
static void null_glVertexAttribL1dv( GLuint index, const GLdouble *v ) { }
static void null_glVertexAttribL1dvEXT( GLuint index, const GLdouble *v ) { }
static void null_glVertexAttribL1i64NV( GLuint index, GLint64EXT x ) { }
static void null_glVertexAttribL1i64vNV( GLuint index, const GLint64EXT *v ) { }
static void null_glVertexAttribL1ui64ARB( GLuint index, GLuint64EXT x ) { }
static void null_glVertexAttribL1ui64NV( GLuint index, GLuint64EXT x ) { }
static void null_glVertexAttribL1ui64vARB( GLuint index, const GLuint64EXT *v ) { }
static void null_glVertexAttribL1ui64vNV( GLuint index, const GLuint64EXT *v ) { }
static void null_glVertexAttribL2d( GLuint index, GLdouble x, GLdouble y ) { }
static void null_glVertexAttribL2dEXT( GLuint index, GLdouble x, GLdouble y ) { }
static void null_glVertexAttribL2dv( GLuint index, const GLdouble *v ) { }
static void null_glVertexAttribL2dvEXT( GLuint index, const GLdouble *v ) { }
static void null_glVertexAttribL2i64NV( GLuint index, GLint64EXT x, GLint64EXT y ) { }
static void null_glVertexAttribL2i64vNV( GLuint index, const GLint64EXT *v ) { }
static void null_glVertexAttribL2ui64NV( GLuint index, GLuint64EXT x, GLuint64EXT y ) { }
static void null_glVertexAttribL2ui64vNV( GLuint index, const GLuint64EXT *v ) { }
static void null_glVertexAttribL3d( GLuint index, GLdouble x, GLdouble y, GLdouble z ) { }
static void null_glVertexAttribL3dEXT( GLuint index, GLdouble x, GLdouble y, GLdouble z ) { }
static void null_glVertexAttribL3dv( GLuint index, const GLdouble *v ) { }
static void null_glVertexAttribL3dvEXT( GLuint index, const GLdouble *v ) { }
static void null_glVertexAttribL3i64NV( GLuint index, GLint64EXT x, GLint64EXT y, GLint64EXT z ) { }
static void null_glVertexAttribL3i64vNV( GLuint index, const GLint64EXT *v ) { }
static void null_glVertexAttribL3ui64NV( GLuint index, GLuint64EXT x, GLuint64EXT y, GLuint64EXT z ) { }
static void null_glVertexAttribL3ui64vNV( GLuint index, const GLuint64EXT *v ) { }
static void null_glVertexAttribL4d( GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w ) { }
static void null_glVertexAttribL4dEXT( GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w ) { }
static void null_glVertexAttribL4dv( GLuint index, const GLdouble *v ) { }
static void null_glVertexAttribL4dvEXT( GLuint index, const GLdouble *v ) { }
static void null_glVertexAttribL4i64NV( GLuint index, GLint64EXT x, GLint64EXT y, GLint64EXT z, GLint64EXT w ) { }
static void null_glVertexAttribL4i64vNV( GLuint index, const GLint64EXT *v ) { }
static void null_glVertexAttribL4ui64NV( GLuint index, GLuint64EXT x, GLuint64EXT y, GLuint64EXT z, GLuint64EXT w ) { }
static void null_glVertexAttribL4ui64vNV( GLuint index, const GLuint64EXT *v ) { }
static void null_glVertexAttribLFormat( GLuint attribindex, GLint size, GLenum type, GLuint relativeoffset ) { }
static void null_glVertexAttribLFormatNV( GLuint index, GLint size, GLenum type, GLsizei stride ) { }
static void null_glVertexAttribLPointer( GLuint index, GLint size, GLenum type, GLsizei stride, const void *pointer ) { }
static void null_glVertexAttribLPointerEXT( GLuint index, GLint size, GLenum type, GLsizei stride, const void *pointer ) { }
static void null_glVertexAttribP1ui( GLuint index, GLenum type, GLboolean normalized, GLuint value ) { }
static void null_glVertexAttribP1uiv( GLuint index, GLenum type, GLboolean normalized, const GLuint *value ) { }
static void null_glVertexAttribP2ui( GLuint index, GLenum type, GLboolean normalized, GLuint value ) { }
static void null_glVertexAttribP2uiv( GLuint index, GLenum type, GLboolean normalized, const GLuint *value ) { }
static void null_glVertexAttribP3ui( GLuint index, GLenum type, GLboolean normalized, GLuint value ) { }
static void null_glVertexAttribP3uiv( GLuint index, GLenum type, GLboolean normalized, const GLuint *value ) { }
static void null_glVertexAttribP4ui( GLuint index, GLenum type, GLboolean normalized, GLuint value ) { }
static void null_glVertexAttribP4uiv( GLuint index, GLenum type, GLboolean normalized, const GLuint *value ) { }
static void null_glVertexAttribParameteriAMD( GLuint index, GLenum pname, GLint param ) { }
static void null_glVertexAttribPointer( GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer ) { }
static void null_glVertexAttribPointerARB( GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer ) { }
static void null_glVertexAttribPointerNV( GLuint index, GLint fsize, GLenum type, GLsizei stride, const void *pointer ) { }
static void null_glVertexAttribs1dvNV( GLuint index, GLsizei count, const GLdouble *v ) { }
static void null_glVertexAttribs1fvNV( GLuint index, GLsizei count, const GLfloat *v ) { }
static void null_glVertexAttribs1hvNV( GLuint index, GLsizei n, const GLhalfNV *v ) { }
static void null_glVertexAttribs1svNV( GLuint index, GLsizei count, const GLshort *v ) { }
static void null_glVertexAttribs2dvNV( GLuint index, GLsizei count, const GLdouble *v ) { }
static void null_glVertexAttribs2fvNV( GLuint index, GLsizei count, const GLfloat *v ) { }
static void null_glVertexAttribs2hvNV( GLuint index, GLsizei n, const GLhalfNV *v ) { }
static void null_glVertexAttribs2svNV( GLuint index, GLsizei count, const GLshort *v ) { }
static void null_glVertexAttribs3dvNV( GLuint index, GLsizei count, const GLdouble *v ) { }
static void null_glVertexAttribs3fvNV( GLuint index, GLsizei count, const GLfloat *v ) { }
static void null_glVertexAttribs3hvNV( GLuint index, GLsizei n, const GLhalfNV *v ) { }
static void null_glVertexAttribs3svNV( GLuint index, GLsizei count, const GLshort *v ) { }
static void null_glVertexAttribs4dvNV( GLuint index, GLsizei count, const GLdouble *v ) { }
static void null_glVertexAttribs4fvNV( GLuint index, GLsizei count, const GLfloat *v ) { }
static void null_glVertexAttribs4hvNV( GLuint index, GLsizei n, const GLhalfNV *v ) { }
static void null_glVertexAttribs4svNV( GLuint index, GLsizei count, const GLshort *v ) { }
static void null_glVertexAttribs4ubvNV( GLuint index, GLsizei count, const GLubyte *v ) { }
static void null_glVertexBindingDivisor( GLuint bindingindex, GLuint divisor ) { }
static void null_glVertexBlendARB( GLint count ) { }
static void null_glVertexBlendEnvfATI( GLenum pname, GLfloat param ) { }
static void null_glVertexBlendEnviATI( GLenum pname, GLint param ) { }
static void null_glVertexFormatNV( GLint size, GLenum type, GLsizei stride ) { }
static void null_glVertexP2ui( GLenum type, GLuint value ) { }
static void null_glVertexP2uiv( GLenum type, const GLuint *value ) { }
static void null_glVertexP3ui( GLenum type, GLuint value ) { }
static void null_glVertexP3uiv( GLenum type, const GLuint *value ) { }
static void null_glVertexP4ui( GLenum type, GLuint value ) { }
static void null_glVertexP4uiv( GLenum type, const GLuint *value ) { }
static void null_glVertexPointerEXT( GLint size, GLenum type, GLsizei stride, GLsizei count, const void *pointer ) { }
static void null_glVertexPointerListIBM( GLint size, GLenum type, GLint stride, const void **pointer, GLint ptrstride ) { }
static void null_glVertexPointervINTEL( GLint size, GLenum type, const void **pointer ) { }
static void null_glVertexStream1dATI( GLenum stream, GLdouble x ) { }
static void null_glVertexStream1dvATI( GLenum stream, const GLdouble *coords ) { }
static void null_glVertexStream1fATI( GLenum stream, GLfloat x ) { }
static void null_glVertexStream1fvATI( GLenum stream, const GLfloat *coords ) { }
static void null_glVertexStream1iATI( GLenum stream, GLint x ) { }
static void null_glVertexStream1ivATI( GLenum stream, const GLint *coords ) { }
static void null_glVertexStream1sATI( GLenum stream, GLshort x ) { }
static void null_glVertexStream1svATI( GLenum stream, const GLshort *coords ) { }
static void null_glVertexStream2dATI( GLenum stream, GLdouble x, GLdouble y ) { }
static void null_glVertexStream2dvATI( GLenum stream, const GLdouble *coords ) { }
static void null_glVertexStream2fATI( GLenum stream, GLfloat x, GLfloat y ) { }
static void null_glVertexStream2fvATI( GLenum stream, const GLfloat *coords ) { }
static void null_glVertexStream2iATI( GLenum stream, GLint x, GLint y ) { }
static void null_glVertexStream2ivATI( GLenum stream, const GLint *coords ) { }
static void null_glVertexStream2sATI( GLenum stream, GLshort x, GLshort y ) { }
static void null_glVertexStream2svATI( GLenum stream, const GLshort *coords ) { }
static void null_glVertexStream3dATI( GLenum stream, GLdouble x, GLdouble y, GLdouble z ) { }
static void null_glVertexStream3dvATI( GLenum stream, const GLdouble *coords ) { }
static void null_glVertexStream3fATI( GLenum stream, GLfloat x, GLfloat y, GLfloat z ) { }
static void null_glVertexStream3fvATI( GLenum stream, const GLfloat *coords ) { }
static void null_glVertexStream3iATI( GLenum stream, GLint x, GLint y, GLint z ) { }
static void null_glVertexStream3ivATI( GLenum stream, const GLint *coords ) { }
static void null_glVertexStream3sATI( GLenum stream, GLshort x, GLshort y, GLshort z ) { }
static void null_glVertexStream3svATI( GLenum stream, const GLshort *coords ) { }
static void null_glVertexStream4dATI( GLenum stream, GLdouble x, GLdouble y, GLdouble z, GLdouble w ) { }
static void null_glVertexStream4dvATI( GLenum stream, const GLdouble *coords ) { }
static void null_glVertexStream4fATI( GLenum stream, GLfloat x, GLfloat y, GLfloat z, GLfloat w ) { }
static void null_glVertexStream4fvATI( GLenum stream, const GLfloat *coords ) { }
static void null_glVertexStream4iATI( GLenum stream, GLint x, GLint y, GLint z, GLint w ) { }
static void null_glVertexStream4ivATI( GLenum stream, const GLint *coords ) { }
static void null_glVertexStream4sATI( GLenum stream, GLshort x, GLshort y, GLshort z, GLshort w ) { }
static void null_glVertexStream4svATI( GLenum stream, const GLshort *coords ) { }
static void null_glVertexWeightPointerEXT( GLint size, GLenum type, GLsizei stride, const void *pointer ) { }
static void null_glVertexWeightfEXT( GLfloat weight ) { }
static void null_glVertexWeightfvEXT( const GLfloat *weight ) { }
static void null_glVertexWeighthNV( GLhalfNV weight ) { }
static void null_glVertexWeighthvNV( const GLhalfNV *weight ) { }
static GLenum null_glVideoCaptureNV( GLuint video_capture_slot, GLuint *sequence_num, GLuint64EXT *capture_time ) { return 0; }
static void null_glVideoCaptureStreamParameterdvNV( GLuint video_capture_slot, GLuint stream, GLenum pname, const GLdouble *params ) { }
static void null_glVideoCaptureStreamParameterfvNV( GLuint video_capture_slot, GLuint stream, GLenum pname, const GLfloat *params ) { }
static void null_glVideoCaptureStreamParameterivNV( GLuint video_capture_slot, GLuint stream, GLenum pname, const GLint *params ) { }
static void null_glViewportArrayv( GLuint first, GLsizei count, const GLfloat *v ) { }
static void null_glViewportIndexedf( GLuint index, GLfloat x, GLfloat y, GLfloat w, GLfloat h ) { }
static void null_glViewportIndexedfv( GLuint index, const GLfloat *v ) { }
static void null_glViewportPositionWScaleNV( GLuint index, GLfloat xcoeff, GLfloat ycoeff ) { }
static void null_glViewportSwizzleNV( GLuint index, GLenum swizzlex, GLenum swizzley, GLenum swizzlez, GLenum swizzlew ) { }
static void null_glWaitSemaphoreEXT( GLuint semaphore, GLuint numBufferBarriers, const GLuint *buffers, GLuint numTextureBarriers, const GLuint *textures, const GLenum *srcLayouts ) { }
static void null_glWaitSemaphoreui64NVX( GLuint waitGpu, GLsizei fenceObjectCount, const GLuint *semaphoreArray, const GLuint64 *fenceValueArray ) { }
static void null_glWaitSync( GLsync sync, GLbitfield flags, GLuint64 timeout ) { }
static void null_glWaitVkSemaphoreNV( GLuint64 vkSemaphore ) { }
static void null_glWeightPathsNV( GLuint resultPath, GLsizei numPaths, const GLuint *paths, const GLfloat *weights ) { }
static void null_glWeightPointerARB( GLint size, GLenum type, GLsizei stride, const void *pointer ) { }
static void null_glWeightbvARB( GLint size, const GLbyte *weights ) { }
static void null_glWeightdvARB( GLint size, const GLdouble *weights ) { }
static void null_glWeightfvARB( GLint size, const GLfloat *weights ) { }
static void null_glWeightivARB( GLint size, const GLint *weights ) { }
static void null_glWeightsvARB( GLint size, const GLshort *weights ) { }
static void null_glWeightubvARB( GLint size, const GLubyte *weights ) { }
static void null_glWeightuivARB( GLint size, const GLuint *weights ) { }
static void null_glWeightusvARB( GLint size, const GLushort *weights ) { }
static void null_glWindowPos2d( GLdouble x, GLdouble y ) { }
static void null_glWindowPos2dARB( GLdouble x, GLdouble y ) { }
static void null_glWindowPos2dMESA( GLdouble x, GLdouble y ) { }
static void null_glWindowPos2dv( const GLdouble *v ) { }
static void null_glWindowPos2dvARB( const GLdouble *v ) { }
static void null_glWindowPos2dvMESA( const GLdouble *v ) { }
static void null_glWindowPos2f( GLfloat x, GLfloat y ) { }
static void null_glWindowPos2fARB( GLfloat x, GLfloat y ) { }
static void null_glWindowPos2fMESA( GLfloat x, GLfloat y ) { }
static void null_glWindowPos2fv( const GLfloat *v ) { }
static void null_glWindowPos2fvARB( const GLfloat *v ) { }
static void null_glWindowPos2fvMESA( const GLfloat *v ) { }
static void null_glWindowPos2i( GLint x, GLint y ) { }
static void null_glWindowPos2iARB( GLint x, GLint y ) { }
static void null_glWindowPos2iMESA( GLint x, GLint y ) { }
static void null_glWindowPos2iv( const GLint *v ) { }
static void null_glWindowPos2ivARB( const GLint *v ) { }
static void null_glWindowPos2ivMESA( const GLint *v ) { }
static void null_glWindowPos2s( GLshort x, GLshort y ) { }
static void null_glWindowPos2sARB( GLshort x, GLshort y ) { }
static void null_glWindowPos2sMESA( GLshort x, GLshort y ) { }
static void null_glWindowPos2sv( const GLshort *v ) { }
static void null_glWindowPos2svARB( const GLshort *v ) { }
static void null_glWindowPos2svMESA( const GLshort *v ) { }
static void null_glWindowPos3d( GLdouble x, GLdouble y, GLdouble z ) { }
static void null_glWindowPos3dARB( GLdouble x, GLdouble y, GLdouble z ) { }
static void null_glWindowPos3dMESA( GLdouble x, GLdouble y, GLdouble z ) { }
static void null_glWindowPos3dv( const GLdouble *v ) { }
static void null_glWindowPos3dvARB( const GLdouble *v ) { }
static void null_glWindowPos3dvMESA( const GLdouble *v ) { }
static void null_glWindowPos3f( GLfloat x, GLfloat y, GLfloat z ) { }
static void null_glWindowPos3fARB( GLfloat x, GLfloat y, GLfloat z ) { }
static void null_glWindowPos3fMESA( GLfloat x, GLfloat y, GLfloat z ) { }
static void null_glWindowPos3fv( const GLfloat *v ) { }
static void null_glWindowPos3fvARB( const GLfloat *v ) { }
static void null_glWindowPos3fvMESA( const GLfloat *v ) { }
static void null_glWindowPos3i( GLint x, GLint y, GLint z ) { }
static void null_glWindowPos3iARB( GLint x, GLint y, GLint z ) { }
static void null_glWindowPos3iMESA( GLint x, GLint y, GLint z ) { }
static void null_glWindowPos3iv( const GLint *v ) { }
static void null_glWindowPos3ivARB( const GLint *v ) { }
static void null_glWindowPos3ivMESA( const GLint *v ) { }
static void null_glWindowPos3s( GLshort x, GLshort y, GLshort z ) { }
static void null_glWindowPos3sARB( GLshort x, GLshort y, GLshort z ) { }
static void null_glWindowPos3sMESA( GLshort x, GLshort y, GLshort z ) { }
static void null_glWindowPos3sv( const GLshort *v ) { }
static void null_glWindowPos3svARB( const GLshort *v ) { }
static void null_glWindowPos3svMESA( const GLshort *v ) { }
static void null_glWindowPos4dMESA( GLdouble x, GLdouble y, GLdouble z, GLdouble w ) { }
static void null_glWindowPos4dvMESA( const GLdouble *v ) { }
static void null_glWindowPos4fMESA( GLfloat x, GLfloat y, GLfloat z, GLfloat w ) { }
static void null_glWindowPos4fvMESA( const GLfloat *v ) { }
static void null_glWindowPos4iMESA( GLint x, GLint y, GLint z, GLint w ) { }
static void null_glWindowPos4ivMESA( const GLint *v ) { }
static void null_glWindowPos4sMESA( GLshort x, GLshort y, GLshort z, GLshort w ) { }
static void null_glWindowPos4svMESA( const GLshort *v ) { }
static void null_glWindowRectanglesEXT( GLenum mode, GLsizei count, const GLint *box ) { }
static void null_glWriteMaskEXT( GLuint res, GLuint in, GLenum outX, GLenum outY, GLenum outZ, GLenum outW ) { }
static void * null_wglAllocateMemoryNV( GLsizei size, GLfloat readfreq, GLfloat writefreq, GLfloat priority ) { return 0; }
static BOOL null_wglBindTexImageARB( struct wgl_pbuffer * hPbuffer, int iBuffer ) { return 0; }
static BOOL null_wglChoosePixelFormatARB( HDC hdc, const int *piAttribIList, const FLOAT *pfAttribFList, UINT nMaxFormats, int *piFormats, UINT *nNumFormats ) { return 0; }
static struct wgl_context * null_wglCreateContextAttribsARB( HDC hDC, struct wgl_context * hShareContext, const int *attribList ) { return 0; }
static struct wgl_pbuffer * null_wglCreatePbufferARB( HDC hDC, int iPixelFormat, int iWidth, int iHeight, const int *piAttribList ) { return 0; }
static BOOL null_wglDestroyPbufferARB( struct wgl_pbuffer * hPbuffer ) { return 0; }
static void null_wglFreeMemoryNV( void *pointer ) { }
static HDC null_wglGetCurrentReadDCARB(void) { return 0; }
static const char * null_wglGetExtensionsStringARB( HDC hdc ) { return 0; }
static const char * null_wglGetExtensionsStringEXT(void) { return 0; }
static HDC null_wglGetPbufferDCARB( struct wgl_pbuffer * hPbuffer ) { return 0; }
static BOOL null_wglGetPixelFormatAttribfvARB( HDC hdc, int iPixelFormat, int iLayerPlane, UINT nAttributes, const int *piAttributes, FLOAT *pfValues ) { return 0; }
static BOOL null_wglGetPixelFormatAttribivARB( HDC hdc, int iPixelFormat, int iLayerPlane, UINT nAttributes, const int *piAttributes, int *piValues ) { return 0; }
static int null_wglGetSwapIntervalEXT(void) { return 0; }
static BOOL null_wglMakeContextCurrentARB( HDC hDrawDC, HDC hReadDC, struct wgl_context * hglrc ) { return 0; }
static BOOL null_wglQueryCurrentRendererIntegerWINE( GLenum attribute, GLuint *value ) { return 0; }
static const GLchar * null_wglQueryCurrentRendererStringWINE( GLenum attribute ) { return 0; }
static BOOL null_wglQueryPbufferARB( struct wgl_pbuffer * hPbuffer, int iAttribute, int *piValue ) { return 0; }
static BOOL null_wglQueryRendererIntegerWINE( HDC dc, GLint renderer, GLenum attribute, GLuint *value ) { return 0; }
static const GLchar * null_wglQueryRendererStringWINE( HDC dc, GLint renderer, GLenum attribute ) { return 0; }
static int null_wglReleasePbufferDCARB( struct wgl_pbuffer * hPbuffer, HDC hDC ) { return 0; }
static BOOL null_wglReleaseTexImageARB( struct wgl_pbuffer * hPbuffer, int iBuffer ) { return 0; }
static BOOL null_wglSetPbufferAttribARB( struct wgl_pbuffer * hPbuffer, const int *piAttribList ) { return 0; }
static BOOL null_wglSetPixelFormatWINE( HDC hdc, int format ) { return 0; }
static BOOL null_wglSwapIntervalEXT( int interval ) { return 0; }

struct opengl_funcs null_opengl_funcs =
{
    {
        null_wglCopyContext,
        null_wglCreateContext,
        null_wglDeleteContext,
        null_wglDescribePixelFormat,
        null_wglGetPixelFormat,
        null_wglGetProcAddress,
        null_wglMakeCurrent,
        null_wglSetPixelFormat,
        null_wglShareLists,
        null_wglSwapBuffers,
    },
    {
        null_glAccum,
        null_glAlphaFunc,
        null_glAreTexturesResident,
        null_glArrayElement,
        null_glBegin,
        null_glBindTexture,
        null_glBitmap,
        null_glBlendFunc,
        null_glCallList,
        null_glCallLists,
        null_glClear,
        null_glClearAccum,
        null_glClearColor,
        null_glClearDepth,
        null_glClearIndex,
        null_glClearStencil,
        null_glClipPlane,
        null_glColor3b,
        null_glColor3bv,
        null_glColor3d,
        null_glColor3dv,
        null_glColor3f,
        null_glColor3fv,
        null_glColor3i,
        null_glColor3iv,
        null_glColor3s,
        null_glColor3sv,
        null_glColor3ub,
        null_glColor3ubv,
        null_glColor3ui,
        null_glColor3uiv,
        null_glColor3us,
        null_glColor3usv,
        null_glColor4b,
        null_glColor4bv,
        null_glColor4d,
        null_glColor4dv,
        null_glColor4f,
        null_glColor4fv,
        null_glColor4i,
        null_glColor4iv,
        null_glColor4s,
        null_glColor4sv,
        null_glColor4ub,
        null_glColor4ubv,
        null_glColor4ui,
        null_glColor4uiv,
        null_glColor4us,
        null_glColor4usv,
        null_glColorMask,
        null_glColorMaterial,
        null_glColorPointer,
        null_glCopyPixels,
        null_glCopyTexImage1D,
        null_glCopyTexImage2D,
        null_glCopyTexSubImage1D,
        null_glCopyTexSubImage2D,
        null_glCullFace,
        null_glDeleteLists,
        null_glDeleteTextures,
        null_glDepthFunc,
        null_glDepthMask,
        null_glDepthRange,
        null_glDisable,
        null_glDisableClientState,
        null_glDrawArrays,
        null_glDrawBuffer,
        null_glDrawElements,
        null_glDrawPixels,
        null_glEdgeFlag,
        null_glEdgeFlagPointer,
        null_glEdgeFlagv,
        null_glEnable,
        null_glEnableClientState,
        null_glEnd,
        null_glEndList,
        null_glEvalCoord1d,
        null_glEvalCoord1dv,
        null_glEvalCoord1f,
        null_glEvalCoord1fv,
        null_glEvalCoord2d,
        null_glEvalCoord2dv,
        null_glEvalCoord2f,
        null_glEvalCoord2fv,
        null_glEvalMesh1,
        null_glEvalMesh2,
        null_glEvalPoint1,
        null_glEvalPoint2,
        null_glFeedbackBuffer,
        null_glFinish,
        null_glFlush,
        null_glFogf,
        null_glFogfv,
        null_glFogi,
        null_glFogiv,
        null_glFrontFace,
        null_glFrustum,
        null_glGenLists,
        null_glGenTextures,
        null_glGetBooleanv,
        null_glGetClipPlane,
        null_glGetDoublev,
        null_glGetError,
        null_glGetFloatv,
        null_glGetIntegerv,
        null_glGetLightfv,
        null_glGetLightiv,
        null_glGetMapdv,
        null_glGetMapfv,
        null_glGetMapiv,
        null_glGetMaterialfv,
        null_glGetMaterialiv,
        null_glGetPixelMapfv,
        null_glGetPixelMapuiv,
        null_glGetPixelMapusv,
        null_glGetPointerv,
        null_glGetPolygonStipple,
        null_glGetString,
        null_glGetTexEnvfv,
        null_glGetTexEnviv,
        null_glGetTexGendv,
        null_glGetTexGenfv,
        null_glGetTexGeniv,
        null_glGetTexImage,
        null_glGetTexLevelParameterfv,
        null_glGetTexLevelParameteriv,
        null_glGetTexParameterfv,
        null_glGetTexParameteriv,
        null_glHint,
        null_glIndexMask,
        null_glIndexPointer,
        null_glIndexd,
        null_glIndexdv,
        null_glIndexf,
        null_glIndexfv,
        null_glIndexi,
        null_glIndexiv,
        null_glIndexs,
        null_glIndexsv,
        null_glIndexub,
        null_glIndexubv,
        null_glInitNames,
        null_glInterleavedArrays,
        null_glIsEnabled,
        null_glIsList,
        null_glIsTexture,
        null_glLightModelf,
        null_glLightModelfv,
        null_glLightModeli,
        null_glLightModeliv,
        null_glLightf,
        null_glLightfv,
        null_glLighti,
        null_glLightiv,
        null_glLineStipple,
        null_glLineWidth,
        null_glListBase,
        null_glLoadIdentity,
        null_glLoadMatrixd,
        null_glLoadMatrixf,
        null_glLoadName,
        null_glLogicOp,
        null_glMap1d,
        null_glMap1f,
        null_glMap2d,
        null_glMap2f,
        null_glMapGrid1d,
        null_glMapGrid1f,
        null_glMapGrid2d,
        null_glMapGrid2f,
        null_glMaterialf,
        null_glMaterialfv,
        null_glMateriali,
        null_glMaterialiv,
        null_glMatrixMode,
        null_glMultMatrixd,
        null_glMultMatrixf,
        null_glNewList,
        null_glNormal3b,
        null_glNormal3bv,
        null_glNormal3d,
        null_glNormal3dv,
        null_glNormal3f,
        null_glNormal3fv,
        null_glNormal3i,
        null_glNormal3iv,
        null_glNormal3s,
        null_glNormal3sv,
        null_glNormalPointer,
        null_glOrtho,
        null_glPassThrough,
        null_glPixelMapfv,
        null_glPixelMapuiv,
        null_glPixelMapusv,
        null_glPixelStoref,
        null_glPixelStorei,
        null_glPixelTransferf,
        null_glPixelTransferi,
        null_glPixelZoom,
        null_glPointSize,
        null_glPolygonMode,
        null_glPolygonOffset,
        null_glPolygonStipple,
        null_glPopAttrib,
        null_glPopClientAttrib,
        null_glPopMatrix,
        null_glPopName,
        null_glPrioritizeTextures,
        null_glPushAttrib,
        null_glPushClientAttrib,
        null_glPushMatrix,
        null_glPushName,
        null_glRasterPos2d,
        null_glRasterPos2dv,
        null_glRasterPos2f,
        null_glRasterPos2fv,
        null_glRasterPos2i,
        null_glRasterPos2iv,
        null_glRasterPos2s,
        null_glRasterPos2sv,
        null_glRasterPos3d,
        null_glRasterPos3dv,
        null_glRasterPos3f,
        null_glRasterPos3fv,
        null_glRasterPos3i,
        null_glRasterPos3iv,
        null_glRasterPos3s,
        null_glRasterPos3sv,
        null_glRasterPos4d,
        null_glRasterPos4dv,
        null_glRasterPos4f,
        null_glRasterPos4fv,
        null_glRasterPos4i,
        null_glRasterPos4iv,
        null_glRasterPos4s,
        null_glRasterPos4sv,
        null_glReadBuffer,
        null_glReadPixels,
        null_glRectd,
        null_glRectdv,
        null_glRectf,
        null_glRectfv,
        null_glRecti,
        null_glRectiv,
        null_glRects,
        null_glRectsv,
        null_glRenderMode,
        null_glRotated,
        null_glRotatef,
        null_glScaled,
        null_glScalef,
        null_glScissor,
        null_glSelectBuffer,
        null_glShadeModel,
        null_glStencilFunc,
        null_glStencilMask,
        null_glStencilOp,
        null_glTexCoord1d,
        null_glTexCoord1dv,
        null_glTexCoord1f,
        null_glTexCoord1fv,
        null_glTexCoord1i,
        null_glTexCoord1iv,
        null_glTexCoord1s,
        null_glTexCoord1sv,
        null_glTexCoord2d,
        null_glTexCoord2dv,
        null_glTexCoord2f,
        null_glTexCoord2fv,
        null_glTexCoord2i,
        null_glTexCoord2iv,
        null_glTexCoord2s,
        null_glTexCoord2sv,
        null_glTexCoord3d,
        null_glTexCoord3dv,
        null_glTexCoord3f,
        null_glTexCoord3fv,
        null_glTexCoord3i,
        null_glTexCoord3iv,
        null_glTexCoord3s,
        null_glTexCoord3sv,
        null_glTexCoord4d,
        null_glTexCoord4dv,
        null_glTexCoord4f,
        null_glTexCoord4fv,
        null_glTexCoord4i,
        null_glTexCoord4iv,
        null_glTexCoord4s,
        null_glTexCoord4sv,
        null_glTexCoordPointer,
        null_glTexEnvf,
        null_glTexEnvfv,
        null_glTexEnvi,
        null_glTexEnviv,
        null_glTexGend,
        null_glTexGendv,
        null_glTexGenf,
        null_glTexGenfv,
        null_glTexGeni,
        null_glTexGeniv,
        null_glTexImage1D,
        null_glTexImage2D,
        null_glTexParameterf,
        null_glTexParameterfv,
        null_glTexParameteri,
        null_glTexParameteriv,
        null_glTexSubImage1D,
        null_glTexSubImage2D,
        null_glTranslated,
        null_glTranslatef,
        null_glVertex2d,
        null_glVertex2dv,
        null_glVertex2f,
        null_glVertex2fv,
        null_glVertex2i,
        null_glVertex2iv,
        null_glVertex2s,
        null_glVertex2sv,
        null_glVertex3d,
        null_glVertex3dv,
        null_glVertex3f,
        null_glVertex3fv,
        null_glVertex3i,
        null_glVertex3iv,
        null_glVertex3s,
        null_glVertex3sv,
        null_glVertex4d,
        null_glVertex4dv,
        null_glVertex4f,
        null_glVertex4fv,
        null_glVertex4i,
        null_glVertex4iv,
        null_glVertex4s,
        null_glVertex4sv,
        null_glVertexPointer,
        null_glViewport,
    },
    {
        null_glAccumxOES,
        null_glAcquireKeyedMutexWin32EXT,
        null_glActiveProgramEXT,
        null_glActiveShaderProgram,
        null_glActiveStencilFaceEXT,
        null_glActiveTexture,
        null_glActiveTextureARB,
        null_glActiveVaryingNV,
        null_glAlphaFragmentOp1ATI,
        null_glAlphaFragmentOp2ATI,
        null_glAlphaFragmentOp3ATI,
        null_glAlphaFuncxOES,
        null_glAlphaToCoverageDitherControlNV,
        null_glApplyFramebufferAttachmentCMAAINTEL,
        null_glApplyTextureEXT,
        null_glAreProgramsResidentNV,
        null_glAreTexturesResidentEXT,
        null_glArrayElementEXT,
        null_glArrayObjectATI,
        null_glAsyncCopyBufferSubDataNVX,
        null_glAsyncCopyImageSubDataNVX,
        null_glAsyncMarkerSGIX,
        null_glAttachObjectARB,
        null_glAttachShader,
        null_glBeginConditionalRender,
        null_glBeginConditionalRenderNV,
        null_glBeginConditionalRenderNVX,
        null_glBeginFragmentShaderATI,
        null_glBeginOcclusionQueryNV,
        null_glBeginPerfMonitorAMD,
        null_glBeginPerfQueryINTEL,
        null_glBeginQuery,
        null_glBeginQueryARB,
        null_glBeginQueryIndexed,
        null_glBeginTransformFeedback,
        null_glBeginTransformFeedbackEXT,
        null_glBeginTransformFeedbackNV,
        null_glBeginVertexShaderEXT,
        null_glBeginVideoCaptureNV,
        null_glBindAttribLocation,
        null_glBindAttribLocationARB,
        null_glBindBuffer,
        null_glBindBufferARB,
        null_glBindBufferBase,
        null_glBindBufferBaseEXT,
        null_glBindBufferBaseNV,
        null_glBindBufferOffsetEXT,
        null_glBindBufferOffsetNV,
        null_glBindBufferRange,
        null_glBindBufferRangeEXT,
        null_glBindBufferRangeNV,
        null_glBindBuffersBase,
        null_glBindBuffersRange,
        null_glBindFragDataLocation,
        null_glBindFragDataLocationEXT,
        null_glBindFragDataLocationIndexed,
        null_glBindFragmentShaderATI,
        null_glBindFramebuffer,
        null_glBindFramebufferEXT,
        null_glBindImageTexture,
        null_glBindImageTextureEXT,
        null_glBindImageTextures,
        null_glBindLightParameterEXT,
        null_glBindMaterialParameterEXT,
        null_glBindMultiTextureEXT,
        null_glBindParameterEXT,
        null_glBindProgramARB,
        null_glBindProgramNV,
        null_glBindProgramPipeline,
        null_glBindRenderbuffer,
        null_glBindRenderbufferEXT,
        null_glBindSampler,
        null_glBindSamplers,
        null_glBindShadingRateImageNV,
        null_glBindTexGenParameterEXT,
        null_glBindTextureEXT,
        null_glBindTextureUnit,
        null_glBindTextureUnitParameterEXT,
        null_glBindTextures,
        null_glBindTransformFeedback,
        null_glBindTransformFeedbackNV,
        null_glBindVertexArray,
        null_glBindVertexArrayAPPLE,
        null_glBindVertexBuffer,
        null_glBindVertexBuffers,
        null_glBindVertexShaderEXT,
        null_glBindVideoCaptureStreamBufferNV,
        null_glBindVideoCaptureStreamTextureNV,
        null_glBinormal3bEXT,
        null_glBinormal3bvEXT,
        null_glBinormal3dEXT,
        null_glBinormal3dvEXT,
        null_glBinormal3fEXT,
        null_glBinormal3fvEXT,
        null_glBinormal3iEXT,
        null_glBinormal3ivEXT,
        null_glBinormal3sEXT,
        null_glBinormal3svEXT,
        null_glBinormalPointerEXT,
        null_glBitmapxOES,
        null_glBlendBarrierKHR,
        null_glBlendBarrierNV,
        null_glBlendColor,
        null_glBlendColorEXT,
        null_glBlendColorxOES,
        null_glBlendEquation,
        null_glBlendEquationEXT,
        null_glBlendEquationIndexedAMD,
        null_glBlendEquationSeparate,
        null_glBlendEquationSeparateEXT,
        null_glBlendEquationSeparateIndexedAMD,
        null_glBlendEquationSeparatei,
        null_glBlendEquationSeparateiARB,
        null_glBlendEquationi,
        null_glBlendEquationiARB,
        null_glBlendFuncIndexedAMD,
        null_glBlendFuncSeparate,
        null_glBlendFuncSeparateEXT,
        null_glBlendFuncSeparateINGR,
        null_glBlendFuncSeparateIndexedAMD,
        null_glBlendFuncSeparatei,
        null_glBlendFuncSeparateiARB,
        null_glBlendFunci,
        null_glBlendFunciARB,
        null_glBlendParameteriNV,
        null_glBlitFramebuffer,
        null_glBlitFramebufferEXT,
        null_glBlitNamedFramebuffer,
        null_glBufferAddressRangeNV,
        null_glBufferAttachMemoryNV,
        null_glBufferData,
        null_glBufferDataARB,
        null_glBufferPageCommitmentARB,
        null_glBufferParameteriAPPLE,
        null_glBufferRegionEnabled,
        null_glBufferStorage,
        null_glBufferStorageExternalEXT,
        null_glBufferStorageMemEXT,
        null_glBufferSubData,
        null_glBufferSubDataARB,
        null_glCallCommandListNV,
        null_glCheckFramebufferStatus,
        null_glCheckFramebufferStatusEXT,
        null_glCheckNamedFramebufferStatus,
        null_glCheckNamedFramebufferStatusEXT,
        null_glClampColor,
        null_glClampColorARB,
        null_glClearAccumxOES,
        null_glClearBufferData,
        null_glClearBufferSubData,
        null_glClearBufferfi,
        null_glClearBufferfv,
        null_glClearBufferiv,
        null_glClearBufferuiv,
        null_glClearColorIiEXT,
        null_glClearColorIuiEXT,
        null_glClearColorxOES,
        null_glClearDepthdNV,
        null_glClearDepthf,
        null_glClearDepthfOES,
        null_glClearDepthxOES,
        null_glClearNamedBufferData,
        null_glClearNamedBufferDataEXT,
        null_glClearNamedBufferSubData,
        null_glClearNamedBufferSubDataEXT,
        null_glClearNamedFramebufferfi,
        null_glClearNamedFramebufferfv,
        null_glClearNamedFramebufferiv,
        null_glClearNamedFramebufferuiv,
        null_glClearTexImage,
        null_glClearTexSubImage,
        null_glClientActiveTexture,
        null_glClientActiveTextureARB,
        null_glClientActiveVertexStreamATI,
        null_glClientAttribDefaultEXT,
        null_glClientWaitSemaphoreui64NVX,
        null_glClientWaitSync,
        null_glClipControl,
        null_glClipPlanefOES,
        null_glClipPlanexOES,
        null_glColor3fVertex3fSUN,
        null_glColor3fVertex3fvSUN,
        null_glColor3hNV,
        null_glColor3hvNV,
        null_glColor3xOES,
        null_glColor3xvOES,
        null_glColor4fNormal3fVertex3fSUN,
        null_glColor4fNormal3fVertex3fvSUN,
        null_glColor4hNV,
        null_glColor4hvNV,
        null_glColor4ubVertex2fSUN,
        null_glColor4ubVertex2fvSUN,
        null_glColor4ubVertex3fSUN,
        null_glColor4ubVertex3fvSUN,
        null_glColor4xOES,
        null_glColor4xvOES,
        null_glColorFormatNV,
        null_glColorFragmentOp1ATI,
        null_glColorFragmentOp2ATI,
        null_glColorFragmentOp3ATI,
        null_glColorMaskIndexedEXT,
        null_glColorMaski,
        null_glColorP3ui,
        null_glColorP3uiv,
        null_glColorP4ui,
        null_glColorP4uiv,
        null_glColorPointerEXT,
        null_glColorPointerListIBM,
        null_glColorPointervINTEL,
        null_glColorSubTable,
        null_glColorSubTableEXT,
        null_glColorTable,
        null_glColorTableEXT,
        null_glColorTableParameterfv,
        null_glColorTableParameterfvSGI,
        null_glColorTableParameteriv,
        null_glColorTableParameterivSGI,
        null_glColorTableSGI,
        null_glCombinerInputNV,
        null_glCombinerOutputNV,
        null_glCombinerParameterfNV,
        null_glCombinerParameterfvNV,
        null_glCombinerParameteriNV,
        null_glCombinerParameterivNV,
        null_glCombinerStageParameterfvNV,
        null_glCommandListSegmentsNV,
        null_glCompileCommandListNV,
        null_glCompileShader,
        null_glCompileShaderARB,
        null_glCompileShaderIncludeARB,
        null_glCompressedMultiTexImage1DEXT,
        null_glCompressedMultiTexImage2DEXT,
        null_glCompressedMultiTexImage3DEXT,
        null_glCompressedMultiTexSubImage1DEXT,
        null_glCompressedMultiTexSubImage2DEXT,
        null_glCompressedMultiTexSubImage3DEXT,
        null_glCompressedTexImage1D,
        null_glCompressedTexImage1DARB,
        null_glCompressedTexImage2D,
        null_glCompressedTexImage2DARB,
        null_glCompressedTexImage3D,
        null_glCompressedTexImage3DARB,
        null_glCompressedTexSubImage1D,
        null_glCompressedTexSubImage1DARB,
        null_glCompressedTexSubImage2D,
        null_glCompressedTexSubImage2DARB,
        null_glCompressedTexSubImage3D,
        null_glCompressedTexSubImage3DARB,
        null_glCompressedTextureImage1DEXT,
        null_glCompressedTextureImage2DEXT,
        null_glCompressedTextureImage3DEXT,
        null_glCompressedTextureSubImage1D,
        null_glCompressedTextureSubImage1DEXT,
        null_glCompressedTextureSubImage2D,
        null_glCompressedTextureSubImage2DEXT,
        null_glCompressedTextureSubImage3D,
        null_glCompressedTextureSubImage3DEXT,
        null_glConservativeRasterParameterfNV,
        null_glConservativeRasterParameteriNV,
        null_glConvolutionFilter1D,
        null_glConvolutionFilter1DEXT,
        null_glConvolutionFilter2D,
        null_glConvolutionFilter2DEXT,
        null_glConvolutionParameterf,
        null_glConvolutionParameterfEXT,
        null_glConvolutionParameterfv,
        null_glConvolutionParameterfvEXT,
        null_glConvolutionParameteri,
        null_glConvolutionParameteriEXT,
        null_glConvolutionParameteriv,
        null_glConvolutionParameterivEXT,
        null_glConvolutionParameterxOES,
        null_glConvolutionParameterxvOES,
        null_glCopyBufferSubData,
        null_glCopyColorSubTable,
        null_glCopyColorSubTableEXT,
        null_glCopyColorTable,
        null_glCopyColorTableSGI,
        null_glCopyConvolutionFilter1D,
        null_glCopyConvolutionFilter1DEXT,
        null_glCopyConvolutionFilter2D,
        null_glCopyConvolutionFilter2DEXT,
        null_glCopyImageSubData,
        null_glCopyImageSubDataNV,
        null_glCopyMultiTexImage1DEXT,
        null_glCopyMultiTexImage2DEXT,
        null_glCopyMultiTexSubImage1DEXT,
        null_glCopyMultiTexSubImage2DEXT,
        null_glCopyMultiTexSubImage3DEXT,
        null_glCopyNamedBufferSubData,
        null_glCopyPathNV,
        null_glCopyTexImage1DEXT,
        null_glCopyTexImage2DEXT,
        null_glCopyTexSubImage1DEXT,
        null_glCopyTexSubImage2DEXT,
        null_glCopyTexSubImage3D,
        null_glCopyTexSubImage3DEXT,
        null_glCopyTextureImage1DEXT,
        null_glCopyTextureImage2DEXT,
        null_glCopyTextureSubImage1D,
        null_glCopyTextureSubImage1DEXT,
        null_glCopyTextureSubImage2D,
        null_glCopyTextureSubImage2DEXT,
        null_glCopyTextureSubImage3D,
        null_glCopyTextureSubImage3DEXT,
        null_glCoverFillPathInstancedNV,
        null_glCoverFillPathNV,
        null_glCoverStrokePathInstancedNV,
        null_glCoverStrokePathNV,
        null_glCoverageModulationNV,
        null_glCoverageModulationTableNV,
        null_glCreateBuffers,
        null_glCreateCommandListsNV,
        null_glCreateFramebuffers,
        null_glCreateMemoryObjectsEXT,
        null_glCreatePerfQueryINTEL,
        null_glCreateProgram,
        null_glCreateProgramObjectARB,
        null_glCreateProgramPipelines,
        null_glCreateProgressFenceNVX,
        null_glCreateQueries,
        null_glCreateRenderbuffers,
        null_glCreateSamplers,
        null_glCreateShader,
        null_glCreateShaderObjectARB,
        null_glCreateShaderProgramEXT,
        null_glCreateShaderProgramv,
        null_glCreateStatesNV,
        null_glCreateSyncFromCLeventARB,
        null_glCreateTextures,
        null_glCreateTransformFeedbacks,
        null_glCreateVertexArrays,
        null_glCullParameterdvEXT,
        null_glCullParameterfvEXT,
        null_glCurrentPaletteMatrixARB,
        null_glDebugMessageCallback,
        null_glDebugMessageCallbackAMD,
        null_glDebugMessageCallbackARB,
        null_glDebugMessageControl,
        null_glDebugMessageControlARB,
        null_glDebugMessageEnableAMD,
        null_glDebugMessageInsert,
        null_glDebugMessageInsertAMD,
        null_glDebugMessageInsertARB,
        null_glDeformSGIX,
        null_glDeformationMap3dSGIX,
        null_glDeformationMap3fSGIX,
        null_glDeleteAsyncMarkersSGIX,
        null_glDeleteBufferRegion,
        null_glDeleteBuffers,
        null_glDeleteBuffersARB,
        null_glDeleteCommandListsNV,
        null_glDeleteFencesAPPLE,
        null_glDeleteFencesNV,
        null_glDeleteFragmentShaderATI,
        null_glDeleteFramebuffers,
        null_glDeleteFramebuffersEXT,
        null_glDeleteMemoryObjectsEXT,
        null_glDeleteNamedStringARB,
        null_glDeleteNamesAMD,
        null_glDeleteObjectARB,
        null_glDeleteObjectBufferATI,
        null_glDeleteOcclusionQueriesNV,
        null_glDeletePathsNV,
        null_glDeletePerfMonitorsAMD,
        null_glDeletePerfQueryINTEL,
        null_glDeleteProgram,
        null_glDeleteProgramPipelines,
        null_glDeleteProgramsARB,
        null_glDeleteProgramsNV,
        null_glDeleteQueries,
        null_glDeleteQueriesARB,
        null_glDeleteQueryResourceTagNV,
        null_glDeleteRenderbuffers,
        null_glDeleteRenderbuffersEXT,
        null_glDeleteSamplers,
        null_glDeleteSemaphoresEXT,
        null_glDeleteShader,
        null_glDeleteStatesNV,
        null_glDeleteSync,
        null_glDeleteTexturesEXT,
        null_glDeleteTransformFeedbacks,
        null_glDeleteTransformFeedbacksNV,
        null_glDeleteVertexArrays,
        null_glDeleteVertexArraysAPPLE,
        null_glDeleteVertexShaderEXT,
        null_glDepthBoundsEXT,
        null_glDepthBoundsdNV,
        null_glDepthRangeArraydvNV,
        null_glDepthRangeArrayv,
        null_glDepthRangeIndexed,
        null_glDepthRangeIndexeddNV,
        null_glDepthRangedNV,
        null_glDepthRangef,
        null_glDepthRangefOES,
        null_glDepthRangexOES,
        null_glDetachObjectARB,
        null_glDetachShader,
        null_glDetailTexFuncSGIS,
        null_glDisableClientStateIndexedEXT,
        null_glDisableClientStateiEXT,
        null_glDisableIndexedEXT,
        null_glDisableVariantClientStateEXT,
        null_glDisableVertexArrayAttrib,
        null_glDisableVertexArrayAttribEXT,
        null_glDisableVertexArrayEXT,
        null_glDisableVertexAttribAPPLE,
        null_glDisableVertexAttribArray,
        null_glDisableVertexAttribArrayARB,
        null_glDisablei,
        null_glDispatchCompute,
        null_glDispatchComputeGroupSizeARB,
        null_glDispatchComputeIndirect,
        null_glDrawArraysEXT,
        null_glDrawArraysIndirect,
        null_glDrawArraysInstanced,
        null_glDrawArraysInstancedARB,
        null_glDrawArraysInstancedBaseInstance,
        null_glDrawArraysInstancedEXT,
        null_glDrawBufferRegion,
        null_glDrawBuffers,
        null_glDrawBuffersARB,
        null_glDrawBuffersATI,
        null_glDrawCommandsAddressNV,
        null_glDrawCommandsNV,
        null_glDrawCommandsStatesAddressNV,
        null_glDrawCommandsStatesNV,
        null_glDrawElementArrayAPPLE,
        null_glDrawElementArrayATI,
        null_glDrawElementsBaseVertex,
        null_glDrawElementsIndirect,
        null_glDrawElementsInstanced,
        null_glDrawElementsInstancedARB,
        null_glDrawElementsInstancedBaseInstance,
        null_glDrawElementsInstancedBaseVertex,
        null_glDrawElementsInstancedBaseVertexBaseInstance,
        null_glDrawElementsInstancedEXT,
        null_glDrawMeshArraysSUN,
        null_glDrawMeshTasksIndirectNV,
        null_glDrawMeshTasksNV,
        null_glDrawRangeElementArrayAPPLE,
        null_glDrawRangeElementArrayATI,
        null_glDrawRangeElements,
        null_glDrawRangeElementsBaseVertex,
        null_glDrawRangeElementsEXT,
        null_glDrawTextureNV,
        null_glDrawTransformFeedback,
        null_glDrawTransformFeedbackInstanced,
        null_glDrawTransformFeedbackNV,
        null_glDrawTransformFeedbackStream,
        null_glDrawTransformFeedbackStreamInstanced,
        null_glDrawVkImageNV,
        null_glEGLImageTargetTexStorageEXT,
        null_glEGLImageTargetTextureStorageEXT,
        null_glEdgeFlagFormatNV,
        null_glEdgeFlagPointerEXT,
        null_glEdgeFlagPointerListIBM,
        null_glElementPointerAPPLE,
        null_glElementPointerATI,
        null_glEnableClientStateIndexedEXT,
        null_glEnableClientStateiEXT,
        null_glEnableIndexedEXT,
        null_glEnableVariantClientStateEXT,
        null_glEnableVertexArrayAttrib,
        null_glEnableVertexArrayAttribEXT,
        null_glEnableVertexArrayEXT,
        null_glEnableVertexAttribAPPLE,
        null_glEnableVertexAttribArray,
        null_glEnableVertexAttribArrayARB,
        null_glEnablei,
        null_glEndConditionalRender,
        null_glEndConditionalRenderNV,
        null_glEndConditionalRenderNVX,
        null_glEndFragmentShaderATI,
        null_glEndOcclusionQueryNV,
        null_glEndPerfMonitorAMD,
        null_glEndPerfQueryINTEL,
        null_glEndQuery,
        null_glEndQueryARB,
        null_glEndQueryIndexed,
        null_glEndTransformFeedback,
        null_glEndTransformFeedbackEXT,
        null_glEndTransformFeedbackNV,
        null_glEndVertexShaderEXT,
        null_glEndVideoCaptureNV,
        null_glEvalCoord1xOES,
        null_glEvalCoord1xvOES,
        null_glEvalCoord2xOES,
        null_glEvalCoord2xvOES,
        null_glEvalMapsNV,
        null_glEvaluateDepthValuesARB,
        null_glExecuteProgramNV,
        null_glExtractComponentEXT,
        null_glFeedbackBufferxOES,
        null_glFenceSync,
        null_glFinalCombinerInputNV,
        null_glFinishAsyncSGIX,
        null_glFinishFenceAPPLE,
        null_glFinishFenceNV,
        null_glFinishObjectAPPLE,
        null_glFinishTextureSUNX,
        null_glFlushMappedBufferRange,
        null_glFlushMappedBufferRangeAPPLE,
        null_glFlushMappedNamedBufferRange,
        null_glFlushMappedNamedBufferRangeEXT,
        null_glFlushPixelDataRangeNV,
        null_glFlushRasterSGIX,
        null_glFlushStaticDataIBM,
        null_glFlushVertexArrayRangeAPPLE,
        null_glFlushVertexArrayRangeNV,
        null_glFogCoordFormatNV,
        null_glFogCoordPointer,
        null_glFogCoordPointerEXT,
        null_glFogCoordPointerListIBM,
        null_glFogCoordd,
        null_glFogCoorddEXT,
        null_glFogCoorddv,
        null_glFogCoorddvEXT,
        null_glFogCoordf,
        null_glFogCoordfEXT,
        null_glFogCoordfv,
        null_glFogCoordfvEXT,
        null_glFogCoordhNV,
        null_glFogCoordhvNV,
        null_glFogFuncSGIS,
        null_glFogxOES,
        null_glFogxvOES,
        null_glFragmentColorMaterialSGIX,
        null_glFragmentCoverageColorNV,
        null_glFragmentLightModelfSGIX,
        null_glFragmentLightModelfvSGIX,
        null_glFragmentLightModeliSGIX,
        null_glFragmentLightModelivSGIX,
        null_glFragmentLightfSGIX,
        null_glFragmentLightfvSGIX,
        null_glFragmentLightiSGIX,
        null_glFragmentLightivSGIX,
        null_glFragmentMaterialfSGIX,
        null_glFragmentMaterialfvSGIX,
        null_glFragmentMaterialiSGIX,
        null_glFragmentMaterialivSGIX,
        null_glFrameTerminatorGREMEDY,
        null_glFrameZoomSGIX,
        null_glFramebufferDrawBufferEXT,
        null_glFramebufferDrawBuffersEXT,
        null_glFramebufferFetchBarrierEXT,
        null_glFramebufferParameteri,
        null_glFramebufferParameteriMESA,
        null_glFramebufferReadBufferEXT,
        null_glFramebufferRenderbuffer,
        null_glFramebufferRenderbufferEXT,
        null_glFramebufferSampleLocationsfvARB,
        null_glFramebufferSampleLocationsfvNV,
        null_glFramebufferSamplePositionsfvAMD,
        null_glFramebufferTexture,
        null_glFramebufferTexture1D,
        null_glFramebufferTexture1DEXT,
        null_glFramebufferTexture2D,
        null_glFramebufferTexture2DEXT,
        null_glFramebufferTexture3D,
        null_glFramebufferTexture3DEXT,
        null_glFramebufferTextureARB,
        null_glFramebufferTextureEXT,
        null_glFramebufferTextureFaceARB,
        null_glFramebufferTextureFaceEXT,
        null_glFramebufferTextureLayer,
        null_glFramebufferTextureLayerARB,
        null_glFramebufferTextureLayerEXT,
        null_glFramebufferTextureMultiviewOVR,
        null_glFreeObjectBufferATI,
        null_glFrustumfOES,
        null_glFrustumxOES,
        null_glGenAsyncMarkersSGIX,
        null_glGenBuffers,
        null_glGenBuffersARB,
        null_glGenFencesAPPLE,
        null_glGenFencesNV,
        null_glGenFragmentShadersATI,
        null_glGenFramebuffers,
        null_glGenFramebuffersEXT,
        null_glGenNamesAMD,
        null_glGenOcclusionQueriesNV,
        null_glGenPathsNV,
        null_glGenPerfMonitorsAMD,
        null_glGenProgramPipelines,
        null_glGenProgramsARB,
        null_glGenProgramsNV,
        null_glGenQueries,
        null_glGenQueriesARB,
        null_glGenQueryResourceTagNV,
        null_glGenRenderbuffers,
        null_glGenRenderbuffersEXT,
        null_glGenSamplers,
        null_glGenSemaphoresEXT,
        null_glGenSymbolsEXT,
        null_glGenTexturesEXT,
        null_glGenTransformFeedbacks,
        null_glGenTransformFeedbacksNV,
        null_glGenVertexArrays,
        null_glGenVertexArraysAPPLE,
        null_glGenVertexShadersEXT,
        null_glGenerateMipmap,
        null_glGenerateMipmapEXT,
        null_glGenerateMultiTexMipmapEXT,
        null_glGenerateTextureMipmap,
        null_glGenerateTextureMipmapEXT,
        null_glGetActiveAtomicCounterBufferiv,
        null_glGetActiveAttrib,
        null_glGetActiveAttribARB,
        null_glGetActiveSubroutineName,
        null_glGetActiveSubroutineUniformName,
        null_glGetActiveSubroutineUniformiv,
        null_glGetActiveUniform,
        null_glGetActiveUniformARB,
        null_glGetActiveUniformBlockName,
        null_glGetActiveUniformBlockiv,
        null_glGetActiveUniformName,
        null_glGetActiveUniformsiv,
        null_glGetActiveVaryingNV,
        null_glGetArrayObjectfvATI,
        null_glGetArrayObjectivATI,
        null_glGetAttachedObjectsARB,
        null_glGetAttachedShaders,
        null_glGetAttribLocation,
        null_glGetAttribLocationARB,
        null_glGetBooleanIndexedvEXT,
        null_glGetBooleani_v,
        null_glGetBufferParameteri64v,
        null_glGetBufferParameteriv,
        null_glGetBufferParameterivARB,
        null_glGetBufferParameterui64vNV,
        null_glGetBufferPointerv,
        null_glGetBufferPointervARB,
        null_glGetBufferSubData,
        null_glGetBufferSubDataARB,
        null_glGetClipPlanefOES,
        null_glGetClipPlanexOES,
        null_glGetColorTable,
        null_glGetColorTableEXT,
        null_glGetColorTableParameterfv,
        null_glGetColorTableParameterfvEXT,
        null_glGetColorTableParameterfvSGI,
        null_glGetColorTableParameteriv,
        null_glGetColorTableParameterivEXT,
        null_glGetColorTableParameterivSGI,
        null_glGetColorTableSGI,
        null_glGetCombinerInputParameterfvNV,
        null_glGetCombinerInputParameterivNV,
        null_glGetCombinerOutputParameterfvNV,
        null_glGetCombinerOutputParameterivNV,
        null_glGetCombinerStageParameterfvNV,
        null_glGetCommandHeaderNV,
        null_glGetCompressedMultiTexImageEXT,
        null_glGetCompressedTexImage,
        null_glGetCompressedTexImageARB,
        null_glGetCompressedTextureImage,
        null_glGetCompressedTextureImageEXT,
        null_glGetCompressedTextureSubImage,
        null_glGetConvolutionFilter,
        null_glGetConvolutionFilterEXT,
        null_glGetConvolutionParameterfv,
        null_glGetConvolutionParameterfvEXT,
        null_glGetConvolutionParameteriv,
        null_glGetConvolutionParameterivEXT,
        null_glGetConvolutionParameterxvOES,
        null_glGetCoverageModulationTableNV,
        null_glGetDebugMessageLog,
        null_glGetDebugMessageLogAMD,
        null_glGetDebugMessageLogARB,
        null_glGetDetailTexFuncSGIS,
        null_glGetDoubleIndexedvEXT,
        null_glGetDoublei_v,
        null_glGetDoublei_vEXT,
        null_glGetFenceivNV,
        null_glGetFinalCombinerInputParameterfvNV,
        null_glGetFinalCombinerInputParameterivNV,
        null_glGetFirstPerfQueryIdINTEL,
        null_glGetFixedvOES,
        null_glGetFloatIndexedvEXT,
        null_glGetFloati_v,
        null_glGetFloati_vEXT,
        null_glGetFogFuncSGIS,
        null_glGetFragDataIndex,
        null_glGetFragDataLocation,
        null_glGetFragDataLocationEXT,
        null_glGetFragmentLightfvSGIX,
        null_glGetFragmentLightivSGIX,
        null_glGetFragmentMaterialfvSGIX,
        null_glGetFragmentMaterialivSGIX,
        null_glGetFramebufferAttachmentParameteriv,
        null_glGetFramebufferAttachmentParameterivEXT,
        null_glGetFramebufferParameterfvAMD,
        null_glGetFramebufferParameteriv,
        null_glGetFramebufferParameterivEXT,
        null_glGetFramebufferParameterivMESA,
        null_glGetGraphicsResetStatus,
        null_glGetGraphicsResetStatusARB,
        null_glGetHandleARB,
        null_glGetHistogram,
        null_glGetHistogramEXT,
        null_glGetHistogramParameterfv,
        null_glGetHistogramParameterfvEXT,
        null_glGetHistogramParameteriv,
        null_glGetHistogramParameterivEXT,
        null_glGetHistogramParameterxvOES,
        null_glGetImageHandleARB,
        null_glGetImageHandleNV,
        null_glGetImageTransformParameterfvHP,
        null_glGetImageTransformParameterivHP,
        null_glGetInfoLogARB,
        null_glGetInstrumentsSGIX,
        null_glGetInteger64i_v,
        null_glGetInteger64v,
        null_glGetIntegerIndexedvEXT,
        null_glGetIntegeri_v,
        null_glGetIntegerui64i_vNV,
        null_glGetIntegerui64vNV,
        null_glGetInternalformatSampleivNV,
        null_glGetInternalformati64v,
        null_glGetInternalformativ,
        null_glGetInvariantBooleanvEXT,
        null_glGetInvariantFloatvEXT,
        null_glGetInvariantIntegervEXT,
        null_glGetLightxOES,
        null_glGetListParameterfvSGIX,
        null_glGetListParameterivSGIX,
        null_glGetLocalConstantBooleanvEXT,
        null_glGetLocalConstantFloatvEXT,
        null_glGetLocalConstantIntegervEXT,
        null_glGetMapAttribParameterfvNV,
        null_glGetMapAttribParameterivNV,
        null_glGetMapControlPointsNV,
        null_glGetMapParameterfvNV,
        null_glGetMapParameterivNV,
        null_glGetMapxvOES,
        null_glGetMaterialxOES,
        null_glGetMemoryObjectDetachedResourcesuivNV,
        null_glGetMemoryObjectParameterivEXT,
        null_glGetMinmax,
        null_glGetMinmaxEXT,
        null_glGetMinmaxParameterfv,
        null_glGetMinmaxParameterfvEXT,
        null_glGetMinmaxParameteriv,
        null_glGetMinmaxParameterivEXT,
        null_glGetMultiTexEnvfvEXT,
        null_glGetMultiTexEnvivEXT,
        null_glGetMultiTexGendvEXT,
        null_glGetMultiTexGenfvEXT,
        null_glGetMultiTexGenivEXT,
        null_glGetMultiTexImageEXT,
        null_glGetMultiTexLevelParameterfvEXT,
        null_glGetMultiTexLevelParameterivEXT,
        null_glGetMultiTexParameterIivEXT,
        null_glGetMultiTexParameterIuivEXT,
        null_glGetMultiTexParameterfvEXT,
        null_glGetMultiTexParameterivEXT,
        null_glGetMultisamplefv,
        null_glGetMultisamplefvNV,
        null_glGetNamedBufferParameteri64v,
        null_glGetNamedBufferParameteriv,
        null_glGetNamedBufferParameterivEXT,
        null_glGetNamedBufferParameterui64vNV,
        null_glGetNamedBufferPointerv,
        null_glGetNamedBufferPointervEXT,
        null_glGetNamedBufferSubData,
        null_glGetNamedBufferSubDataEXT,
        null_glGetNamedFramebufferAttachmentParameteriv,
        null_glGetNamedFramebufferAttachmentParameterivEXT,
        null_glGetNamedFramebufferParameterfvAMD,
        null_glGetNamedFramebufferParameteriv,
        null_glGetNamedFramebufferParameterivEXT,
        null_glGetNamedProgramLocalParameterIivEXT,
        null_glGetNamedProgramLocalParameterIuivEXT,
        null_glGetNamedProgramLocalParameterdvEXT,
        null_glGetNamedProgramLocalParameterfvEXT,
        null_glGetNamedProgramStringEXT,
        null_glGetNamedProgramivEXT,
        null_glGetNamedRenderbufferParameteriv,
        null_glGetNamedRenderbufferParameterivEXT,
        null_glGetNamedStringARB,
        null_glGetNamedStringivARB,
        null_glGetNextPerfQueryIdINTEL,
        null_glGetObjectBufferfvATI,
        null_glGetObjectBufferivATI,
        null_glGetObjectLabel,
        null_glGetObjectLabelEXT,
        null_glGetObjectParameterfvARB,
        null_glGetObjectParameterivAPPLE,
        null_glGetObjectParameterivARB,
        null_glGetObjectPtrLabel,
        null_glGetOcclusionQueryivNV,
        null_glGetOcclusionQueryuivNV,
        null_glGetPathColorGenfvNV,
        null_glGetPathColorGenivNV,
        null_glGetPathCommandsNV,
        null_glGetPathCoordsNV,
        null_glGetPathDashArrayNV,
        null_glGetPathLengthNV,
        null_glGetPathMetricRangeNV,
        null_glGetPathMetricsNV,
        null_glGetPathParameterfvNV,
        null_glGetPathParameterivNV,
        null_glGetPathSpacingNV,
        null_glGetPathTexGenfvNV,
        null_glGetPathTexGenivNV,
        null_glGetPerfCounterInfoINTEL,
        null_glGetPerfMonitorCounterDataAMD,
        null_glGetPerfMonitorCounterInfoAMD,
        null_glGetPerfMonitorCounterStringAMD,
        null_glGetPerfMonitorCountersAMD,
        null_glGetPerfMonitorGroupStringAMD,
        null_glGetPerfMonitorGroupsAMD,
        null_glGetPerfQueryDataINTEL,
        null_glGetPerfQueryIdByNameINTEL,
        null_glGetPerfQueryInfoINTEL,
        null_glGetPixelMapxv,
        null_glGetPixelTexGenParameterfvSGIS,
        null_glGetPixelTexGenParameterivSGIS,
        null_glGetPixelTransformParameterfvEXT,
        null_glGetPixelTransformParameterivEXT,
        null_glGetPointerIndexedvEXT,
        null_glGetPointeri_vEXT,
        null_glGetPointervEXT,
        null_glGetProgramBinary,
        null_glGetProgramEnvParameterIivNV,
        null_glGetProgramEnvParameterIuivNV,
        null_glGetProgramEnvParameterdvARB,
        null_glGetProgramEnvParameterfvARB,
        null_glGetProgramInfoLog,
        null_glGetProgramInterfaceiv,
        null_glGetProgramLocalParameterIivNV,
        null_glGetProgramLocalParameterIuivNV,
        null_glGetProgramLocalParameterdvARB,
        null_glGetProgramLocalParameterfvARB,
        null_glGetProgramNamedParameterdvNV,
        null_glGetProgramNamedParameterfvNV,
        null_glGetProgramParameterdvNV,
        null_glGetProgramParameterfvNV,
        null_glGetProgramPipelineInfoLog,
        null_glGetProgramPipelineiv,
        null_glGetProgramResourceIndex,
        null_glGetProgramResourceLocation,
        null_glGetProgramResourceLocationIndex,
        null_glGetProgramResourceName,
        null_glGetProgramResourcefvNV,
        null_glGetProgramResourceiv,
        null_glGetProgramStageiv,
        null_glGetProgramStringARB,
        null_glGetProgramStringNV,
        null_glGetProgramSubroutineParameteruivNV,
        null_glGetProgramiv,
        null_glGetProgramivARB,
        null_glGetProgramivNV,
        null_glGetQueryBufferObjecti64v,
        null_glGetQueryBufferObjectiv,
        null_glGetQueryBufferObjectui64v,
        null_glGetQueryBufferObjectuiv,
        null_glGetQueryIndexediv,
        null_glGetQueryObjecti64v,
        null_glGetQueryObjecti64vEXT,
        null_glGetQueryObjectiv,
        null_glGetQueryObjectivARB,
        null_glGetQueryObjectui64v,
        null_glGetQueryObjectui64vEXT,
        null_glGetQueryObjectuiv,
        null_glGetQueryObjectuivARB,
        null_glGetQueryiv,
        null_glGetQueryivARB,
        null_glGetRenderbufferParameteriv,
        null_glGetRenderbufferParameterivEXT,
        null_glGetSamplerParameterIiv,
        null_glGetSamplerParameterIuiv,
        null_glGetSamplerParameterfv,
        null_glGetSamplerParameteriv,
        null_glGetSemaphoreParameterui64vEXT,
        null_glGetSeparableFilter,
        null_glGetSeparableFilterEXT,
        null_glGetShaderInfoLog,
        null_glGetShaderPrecisionFormat,
        null_glGetShaderSource,
        null_glGetShaderSourceARB,
        null_glGetShaderiv,
        null_glGetShadingRateImagePaletteNV,
        null_glGetShadingRateSampleLocationivNV,
        null_glGetSharpenTexFuncSGIS,
        null_glGetStageIndexNV,
        null_glGetStringi,
        null_glGetSubroutineIndex,
        null_glGetSubroutineUniformLocation,
        null_glGetSynciv,
        null_glGetTexBumpParameterfvATI,
        null_glGetTexBumpParameterivATI,
        null_glGetTexEnvxvOES,
        null_glGetTexFilterFuncSGIS,
        null_glGetTexGenxvOES,
        null_glGetTexLevelParameterxvOES,
        null_glGetTexParameterIiv,
        null_glGetTexParameterIivEXT,
        null_glGetTexParameterIuiv,
        null_glGetTexParameterIuivEXT,
        null_glGetTexParameterPointervAPPLE,
        null_glGetTexParameterxvOES,
        null_glGetTextureHandleARB,
        null_glGetTextureHandleNV,
        null_glGetTextureImage,
        null_glGetTextureImageEXT,
        null_glGetTextureLevelParameterfv,
        null_glGetTextureLevelParameterfvEXT,
        null_glGetTextureLevelParameteriv,
        null_glGetTextureLevelParameterivEXT,
        null_glGetTextureParameterIiv,
        null_glGetTextureParameterIivEXT,
        null_glGetTextureParameterIuiv,
        null_glGetTextureParameterIuivEXT,
        null_glGetTextureParameterfv,
        null_glGetTextureParameterfvEXT,
        null_glGetTextureParameteriv,
        null_glGetTextureParameterivEXT,
        null_glGetTextureSamplerHandleARB,
        null_glGetTextureSamplerHandleNV,
        null_glGetTextureSubImage,
        null_glGetTrackMatrixivNV,
        null_glGetTransformFeedbackVarying,
        null_glGetTransformFeedbackVaryingEXT,
        null_glGetTransformFeedbackVaryingNV,
        null_glGetTransformFeedbacki64_v,
        null_glGetTransformFeedbacki_v,
        null_glGetTransformFeedbackiv,
        null_glGetUniformBlockIndex,
        null_glGetUniformBufferSizeEXT,
        null_glGetUniformIndices,
        null_glGetUniformLocation,
        null_glGetUniformLocationARB,
        null_glGetUniformOffsetEXT,
        null_glGetUniformSubroutineuiv,
        null_glGetUniformdv,
        null_glGetUniformfv,
        null_glGetUniformfvARB,
        null_glGetUniformi64vARB,
        null_glGetUniformi64vNV,
        null_glGetUniformiv,
        null_glGetUniformivARB,
        null_glGetUniformui64vARB,
        null_glGetUniformui64vNV,
        null_glGetUniformuiv,
        null_glGetUniformuivEXT,
        null_glGetUnsignedBytei_vEXT,
        null_glGetUnsignedBytevEXT,
        null_glGetVariantArrayObjectfvATI,
        null_glGetVariantArrayObjectivATI,
        null_glGetVariantBooleanvEXT,
        null_glGetVariantFloatvEXT,
        null_glGetVariantIntegervEXT,
        null_glGetVariantPointervEXT,
        null_glGetVaryingLocationNV,
        null_glGetVertexArrayIndexed64iv,
        null_glGetVertexArrayIndexediv,
        null_glGetVertexArrayIntegeri_vEXT,
        null_glGetVertexArrayIntegervEXT,
        null_glGetVertexArrayPointeri_vEXT,
        null_glGetVertexArrayPointervEXT,
        null_glGetVertexArrayiv,
        null_glGetVertexAttribArrayObjectfvATI,
        null_glGetVertexAttribArrayObjectivATI,
        null_glGetVertexAttribIiv,
        null_glGetVertexAttribIivEXT,
        null_glGetVertexAttribIuiv,
        null_glGetVertexAttribIuivEXT,
        null_glGetVertexAttribLdv,
        null_glGetVertexAttribLdvEXT,
        null_glGetVertexAttribLi64vNV,
        null_glGetVertexAttribLui64vARB,
        null_glGetVertexAttribLui64vNV,
        null_glGetVertexAttribPointerv,
        null_glGetVertexAttribPointervARB,
        null_glGetVertexAttribPointervNV,
        null_glGetVertexAttribdv,
        null_glGetVertexAttribdvARB,
        null_glGetVertexAttribdvNV,
        null_glGetVertexAttribfv,
        null_glGetVertexAttribfvARB,
        null_glGetVertexAttribfvNV,
        null_glGetVertexAttribiv,
        null_glGetVertexAttribivARB,
        null_glGetVertexAttribivNV,
        null_glGetVideoCaptureStreamdvNV,
        null_glGetVideoCaptureStreamfvNV,
        null_glGetVideoCaptureStreamivNV,
        null_glGetVideoCaptureivNV,
        null_glGetVideoi64vNV,
        null_glGetVideoivNV,
        null_glGetVideoui64vNV,
        null_glGetVideouivNV,
        null_glGetVkProcAddrNV,
        null_glGetnColorTable,
        null_glGetnColorTableARB,
        null_glGetnCompressedTexImage,
        null_glGetnCompressedTexImageARB,
        null_glGetnConvolutionFilter,
        null_glGetnConvolutionFilterARB,
        null_glGetnHistogram,
        null_glGetnHistogramARB,
        null_glGetnMapdv,
        null_glGetnMapdvARB,
        null_glGetnMapfv,
        null_glGetnMapfvARB,
        null_glGetnMapiv,
        null_glGetnMapivARB,
        null_glGetnMinmax,
        null_glGetnMinmaxARB,
        null_glGetnPixelMapfv,
        null_glGetnPixelMapfvARB,
        null_glGetnPixelMapuiv,
        null_glGetnPixelMapuivARB,
        null_glGetnPixelMapusv,
        null_glGetnPixelMapusvARB,
        null_glGetnPolygonStipple,
        null_glGetnPolygonStippleARB,
        null_glGetnSeparableFilter,
        null_glGetnSeparableFilterARB,
        null_glGetnTexImage,
        null_glGetnTexImageARB,
        null_glGetnUniformdv,
        null_glGetnUniformdvARB,
        null_glGetnUniformfv,
        null_glGetnUniformfvARB,
        null_glGetnUniformi64vARB,
        null_glGetnUniformiv,
        null_glGetnUniformivARB,
        null_glGetnUniformui64vARB,
        null_glGetnUniformuiv,
        null_glGetnUniformuivARB,
        null_glGlobalAlphaFactorbSUN,
        null_glGlobalAlphaFactordSUN,
        null_glGlobalAlphaFactorfSUN,
        null_glGlobalAlphaFactoriSUN,
        null_glGlobalAlphaFactorsSUN,
        null_glGlobalAlphaFactorubSUN,
        null_glGlobalAlphaFactoruiSUN,
        null_glGlobalAlphaFactorusSUN,
        null_glHintPGI,
        null_glHistogram,
        null_glHistogramEXT,
        null_glIglooInterfaceSGIX,
        null_glImageTransformParameterfHP,
        null_glImageTransformParameterfvHP,
        null_glImageTransformParameteriHP,
        null_glImageTransformParameterivHP,
        null_glImportMemoryFdEXT,
        null_glImportMemoryWin32HandleEXT,
        null_glImportMemoryWin32NameEXT,
        null_glImportSemaphoreFdEXT,
        null_glImportSemaphoreWin32HandleEXT,
        null_glImportSemaphoreWin32NameEXT,
        null_glImportSyncEXT,
        null_glIndexFormatNV,
        null_glIndexFuncEXT,
        null_glIndexMaterialEXT,
        null_glIndexPointerEXT,
        null_glIndexPointerListIBM,
        null_glIndexxOES,
        null_glIndexxvOES,
        null_glInsertComponentEXT,
        null_glInsertEventMarkerEXT,
        null_glInstrumentsBufferSGIX,
        null_glInterpolatePathsNV,
        null_glInvalidateBufferData,
        null_glInvalidateBufferSubData,
        null_glInvalidateFramebuffer,
        null_glInvalidateNamedFramebufferData,
        null_glInvalidateNamedFramebufferSubData,
        null_glInvalidateSubFramebuffer,
        null_glInvalidateTexImage,
        null_glInvalidateTexSubImage,
        null_glIsAsyncMarkerSGIX,
        null_glIsBuffer,
        null_glIsBufferARB,
        null_glIsBufferResidentNV,
        null_glIsCommandListNV,
        null_glIsEnabledIndexedEXT,
        null_glIsEnabledi,
        null_glIsFenceAPPLE,
        null_glIsFenceNV,
        null_glIsFramebuffer,
        null_glIsFramebufferEXT,
        null_glIsImageHandleResidentARB,
        null_glIsImageHandleResidentNV,
        null_glIsMemoryObjectEXT,
        null_glIsNameAMD,
        null_glIsNamedBufferResidentNV,
        null_glIsNamedStringARB,
        null_glIsObjectBufferATI,
        null_glIsOcclusionQueryNV,
        null_glIsPathNV,
        null_glIsPointInFillPathNV,
        null_glIsPointInStrokePathNV,
        null_glIsProgram,
        null_glIsProgramARB,
        null_glIsProgramNV,
        null_glIsProgramPipeline,
        null_glIsQuery,
        null_glIsQueryARB,
        null_glIsRenderbuffer,
        null_glIsRenderbufferEXT,
        null_glIsSampler,
        null_glIsSemaphoreEXT,
        null_glIsShader,
        null_glIsStateNV,
        null_glIsSync,
        null_glIsTextureEXT,
        null_glIsTextureHandleResidentARB,
        null_glIsTextureHandleResidentNV,
        null_glIsTransformFeedback,
        null_glIsTransformFeedbackNV,
        null_glIsVariantEnabledEXT,
        null_glIsVertexArray,
        null_glIsVertexArrayAPPLE,
        null_glIsVertexAttribEnabledAPPLE,
        null_glLGPUCopyImageSubDataNVX,
        null_glLGPUInterlockNVX,
        null_glLGPUNamedBufferSubDataNVX,
        null_glLabelObjectEXT,
        null_glLightEnviSGIX,
        null_glLightModelxOES,
        null_glLightModelxvOES,
        null_glLightxOES,
        null_glLightxvOES,
        null_glLineWidthxOES,
        null_glLinkProgram,
        null_glLinkProgramARB,
        null_glListDrawCommandsStatesClientNV,
        null_glListParameterfSGIX,
        null_glListParameterfvSGIX,
        null_glListParameteriSGIX,
        null_glListParameterivSGIX,
        null_glLoadIdentityDeformationMapSGIX,
        null_glLoadMatrixxOES,
        null_glLoadProgramNV,
        null_glLoadTransposeMatrixd,
        null_glLoadTransposeMatrixdARB,
        null_glLoadTransposeMatrixf,
        null_glLoadTransposeMatrixfARB,
        null_glLoadTransposeMatrixxOES,
        null_glLockArraysEXT,
        null_glMTexCoord2fSGIS,
        null_glMTexCoord2fvSGIS,
        null_glMakeBufferNonResidentNV,
        null_glMakeBufferResidentNV,
        null_glMakeImageHandleNonResidentARB,
        null_glMakeImageHandleNonResidentNV,
        null_glMakeImageHandleResidentARB,
        null_glMakeImageHandleResidentNV,
        null_glMakeNamedBufferNonResidentNV,
        null_glMakeNamedBufferResidentNV,
        null_glMakeTextureHandleNonResidentARB,
        null_glMakeTextureHandleNonResidentNV,
        null_glMakeTextureHandleResidentARB,
        null_glMakeTextureHandleResidentNV,
        null_glMap1xOES,
        null_glMap2xOES,
        null_glMapBuffer,
        null_glMapBufferARB,
        null_glMapBufferRange,
        null_glMapControlPointsNV,
        null_glMapGrid1xOES,
        null_glMapGrid2xOES,
        null_glMapNamedBuffer,
        null_glMapNamedBufferEXT,
        null_glMapNamedBufferRange,
        null_glMapNamedBufferRangeEXT,
        null_glMapObjectBufferATI,
        null_glMapParameterfvNV,
        null_glMapParameterivNV,
        null_glMapTexture2DINTEL,
        null_glMapVertexAttrib1dAPPLE,
        null_glMapVertexAttrib1fAPPLE,
        null_glMapVertexAttrib2dAPPLE,
        null_glMapVertexAttrib2fAPPLE,
        null_glMaterialxOES,
        null_glMaterialxvOES,
        null_glMatrixFrustumEXT,
        null_glMatrixIndexPointerARB,
        null_glMatrixIndexubvARB,
        null_glMatrixIndexuivARB,
        null_glMatrixIndexusvARB,
        null_glMatrixLoad3x2fNV,
        null_glMatrixLoad3x3fNV,
        null_glMatrixLoadIdentityEXT,
        null_glMatrixLoadTranspose3x3fNV,
        null_glMatrixLoadTransposedEXT,
        null_glMatrixLoadTransposefEXT,
        null_glMatrixLoaddEXT,
        null_glMatrixLoadfEXT,
        null_glMatrixMult3x2fNV,
        null_glMatrixMult3x3fNV,
        null_glMatrixMultTranspose3x3fNV,
        null_glMatrixMultTransposedEXT,
        null_glMatrixMultTransposefEXT,
        null_glMatrixMultdEXT,
        null_glMatrixMultfEXT,
        null_glMatrixOrthoEXT,
        null_glMatrixPopEXT,
        null_glMatrixPushEXT,
        null_glMatrixRotatedEXT,
        null_glMatrixRotatefEXT,
        null_glMatrixScaledEXT,
        null_glMatrixScalefEXT,
        null_glMatrixTranslatedEXT,
        null_glMatrixTranslatefEXT,
        null_glMaxShaderCompilerThreadsARB,
        null_glMaxShaderCompilerThreadsKHR,
        null_glMemoryBarrier,
        null_glMemoryBarrierByRegion,
        null_glMemoryBarrierEXT,
        null_glMemoryObjectParameterivEXT,
        null_glMinSampleShading,
        null_glMinSampleShadingARB,
        null_glMinmax,
        null_glMinmaxEXT,
        null_glMultMatrixxOES,
        null_glMultTransposeMatrixd,
        null_glMultTransposeMatrixdARB,
        null_glMultTransposeMatrixf,
        null_glMultTransposeMatrixfARB,
        null_glMultTransposeMatrixxOES,
        null_glMultiDrawArrays,
        null_glMultiDrawArraysEXT,
        null_glMultiDrawArraysIndirect,
        null_glMultiDrawArraysIndirectAMD,
        null_glMultiDrawArraysIndirectBindlessCountNV,
        null_glMultiDrawArraysIndirectBindlessNV,
        null_glMultiDrawArraysIndirectCount,
        null_glMultiDrawArraysIndirectCountARB,
        null_glMultiDrawElementArrayAPPLE,
        null_glMultiDrawElements,
        null_glMultiDrawElementsBaseVertex,
        null_glMultiDrawElementsEXT,
        null_glMultiDrawElementsIndirect,
        null_glMultiDrawElementsIndirectAMD,
        null_glMultiDrawElementsIndirectBindlessCountNV,
        null_glMultiDrawElementsIndirectBindlessNV,
        null_glMultiDrawElementsIndirectCount,
        null_glMultiDrawElementsIndirectCountARB,
        null_glMultiDrawMeshTasksIndirectCountNV,
        null_glMultiDrawMeshTasksIndirectNV,
        null_glMultiDrawRangeElementArrayAPPLE,
        null_glMultiModeDrawArraysIBM,
        null_glMultiModeDrawElementsIBM,
        null_glMultiTexBufferEXT,
        null_glMultiTexCoord1bOES,
        null_glMultiTexCoord1bvOES,
        null_glMultiTexCoord1d,
        null_glMultiTexCoord1dARB,
        null_glMultiTexCoord1dSGIS,
        null_glMultiTexCoord1dv,
        null_glMultiTexCoord1dvARB,
        null_glMultiTexCoord1dvSGIS,
        null_glMultiTexCoord1f,
        null_glMultiTexCoord1fARB,
        null_glMultiTexCoord1fSGIS,
        null_glMultiTexCoord1fv,
        null_glMultiTexCoord1fvARB,
        null_glMultiTexCoord1fvSGIS,
        null_glMultiTexCoord1hNV,
        null_glMultiTexCoord1hvNV,
        null_glMultiTexCoord1i,
        null_glMultiTexCoord1iARB,
        null_glMultiTexCoord1iSGIS,
        null_glMultiTexCoord1iv,
        null_glMultiTexCoord1ivARB,
        null_glMultiTexCoord1ivSGIS,
        null_glMultiTexCoord1s,
        null_glMultiTexCoord1sARB,
        null_glMultiTexCoord1sSGIS,
        null_glMultiTexCoord1sv,
        null_glMultiTexCoord1svARB,
        null_glMultiTexCoord1svSGIS,
        null_glMultiTexCoord1xOES,
        null_glMultiTexCoord1xvOES,
        null_glMultiTexCoord2bOES,
        null_glMultiTexCoord2bvOES,
        null_glMultiTexCoord2d,
        null_glMultiTexCoord2dARB,
        null_glMultiTexCoord2dSGIS,
        null_glMultiTexCoord2dv,
        null_glMultiTexCoord2dvARB,
        null_glMultiTexCoord2dvSGIS,
        null_glMultiTexCoord2f,
        null_glMultiTexCoord2fARB,
        null_glMultiTexCoord2fSGIS,
        null_glMultiTexCoord2fv,
        null_glMultiTexCoord2fvARB,
        null_glMultiTexCoord2fvSGIS,
        null_glMultiTexCoord2hNV,
        null_glMultiTexCoord2hvNV,
        null_glMultiTexCoord2i,
        null_glMultiTexCoord2iARB,
        null_glMultiTexCoord2iSGIS,
        null_glMultiTexCoord2iv,
        null_glMultiTexCoord2ivARB,
        null_glMultiTexCoord2ivSGIS,
        null_glMultiTexCoord2s,
        null_glMultiTexCoord2sARB,
        null_glMultiTexCoord2sSGIS,
        null_glMultiTexCoord2sv,
        null_glMultiTexCoord2svARB,
        null_glMultiTexCoord2svSGIS,
        null_glMultiTexCoord2xOES,
        null_glMultiTexCoord2xvOES,
        null_glMultiTexCoord3bOES,
        null_glMultiTexCoord3bvOES,
        null_glMultiTexCoord3d,
        null_glMultiTexCoord3dARB,
        null_glMultiTexCoord3dSGIS,
        null_glMultiTexCoord3dv,
        null_glMultiTexCoord3dvARB,
        null_glMultiTexCoord3dvSGIS,
        null_glMultiTexCoord3f,
        null_glMultiTexCoord3fARB,
        null_glMultiTexCoord3fSGIS,
        null_glMultiTexCoord3fv,
        null_glMultiTexCoord3fvARB,
        null_glMultiTexCoord3fvSGIS,
        null_glMultiTexCoord3hNV,
        null_glMultiTexCoord3hvNV,
        null_glMultiTexCoord3i,
        null_glMultiTexCoord3iARB,
        null_glMultiTexCoord3iSGIS,
        null_glMultiTexCoord3iv,
        null_glMultiTexCoord3ivARB,
        null_glMultiTexCoord3ivSGIS,
        null_glMultiTexCoord3s,
        null_glMultiTexCoord3sARB,
        null_glMultiTexCoord3sSGIS,
        null_glMultiTexCoord3sv,
        null_glMultiTexCoord3svARB,
        null_glMultiTexCoord3svSGIS,
        null_glMultiTexCoord3xOES,
        null_glMultiTexCoord3xvOES,
        null_glMultiTexCoord4bOES,
        null_glMultiTexCoord4bvOES,
        null_glMultiTexCoord4d,
        null_glMultiTexCoord4dARB,
        null_glMultiTexCoord4dSGIS,
        null_glMultiTexCoord4dv,
        null_glMultiTexCoord4dvARB,
        null_glMultiTexCoord4dvSGIS,
        null_glMultiTexCoord4f,
        null_glMultiTexCoord4fARB,
        null_glMultiTexCoord4fSGIS,
        null_glMultiTexCoord4fv,
        null_glMultiTexCoord4fvARB,
        null_glMultiTexCoord4fvSGIS,
        null_glMultiTexCoord4hNV,
        null_glMultiTexCoord4hvNV,
        null_glMultiTexCoord4i,
        null_glMultiTexCoord4iARB,
        null_glMultiTexCoord4iSGIS,
        null_glMultiTexCoord4iv,
        null_glMultiTexCoord4ivARB,
        null_glMultiTexCoord4ivSGIS,
        null_glMultiTexCoord4s,
        null_glMultiTexCoord4sARB,
        null_glMultiTexCoord4sSGIS,
        null_glMultiTexCoord4sv,
        null_glMultiTexCoord4svARB,
        null_glMultiTexCoord4svSGIS,
        null_glMultiTexCoord4xOES,
        null_glMultiTexCoord4xvOES,
        null_glMultiTexCoordP1ui,
        null_glMultiTexCoordP1uiv,
        null_glMultiTexCoordP2ui,
        null_glMultiTexCoordP2uiv,
        null_glMultiTexCoordP3ui,
        null_glMultiTexCoordP3uiv,
        null_glMultiTexCoordP4ui,
        null_glMultiTexCoordP4uiv,
        null_glMultiTexCoordPointerEXT,
        null_glMultiTexCoordPointerSGIS,
        null_glMultiTexEnvfEXT,
        null_glMultiTexEnvfvEXT,
        null_glMultiTexEnviEXT,
        null_glMultiTexEnvivEXT,
        null_glMultiTexGendEXT,
        null_glMultiTexGendvEXT,
        null_glMultiTexGenfEXT,
        null_glMultiTexGenfvEXT,
        null_glMultiTexGeniEXT,
        null_glMultiTexGenivEXT,
        null_glMultiTexImage1DEXT,
        null_glMultiTexImage2DEXT,
        null_glMultiTexImage3DEXT,
        null_glMultiTexParameterIivEXT,
        null_glMultiTexParameterIuivEXT,
        null_glMultiTexParameterfEXT,
        null_glMultiTexParameterfvEXT,
        null_glMultiTexParameteriEXT,
        null_glMultiTexParameterivEXT,
        null_glMultiTexRenderbufferEXT,
        null_glMultiTexSubImage1DEXT,
        null_glMultiTexSubImage2DEXT,
        null_glMultiTexSubImage3DEXT,
        null_glMulticastBarrierNV,
        null_glMulticastBlitFramebufferNV,
        null_glMulticastBufferSubDataNV,
        null_glMulticastCopyBufferSubDataNV,
        null_glMulticastCopyImageSubDataNV,
        null_glMulticastFramebufferSampleLocationsfvNV,
        null_glMulticastGetQueryObjecti64vNV,
        null_glMulticastGetQueryObjectivNV,
        null_glMulticastGetQueryObjectui64vNV,
        null_glMulticastGetQueryObjectuivNV,
        null_glMulticastScissorArrayvNVX,
        null_glMulticastViewportArrayvNVX,
        null_glMulticastViewportPositionWScaleNVX,
        null_glMulticastWaitSyncNV,
        null_glNamedBufferAttachMemoryNV,
        null_glNamedBufferData,
        null_glNamedBufferDataEXT,
        null_glNamedBufferPageCommitmentARB,
        null_glNamedBufferPageCommitmentEXT,
        null_glNamedBufferStorage,
        null_glNamedBufferStorageEXT,
        null_glNamedBufferStorageExternalEXT,
        null_glNamedBufferStorageMemEXT,
        null_glNamedBufferSubData,
        null_glNamedBufferSubDataEXT,
        null_glNamedCopyBufferSubDataEXT,
        null_glNamedFramebufferDrawBuffer,
        null_glNamedFramebufferDrawBuffers,
        null_glNamedFramebufferParameteri,
        null_glNamedFramebufferParameteriEXT,
        null_glNamedFramebufferReadBuffer,
        null_glNamedFramebufferRenderbuffer,
        null_glNamedFramebufferRenderbufferEXT,
        null_glNamedFramebufferSampleLocationsfvARB,
        null_glNamedFramebufferSampleLocationsfvNV,
        null_glNamedFramebufferSamplePositionsfvAMD,
        null_glNamedFramebufferTexture,
        null_glNamedFramebufferTexture1DEXT,
        null_glNamedFramebufferTexture2DEXT,
        null_glNamedFramebufferTexture3DEXT,
        null_glNamedFramebufferTextureEXT,
        null_glNamedFramebufferTextureFaceEXT,
        null_glNamedFramebufferTextureLayer,
        null_glNamedFramebufferTextureLayerEXT,
        null_glNamedProgramLocalParameter4dEXT,
        null_glNamedProgramLocalParameter4dvEXT,
        null_glNamedProgramLocalParameter4fEXT,
        null_glNamedProgramLocalParameter4fvEXT,
        null_glNamedProgramLocalParameterI4iEXT,
        null_glNamedProgramLocalParameterI4ivEXT,
        null_glNamedProgramLocalParameterI4uiEXT,
        null_glNamedProgramLocalParameterI4uivEXT,
        null_glNamedProgramLocalParameters4fvEXT,
        null_glNamedProgramLocalParametersI4ivEXT,
        null_glNamedProgramLocalParametersI4uivEXT,
        null_glNamedProgramStringEXT,
        null_glNamedRenderbufferStorage,
        null_glNamedRenderbufferStorageEXT,
        null_glNamedRenderbufferStorageMultisample,
        null_glNamedRenderbufferStorageMultisampleAdvancedAMD,
        null_glNamedRenderbufferStorageMultisampleCoverageEXT,
        null_glNamedRenderbufferStorageMultisampleEXT,
        null_glNamedStringARB,
        null_glNewBufferRegion,
        null_glNewObjectBufferATI,
        null_glNormal3fVertex3fSUN,
        null_glNormal3fVertex3fvSUN,
        null_glNormal3hNV,
        null_glNormal3hvNV,
        null_glNormal3xOES,
        null_glNormal3xvOES,
        null_glNormalFormatNV,
        null_glNormalP3ui,
        null_glNormalP3uiv,
        null_glNormalPointerEXT,
        null_glNormalPointerListIBM,
        null_glNormalPointervINTEL,
        null_glNormalStream3bATI,
        null_glNormalStream3bvATI,
        null_glNormalStream3dATI,
        null_glNormalStream3dvATI,
        null_glNormalStream3fATI,
        null_glNormalStream3fvATI,
        null_glNormalStream3iATI,
        null_glNormalStream3ivATI,
        null_glNormalStream3sATI,
        null_glNormalStream3svATI,
        null_glObjectLabel,
        null_glObjectPtrLabel,
        null_glObjectPurgeableAPPLE,
        null_glObjectUnpurgeableAPPLE,
        null_glOrthofOES,
        null_glOrthoxOES,
        null_glPNTrianglesfATI,
        null_glPNTrianglesiATI,
        null_glPassTexCoordATI,
        null_glPassThroughxOES,
        null_glPatchParameterfv,
        null_glPatchParameteri,
        null_glPathColorGenNV,
        null_glPathCommandsNV,
        null_glPathCoordsNV,
        null_glPathCoverDepthFuncNV,
        null_glPathDashArrayNV,
        null_glPathFogGenNV,
        null_glPathGlyphIndexArrayNV,
        null_glPathGlyphIndexRangeNV,
        null_glPathGlyphRangeNV,
        null_glPathGlyphsNV,
        null_glPathMemoryGlyphIndexArrayNV,
        null_glPathParameterfNV,
        null_glPathParameterfvNV,
        null_glPathParameteriNV,
        null_glPathParameterivNV,
        null_glPathStencilDepthOffsetNV,
        null_glPathStencilFuncNV,
        null_glPathStringNV,
        null_glPathSubCommandsNV,
        null_glPathSubCoordsNV,
        null_glPathTexGenNV,
        null_glPauseTransformFeedback,
        null_glPauseTransformFeedbackNV,
        null_glPixelDataRangeNV,
        null_glPixelMapx,
        null_glPixelStorex,
        null_glPixelTexGenParameterfSGIS,
        null_glPixelTexGenParameterfvSGIS,
        null_glPixelTexGenParameteriSGIS,
        null_glPixelTexGenParameterivSGIS,
        null_glPixelTexGenSGIX,
        null_glPixelTransferxOES,
        null_glPixelTransformParameterfEXT,
        null_glPixelTransformParameterfvEXT,
        null_glPixelTransformParameteriEXT,
        null_glPixelTransformParameterivEXT,
        null_glPixelZoomxOES,
        null_glPointAlongPathNV,
        null_glPointParameterf,
        null_glPointParameterfARB,
        null_glPointParameterfEXT,
        null_glPointParameterfSGIS,
        null_glPointParameterfv,
        null_glPointParameterfvARB,
        null_glPointParameterfvEXT,
        null_glPointParameterfvSGIS,
        null_glPointParameteri,
        null_glPointParameteriNV,
        null_glPointParameteriv,
        null_glPointParameterivNV,
        null_glPointParameterxvOES,
        null_glPointSizexOES,
        null_glPollAsyncSGIX,
        null_glPollInstrumentsSGIX,
        null_glPolygonOffsetClamp,
        null_glPolygonOffsetClampEXT,
        null_glPolygonOffsetEXT,
        null_glPolygonOffsetxOES,
        null_glPopDebugGroup,
        null_glPopGroupMarkerEXT,
        null_glPresentFrameDualFillNV,
        null_glPresentFrameKeyedNV,
        null_glPrimitiveBoundingBoxARB,
        null_glPrimitiveRestartIndex,
        null_glPrimitiveRestartIndexNV,
        null_glPrimitiveRestartNV,
        null_glPrioritizeTexturesEXT,
        null_glPrioritizeTexturesxOES,
        null_glProgramBinary,
        null_glProgramBufferParametersIivNV,
        null_glProgramBufferParametersIuivNV,
        null_glProgramBufferParametersfvNV,
        null_glProgramEnvParameter4dARB,
        null_glProgramEnvParameter4dvARB,
        null_glProgramEnvParameter4fARB,
        null_glProgramEnvParameter4fvARB,
        null_glProgramEnvParameterI4iNV,
        null_glProgramEnvParameterI4ivNV,
        null_glProgramEnvParameterI4uiNV,
        null_glProgramEnvParameterI4uivNV,
        null_glProgramEnvParameters4fvEXT,
        null_glProgramEnvParametersI4ivNV,
        null_glProgramEnvParametersI4uivNV,
        null_glProgramLocalParameter4dARB,
        null_glProgramLocalParameter4dvARB,
        null_glProgramLocalParameter4fARB,
        null_glProgramLocalParameter4fvARB,
        null_glProgramLocalParameterI4iNV,
        null_glProgramLocalParameterI4ivNV,
        null_glProgramLocalParameterI4uiNV,
        null_glProgramLocalParameterI4uivNV,
        null_glProgramLocalParameters4fvEXT,
        null_glProgramLocalParametersI4ivNV,
        null_glProgramLocalParametersI4uivNV,
        null_glProgramNamedParameter4dNV,
        null_glProgramNamedParameter4dvNV,
        null_glProgramNamedParameter4fNV,
        null_glProgramNamedParameter4fvNV,
        null_glProgramParameter4dNV,
        null_glProgramParameter4dvNV,
        null_glProgramParameter4fNV,
        null_glProgramParameter4fvNV,
        null_glProgramParameteri,
        null_glProgramParameteriARB,
        null_glProgramParameteriEXT,
        null_glProgramParameters4dvNV,
        null_glProgramParameters4fvNV,
        null_glProgramPathFragmentInputGenNV,
        null_glProgramStringARB,
        null_glProgramSubroutineParametersuivNV,
        null_glProgramUniform1d,
        null_glProgramUniform1dEXT,
        null_glProgramUniform1dv,
        null_glProgramUniform1dvEXT,
        null_glProgramUniform1f,
        null_glProgramUniform1fEXT,
        null_glProgramUniform1fv,
        null_glProgramUniform1fvEXT,
        null_glProgramUniform1i,
        null_glProgramUniform1i64ARB,
        null_glProgramUniform1i64NV,
        null_glProgramUniform1i64vARB,
        null_glProgramUniform1i64vNV,
        null_glProgramUniform1iEXT,
        null_glProgramUniform1iv,
        null_glProgramUniform1ivEXT,
        null_glProgramUniform1ui,
        null_glProgramUniform1ui64ARB,
        null_glProgramUniform1ui64NV,
        null_glProgramUniform1ui64vARB,
        null_glProgramUniform1ui64vNV,
        null_glProgramUniform1uiEXT,
        null_glProgramUniform1uiv,
        null_glProgramUniform1uivEXT,
        null_glProgramUniform2d,
        null_glProgramUniform2dEXT,
        null_glProgramUniform2dv,
        null_glProgramUniform2dvEXT,
        null_glProgramUniform2f,
        null_glProgramUniform2fEXT,
        null_glProgramUniform2fv,
        null_glProgramUniform2fvEXT,
        null_glProgramUniform2i,
        null_glProgramUniform2i64ARB,
        null_glProgramUniform2i64NV,
        null_glProgramUniform2i64vARB,
        null_glProgramUniform2i64vNV,
        null_glProgramUniform2iEXT,
        null_glProgramUniform2iv,
        null_glProgramUniform2ivEXT,
        null_glProgramUniform2ui,
        null_glProgramUniform2ui64ARB,
        null_glProgramUniform2ui64NV,
        null_glProgramUniform2ui64vARB,
        null_glProgramUniform2ui64vNV,
        null_glProgramUniform2uiEXT,
        null_glProgramUniform2uiv,
        null_glProgramUniform2uivEXT,
        null_glProgramUniform3d,
        null_glProgramUniform3dEXT,
        null_glProgramUniform3dv,
        null_glProgramUniform3dvEXT,
        null_glProgramUniform3f,
        null_glProgramUniform3fEXT,
        null_glProgramUniform3fv,
        null_glProgramUniform3fvEXT,
        null_glProgramUniform3i,
        null_glProgramUniform3i64ARB,
        null_glProgramUniform3i64NV,
        null_glProgramUniform3i64vARB,
        null_glProgramUniform3i64vNV,
        null_glProgramUniform3iEXT,
        null_glProgramUniform3iv,
        null_glProgramUniform3ivEXT,
        null_glProgramUniform3ui,
        null_glProgramUniform3ui64ARB,
        null_glProgramUniform3ui64NV,
        null_glProgramUniform3ui64vARB,
        null_glProgramUniform3ui64vNV,
        null_glProgramUniform3uiEXT,
        null_glProgramUniform3uiv,
        null_glProgramUniform3uivEXT,
        null_glProgramUniform4d,
        null_glProgramUniform4dEXT,
        null_glProgramUniform4dv,
        null_glProgramUniform4dvEXT,
        null_glProgramUniform4f,
        null_glProgramUniform4fEXT,
        null_glProgramUniform4fv,
        null_glProgramUniform4fvEXT,
        null_glProgramUniform4i,
        null_glProgramUniform4i64ARB,
        null_glProgramUniform4i64NV,
        null_glProgramUniform4i64vARB,
        null_glProgramUniform4i64vNV,
        null_glProgramUniform4iEXT,
        null_glProgramUniform4iv,
        null_glProgramUniform4ivEXT,
        null_glProgramUniform4ui,
        null_glProgramUniform4ui64ARB,
        null_glProgramUniform4ui64NV,
        null_glProgramUniform4ui64vARB,
        null_glProgramUniform4ui64vNV,
        null_glProgramUniform4uiEXT,
        null_glProgramUniform4uiv,
        null_glProgramUniform4uivEXT,
        null_glProgramUniformHandleui64ARB,
        null_glProgramUniformHandleui64NV,
        null_glProgramUniformHandleui64vARB,
        null_glProgramUniformHandleui64vNV,
        null_glProgramUniformMatrix2dv,
        null_glProgramUniformMatrix2dvEXT,
        null_glProgramUniformMatrix2fv,
        null_glProgramUniformMatrix2fvEXT,
        null_glProgramUniformMatrix2x3dv,
        null_glProgramUniformMatrix2x3dvEXT,
        null_glProgramUniformMatrix2x3fv,
        null_glProgramUniformMatrix2x3fvEXT,
        null_glProgramUniformMatrix2x4dv,
        null_glProgramUniformMatrix2x4dvEXT,
        null_glProgramUniformMatrix2x4fv,
        null_glProgramUniformMatrix2x4fvEXT,
        null_glProgramUniformMatrix3dv,
        null_glProgramUniformMatrix3dvEXT,
        null_glProgramUniformMatrix3fv,
        null_glProgramUniformMatrix3fvEXT,
        null_glProgramUniformMatrix3x2dv,
        null_glProgramUniformMatrix3x2dvEXT,
        null_glProgramUniformMatrix3x2fv,
        null_glProgramUniformMatrix3x2fvEXT,
        null_glProgramUniformMatrix3x4dv,
        null_glProgramUniformMatrix3x4dvEXT,
        null_glProgramUniformMatrix3x4fv,
        null_glProgramUniformMatrix3x4fvEXT,
        null_glProgramUniformMatrix4dv,
        null_glProgramUniformMatrix4dvEXT,
        null_glProgramUniformMatrix4fv,
        null_glProgramUniformMatrix4fvEXT,
        null_glProgramUniformMatrix4x2dv,
        null_glProgramUniformMatrix4x2dvEXT,
        null_glProgramUniformMatrix4x2fv,
        null_glProgramUniformMatrix4x2fvEXT,
        null_glProgramUniformMatrix4x3dv,
        null_glProgramUniformMatrix4x3dvEXT,
        null_glProgramUniformMatrix4x3fv,
        null_glProgramUniformMatrix4x3fvEXT,
        null_glProgramUniformui64NV,
        null_glProgramUniformui64vNV,
        null_glProgramVertexLimitNV,
        null_glProvokingVertex,
        null_glProvokingVertexEXT,
        null_glPushClientAttribDefaultEXT,
        null_glPushDebugGroup,
        null_glPushGroupMarkerEXT,
        null_glQueryCounter,
        null_glQueryMatrixxOES,
        null_glQueryObjectParameteruiAMD,
        null_glQueryResourceNV,
        null_glQueryResourceTagNV,
        null_glRasterPos2xOES,
        null_glRasterPos2xvOES,
        null_glRasterPos3xOES,
        null_glRasterPos3xvOES,
        null_glRasterPos4xOES,
        null_glRasterPos4xvOES,
        null_glRasterSamplesEXT,
        null_glReadBufferRegion,
        null_glReadInstrumentsSGIX,
        null_glReadnPixels,
        null_glReadnPixelsARB,
        null_glRectxOES,
        null_glRectxvOES,
        null_glReferencePlaneSGIX,
        null_glReleaseKeyedMutexWin32EXT,
        null_glReleaseShaderCompiler,
        null_glRenderGpuMaskNV,
        null_glRenderbufferStorage,
        null_glRenderbufferStorageEXT,
        null_glRenderbufferStorageMultisample,
        null_glRenderbufferStorageMultisampleAdvancedAMD,
        null_glRenderbufferStorageMultisampleCoverageNV,
        null_glRenderbufferStorageMultisampleEXT,
        null_glReplacementCodePointerSUN,
        null_glReplacementCodeubSUN,
        null_glReplacementCodeubvSUN,
        null_glReplacementCodeuiColor3fVertex3fSUN,
        null_glReplacementCodeuiColor3fVertex3fvSUN,
        null_glReplacementCodeuiColor4fNormal3fVertex3fSUN,
        null_glReplacementCodeuiColor4fNormal3fVertex3fvSUN,
        null_glReplacementCodeuiColor4ubVertex3fSUN,
        null_glReplacementCodeuiColor4ubVertex3fvSUN,
        null_glReplacementCodeuiNormal3fVertex3fSUN,
        null_glReplacementCodeuiNormal3fVertex3fvSUN,
        null_glReplacementCodeuiSUN,
        null_glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fSUN,
        null_glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fvSUN,
        null_glReplacementCodeuiTexCoord2fNormal3fVertex3fSUN,
        null_glReplacementCodeuiTexCoord2fNormal3fVertex3fvSUN,
        null_glReplacementCodeuiTexCoord2fVertex3fSUN,
        null_glReplacementCodeuiTexCoord2fVertex3fvSUN,
        null_glReplacementCodeuiVertex3fSUN,
        null_glReplacementCodeuiVertex3fvSUN,
        null_glReplacementCodeuivSUN,
        null_glReplacementCodeusSUN,
        null_glReplacementCodeusvSUN,
        null_glRequestResidentProgramsNV,
        null_glResetHistogram,
        null_glResetHistogramEXT,
        null_glResetMemoryObjectParameterNV,
        null_glResetMinmax,
        null_glResetMinmaxEXT,
        null_glResizeBuffersMESA,
        null_glResolveDepthValuesNV,
        null_glResumeTransformFeedback,
        null_glResumeTransformFeedbackNV,
        null_glRotatexOES,
        null_glSampleCoverage,
        null_glSampleCoverageARB,
        null_glSampleMapATI,
        null_glSampleMaskEXT,
        null_glSampleMaskIndexedNV,
        null_glSampleMaskSGIS,
        null_glSampleMaski,
        null_glSamplePatternEXT,
        null_glSamplePatternSGIS,
        null_glSamplerParameterIiv,
        null_glSamplerParameterIuiv,
        null_glSamplerParameterf,
        null_glSamplerParameterfv,
        null_glSamplerParameteri,
        null_glSamplerParameteriv,
        null_glScalexOES,
        null_glScissorArrayv,
        null_glScissorExclusiveArrayvNV,
        null_glScissorExclusiveNV,
        null_glScissorIndexed,
        null_glScissorIndexedv,
        null_glSecondaryColor3b,
        null_glSecondaryColor3bEXT,
        null_glSecondaryColor3bv,
        null_glSecondaryColor3bvEXT,
        null_glSecondaryColor3d,
        null_glSecondaryColor3dEXT,
        null_glSecondaryColor3dv,
        null_glSecondaryColor3dvEXT,
        null_glSecondaryColor3f,
        null_glSecondaryColor3fEXT,
        null_glSecondaryColor3fv,
        null_glSecondaryColor3fvEXT,
        null_glSecondaryColor3hNV,
        null_glSecondaryColor3hvNV,
        null_glSecondaryColor3i,
        null_glSecondaryColor3iEXT,
        null_glSecondaryColor3iv,
        null_glSecondaryColor3ivEXT,
        null_glSecondaryColor3s,
        null_glSecondaryColor3sEXT,
        null_glSecondaryColor3sv,
        null_glSecondaryColor3svEXT,
        null_glSecondaryColor3ub,
        null_glSecondaryColor3ubEXT,
        null_glSecondaryColor3ubv,
        null_glSecondaryColor3ubvEXT,
        null_glSecondaryColor3ui,
        null_glSecondaryColor3uiEXT,
        null_glSecondaryColor3uiv,
        null_glSecondaryColor3uivEXT,
        null_glSecondaryColor3us,
        null_glSecondaryColor3usEXT,
        null_glSecondaryColor3usv,
        null_glSecondaryColor3usvEXT,
        null_glSecondaryColorFormatNV,
        null_glSecondaryColorP3ui,
        null_glSecondaryColorP3uiv,
        null_glSecondaryColorPointer,
        null_glSecondaryColorPointerEXT,
        null_glSecondaryColorPointerListIBM,
        null_glSelectPerfMonitorCountersAMD,
        null_glSelectTextureCoordSetSGIS,
        null_glSelectTextureSGIS,
        null_glSemaphoreParameterui64vEXT,
        null_glSeparableFilter2D,
        null_glSeparableFilter2DEXT,
        null_glSetFenceAPPLE,
        null_glSetFenceNV,
        null_glSetFragmentShaderConstantATI,
        null_glSetInvariantEXT,
        null_glSetLocalConstantEXT,
        null_glSetMultisamplefvAMD,
        null_glShaderBinary,
        null_glShaderOp1EXT,
        null_glShaderOp2EXT,
        null_glShaderOp3EXT,
        null_glShaderSource,
        null_glShaderSourceARB,
        null_glShaderStorageBlockBinding,
        null_glShadingRateImageBarrierNV,
        null_glShadingRateImagePaletteNV,
        null_glShadingRateSampleOrderCustomNV,
        null_glShadingRateSampleOrderNV,
        null_glSharpenTexFuncSGIS,
        null_glSignalSemaphoreEXT,
        null_glSignalSemaphoreui64NVX,
        null_glSignalVkFenceNV,
        null_glSignalVkSemaphoreNV,
        null_glSpecializeShader,
        null_glSpecializeShaderARB,
        null_glSpriteParameterfSGIX,
        null_glSpriteParameterfvSGIX,
        null_glSpriteParameteriSGIX,
        null_glSpriteParameterivSGIX,
        null_glStartInstrumentsSGIX,
        null_glStateCaptureNV,
        null_glStencilClearTagEXT,
        null_glStencilFillPathInstancedNV,
        null_glStencilFillPathNV,
        null_glStencilFuncSeparate,
        null_glStencilFuncSeparateATI,
        null_glStencilMaskSeparate,
        null_glStencilOpSeparate,
        null_glStencilOpSeparateATI,
        null_glStencilOpValueAMD,
        null_glStencilStrokePathInstancedNV,
        null_glStencilStrokePathNV,
        null_glStencilThenCoverFillPathInstancedNV,
        null_glStencilThenCoverFillPathNV,
        null_glStencilThenCoverStrokePathInstancedNV,
        null_glStencilThenCoverStrokePathNV,
        null_glStopInstrumentsSGIX,
        null_glStringMarkerGREMEDY,
        null_glSubpixelPrecisionBiasNV,
        null_glSwizzleEXT,
        null_glSyncTextureINTEL,
        null_glTagSampleBufferSGIX,
        null_glTangent3bEXT,
        null_glTangent3bvEXT,
        null_glTangent3dEXT,
        null_glTangent3dvEXT,
        null_glTangent3fEXT,
        null_glTangent3fvEXT,
        null_glTangent3iEXT,
        null_glTangent3ivEXT,
        null_glTangent3sEXT,
        null_glTangent3svEXT,
        null_glTangentPointerEXT,
        null_glTbufferMask3DFX,
        null_glTessellationFactorAMD,
        null_glTessellationModeAMD,
        null_glTestFenceAPPLE,
        null_glTestFenceNV,
        null_glTestObjectAPPLE,
        null_glTexAttachMemoryNV,
        null_glTexBuffer,
        null_glTexBufferARB,
        null_glTexBufferEXT,
        null_glTexBufferRange,
        null_glTexBumpParameterfvATI,
        null_glTexBumpParameterivATI,
        null_glTexCoord1bOES,
        null_glTexCoord1bvOES,
        null_glTexCoord1hNV,
        null_glTexCoord1hvNV,
        null_glTexCoord1xOES,
        null_glTexCoord1xvOES,
        null_glTexCoord2bOES,
        null_glTexCoord2bvOES,
        null_glTexCoord2fColor3fVertex3fSUN,
        null_glTexCoord2fColor3fVertex3fvSUN,
        null_glTexCoord2fColor4fNormal3fVertex3fSUN,
        null_glTexCoord2fColor4fNormal3fVertex3fvSUN,
        null_glTexCoord2fColor4ubVertex3fSUN,
        null_glTexCoord2fColor4ubVertex3fvSUN,
        null_glTexCoord2fNormal3fVertex3fSUN,
        null_glTexCoord2fNormal3fVertex3fvSUN,
        null_glTexCoord2fVertex3fSUN,
        null_glTexCoord2fVertex3fvSUN,
        null_glTexCoord2hNV,
        null_glTexCoord2hvNV,
        null_glTexCoord2xOES,
        null_glTexCoord2xvOES,
        null_glTexCoord3bOES,
        null_glTexCoord3bvOES,
        null_glTexCoord3hNV,
        null_glTexCoord3hvNV,
        null_glTexCoord3xOES,
        null_glTexCoord3xvOES,
        null_glTexCoord4bOES,
        null_glTexCoord4bvOES,
        null_glTexCoord4fColor4fNormal3fVertex4fSUN,
        null_glTexCoord4fColor4fNormal3fVertex4fvSUN,
        null_glTexCoord4fVertex4fSUN,
        null_glTexCoord4fVertex4fvSUN,
        null_glTexCoord4hNV,
        null_glTexCoord4hvNV,
        null_glTexCoord4xOES,
        null_glTexCoord4xvOES,
        null_glTexCoordFormatNV,
        null_glTexCoordP1ui,
        null_glTexCoordP1uiv,
        null_glTexCoordP2ui,
        null_glTexCoordP2uiv,
        null_glTexCoordP3ui,
        null_glTexCoordP3uiv,
        null_glTexCoordP4ui,
        null_glTexCoordP4uiv,
        null_glTexCoordPointerEXT,
        null_glTexCoordPointerListIBM,
        null_glTexCoordPointervINTEL,
        null_glTexEnvxOES,
        null_glTexEnvxvOES,
        null_glTexFilterFuncSGIS,
        null_glTexGenxOES,
        null_glTexGenxvOES,
        null_glTexImage2DMultisample,
        null_glTexImage2DMultisampleCoverageNV,
        null_glTexImage3D,
        null_glTexImage3DEXT,
        null_glTexImage3DMultisample,
        null_glTexImage3DMultisampleCoverageNV,
        null_glTexImage4DSGIS,
        null_glTexPageCommitmentARB,
        null_glTexParameterIiv,
        null_glTexParameterIivEXT,
        null_glTexParameterIuiv,
        null_glTexParameterIuivEXT,
        null_glTexParameterxOES,
        null_glTexParameterxvOES,
        null_glTexRenderbufferNV,
        null_glTexStorage1D,
        null_glTexStorage2D,
        null_glTexStorage2DMultisample,
        null_glTexStorage3D,
        null_glTexStorage3DMultisample,
        null_glTexStorageMem1DEXT,
        null_glTexStorageMem2DEXT,
        null_glTexStorageMem2DMultisampleEXT,
        null_glTexStorageMem3DEXT,
        null_glTexStorageMem3DMultisampleEXT,
        null_glTexStorageSparseAMD,
        null_glTexSubImage1DEXT,
        null_glTexSubImage2DEXT,
        null_glTexSubImage3D,
        null_glTexSubImage3DEXT,
        null_glTexSubImage4DSGIS,
        null_glTextureAttachMemoryNV,
        null_glTextureBarrier,
        null_glTextureBarrierNV,
        null_glTextureBuffer,
        null_glTextureBufferEXT,
        null_glTextureBufferRange,
        null_glTextureBufferRangeEXT,
        null_glTextureColorMaskSGIS,
        null_glTextureImage1DEXT,
        null_glTextureImage2DEXT,
        null_glTextureImage2DMultisampleCoverageNV,
        null_glTextureImage2DMultisampleNV,
        null_glTextureImage3DEXT,
        null_glTextureImage3DMultisampleCoverageNV,
        null_glTextureImage3DMultisampleNV,
        null_glTextureLightEXT,
        null_glTextureMaterialEXT,
        null_glTextureNormalEXT,
        null_glTexturePageCommitmentEXT,
        null_glTextureParameterIiv,
        null_glTextureParameterIivEXT,
        null_glTextureParameterIuiv,
        null_glTextureParameterIuivEXT,
        null_glTextureParameterf,
        null_glTextureParameterfEXT,
        null_glTextureParameterfv,
        null_glTextureParameterfvEXT,
        null_glTextureParameteri,
        null_glTextureParameteriEXT,
        null_glTextureParameteriv,
        null_glTextureParameterivEXT,
        null_glTextureRangeAPPLE,
        null_glTextureRenderbufferEXT,
        null_glTextureStorage1D,
        null_glTextureStorage1DEXT,
        null_glTextureStorage2D,
        null_glTextureStorage2DEXT,
        null_glTextureStorage2DMultisample,
        null_glTextureStorage2DMultisampleEXT,
        null_glTextureStorage3D,
        null_glTextureStorage3DEXT,
        null_glTextureStorage3DMultisample,
        null_glTextureStorage3DMultisampleEXT,
        null_glTextureStorageMem1DEXT,
        null_glTextureStorageMem2DEXT,
        null_glTextureStorageMem2DMultisampleEXT,
        null_glTextureStorageMem3DEXT,
        null_glTextureStorageMem3DMultisampleEXT,
        null_glTextureStorageSparseAMD,
        null_glTextureSubImage1D,
        null_glTextureSubImage1DEXT,
        null_glTextureSubImage2D,
        null_glTextureSubImage2DEXT,
        null_glTextureSubImage3D,
        null_glTextureSubImage3DEXT,
        null_glTextureView,
        null_glTrackMatrixNV,
        null_glTransformFeedbackAttribsNV,
        null_glTransformFeedbackBufferBase,
        null_glTransformFeedbackBufferRange,
        null_glTransformFeedbackStreamAttribsNV,
        null_glTransformFeedbackVaryings,
        null_glTransformFeedbackVaryingsEXT,
        null_glTransformFeedbackVaryingsNV,
        null_glTransformPathNV,
        null_glTranslatexOES,
        null_glUniform1d,
        null_glUniform1dv,
        null_glUniform1f,
        null_glUniform1fARB,
        null_glUniform1fv,
        null_glUniform1fvARB,
        null_glUniform1i,
        null_glUniform1i64ARB,
        null_glUniform1i64NV,
        null_glUniform1i64vARB,
        null_glUniform1i64vNV,
        null_glUniform1iARB,
        null_glUniform1iv,
        null_glUniform1ivARB,
        null_glUniform1ui,
        null_glUniform1ui64ARB,
        null_glUniform1ui64NV,
        null_glUniform1ui64vARB,
        null_glUniform1ui64vNV,
        null_glUniform1uiEXT,
        null_glUniform1uiv,
        null_glUniform1uivEXT,
        null_glUniform2d,
        null_glUniform2dv,
        null_glUniform2f,
        null_glUniform2fARB,
        null_glUniform2fv,
        null_glUniform2fvARB,
        null_glUniform2i,
        null_glUniform2i64ARB,
        null_glUniform2i64NV,
        null_glUniform2i64vARB,
        null_glUniform2i64vNV,
        null_glUniform2iARB,
        null_glUniform2iv,
        null_glUniform2ivARB,
        null_glUniform2ui,
        null_glUniform2ui64ARB,
        null_glUniform2ui64NV,
        null_glUniform2ui64vARB,
        null_glUniform2ui64vNV,
        null_glUniform2uiEXT,
        null_glUniform2uiv,
        null_glUniform2uivEXT,
        null_glUniform3d,
        null_glUniform3dv,
        null_glUniform3f,
        null_glUniform3fARB,
        null_glUniform3fv,
        null_glUniform3fvARB,
        null_glUniform3i,
        null_glUniform3i64ARB,
        null_glUniform3i64NV,
        null_glUniform3i64vARB,
        null_glUniform3i64vNV,
        null_glUniform3iARB,
        null_glUniform3iv,
        null_glUniform3ivARB,
        null_glUniform3ui,
        null_glUniform3ui64ARB,
        null_glUniform3ui64NV,
        null_glUniform3ui64vARB,
        null_glUniform3ui64vNV,
        null_glUniform3uiEXT,
        null_glUniform3uiv,
        null_glUniform3uivEXT,
        null_glUniform4d,
        null_glUniform4dv,
        null_glUniform4f,
        null_glUniform4fARB,
        null_glUniform4fv,
        null_glUniform4fvARB,
        null_glUniform4i,
        null_glUniform4i64ARB,
        null_glUniform4i64NV,
        null_glUniform4i64vARB,
        null_glUniform4i64vNV,
        null_glUniform4iARB,
        null_glUniform4iv,
        null_glUniform4ivARB,
        null_glUniform4ui,
        null_glUniform4ui64ARB,
        null_glUniform4ui64NV,
        null_glUniform4ui64vARB,
        null_glUniform4ui64vNV,
        null_glUniform4uiEXT,
        null_glUniform4uiv,
        null_glUniform4uivEXT,
        null_glUniformBlockBinding,
        null_glUniformBufferEXT,
        null_glUniformHandleui64ARB,
        null_glUniformHandleui64NV,
        null_glUniformHandleui64vARB,
        null_glUniformHandleui64vNV,
        null_glUniformMatrix2dv,
        null_glUniformMatrix2fv,
        null_glUniformMatrix2fvARB,
        null_glUniformMatrix2x3dv,
        null_glUniformMatrix2x3fv,
        null_glUniformMatrix2x4dv,
        null_glUniformMatrix2x4fv,
        null_glUniformMatrix3dv,
        null_glUniformMatrix3fv,
        null_glUniformMatrix3fvARB,
        null_glUniformMatrix3x2dv,
        null_glUniformMatrix3x2fv,
        null_glUniformMatrix3x4dv,
        null_glUniformMatrix3x4fv,
        null_glUniformMatrix4dv,
        null_glUniformMatrix4fv,
        null_glUniformMatrix4fvARB,
        null_glUniformMatrix4x2dv,
        null_glUniformMatrix4x2fv,
        null_glUniformMatrix4x3dv,
        null_glUniformMatrix4x3fv,
        null_glUniformSubroutinesuiv,
        null_glUniformui64NV,
        null_glUniformui64vNV,
        null_glUnlockArraysEXT,
        null_glUnmapBuffer,
        null_glUnmapBufferARB,
        null_glUnmapNamedBuffer,
        null_glUnmapNamedBufferEXT,
        null_glUnmapObjectBufferATI,
        null_glUnmapTexture2DINTEL,
        null_glUpdateObjectBufferATI,
        null_glUploadGpuMaskNVX,
        null_glUseProgram,
        null_glUseProgramObjectARB,
        null_glUseProgramStages,
        null_glUseShaderProgramEXT,
        null_glVDPAUFiniNV,
        null_glVDPAUGetSurfaceivNV,
        null_glVDPAUInitNV,
        null_glVDPAUIsSurfaceNV,
        null_glVDPAUMapSurfacesNV,
        null_glVDPAURegisterOutputSurfaceNV,
        null_glVDPAURegisterVideoSurfaceNV,
        null_glVDPAURegisterVideoSurfaceWithPictureStructureNV,
        null_glVDPAUSurfaceAccessNV,
        null_glVDPAUUnmapSurfacesNV,
        null_glVDPAUUnregisterSurfaceNV,
        null_glValidateProgram,
        null_glValidateProgramARB,
        null_glValidateProgramPipeline,
        null_glVariantArrayObjectATI,
        null_glVariantPointerEXT,
        null_glVariantbvEXT,
        null_glVariantdvEXT,
        null_glVariantfvEXT,
        null_glVariantivEXT,
        null_glVariantsvEXT,
        null_glVariantubvEXT,
        null_glVariantuivEXT,
        null_glVariantusvEXT,
        null_glVertex2bOES,
        null_glVertex2bvOES,
        null_glVertex2hNV,
        null_glVertex2hvNV,
        null_glVertex2xOES,
        null_glVertex2xvOES,
        null_glVertex3bOES,
        null_glVertex3bvOES,
        null_glVertex3hNV,
        null_glVertex3hvNV,
        null_glVertex3xOES,
        null_glVertex3xvOES,
        null_glVertex4bOES,
        null_glVertex4bvOES,
        null_glVertex4hNV,
        null_glVertex4hvNV,
        null_glVertex4xOES,
        null_glVertex4xvOES,
        null_glVertexArrayAttribBinding,
        null_glVertexArrayAttribFormat,
        null_glVertexArrayAttribIFormat,
        null_glVertexArrayAttribLFormat,
        null_glVertexArrayBindVertexBufferEXT,
        null_glVertexArrayBindingDivisor,
        null_glVertexArrayColorOffsetEXT,
        null_glVertexArrayEdgeFlagOffsetEXT,
        null_glVertexArrayElementBuffer,
        null_glVertexArrayFogCoordOffsetEXT,
        null_glVertexArrayIndexOffsetEXT,
        null_glVertexArrayMultiTexCoordOffsetEXT,
        null_glVertexArrayNormalOffsetEXT,
        null_glVertexArrayParameteriAPPLE,
        null_glVertexArrayRangeAPPLE,
        null_glVertexArrayRangeNV,
        null_glVertexArraySecondaryColorOffsetEXT,
        null_glVertexArrayTexCoordOffsetEXT,
        null_glVertexArrayVertexAttribBindingEXT,
        null_glVertexArrayVertexAttribDivisorEXT,
        null_glVertexArrayVertexAttribFormatEXT,
        null_glVertexArrayVertexAttribIFormatEXT,
        null_glVertexArrayVertexAttribIOffsetEXT,
        null_glVertexArrayVertexAttribLFormatEXT,
        null_glVertexArrayVertexAttribLOffsetEXT,
        null_glVertexArrayVertexAttribOffsetEXT,
        null_glVertexArrayVertexBindingDivisorEXT,
        null_glVertexArrayVertexBuffer,
        null_glVertexArrayVertexBuffers,
        null_glVertexArrayVertexOffsetEXT,
        null_glVertexAttrib1d,
        null_glVertexAttrib1dARB,
        null_glVertexAttrib1dNV,
        null_glVertexAttrib1dv,
        null_glVertexAttrib1dvARB,
        null_glVertexAttrib1dvNV,
        null_glVertexAttrib1f,
        null_glVertexAttrib1fARB,
        null_glVertexAttrib1fNV,
        null_glVertexAttrib1fv,
        null_glVertexAttrib1fvARB,
        null_glVertexAttrib1fvNV,
        null_glVertexAttrib1hNV,
        null_glVertexAttrib1hvNV,
        null_glVertexAttrib1s,
        null_glVertexAttrib1sARB,
        null_glVertexAttrib1sNV,
        null_glVertexAttrib1sv,
        null_glVertexAttrib1svARB,
        null_glVertexAttrib1svNV,
        null_glVertexAttrib2d,
        null_glVertexAttrib2dARB,
        null_glVertexAttrib2dNV,
        null_glVertexAttrib2dv,
        null_glVertexAttrib2dvARB,
        null_glVertexAttrib2dvNV,
        null_glVertexAttrib2f,
        null_glVertexAttrib2fARB,
        null_glVertexAttrib2fNV,
        null_glVertexAttrib2fv,
        null_glVertexAttrib2fvARB,
        null_glVertexAttrib2fvNV,
        null_glVertexAttrib2hNV,
        null_glVertexAttrib2hvNV,
        null_glVertexAttrib2s,
        null_glVertexAttrib2sARB,
        null_glVertexAttrib2sNV,
        null_glVertexAttrib2sv,
        null_glVertexAttrib2svARB,
        null_glVertexAttrib2svNV,
        null_glVertexAttrib3d,
        null_glVertexAttrib3dARB,
        null_glVertexAttrib3dNV,
        null_glVertexAttrib3dv,
        null_glVertexAttrib3dvARB,
        null_glVertexAttrib3dvNV,
        null_glVertexAttrib3f,
        null_glVertexAttrib3fARB,
        null_glVertexAttrib3fNV,
        null_glVertexAttrib3fv,
        null_glVertexAttrib3fvARB,
        null_glVertexAttrib3fvNV,
        null_glVertexAttrib3hNV,
        null_glVertexAttrib3hvNV,
        null_glVertexAttrib3s,
        null_glVertexAttrib3sARB,
        null_glVertexAttrib3sNV,
        null_glVertexAttrib3sv,
        null_glVertexAttrib3svARB,
        null_glVertexAttrib3svNV,
        null_glVertexAttrib4Nbv,
        null_glVertexAttrib4NbvARB,
        null_glVertexAttrib4Niv,
        null_glVertexAttrib4NivARB,
        null_glVertexAttrib4Nsv,
        null_glVertexAttrib4NsvARB,
        null_glVertexAttrib4Nub,
        null_glVertexAttrib4NubARB,
        null_glVertexAttrib4Nubv,
        null_glVertexAttrib4NubvARB,
        null_glVertexAttrib4Nuiv,
        null_glVertexAttrib4NuivARB,
        null_glVertexAttrib4Nusv,
        null_glVertexAttrib4NusvARB,
        null_glVertexAttrib4bv,
        null_glVertexAttrib4bvARB,
        null_glVertexAttrib4d,
        null_glVertexAttrib4dARB,
        null_glVertexAttrib4dNV,
        null_glVertexAttrib4dv,
        null_glVertexAttrib4dvARB,
        null_glVertexAttrib4dvNV,
        null_glVertexAttrib4f,
        null_glVertexAttrib4fARB,
        null_glVertexAttrib4fNV,
        null_glVertexAttrib4fv,
        null_glVertexAttrib4fvARB,
        null_glVertexAttrib4fvNV,
        null_glVertexAttrib4hNV,
        null_glVertexAttrib4hvNV,
        null_glVertexAttrib4iv,
        null_glVertexAttrib4ivARB,
        null_glVertexAttrib4s,
        null_glVertexAttrib4sARB,
        null_glVertexAttrib4sNV,
        null_glVertexAttrib4sv,
        null_glVertexAttrib4svARB,
        null_glVertexAttrib4svNV,
        null_glVertexAttrib4ubNV,
        null_glVertexAttrib4ubv,
        null_glVertexAttrib4ubvARB,
        null_glVertexAttrib4ubvNV,
        null_glVertexAttrib4uiv,
        null_glVertexAttrib4uivARB,
        null_glVertexAttrib4usv,
        null_glVertexAttrib4usvARB,
        null_glVertexAttribArrayObjectATI,
        null_glVertexAttribBinding,
        null_glVertexAttribDivisor,
        null_glVertexAttribDivisorARB,
        null_glVertexAttribFormat,
        null_glVertexAttribFormatNV,
        null_glVertexAttribI1i,
        null_glVertexAttribI1iEXT,
        null_glVertexAttribI1iv,
        null_glVertexAttribI1ivEXT,
        null_glVertexAttribI1ui,
        null_glVertexAttribI1uiEXT,
        null_glVertexAttribI1uiv,
        null_glVertexAttribI1uivEXT,
        null_glVertexAttribI2i,
        null_glVertexAttribI2iEXT,
        null_glVertexAttribI2iv,
        null_glVertexAttribI2ivEXT,
        null_glVertexAttribI2ui,
        null_glVertexAttribI2uiEXT,
        null_glVertexAttribI2uiv,
        null_glVertexAttribI2uivEXT,
        null_glVertexAttribI3i,
        null_glVertexAttribI3iEXT,
        null_glVertexAttribI3iv,
        null_glVertexAttribI3ivEXT,
        null_glVertexAttribI3ui,
        null_glVertexAttribI3uiEXT,
        null_glVertexAttribI3uiv,
        null_glVertexAttribI3uivEXT,
        null_glVertexAttribI4bv,
        null_glVertexAttribI4bvEXT,
        null_glVertexAttribI4i,
        null_glVertexAttribI4iEXT,
        null_glVertexAttribI4iv,
        null_glVertexAttribI4ivEXT,
        null_glVertexAttribI4sv,
        null_glVertexAttribI4svEXT,
        null_glVertexAttribI4ubv,
        null_glVertexAttribI4ubvEXT,
        null_glVertexAttribI4ui,
        null_glVertexAttribI4uiEXT,
        null_glVertexAttribI4uiv,
        null_glVertexAttribI4uivEXT,
        null_glVertexAttribI4usv,
        null_glVertexAttribI4usvEXT,
        null_glVertexAttribIFormat,
        null_glVertexAttribIFormatNV,
        null_glVertexAttribIPointer,
        null_glVertexAttribIPointerEXT,
        null_glVertexAttribL1d,
        null_glVertexAttribL1dEXT,
        null_glVertexAttribL1dv,
        null_glVertexAttribL1dvEXT,
        null_glVertexAttribL1i64NV,
        null_glVertexAttribL1i64vNV,
        null_glVertexAttribL1ui64ARB,
        null_glVertexAttribL1ui64NV,
        null_glVertexAttribL1ui64vARB,
        null_glVertexAttribL1ui64vNV,
        null_glVertexAttribL2d,
        null_glVertexAttribL2dEXT,
        null_glVertexAttribL2dv,
        null_glVertexAttribL2dvEXT,
        null_glVertexAttribL2i64NV,
        null_glVertexAttribL2i64vNV,
        null_glVertexAttribL2ui64NV,
        null_glVertexAttribL2ui64vNV,
        null_glVertexAttribL3d,
        null_glVertexAttribL3dEXT,
        null_glVertexAttribL3dv,
        null_glVertexAttribL3dvEXT,
        null_glVertexAttribL3i64NV,
        null_glVertexAttribL3i64vNV,
        null_glVertexAttribL3ui64NV,
        null_glVertexAttribL3ui64vNV,
        null_glVertexAttribL4d,
        null_glVertexAttribL4dEXT,
        null_glVertexAttribL4dv,
        null_glVertexAttribL4dvEXT,
        null_glVertexAttribL4i64NV,
        null_glVertexAttribL4i64vNV,
        null_glVertexAttribL4ui64NV,
        null_glVertexAttribL4ui64vNV,
        null_glVertexAttribLFormat,
        null_glVertexAttribLFormatNV,
        null_glVertexAttribLPointer,
        null_glVertexAttribLPointerEXT,
        null_glVertexAttribP1ui,
        null_glVertexAttribP1uiv,
        null_glVertexAttribP2ui,
        null_glVertexAttribP2uiv,
        null_glVertexAttribP3ui,
        null_glVertexAttribP3uiv,
        null_glVertexAttribP4ui,
        null_glVertexAttribP4uiv,
        null_glVertexAttribParameteriAMD,
        null_glVertexAttribPointer,
        null_glVertexAttribPointerARB,
        null_glVertexAttribPointerNV,
        null_glVertexAttribs1dvNV,
        null_glVertexAttribs1fvNV,
        null_glVertexAttribs1hvNV,
        null_glVertexAttribs1svNV,
        null_glVertexAttribs2dvNV,
        null_glVertexAttribs2fvNV,
        null_glVertexAttribs2hvNV,
        null_glVertexAttribs2svNV,
        null_glVertexAttribs3dvNV,
        null_glVertexAttribs3fvNV,
        null_glVertexAttribs3hvNV,
        null_glVertexAttribs3svNV,
        null_glVertexAttribs4dvNV,
        null_glVertexAttribs4fvNV,
        null_glVertexAttribs4hvNV,
        null_glVertexAttribs4svNV,
        null_glVertexAttribs4ubvNV,
        null_glVertexBindingDivisor,
        null_glVertexBlendARB,
        null_glVertexBlendEnvfATI,
        null_glVertexBlendEnviATI,
        null_glVertexFormatNV,
        null_glVertexP2ui,
        null_glVertexP2uiv,
        null_glVertexP3ui,
        null_glVertexP3uiv,
        null_glVertexP4ui,
        null_glVertexP4uiv,
        null_glVertexPointerEXT,
        null_glVertexPointerListIBM,
        null_glVertexPointervINTEL,
        null_glVertexStream1dATI,
        null_glVertexStream1dvATI,
        null_glVertexStream1fATI,
        null_glVertexStream1fvATI,
        null_glVertexStream1iATI,
        null_glVertexStream1ivATI,
        null_glVertexStream1sATI,
        null_glVertexStream1svATI,
        null_glVertexStream2dATI,
        null_glVertexStream2dvATI,
        null_glVertexStream2fATI,
        null_glVertexStream2fvATI,
        null_glVertexStream2iATI,
        null_glVertexStream2ivATI,
        null_glVertexStream2sATI,
        null_glVertexStream2svATI,
        null_glVertexStream3dATI,
        null_glVertexStream3dvATI,
        null_glVertexStream3fATI,
        null_glVertexStream3fvATI,
        null_glVertexStream3iATI,
        null_glVertexStream3ivATI,
        null_glVertexStream3sATI,
        null_glVertexStream3svATI,
        null_glVertexStream4dATI,
        null_glVertexStream4dvATI,
        null_glVertexStream4fATI,
        null_glVertexStream4fvATI,
        null_glVertexStream4iATI,
        null_glVertexStream4ivATI,
        null_glVertexStream4sATI,
        null_glVertexStream4svATI,
        null_glVertexWeightPointerEXT,
        null_glVertexWeightfEXT,
        null_glVertexWeightfvEXT,
        null_glVertexWeighthNV,
        null_glVertexWeighthvNV,
        null_glVideoCaptureNV,
        null_glVideoCaptureStreamParameterdvNV,
        null_glVideoCaptureStreamParameterfvNV,
        null_glVideoCaptureStreamParameterivNV,
        null_glViewportArrayv,
        null_glViewportIndexedf,
        null_glViewportIndexedfv,
        null_glViewportPositionWScaleNV,
        null_glViewportSwizzleNV,
        null_glWaitSemaphoreEXT,
        null_glWaitSemaphoreui64NVX,
        null_glWaitSync,
        null_glWaitVkSemaphoreNV,
        null_glWeightPathsNV,
        null_glWeightPointerARB,
        null_glWeightbvARB,
        null_glWeightdvARB,
        null_glWeightfvARB,
        null_glWeightivARB,
        null_glWeightsvARB,
        null_glWeightubvARB,
        null_glWeightuivARB,
        null_glWeightusvARB,
        null_glWindowPos2d,
        null_glWindowPos2dARB,
        null_glWindowPos2dMESA,
        null_glWindowPos2dv,
        null_glWindowPos2dvARB,
        null_glWindowPos2dvMESA,
        null_glWindowPos2f,
        null_glWindowPos2fARB,
        null_glWindowPos2fMESA,
        null_glWindowPos2fv,
        null_glWindowPos2fvARB,
        null_glWindowPos2fvMESA,
        null_glWindowPos2i,
        null_glWindowPos2iARB,
        null_glWindowPos2iMESA,
        null_glWindowPos2iv,
        null_glWindowPos2ivARB,
        null_glWindowPos2ivMESA,
        null_glWindowPos2s,
        null_glWindowPos2sARB,
        null_glWindowPos2sMESA,
        null_glWindowPos2sv,
        null_glWindowPos2svARB,
        null_glWindowPos2svMESA,
        null_glWindowPos3d,
        null_glWindowPos3dARB,
        null_glWindowPos3dMESA,
        null_glWindowPos3dv,
        null_glWindowPos3dvARB,
        null_glWindowPos3dvMESA,
        null_glWindowPos3f,
        null_glWindowPos3fARB,
        null_glWindowPos3fMESA,
        null_glWindowPos3fv,
        null_glWindowPos3fvARB,
        null_glWindowPos3fvMESA,
        null_glWindowPos3i,
        null_glWindowPos3iARB,
        null_glWindowPos3iMESA,
        null_glWindowPos3iv,
        null_glWindowPos3ivARB,
        null_glWindowPos3ivMESA,
        null_glWindowPos3s,
        null_glWindowPos3sARB,
        null_glWindowPos3sMESA,
        null_glWindowPos3sv,
        null_glWindowPos3svARB,
        null_glWindowPos3svMESA,
        null_glWindowPos4dMESA,
        null_glWindowPos4dvMESA,
        null_glWindowPos4fMESA,
        null_glWindowPos4fvMESA,
        null_glWindowPos4iMESA,
        null_glWindowPos4ivMESA,
        null_glWindowPos4sMESA,
        null_glWindowPos4svMESA,
        null_glWindowRectanglesEXT,
        null_glWriteMaskEXT,
        null_wglAllocateMemoryNV,
        null_wglBindTexImageARB,
        null_wglChoosePixelFormatARB,
        null_wglCreateContextAttribsARB,
        null_wglCreatePbufferARB,
        null_wglDestroyPbufferARB,
        null_wglFreeMemoryNV,
        null_wglGetCurrentReadDCARB,
        null_wglGetExtensionsStringARB,
        null_wglGetExtensionsStringEXT,
        null_wglGetPbufferDCARB,
        null_wglGetPixelFormatAttribfvARB,
        null_wglGetPixelFormatAttribivARB,
        null_wglGetSwapIntervalEXT,
        null_wglMakeContextCurrentARB,
        null_wglQueryCurrentRendererIntegerWINE,
        null_wglQueryCurrentRendererStringWINE,
        null_wglQueryPbufferARB,
        null_wglQueryRendererIntegerWINE,
        null_wglQueryRendererStringWINE,
        null_wglReleasePbufferDCARB,
        null_wglReleaseTexImageARB,
        null_wglSetPbufferAttribARB,
        null_wglSetPixelFormatWINE,
        null_wglSwapIntervalEXT,
    },
};
