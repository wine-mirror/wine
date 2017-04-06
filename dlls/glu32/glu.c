/*
 * Copyright 2001 Marcus Meissner
 * Copyright 2017 Alexandre Julliard
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

#include "config.h"
#include "wine/port.h"

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"

#include "wine/library.h"
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

static int  (*p_gluBeginCurve)(void *arg0);
static int  (*p_gluBeginSurface)(void *arg0);
static int  (*p_gluBeginTrim)(void *arg0);
static int  (*p_gluBuild1DMipmaps)(int arg0,int arg1,int arg2,int arg3,int arg4,void *arg5);
static int  (*p_gluBuild2DMipmaps)(int arg0,int arg1,int arg2,int arg3,int arg4,int arg5,void *arg6);
static int  (*p_gluCylinder)(void *arg0,double arg1,double arg2,double arg3,int arg4,int arg5);
static int  (*p_gluDeleteNurbsRenderer)(void *arg0);
static int  (*p_gluDeleteQuadric)(void *arg0);
static void (*p_gluDeleteTess)(void *);
static int  (*p_gluDisk)(void *arg0,double arg1,double arg2,int arg3,int arg4);
static int  (*p_gluEndCurve)(void *arg0);
static int  (*p_gluEndSurface)(void *arg0);
static int  (*p_gluEndTrim)(void *arg0);
static int  (*p_gluErrorString)(int arg0);
static int  (*p_gluGetNurbsProperty)(void *arg0,int arg1,void *arg2);
static int  (*p_gluGetString)(int arg0);
static int  (*p_gluLoadSamplingMatrices)(void *arg0,void *arg1,void *arg2,void *arg3);
static int  (*p_gluLookAt)(double arg0,double arg1,double arg2,double arg3,double arg4,double arg5,double arg6,double arg7,double arg8);
static int  (*p_gluNewNurbsRenderer)(void);
static int  (*p_gluNewQuadric)(void);
static void*(*p_gluNewTess)(void);
static int  (*p_gluNurbsCallback)(void *arg0,int arg1,void *arg2);
static int  (*p_gluNurbsCurve)(void *arg0,int arg1,void *arg2,int arg3,void *arg4,int arg5,int arg6);
static int  (*p_gluNurbsProperty)(void *arg0,int arg1,int arg2);
static int  (*p_gluNurbsSurface)(void *arg0,int arg1,void *arg2,int arg3,void *arg4,int arg5,int arg6,void *arg7,int arg8,int arg9,int arg10);
static int  (*p_gluOrtho2D)(double arg0,double arg1,double arg2,double arg3);
static int  (*p_gluPartialDisk)(void *arg0,double arg1,double arg2,int arg3,int arg4,double arg5,double arg6);
static int  (*p_gluPerspective)(double arg0,double arg1,double arg2,double arg3);
static int  (*p_gluPickMatrix)(double arg0,double arg1,double arg2,double arg3,void *arg4);
static int  (*p_gluProject)(double arg0,double arg1,double arg2,void *arg3,void *arg4,void *arg5,void *arg6,void *arg7,void *arg8);
static int  (*p_gluPwlCurve)(void *arg0,int arg1,void *arg2,int arg3,int arg4);
static int  (*p_gluQuadricCallback)(void *arg0,int arg1,void *arg2);
static int  (*p_gluQuadricDrawStyle)(void *arg0,int arg1);
static int  (*p_gluQuadricNormals)(void *arg0,int arg1);
static int  (*p_gluQuadricOrientation)(void *arg0,int arg1);
static int  (*p_gluQuadricTexture)(void *arg0,int arg1);
static int  (*p_gluScaleImage)(int arg0,int arg1,int arg2,int arg3,void *arg4,int arg5,int arg6,int arg7,void *arg8);
static int  (*p_gluSphere)(void *arg0,double arg1,int arg2,int arg3);
static void (*p_gluTessBeginContour)(void *);
static void (*p_gluTessBeginPolygon)(void *, void *);
static void (*p_gluTessCallback)(void *,int,void *);
static void (*p_gluTessEndContour)(void *);
static void (*p_gluTessEndPolygon)(void *);
static void (*p_gluTessNormal)(void *, double, double, double);
static void (*p_gluTessProperty)(void *, int, double);
static void (*p_gluTessVertex)(void *, void *, void *);
static int  (*p_gluUnProject)(double arg0,double arg1,double arg2,void *arg3,void *arg4,void *arg5,void *arg6,void *arg7,void *arg8);

static BOOL load_libglu(void)
{
    char error[256];
    void *handle = wine_dlopen( SONAME_LIBGLU, RTLD_NOW, error, sizeof(error) );

    if (!handle)
    {
        ERR( "Failed to load %s: %s\n", SONAME_LIBGLU, error );
        return FALSE;
    }
#define LOAD_FUNCPTR(f) if (!(p_##f = wine_dlsym( handle, #f, NULL, 0 ))) { ERR( "Can't find %s in %s\n", #f, SONAME_LIBGLU ); return FALSE; }
    LOAD_FUNCPTR(gluBeginCurve);
    LOAD_FUNCPTR(gluBeginSurface);
    LOAD_FUNCPTR(gluBeginTrim);
    LOAD_FUNCPTR(gluBuild1DMipmaps);
    LOAD_FUNCPTR(gluBuild2DMipmaps);
    LOAD_FUNCPTR(gluCylinder);
    LOAD_FUNCPTR(gluDeleteNurbsRenderer);
    LOAD_FUNCPTR(gluDeleteQuadric);
    LOAD_FUNCPTR(gluDeleteTess);
    LOAD_FUNCPTR(gluDisk);
    LOAD_FUNCPTR(gluEndCurve);
    LOAD_FUNCPTR(gluEndSurface);
    LOAD_FUNCPTR(gluEndTrim);
    LOAD_FUNCPTR(gluErrorString);
    LOAD_FUNCPTR(gluGetNurbsProperty);
    LOAD_FUNCPTR(gluGetString);
    LOAD_FUNCPTR(gluLoadSamplingMatrices);
    LOAD_FUNCPTR(gluLookAt);
    LOAD_FUNCPTR(gluNewNurbsRenderer);
    LOAD_FUNCPTR(gluNewQuadric);
    LOAD_FUNCPTR(gluNewTess);
    LOAD_FUNCPTR(gluNurbsCallback);
    LOAD_FUNCPTR(gluNurbsCurve);
    LOAD_FUNCPTR(gluNurbsProperty);
    LOAD_FUNCPTR(gluNurbsSurface);
    LOAD_FUNCPTR(gluOrtho2D);
    LOAD_FUNCPTR(gluPartialDisk);
    LOAD_FUNCPTR(gluPerspective);
    LOAD_FUNCPTR(gluPickMatrix);
    LOAD_FUNCPTR(gluProject);
    LOAD_FUNCPTR(gluPwlCurve);
    LOAD_FUNCPTR(gluQuadricCallback);
    LOAD_FUNCPTR(gluQuadricDrawStyle);
    LOAD_FUNCPTR(gluQuadricNormals);
    LOAD_FUNCPTR(gluQuadricOrientation);
    LOAD_FUNCPTR(gluQuadricTexture);
    LOAD_FUNCPTR(gluScaleImage);
    LOAD_FUNCPTR(gluSphere);
    LOAD_FUNCPTR(gluTessBeginContour);
    LOAD_FUNCPTR(gluTessBeginPolygon);
    LOAD_FUNCPTR(gluTessCallback);
    LOAD_FUNCPTR(gluTessEndContour);
    LOAD_FUNCPTR(gluTessEndPolygon);
    LOAD_FUNCPTR(gluTessNormal);
    LOAD_FUNCPTR(gluTessProperty);
    LOAD_FUNCPTR(gluTessVertex);
    LOAD_FUNCPTR(gluUnProject);
#undef LOAD_FUNCPTR
    TRACE( "loaded %s\n", SONAME_LIBGLU );
    return TRUE;
}


/***********************************************************************
 *		DllMain
 */
BOOL WINAPI DllMain( HINSTANCE inst, DWORD reason, LPVOID reserved )
{
    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls( inst );
        return load_libglu();
    }
    return TRUE;
}

/***********************************************************************
 *		gluLookAt (GLU32.@)
 */
int WINAPI wine_gluLookAt(double arg0,double arg1,double arg2,double arg3,double arg4,double arg5,double arg6,double arg7,double arg8) {
	return p_gluLookAt(arg0,arg1,arg2,arg3,arg4,arg5,arg6,arg7,arg8);
}

/***********************************************************************
 *		gluOrtho2D (GLU32.@)
 */
int WINAPI wine_gluOrtho2D(double arg0,double arg1,double arg2,double arg3) {
	return p_gluOrtho2D(arg0,arg1,arg2,arg3);
}

/***********************************************************************
 *		gluPerspective (GLU32.@)
 */
int WINAPI wine_gluPerspective(double arg0,double arg1,double arg2,double arg3) {
	return p_gluPerspective(arg0,arg1,arg2,arg3);
}

/***********************************************************************
 *		gluPickMatrix (GLU32.@)
 */
int WINAPI wine_gluPickMatrix(double arg0,double arg1,double arg2,double arg3,void *arg4) {
	return p_gluPickMatrix(arg0,arg1,arg2,arg3,arg4);
}

/***********************************************************************
 *		gluProject (GLU32.@)
 */
int WINAPI wine_gluProject(double arg0,double arg1,double arg2,void *arg3,void *arg4,void *arg5,void *arg6,void *arg7,void *arg8) {
	return p_gluProject(arg0,arg1,arg2,arg3,arg4,arg5,arg6,arg7,arg8);
}

/***********************************************************************
 *		gluUnProject (GLU32.@)
 */
int WINAPI wine_gluUnProject(double arg0,double arg1,double arg2,void *arg3,void *arg4,void *arg5,void *arg6,void *arg7,void *arg8) {
	return p_gluUnProject(arg0,arg1,arg2,arg3,arg4,arg5,arg6,arg7,arg8);
}

/***********************************************************************
 *		gluErrorString (GLU32.@)
 */
int WINAPI wine_gluErrorString(int arg0) {
	return p_gluErrorString(arg0);
}

/***********************************************************************
 *		gluScaleImage (GLU32.@)
 */
int WINAPI wine_gluScaleImage(int arg0,int arg1,int arg2,int arg3,void *arg4,int arg5,int arg6,int arg7,void *arg8) {
	return p_gluScaleImage(arg0,arg1,arg2,arg3,arg4,arg5,arg6,arg7,arg8);
}

/***********************************************************************
 *		gluBuild1DMipmaps (GLU32.@)
 */
int WINAPI wine_gluBuild1DMipmaps(int arg0,int arg1,int arg2,int arg3,int arg4,void *arg5) {
	return p_gluBuild1DMipmaps(arg0,arg1,arg2,arg3,arg4,arg5);
}

/***********************************************************************
 *		gluBuild2DMipmaps (GLU32.@)
 */
int WINAPI wine_gluBuild2DMipmaps(int arg0,int arg1,int arg2,int arg3,int arg4,int arg5,void *arg6) {
	return p_gluBuild2DMipmaps(arg0,arg1,arg2,arg3,arg4,arg5,arg6);
}

/***********************************************************************
 *		gluNewQuadric (GLU32.@)
 */
int WINAPI wine_gluNewQuadric(void) {
	return p_gluNewQuadric();
}

/***********************************************************************
 *		gluDeleteQuadric (GLU32.@)
 */
int WINAPI wine_gluDeleteQuadric(void *arg0) {
	return p_gluDeleteQuadric(arg0);
}

/***********************************************************************
 *		gluQuadricDrawStyle (GLU32.@)
 */
int WINAPI wine_gluQuadricDrawStyle(void *arg0,int arg1) {
	return p_gluQuadricDrawStyle(arg0,arg1);
}

/***********************************************************************
 *		gluQuadricOrientation (GLU32.@)
 */
int WINAPI wine_gluQuadricOrientation(void *arg0,int arg1) {
	return p_gluQuadricOrientation(arg0,arg1);
}

/***********************************************************************
 *		gluQuadricNormals (GLU32.@)
 */
int WINAPI wine_gluQuadricNormals(void *arg0,int arg1) {
	return p_gluQuadricNormals(arg0,arg1);
}

/***********************************************************************
 *		gluQuadricTexture (GLU32.@)
 */
int WINAPI wine_gluQuadricTexture(void *arg0,int arg1) {
	return p_gluQuadricTexture(arg0,arg1);
}

/***********************************************************************
 *		gluQuadricCallback (GLU32.@)
 */
int WINAPI wine_gluQuadricCallback(void *arg0,int arg1,void *arg2) {
	return p_gluQuadricCallback(arg0,arg1,arg2);
}

/***********************************************************************
 *		gluCylinder (GLU32.@)
 */
int WINAPI wine_gluCylinder(void *arg0,double arg1,double arg2,double arg3,int arg4,int arg5) {
	return p_gluCylinder(arg0,arg1,arg2,arg3,arg4,arg5);
}

/***********************************************************************
 *		gluSphere (GLU32.@)
 */
int WINAPI wine_gluSphere(void *arg0,double arg1,int arg2,int arg3) {
	return p_gluSphere(arg0,arg1,arg2,arg3);
}

/***********************************************************************
 *		gluDisk (GLU32.@)
 */
int WINAPI wine_gluDisk(void *arg0,double arg1,double arg2,int arg3,int arg4) {
	return p_gluDisk(arg0,arg1,arg2,arg3,arg4);
}

/***********************************************************************
 *		gluPartialDisk (GLU32.@)
 */
int WINAPI wine_gluPartialDisk(void *arg0,double arg1,double arg2,int arg3,int arg4,double arg5,double arg6) {
	return p_gluPartialDisk(arg0,arg1,arg2,arg3,arg4,arg5,arg6);
}

/***********************************************************************
 *		gluNewNurbsRenderer (GLU32.@)
 */
int WINAPI wine_gluNewNurbsRenderer(void) {
	return p_gluNewNurbsRenderer();
}

/***********************************************************************
 *		gluDeleteNurbsRenderer (GLU32.@)
 */
int WINAPI wine_gluDeleteNurbsRenderer(void *arg0) {
	return p_gluDeleteNurbsRenderer(arg0);
}

/***********************************************************************
 *		gluLoadSamplingMatrices (GLU32.@)
 */
int WINAPI wine_gluLoadSamplingMatrices(void *arg0,void *arg1,void *arg2,void *arg3) {
	return p_gluLoadSamplingMatrices(arg0,arg1,arg2,arg3);
}

/***********************************************************************
 *		gluNurbsProperty (GLU32.@)
 */
int WINAPI wine_gluNurbsProperty(void *arg0,int arg1,int arg2) {
	return p_gluNurbsProperty(arg0,arg1,arg2);
}

/***********************************************************************
 *		gluGetNurbsProperty (GLU32.@)
 */
int WINAPI wine_gluGetNurbsProperty(void *arg0,int arg1,void *arg2) {
	return p_gluGetNurbsProperty(arg0,arg1,arg2);
}

/***********************************************************************
 *		gluBeginCurve (GLU32.@)
 */
int WINAPI wine_gluBeginCurve(void *arg0) {
	return p_gluBeginCurve(arg0);
}

/***********************************************************************
 *		gluEndCurve (GLU32.@)
 */
int WINAPI wine_gluEndCurve(void *arg0) {
	return p_gluEndCurve(arg0);
}

/***********************************************************************
 *		gluNurbsCurve (GLU32.@)
 */
int WINAPI wine_gluNurbsCurve(void *arg0,int arg1,void *arg2,int arg3,void *arg4,int arg5,int arg6) {
	return p_gluNurbsCurve(arg0,arg1,arg2,arg3,arg4,arg5,arg6);
}

/***********************************************************************
 *		gluBeginSurface (GLU32.@)
 */
int WINAPI wine_gluBeginSurface(void *arg0) {
	return p_gluBeginSurface(arg0);
}

/***********************************************************************
 *		gluEndSurface (GLU32.@)
 */
int WINAPI wine_gluEndSurface(void *arg0) {
	return p_gluEndSurface(arg0);
}

/***********************************************************************
 *		gluNurbsSurface (GLU32.@)
 */
int WINAPI wine_gluNurbsSurface(void *arg0,int arg1,void *arg2,int arg3,void *arg4,int arg5,int arg6,void *arg7,int arg8,int arg9,int arg10) {
	return p_gluNurbsSurface(arg0,arg1,arg2,arg3,arg4,arg5,arg6,arg7,arg8,arg9,arg10);
}

/***********************************************************************
 *		gluBeginTrim (GLU32.@)
 */
int WINAPI wine_gluBeginTrim(void *arg0) {
	return p_gluBeginTrim(arg0);
}

/***********************************************************************
 *		gluEndTrim (GLU32.@)
 */
int WINAPI wine_gluEndTrim(void *arg0) {
	return p_gluEndTrim(arg0);
}

/***********************************************************************
 *		gluPwlCurve (GLU32.@)
 */
int WINAPI wine_gluPwlCurve(void *arg0,int arg1,void *arg2,int arg3,int arg4) {
	return p_gluPwlCurve(arg0,arg1,arg2,arg3,arg4);
}

/***********************************************************************
 *		gluNurbsCallback (GLU32.@)
 */
int WINAPI wine_gluNurbsCallback(void *arg0,int arg1,void *arg2) {
	return p_gluNurbsCallback(arg0,arg1,arg2);
}

/***********************************************************************
 *		gluGetString (GLU32.@)
 */
int WINAPI wine_gluGetString(int arg0) {
	return p_gluGetString(arg0);
}

/***********************************************************************
 *		gluCheckExtension (GLU32.@)
 */
int WINAPI
wine_gluCheckExtension( const char *extName, void *extString ) {
    return 0;
}

/***********************************************************************
 *		gluNewTess (GLU32.@)
 */
void * WINAPI wine_gluNewTess(void)
{
    void *tess;
    wine_tess_t *ret;

    if((tess = p_gluNewTess()) == NULL)
       return NULL;

    ret = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*ret));
    if(!ret) {
        p_gluDeleteTess(tess);
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
    p_gluDeleteTess(wine_tess->tess);
    HeapFree(GetProcessHeap(), 0, wine_tess);
    return;
}

/***********************************************************************
 *		gluTessBeginPolygon (GLU32.@)
 */
void WINAPI wine_gluTessBeginPolygon(void *tess, void *polygon_data)
{
    wine_tess_t *wine_tess = tess;
    wine_tess->polygon_data = polygon_data;

    p_gluTessBeginPolygon(wine_tess->tess, wine_tess);
}

/***********************************************************************
 *		gluTessEndPolygon (GLU32.@)
 */
void WINAPI wine_gluTessEndPolygon(void *tess)
{
    wine_tess_t *wine_tess = tess;
    p_gluTessEndPolygon(wine_tess->tess);
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
    p_gluTessCallback(wine_tess->tess, which, fn);
}

/***********************************************************************
 *		gluTessBeginContour (GLU32.@)
 */
void WINAPI wine_gluTessBeginContour(void *tess)
{
    wine_tess_t *wine_tess = tess;
    p_gluTessBeginContour(wine_tess->tess);
}

/***********************************************************************
 *		gluTessEndContour (GLU32.@)
 */
void WINAPI wine_gluTessEndContour(void *tess)
{
    wine_tess_t *wine_tess = tess;
    p_gluTessEndContour(wine_tess->tess);
}

/***********************************************************************
 *		gluTessVertex (GLU32.@)
 */
void WINAPI wine_gluTessVertex(void *tess,void *arg1,void *arg2)
{
    wine_tess_t *wine_tess = tess;
    p_gluTessVertex(wine_tess->tess, arg1, arg2);
}


/***********************************************************************
 *		gluTessProperty (GLU32.@)
 */
void WINAPI wine_gluTessProperty(void *tess, int arg1, double arg2)
{
    wine_tess_t *wine_tess = tess;
    p_gluTessProperty(wine_tess->tess, arg1, arg2);
}

/***********************************************************************
 *		gluTessNormal (GLU32.@)
 */
void WINAPI wine_gluTessNormal(void *tess, double arg1, double arg2, double arg3)
{
    wine_tess_t *wine_tess = tess;
    p_gluTessNormal(wine_tess->tess, arg1, arg2, arg3);
}

/***********************************************************************
 *      gluBeginPolygon (GLU32.@)
 */
void WINAPI wine_gluBeginPolygon(void *tess)
{
    wine_tess_t *wine_tess = tess;
    wine_tess->polygon_data = NULL;
    p_gluTessBeginPolygon(wine_tess->tess, wine_tess);
    p_gluTessBeginContour(wine_tess->tess);
}

/***********************************************************************
 *      gluEndPolygon (GLU32.@)
 */
void WINAPI wine_gluEndPolygon(void *tess)
{
    wine_tess_t *wine_tess = tess;
    p_gluTessEndContour(wine_tess->tess);
    p_gluTessEndPolygon(wine_tess->tess);
}

/***********************************************************************
 *      gluNextContour (GLU32.@)
 */
void WINAPI wine_gluNextContour(void *tess, int arg1)
{
    wine_tess_t *wine_tess = tess;
    p_gluTessEndContour(wine_tess->tess);
    p_gluTessBeginContour(wine_tess->tess);
}
