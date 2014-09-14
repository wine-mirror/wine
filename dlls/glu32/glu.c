/*
 * Copyright 2001 Marcus Meissner
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(glu);

/* The only non-trivial bit of this is the *Tess* functions.  Here we
   need to wrap the callbacks up in a thunk to switch calling
   conventions, so we use our own tesselator type to store the
   application's callbacks.  wine_gluTessCallback always sets the
   *_DATA type of callback so that we have access to the polygon_data
   (which is in fact just wine_tess_t), in the thunk itself we can
   check whether we should call the _DATA or non _DATA type. */

typedef struct {
    void *tess;
    void *polygon_data;
    void (CALLBACK *cb_tess_begin)(int);
    void (CALLBACK *cb_tess_begin_data)(int, void *);
    void (CALLBACK *cb_tess_vertex)(void *);
    void (CALLBACK *cb_tess_vertex_data)(void *, void *);
    void (CALLBACK *cb_tess_end)(void);
    void (CALLBACK *cb_tess_end_data)(void *);
    void (CALLBACK *cb_tess_error)(int);
    void (CALLBACK *cb_tess_error_data)(int, void *);
    void (CALLBACK *cb_tess_edge_flag)(int);
    void (CALLBACK *cb_tess_edge_flag_data)(int, void *);
    void (CALLBACK *cb_tess_combine)(double *, void *, float *, void **);
    void (CALLBACK *cb_tess_combine_data)(double *, void *, float *, void **, void *);
} wine_tess_t;

#define GLU_TESS_BEGIN          100100
#define GLU_TESS_VERTEX         100101
#define GLU_TESS_END            100102
#define GLU_TESS_ERROR          100103
#define GLU_TESS_EDGE_FLAG      100104
#define GLU_TESS_COMBINE        100105
#define GLU_TESS_BEGIN_DATA     100106
#define GLU_TESS_VERTEX_DATA    100107
#define GLU_TESS_END_DATA       100108
#define GLU_TESS_ERROR_DATA     100109
#define GLU_TESS_EDGE_FLAG_DATA 100110
#define GLU_TESS_COMBINE_DATA   100111

/***********************************************************************
 *		gluLookAt (GLU32.@)
 */
extern int gluLookAt(double arg0,double arg1,double arg2,double arg3,double arg4,double arg5,double arg6,double arg7,double arg8);
int WINAPI wine_gluLookAt(double arg0,double arg1,double arg2,double arg3,double arg4,double arg5,double arg6,double arg7,double arg8) {
	return gluLookAt(arg0,arg1,arg2,arg3,arg4,arg5,arg6,arg7,arg8);
}

/***********************************************************************
 *		gluOrtho2D (GLU32.@)
 */
extern int gluOrtho2D(double arg0,double arg1,double arg2,double arg3);
int WINAPI wine_gluOrtho2D(double arg0,double arg1,double arg2,double arg3) {
	return gluOrtho2D(arg0,arg1,arg2,arg3);
}

/***********************************************************************
 *		gluPerspective (GLU32.@)
 */
extern int gluPerspective(double arg0,double arg1,double arg2,double arg3);
int WINAPI wine_gluPerspective(double arg0,double arg1,double arg2,double arg3) {
	return gluPerspective(arg0,arg1,arg2,arg3);
}

/***********************************************************************
 *		gluPickMatrix (GLU32.@)
 */
extern int gluPickMatrix(double arg0,double arg1,double arg2,double arg3,void *arg4);
int WINAPI wine_gluPickMatrix(double arg0,double arg1,double arg2,double arg3,void *arg4) {
	return gluPickMatrix(arg0,arg1,arg2,arg3,arg4);
}

/***********************************************************************
 *		gluProject (GLU32.@)
 */
extern int gluProject(double arg0,double arg1,double arg2,void *arg3,void *arg4,void *arg5,void *arg6,void *arg7,void *arg8);
int WINAPI wine_gluProject(double arg0,double arg1,double arg2,void *arg3,void *arg4,void *arg5,void *arg6,void *arg7,void *arg8) {
	return gluProject(arg0,arg1,arg2,arg3,arg4,arg5,arg6,arg7,arg8);
}

/***********************************************************************
 *		gluUnProject (GLU32.@)
 */
extern int gluUnProject(double arg0,double arg1,double arg2,void *arg3,void *arg4,void *arg5,void *arg6,void *arg7,void *arg8);
int WINAPI wine_gluUnProject(double arg0,double arg1,double arg2,void *arg3,void *arg4,void *arg5,void *arg6,void *arg7,void *arg8) {
	return gluUnProject(arg0,arg1,arg2,arg3,arg4,arg5,arg6,arg7,arg8);
}

/***********************************************************************
 *		gluErrorString (GLU32.@)
 */
extern int gluErrorString(int arg0);
int WINAPI wine_gluErrorString(int arg0) {
	return gluErrorString(arg0);
}

/***********************************************************************
 *		gluScaleImage (GLU32.@)
 */
extern int gluScaleImage(int arg0,int arg1,int arg2,int arg3,void *arg4,int arg5,int arg6,int arg7,void *arg8);
int WINAPI wine_gluScaleImage(int arg0,int arg1,int arg2,int arg3,void *arg4,int arg5,int arg6,int arg7,void *arg8) {
	return gluScaleImage(arg0,arg1,arg2,arg3,arg4,arg5,arg6,arg7,arg8);
}

/***********************************************************************
 *		gluBuild1DMipmaps (GLU32.@)
 */
extern int gluBuild1DMipmaps(int arg0,int arg1,int arg2,int arg3,int arg4,void *arg5);
int WINAPI wine_gluBuild1DMipmaps(int arg0,int arg1,int arg2,int arg3,int arg4,void *arg5) {
	return gluBuild1DMipmaps(arg0,arg1,arg2,arg3,arg4,arg5);
}

/***********************************************************************
 *		gluBuild2DMipmaps (GLU32.@)
 */
extern int gluBuild2DMipmaps(int arg0,int arg1,int arg2,int arg3,int arg4,int arg5,void *arg6);
int WINAPI wine_gluBuild2DMipmaps(int arg0,int arg1,int arg2,int arg3,int arg4,int arg5,void *arg6) {
	return gluBuild2DMipmaps(arg0,arg1,arg2,arg3,arg4,arg5,arg6);
}

/***********************************************************************
 *		gluNewQuadric (GLU32.@)
 */
extern int gluNewQuadric(void);
int WINAPI wine_gluNewQuadric(void) {
	return gluNewQuadric();
}

/***********************************************************************
 *		gluDeleteQuadric (GLU32.@)
 */
extern int gluDeleteQuadric(void *arg0);
int WINAPI wine_gluDeleteQuadric(void *arg0) {
	return gluDeleteQuadric(arg0);
}

/***********************************************************************
 *		gluQuadricDrawStyle (GLU32.@)
 */
extern int gluQuadricDrawStyle(void *arg0,int arg1);
int WINAPI wine_gluQuadricDrawStyle(void *arg0,int arg1) {
	return gluQuadricDrawStyle(arg0,arg1);
}

/***********************************************************************
 *		gluQuadricOrientation (GLU32.@)
 */
extern int gluQuadricOrientation(void *arg0,int arg1);
int WINAPI wine_gluQuadricOrientation(void *arg0,int arg1) {
	return gluQuadricOrientation(arg0,arg1);
}

/***********************************************************************
 *		gluQuadricNormals (GLU32.@)
 */
extern int gluQuadricNormals(void *arg0,int arg1);
int WINAPI wine_gluQuadricNormals(void *arg0,int arg1) {
	return gluQuadricNormals(arg0,arg1);
}

/***********************************************************************
 *		gluQuadricTexture (GLU32.@)
 */
extern int gluQuadricTexture(void *arg0,int arg1);
int WINAPI wine_gluQuadricTexture(void *arg0,int arg1) {
	return gluQuadricTexture(arg0,arg1);
}

/***********************************************************************
 *		gluQuadricCallback (GLU32.@)
 */
extern int gluQuadricCallback(void *arg0,int arg1,void *arg2);
int WINAPI wine_gluQuadricCallback(void *arg0,int arg1,void *arg2) {
	return gluQuadricCallback(arg0,arg1,arg2);
}

/***********************************************************************
 *		gluCylinder (GLU32.@)
 */
extern int gluCylinder(void *arg0,double arg1,double arg2,double arg3,int arg4,int arg5);
int WINAPI wine_gluCylinder(void *arg0,double arg1,double arg2,double arg3,int arg4,int arg5) {
	return gluCylinder(arg0,arg1,arg2,arg3,arg4,arg5);
}

/***********************************************************************
 *		gluSphere (GLU32.@)
 */
extern int gluSphere(void *arg0,double arg1,int arg2,int arg3);
int WINAPI wine_gluSphere(void *arg0,double arg1,int arg2,int arg3) {
	return gluSphere(arg0,arg1,arg2,arg3);
}

/***********************************************************************
 *		gluDisk (GLU32.@)
 */
extern int gluDisk(void *arg0,double arg1,double arg2,int arg3,int arg4);
int WINAPI wine_gluDisk(void *arg0,double arg1,double arg2,int arg3,int arg4) {
	return gluDisk(arg0,arg1,arg2,arg3,arg4);
}

/***********************************************************************
 *		gluPartialDisk (GLU32.@)
 */
extern int gluPartialDisk(void *arg0,double arg1,double arg2,int arg3,int arg4,double arg5,double arg6);
int WINAPI wine_gluPartialDisk(void *arg0,double arg1,double arg2,int arg3,int arg4,double arg5,double arg6) {
	return gluPartialDisk(arg0,arg1,arg2,arg3,arg4,arg5,arg6);
}

/***********************************************************************
 *		gluNewNurbsRenderer (GLU32.@)
 */
extern int gluNewNurbsRenderer(void);
int WINAPI wine_gluNewNurbsRenderer(void) {
	return gluNewNurbsRenderer();
}

/***********************************************************************
 *		gluDeleteNurbsRenderer (GLU32.@)
 */
extern int gluDeleteNurbsRenderer(void *arg0);
int WINAPI wine_gluDeleteNurbsRenderer(void *arg0) {
	return gluDeleteNurbsRenderer(arg0);
}

/***********************************************************************
 *		gluLoadSamplingMatrices (GLU32.@)
 */
extern int gluLoadSamplingMatrices(void *arg0,void *arg1,void *arg2,void *arg3);
int WINAPI wine_gluLoadSamplingMatrices(void *arg0,void *arg1,void *arg2,void *arg3) {
	return gluLoadSamplingMatrices(arg0,arg1,arg2,arg3);
}

/***********************************************************************
 *		gluNurbsProperty (GLU32.@)
 */
extern int gluNurbsProperty(void *arg0,int arg1,int arg2);
int WINAPI wine_gluNurbsProperty(void *arg0,int arg1,int arg2) {
	return gluNurbsProperty(arg0,arg1,arg2);
}

/***********************************************************************
 *		gluGetNurbsProperty (GLU32.@)
 */
extern int gluGetNurbsProperty(void *arg0,int arg1,void *arg2);
int WINAPI wine_gluGetNurbsProperty(void *arg0,int arg1,void *arg2) {
	return gluGetNurbsProperty(arg0,arg1,arg2);
}

/***********************************************************************
 *		gluBeginCurve (GLU32.@)
 */
extern int gluBeginCurve(void *arg0);
int WINAPI wine_gluBeginCurve(void *arg0) {
	return gluBeginCurve(arg0);
}

/***********************************************************************
 *		gluEndCurve (GLU32.@)
 */
extern int gluEndCurve(void *arg0);
int WINAPI wine_gluEndCurve(void *arg0) {
	return gluEndCurve(arg0);
}

/***********************************************************************
 *		gluNurbsCurve (GLU32.@)
 */
extern int gluNurbsCurve(void *arg0,int arg1,void *arg2,int arg3,void *arg4,int arg5,int arg6);
int WINAPI wine_gluNurbsCurve(void *arg0,int arg1,void *arg2,int arg3,void *arg4,int arg5,int arg6) {
	return gluNurbsCurve(arg0,arg1,arg2,arg3,arg4,arg5,arg6);
}

/***********************************************************************
 *		gluBeginSurface (GLU32.@)
 */
extern int gluBeginSurface(void *arg0);
int WINAPI wine_gluBeginSurface(void *arg0) {
	return gluBeginSurface(arg0);
}

/***********************************************************************
 *		gluEndSurface (GLU32.@)
 */
extern int gluEndSurface(void *arg0);
int WINAPI wine_gluEndSurface(void *arg0) {
	return gluEndSurface(arg0);
}

/***********************************************************************
 *		gluNurbsSurface (GLU32.@)
 */
extern int gluNurbsSurface(void *arg0,int arg1,void *arg2,int arg3,void *arg4,int arg5,int arg6,void *arg7,int arg8,int arg9,int arg10);
int WINAPI wine_gluNurbsSurface(void *arg0,int arg1,void *arg2,int arg3,void *arg4,int arg5,int arg6,void *arg7,int arg8,int arg9,int arg10) {
	return gluNurbsSurface(arg0,arg1,arg2,arg3,arg4,arg5,arg6,arg7,arg8,arg9,arg10);
}

/***********************************************************************
 *		gluBeginTrim (GLU32.@)
 */
extern int gluBeginTrim(void *arg0);
int WINAPI wine_gluBeginTrim(void *arg0) {
	return gluBeginTrim(arg0);
}

/***********************************************************************
 *		gluEndTrim (GLU32.@)
 */
extern int gluEndTrim(void *arg0);
int WINAPI wine_gluEndTrim(void *arg0) {
	return gluEndTrim(arg0);
}

/***********************************************************************
 *		gluPwlCurve (GLU32.@)
 */
extern int gluPwlCurve(void *arg0,int arg1,void *arg2,int arg3,int arg4);
int WINAPI wine_gluPwlCurve(void *arg0,int arg1,void *arg2,int arg3,int arg4) {
	return gluPwlCurve(arg0,arg1,arg2,arg3,arg4);
}

/***********************************************************************
 *		gluNurbsCallback (GLU32.@)
 */
extern int gluNurbsCallback(void *arg0,int arg1,void *arg2);
int WINAPI wine_gluNurbsCallback(void *arg0,int arg1,void *arg2) {
	return gluNurbsCallback(arg0,arg1,arg2);
}

/***********************************************************************
 *		gluGetString (GLU32.@)
 */
extern int gluGetString(int arg0);
int WINAPI wine_gluGetString(int arg0) {
	return gluGetString(arg0);
}

/***********************************************************************
 *		gluCheckExtension (GLU32.@)
 */
int WINAPI
wine_gluCheckExtension( const char *extName, void *extString ) {
    return 0;
}

extern void *gluNewTess(void);
extern void gluDeleteTess(void *);

/***********************************************************************
 *		gluNewTess (GLU32.@)
 */
void * WINAPI wine_gluNewTess(void)
{
    void *tess;
    wine_tess_t *ret;

    if((tess = gluNewTess()) == NULL)
       return NULL;

    ret = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*ret));
    if(!ret) {
        gluDeleteTess(tess);
        return NULL;
    }
    ret->tess = tess;
    return ret;
}

/***********************************************************************
 *		gluDeleteTess (GLU32.@)
 */
void WINAPI wine_gluDeleteTess(void *tess)
{
    wine_tess_t *wine_tess = tess;
    gluDeleteTess(wine_tess->tess);
    HeapFree(GetProcessHeap(), 0, wine_tess);
    return;
}

/***********************************************************************
 *		gluTessBeginPolygon (GLU32.@)
 */
extern void gluTessBeginPolygon(void *, void *);
void WINAPI wine_gluTessBeginPolygon(void *tess, void *polygon_data)
{
    wine_tess_t *wine_tess = tess;
    wine_tess->polygon_data = polygon_data;

    gluTessBeginPolygon(wine_tess->tess, wine_tess);
}

/***********************************************************************
 *		gluTessEndPolygon (GLU32.@)
 */
extern void gluTessEndPolygon(void *);
void WINAPI wine_gluTessEndPolygon(void *tess)
{
    wine_tess_t *wine_tess = tess;
    gluTessEndPolygon(wine_tess->tess);
}


static void wine_glu_tess_begin_data(int type, wine_tess_t *wine_tess)
{
    if(wine_tess->cb_tess_begin_data)
        wine_tess->cb_tess_begin_data(type, wine_tess->polygon_data);
    else
        wine_tess->cb_tess_begin(type);
}

static void wine_glu_tess_vertex_data(void *vertex_data, wine_tess_t *wine_tess)
{
    if(wine_tess->cb_tess_vertex_data)
        wine_tess->cb_tess_vertex_data(vertex_data, wine_tess->polygon_data);
    else
        wine_tess->cb_tess_vertex(vertex_data);
}

static void wine_glu_tess_end_data(wine_tess_t *wine_tess)
{
    if(wine_tess->cb_tess_end_data)
        wine_tess->cb_tess_end_data(wine_tess->polygon_data);
    else
        wine_tess->cb_tess_end();
}

static void wine_glu_tess_error_data(int error, wine_tess_t *wine_tess)
{
    if(wine_tess->cb_tess_error_data)
        wine_tess->cb_tess_error_data(error, wine_tess->polygon_data);
    else
        wine_tess->cb_tess_error(error);
}

static void wine_glu_tess_edge_flag_data(int flag, wine_tess_t *wine_tess)
{
    if(wine_tess->cb_tess_edge_flag_data)
        wine_tess->cb_tess_edge_flag_data(flag, wine_tess->polygon_data);
    else
        wine_tess->cb_tess_edge_flag(flag);
}

static void wine_glu_tess_combine_data(double *coords, void *vertex_data, float *weight, void **outData,
                                       wine_tess_t *wine_tess)
{
    if(wine_tess->cb_tess_combine_data)
        wine_tess->cb_tess_combine_data(coords, vertex_data, weight, outData, wine_tess->polygon_data);
    else
        wine_tess->cb_tess_combine(coords, vertex_data, weight, outData);
}


/***********************************************************************
 *		gluTessCallback (GLU32.@)
 */
extern void gluTessCallback(void *,int,void *);
void WINAPI wine_gluTessCallback(void *tess,int which,void *fn)
{
    wine_tess_t *wine_tess = tess;
    switch(which) {
    case GLU_TESS_BEGIN:
        wine_tess->cb_tess_begin = fn;
        fn = wine_glu_tess_begin_data;
        which += 6;
        break;
    case GLU_TESS_VERTEX:
        wine_tess->cb_tess_vertex = fn;
        fn = wine_glu_tess_vertex_data;
        which += 6;
        break;
    case GLU_TESS_END:
        wine_tess->cb_tess_end = fn;
        fn = wine_glu_tess_end_data;
        which += 6;
        break;
    case GLU_TESS_ERROR:
        wine_tess->cb_tess_error = fn;
        fn = wine_glu_tess_error_data;
        which += 6;
        break;
    case GLU_TESS_EDGE_FLAG:
        wine_tess->cb_tess_edge_flag = fn;
        fn = wine_glu_tess_edge_flag_data;
        which += 6;
        break;
    case GLU_TESS_COMBINE:
        wine_tess->cb_tess_combine = fn;
        fn = wine_glu_tess_combine_data;
        which += 6;
        break;
    case GLU_TESS_BEGIN_DATA:
        wine_tess->cb_tess_begin_data = fn;
        fn = wine_glu_tess_begin_data;
        break;
    case GLU_TESS_VERTEX_DATA:
        wine_tess->cb_tess_vertex_data = fn;
        fn = wine_glu_tess_vertex_data;
        break;
    case GLU_TESS_END_DATA:
        wine_tess->cb_tess_end_data = fn;
        fn = wine_glu_tess_end_data;
        break;
    case GLU_TESS_ERROR_DATA:
        wine_tess->cb_tess_error_data = fn;
        fn = wine_glu_tess_error_data;
        break;
    case GLU_TESS_EDGE_FLAG_DATA:
        wine_tess->cb_tess_edge_flag_data = fn;
        fn = wine_glu_tess_edge_flag_data;
        break;
    case GLU_TESS_COMBINE_DATA:
        wine_tess->cb_tess_combine_data = fn;
        fn = wine_glu_tess_combine_data;
        break;
    default:
        ERR("Unknown callback %d\n", which);
        break;
    }
    gluTessCallback(wine_tess->tess, which, fn);
}

/***********************************************************************
 *		gluTessBeginContour (GLU32.@)
 */
extern void gluTessBeginContour(void *);
void WINAPI wine_gluTessBeginContour(void *tess)
{
    wine_tess_t *wine_tess = tess;
    gluTessBeginContour(wine_tess->tess);
}

/***********************************************************************
 *		gluTessEndContour (GLU32.@)
 */
extern void gluTessEndContour(void *);
void WINAPI wine_gluTessEndContour(void *tess)
{
    wine_tess_t *wine_tess = tess;
    gluTessEndContour(wine_tess->tess);
}

/***********************************************************************
 *		gluTessVertex (GLU32.@)
 */
extern void gluTessVertex(void *, void *, void *);
void WINAPI wine_gluTessVertex(void *tess,void *arg1,void *arg2)
{
    wine_tess_t *wine_tess = tess;
    gluTessVertex(wine_tess->tess, arg1, arg2);
}


/***********************************************************************
 *		gluTessProperty (GLU32.@)
 */
extern void gluTessProperty(void *, int, double);
void WINAPI wine_gluTessProperty(void *tess, int arg1, double arg2)
{
    wine_tess_t *wine_tess = tess;
    gluTessProperty(wine_tess->tess, arg1, arg2);
}

/***********************************************************************
 *		gluTessNormal (GLU32.@)
 */
extern void gluTessNormal(void *, double, double, double);
void WINAPI wine_gluTessNormal(void *tess, double arg1, double arg2, double arg3)
{
    wine_tess_t *wine_tess = tess;
    gluTessNormal(wine_tess->tess, arg1, arg2, arg3);
}

/***********************************************************************
 *      gluBeginPolygon (GLU32.@)
 */
void WINAPI wine_gluBeginPolygon(void *tess)
{
    wine_tess_t *wine_tess = tess;
    wine_tess->polygon_data = NULL;
    gluTessBeginPolygon(wine_tess->tess, wine_tess);
    gluTessBeginContour(wine_tess->tess);
}

/***********************************************************************
 *      gluEndPolygon (GLU32.@)
 */
void WINAPI wine_gluEndPolygon(void *tess)
{
    wine_tess_t *wine_tess = tess;
    gluTessEndContour(wine_tess->tess);
    gluTessEndPolygon(wine_tess->tess);
}

/***********************************************************************
 *      gluNextContour (GLU32.@)
 */
void WINAPI wine_gluNextContour(void *tess, int arg1)
{
    wine_tess_t *wine_tess = tess;
    gluTessEndContour(wine_tess->tess);
    gluTessBeginContour(wine_tess->tess);
}
