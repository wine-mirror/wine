#include "winbase.h"

extern int gluLookAt(double arg0,double arg1,double arg2,double arg3,double arg4,double arg5,double arg6,double arg7,double arg8);
int WINAPI wine_gluLookAt(double arg0,double arg1,double arg2,double arg3,double arg4,double arg5,double arg6,double arg7,double arg8) {
	return gluLookAt(arg0,arg1,arg2,arg3,arg4,arg5,arg6,arg7,arg8);
}

extern int gluOrtho2D(double arg0,double arg1,double arg2,double arg3);
int WINAPI wine_gluOrtho2D(double arg0,double arg1,double arg2,double arg3) {
	return gluOrtho2D(arg0,arg1,arg2,arg3);
}

extern int gluPerspective(double arg0,double arg1,double arg2,double arg3);
int WINAPI wine_gluPerspective(double arg0,double arg1,double arg2,double arg3) {
	return gluPerspective(arg0,arg1,arg2,arg3);
}

extern int gluPickMatrix(double arg0,double arg1,double arg2,double arg3,void *arg4);
int WINAPI wine_gluPickMatrix(double arg0,double arg1,double arg2,double arg3,void *arg4) {
	return gluPickMatrix(arg0,arg1,arg2,arg3,arg4);
}

extern int gluProject(double arg0,double arg1,double arg2,void *arg3,void *arg4,void *arg5,void *arg6,void *arg7,void *arg8);
int WINAPI wine_gluProject(double arg0,double arg1,double arg2,void *arg3,void *arg4,void *arg5,void *arg6,void *arg7,void *arg8) {
	return gluProject(arg0,arg1,arg2,arg3,arg4,arg5,arg6,arg7,arg8);
}

extern int gluUnProject(double arg0,double arg1,double arg2,void *arg3,void *arg4,void *arg5,void *arg6,void *arg7,void *arg8);
int WINAPI wine_gluUnProject(double arg0,double arg1,double arg2,void *arg3,void *arg4,void *arg5,void *arg6,void *arg7,void *arg8) {
	return gluUnProject(arg0,arg1,arg2,arg3,arg4,arg5,arg6,arg7,arg8);
}

extern int gluErrorString(int arg0);
int WINAPI wine_gluErrorString(int arg0) {
	return gluErrorString(arg0);
}

extern int gluScaleImage(int arg0,int arg1,int arg2,int arg3,void *arg4,int arg5,int arg6,int arg7,void *arg8);
int WINAPI wine_gluScaleImage(int arg0,int arg1,int arg2,int arg3,void *arg4,int arg5,int arg6,int arg7,void *arg8) {
	return gluScaleImage(arg0,arg1,arg2,arg3,arg4,arg5,arg6,arg7,arg8);
}

extern int gluBuild1DMipmaps(int arg0,int arg1,int arg2,int arg3,int arg4,void *arg5);
int WINAPI wine_gluBuild1DMipmaps(int arg0,int arg1,int arg2,int arg3,int arg4,void *arg5) {
	return gluBuild1DMipmaps(arg0,arg1,arg2,arg3,arg4,arg5);
}

extern int gluBuild2DMipmaps(int arg0,int arg1,int arg2,int arg3,int arg4,int arg5,void *arg6);
int WINAPI wine_gluBuild2DMipmaps(int arg0,int arg1,int arg2,int arg3,int arg4,int arg5,void *arg6) {
	return gluBuild2DMipmaps(arg0,arg1,arg2,arg3,arg4,arg5,arg6);
}

extern int gluNewQuadric();
int WINAPI wine_gluNewQuadric() {
	return gluNewQuadric();
}

extern int gluDeleteQuadric(void *arg0);
int WINAPI wine_gluDeleteQuadric(void *arg0) {
	return gluDeleteQuadric(arg0);
}

extern int gluQuadricDrawStyle(void *arg0,int arg1);
int WINAPI wine_gluQuadricDrawStyle(void *arg0,int arg1) {
	return gluQuadricDrawStyle(arg0,arg1);
}

extern int gluQuadricOrientation(void *arg0,int arg1);
int WINAPI wine_gluQuadricOrientation(void *arg0,int arg1) {
	return gluQuadricOrientation(arg0,arg1);
}

extern int gluQuadricNormals(void *arg0,int arg1);
int WINAPI wine_gluQuadricNormals(void *arg0,int arg1) {
	return gluQuadricNormals(arg0,arg1);
}

extern int gluQuadricTexture(void *arg0,int arg1);
int WINAPI wine_gluQuadricTexture(void *arg0,int arg1) {
	return gluQuadricTexture(arg0,arg1);
}

extern int gluQuadricCallback(void *arg0,int arg1,void *arg2);
int WINAPI wine_gluQuadricCallback(void *arg0,int arg1,void *arg2) {
	return gluQuadricCallback(arg0,arg1,arg2);
}

extern int gluCylinder(void *arg0,double arg1,double arg2,double arg3,int arg4,int arg5);
int WINAPI wine_gluCylinder(void *arg0,double arg1,double arg2,double arg3,int arg4,int arg5) {
	return gluCylinder(arg0,arg1,arg2,arg3,arg4,arg5);
}

extern int gluSphere(void *arg0,double arg1,int arg2,int arg3);
int WINAPI wine_gluSphere(void *arg0,double arg1,int arg2,int arg3) {
	return gluSphere(arg0,arg1,arg2,arg3);
}

extern int gluDisk(void *arg0,double arg1,double arg2,int arg3,int arg4);
int WINAPI wine_gluDisk(void *arg0,double arg1,double arg2,int arg3,int arg4) {
	return gluDisk(arg0,arg1,arg2,arg3,arg4);
}

extern int gluPartialDisk(void *arg0,double arg1,double arg2,int arg3,int arg4,double arg5,double arg6);
int WINAPI wine_gluPartialDisk(void *arg0,double arg1,double arg2,int arg3,int arg4,double arg5,double arg6) {
	return gluPartialDisk(arg0,arg1,arg2,arg3,arg4,arg5,arg6);
}

extern int gluNewNurbsRenderer();
int WINAPI wine_gluNewNurbsRenderer() {
	return gluNewNurbsRenderer();
}

extern int gluDeleteNurbsRenderer(void *arg0);
int WINAPI wine_gluDeleteNurbsRenderer(void *arg0) {
	return gluDeleteNurbsRenderer(arg0);
}

extern int gluLoadSamplingMatrices(void *arg0,void *arg1,void *arg2,void *arg3);
int WINAPI wine_gluLoadSamplingMatrices(void *arg0,void *arg1,void *arg2,void *arg3) {
	return gluLoadSamplingMatrices(arg0,arg1,arg2,arg3);
}

extern int gluNurbsProperty(void *arg0,int arg1,int arg2);
int WINAPI wine_gluNurbsProperty(void *arg0,int arg1,int arg2) {
	return gluNurbsProperty(arg0,arg1,arg2);
}

extern int gluGetNurbsProperty(void *arg0,int arg1,void *arg2);
int WINAPI wine_gluGetNurbsProperty(void *arg0,int arg1,void *arg2) {
	return gluGetNurbsProperty(arg0,arg1,arg2);
}

extern int gluBeginCurve(void *arg0);
int WINAPI wine_gluBeginCurve(void *arg0) {
	return gluBeginCurve(arg0);
}

extern int gluEndCurve(void *arg0);
int WINAPI wine_gluEndCurve(void *arg0) {
	return gluEndCurve(arg0);
}

extern int gluNurbsCurve(void *arg0,int arg1,void *arg2,int arg3,void *arg4,int arg5,int arg6);
int WINAPI wine_gluNurbsCurve(void *arg0,int arg1,void *arg2,int arg3,void *arg4,int arg5,int arg6) {
	return gluNurbsCurve(arg0,arg1,arg2,arg3,arg4,arg5,arg6);
}

extern int gluBeginSurface(void *arg0);
int WINAPI wine_gluBeginSurface(void *arg0) {
	return gluBeginSurface(arg0);
}

extern int gluEndSurface(void *arg0);
int WINAPI wine_gluEndSurface(void *arg0) {
	return gluEndSurface(arg0);
}

extern int gluNurbsSurface(void *arg0,int arg1,void *arg2,int arg3,void *arg4,int arg5,int arg6,void *arg7,int arg8,int arg9,int arg10);
int WINAPI wine_gluNurbsSurface(void *arg0,int arg1,void *arg2,int arg3,void *arg4,int arg5,int arg6,void *arg7,int arg8,int arg9,int arg10) {
	return gluNurbsSurface(arg0,arg1,arg2,arg3,arg4,arg5,arg6,arg7,arg8,arg9,arg10);
}

extern int gluBeginTrim(void *arg0);
int WINAPI wine_gluBeginTrim(void *arg0) {
	return gluBeginTrim(arg0);
}

extern int gluEndTrim(void *arg0);
int WINAPI wine_gluEndTrim(void *arg0) {
	return gluEndTrim(arg0);
}

extern int gluPwlCurve(void *arg0,int arg1,void *arg2,int arg3,int arg4);
int WINAPI wine_gluPwlCurve(void *arg0,int arg1,void *arg2,int arg3,int arg4) {
	return gluPwlCurve(arg0,arg1,arg2,arg3,arg4);
}

extern int gluNurbsCallback(void *arg0,int arg1,void *arg2);
int WINAPI wine_gluNurbsCallback(void *arg0,int arg1,void *arg2) {
	return gluNurbsCallback(arg0,arg1,arg2);
}

extern int gluNewTess();
int WINAPI wine_gluNewTess() {
	return gluNewTess();
}

extern int gluDeleteTess(void *arg0);
int WINAPI wine_gluDeleteTess(void *arg0) {
	return gluDeleteTess(arg0);
}

extern int gluTessVertex(void *arg0,void *arg1,void *arg2);
int WINAPI wine_gluTessVertex(void *arg0,void *arg1,void *arg2) {
	return gluTessVertex(arg0,arg1,arg2);
}

extern int gluTessCallback(void *arg0,int arg1,void *arg2);
int WINAPI wine_gluTessCallback(void *arg0,int arg1,void *arg2) {
	return gluTessCallback(arg0,arg1,arg2);
}

extern int gluBeginPolygon(void *arg0);
int WINAPI wine_gluBeginPolygon(void *arg0) {
	return gluBeginPolygon(arg0);
}

extern int gluEndPolygon(void *arg0);
int WINAPI wine_gluEndPolygon(void *arg0) {
	return gluEndPolygon(arg0);
}

extern int gluNextContour(void *arg0,int arg1);
int WINAPI wine_gluNextContour(void *arg0,int arg1) {
	return gluNextContour(arg0,arg1);
}

extern int gluGetString(int arg0);
int WINAPI wine_gluGetString(int arg0) {
	return gluGetString(arg0);
}

int WINAPI
wine_gluCheckExtension( const char *extName, void *extString ) {
    return 0;
}
