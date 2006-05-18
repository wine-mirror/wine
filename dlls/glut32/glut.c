/*
 * Copyright 2003 Jacek Caban
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

#include "windef.h"

static void (*__glutExitFunc)(int ret) = 0;

/***************************************************
 *	DllMain [glut32.init]
 */
BOOL WINAPI DllMain(HINSTANCE hInstDll, DWORD fdwReason, LPVOID lpvReserved)
{
   if(fdwReason == DLL_PROCESS_DETACH)	{
      if(__glutExitFunc)
         __glutExitFunc(0);
   }
   return TRUE;
}

/****************************************************
 *	glutGetColor (glut32.@)
 */
extern float glutGetColor(int arg1, int arg2);
float WINAPI wine_glutGetColor(int arg1, int arg2)
{
   return glutGetColor(arg1, arg2);
}

/****************************************************
 *	glutBitmapLength (glut32.@)
 */
extern int glutBitmapLength(void *arg1, void *arg2);
int WINAPI wine_glutBitmapLength(void *arg1, void *arg2)
{
   return glutBitmapLength(arg1, arg2);
}

/****************************************************
 *	glutBitmapWith (glut32.@)
 */
extern int glutBitmapWidth(void *arg1, int arg2);
int WINAPI wine_glutBitmapWidth(void *arg1, int arg2)
{
   return glutBitmapWidth(arg1, arg2);
}

/****************************************************
 *	glutCreateMenu (glut32.@)
 */
extern int glutCreateMenu(void *arg);
int WINAPI wine_glutCreateMenu(void *arg)
{
   return glutCreateMenu(arg);
}

/****************************************************
 *	__glutCreateMenuWithExit (glut32.@)
 */
int WINAPI wine___glutCreateMenuWithExit(void *arg1, void (*exitfunc)(int))
{
   __glutExitFunc = exitfunc;
   return glutCreateMenu(arg1);
}

/*****************************************************
 *	glutCreateSubWindow (glut32.@)
 */
extern int glutCreateSubWindow(int arg1, int arg2, int arg3, int arg4, int arg5);
int WINAPI wine_glutCreateSubWindow(int arg1, int arg2, int arg3, int arg4, int arg5)
{
   return glutCreateSubWindow(arg1, arg2, arg3, arg4, arg5);
}

/*****************************************************
 *	glutCreateWindow (glut32.@)
 */
extern int glutCreateWindow(void *arg);
int WINAPI wine_glutCreateWindow(void *arg)
{
   return glutCreateWindow(arg);
}

/****************************************************
 *	__glutCreateWindowWithExit (glut32.@)
 */
int WINAPI wine___glutCreateWindowWithExit(void *arg, void (*exitfunc)(int))
{
   __glutExitFunc = exitfunc;
   return glutCreateWindow(arg);
}

/*****************************************************
 *	glutDeviceGet (glut32.@)
 */
extern int glutDeviceGet(int arg);
int WINAPI wine_glutDeviceGet(int arg)
{
   return glutDeviceGet(arg);
}

/****************************************************
 *	glutEnterGameMode (glut32.@)
 */
extern int glutEnterGameMode(void);
int WINAPI wine_glutEnterGameMode(void)
{
   return glutEnterGameMode();
}

/****************************************************
 *	glutExtensionSupported (glut32.@)
 */
extern int glutExtensionSupported(void *arg);
int WINAPI wine_glutExtensionSupported(void *arg)
{
   return glutExtensionSupported(arg);
}

/****************************************************
 *	glutGameModeGet (glut32.@)
 */
extern int glutGameModeGet(int arg);
int WINAPI wine_glutGameModeGet(int arg)
{
   return glutGameModeGet(arg);
}

/****************************************************
 *	glutGetMenu (glut32.@)
 */
extern int glutGetMenu(void);
int WINAPI wine_glutGetMenu(void)
{
   return wine_glutGetMenu();
}

/****************************************************
 *	glutGetModifiers (glut32.@)
 */
extern int glutGetModifiers(void);
int WINAPI wine_glutGetModifiers(void)
{
   return glutGetModifiers();
}

/****************************************************
 *	glutGet (glut32.@)
 */
extern int glutGet(int arg);
int WINAPI wine_glutGet(int arg)
{
   return glutGet(arg);
}

/****************************************************
 *	glutGetWindow (glut32.@)
 */
extern int glutGetWindow(void);
int WINAPI wine_glutGetWindow(void)
{
   return glutGetWindow();
}

/****************************************************
 *	glutLayerGet (glut32.@)
 */
extern int glutLayerGet(int arg);
int WINAPI wine_glutLayerGet(int arg)
{
   return glutLayerGet(arg);
}

/****************************************************
 *	glutStrokeLength (glut32.@)
 */
extern int glutStrokeLength(void *arg1, void* arg2);
int WINAPI wine_glutStrokeLength(void *arg1, void *arg2)
{
   return glutStrokeLength(arg1, arg2);
}

/****************************************************
 *	glutStrokeWidth (glut32.@)
 */
extern int glutStrokeWidth(void *arg1, int arg2);
int WINAPI wine_glutStrokeWidth(void *arg1, int arg2)
{
   return glutStrokeWidth(arg1, arg2);
}

/****************************************************
 *	glutVideoResizeGet (glut32.@)
 */
extern int glutVideoResizeGet(int arg);
int WINAPI wine_glutVideoResizeGet(int arg)
{
   return glutVideoResizeGet(arg);
}

/****************************************************
 *	glutAddMenuEntry (glut32.@)
 */
extern void glutAddMenuEntry(void *arg1, int arg2);
void WINAPI wine_glutAddMenuEntry(void *arg1, int arg2)
{
   glutAddMenuEntry(arg1, arg2);
}

/****************************************************
 *	glutAddSubMenu (glut32.@)
 */
extern void glutAddSubMenu(void *arg1, int arg2);
void WINAPI wine_glutAddSubMenu(void *arg1, int arg2)
{
   glutAddSubMenu(arg1, arg2);
}

/****************************************************
 *	glutAttachMenu (glut32.@)
 */
extern void glutAttachMenu(int arg);
void WINAPI wine_glutAttachMenu(int arg)
{
   glutAttachMenu(arg);
}

/****************************************************
 *	glutBitmapCharacter (glut32.@)
 */
extern void glutBitmapCharacter(void *arg1, int arg2);
void WINAPI wine_glutBitmapCharacter(void *arg1, int arg2)
{
   glutBitmapCharacter(arg1, arg2);
}

/****************************************************
 *	glutButtonBoxFunc (glut32.@)
 */
extern void glutButtonBoxFunc(void *arg);
void WINAPI wine_glutButtonBoxFunc(void *arg)
{
   glutButtonBoxFunc(arg);
}

/****************************************************
 *	glutChangeToMenuEntry (glut32.@)
 */
extern void glutChangeToMenuEntry(int arg1, void *arg2, int arg3);
void WINAPI wine_glutChangeToMenuEntry(int arg1, void *arg2, int arg3)
{
   glutChangeToMenuEntry(arg1, arg2, arg3);
}

/****************************************************
 *	glutChangeToSubMenu (glut32.@)
 */
extern void glutChangeToSubMenu(int arg1, void *arg2, int arg3);
void WINAPI wine_glutChangeToSubMenu(int arg1, void *arg2, int arg3)
{
   glutChangeToSubMenu(arg1, arg2, arg3);
}

/****************************************************
 *	glutCopyColormap (glut32.@)
 */
extern void glutCopyColormap(int arg);
void WINAPI wine_glutCopyColormap(int arg)
{
   glutCopyColormap(arg);
}

/****************************************************
 *	glutDestroyMenu (glut32.@)
 */
extern void glutDestroyMenu(int arg);
void WINAPI wine_glutDestroyMenu(int arg)
{
   glutDestroyMenu(arg);
}

/****************************************************
 *	glutDestroyWindow (glut32.@)
 */
extern void glutDestroyWindow(int arg);
void WINAPI wine_glutDestroyWindow(int arg)
{
   glutDestroyWindow(arg);
}

/****************************************************
 *	glutDetachMenu (glut32.@)
 */
extern void glutDetachMenu(int arg);
void WINAPI wine_glutDetachMenu(int arg)
{
   glutDetachMenu(arg);
}

/****************************************************
 *	glutDialsFunc (glut32.@)
 */
extern void glutDialsFunc(void *arg);
void WINAPI wine_glutDialsFunc(void *arg)
{
   glutDialsFunc(arg);
}

/*******************************************************
 *	glutDisplayFunc (glut32.@)
 */
extern void glutDisplayFunc(void *arg);
void WINAPI wine_glutDisplayFunc(void *arg)
{
   glutDisplayFunc(arg);
}

/*******************************************************
 *	glutEntryFunc (glut32.@)
 */
extern void glutEntryFunc(void *arg);
void WINAPI wine_glutEntryFunc(void *arg)
{
   glutEntryFunc(arg);
}

/*******************************************************
 *	glutEstablishOverlay (glut32.@)
 */
extern void glutEstablishOverlay(void);
void WINAPI wine_glutEstablishOverlay(void)
{
   glutEstablishOverlay();
}

/*******************************************************
 *	glutForceJoystickFunc (glut32.@)
 */
extern void glutForceJoystickFunc(void);
void WINAPI wine_glutForceJoystickFunc(void)
{
   glutForceJoystickFunc();
}

/*******************************************************
 *	glutFullScreen (glut32.@)
 */
extern void glutFullScreen(void);
void WINAPI wine_glutFullScreen(void)
{
   glutFullScreen();
}

/*******************************************************
 *	glutGameModeString (glut32.@)
 */
extern void glutGameModeString(void *arg);
void WINAPI wine_glutGameModeString(void *arg)
{
   glutGameModeString(arg);
}

/*******************************************************
 *	glutHideOverlay (glut32.@)
 */
extern void glutHideOverlay(void);
void WINAPI wine_glutHideOverlay(void)
{
   glutHideOverlay();
}

/*******************************************************
 *	glutHideWindow (glut32.@)
 */
extern void glutHideWindow(void);
void WINAPI wine_glutHideWindow(void)
{
   glutHideWindow();
}

/*******************************************************
 *	glutIconifyWindow (glut32.@)
 */
extern void glutIconifyWindow(void);
void WINAPI wine_glutIconifyWindow(void)
{
   glutIconifyWindow();
}

/*********************************************
 *	glutIdleFunc (glut32.@)
 */
extern void glutIdleFunc(void *arg);
void WINAPI wine_glutIdleFunc(void *arg)
{
   glutIdleFunc(arg);
}

/*********************************************
 *	glutIgnoreKeyRepeat (glut32.@)
 */
extern void glutIgnoreKeyRepeat(int arg);
void WINAPI wine_glutIgnoreKeyRepeat(int arg)
{
   glutIgnoreKeyRepeat(arg);
}

/**********************************************
 *	glutInitDisplayMode (glut32.@)
 */
extern void glutInitDisplayMode(unsigned int arg);
void WINAPI wine_glutInitDisplayMode(unsigned int arg)
{
   glutInitDisplayMode(arg);
}

/**********************************************
 *	glutInitDisplayString (glut32.@)
 */
extern void glutInitDisplayString(void *arg);
void WINAPI wine_glutInitDisplayString(void *arg)
{
   glutInitDisplayString(arg);
}

/**********************************************
 *	glutInit (glut32.@)
 */
extern void glutInit(void *arg1, void *arg2);
void WINAPI wine_glutInit(void *arg1, void **arg2)
{
   glutInit(arg1, arg2);
}

/**********************************************
 *	__glutInitWithExit (glut32.@)
 */
void WINAPI wine___glutInitWithExit(void *arg1, void *arg2, void (*exitfunc)(int))
{
   __glutExitFunc = exitfunc;
   glutInit(arg1, arg2);
}

/***********************************************
 *	glutInitWindowPosition (glut32.@)
 */
extern void glutInitWindowPosition(int arg1, int arg2);
void WINAPI wine_glutInitWindowPosition(int arg1, int arg2)
{
   glutInitWindowPosition(arg1, arg2);
}

/***********************************************
 *	glutInitWindowSize (glut32.@)
 */
extern void glutInitWindowSize(int arg1, int arg2);
void WINAPI wine_glutInitWindowSize(int arg1, int arg2)
{
   glutInitWindowSize(arg1, arg2);
}

/***********************************************
 *	glutJoystickFunc (glut32.@)
 */
extern void glutJoystickFunc(void *arg1, int arg2);
void WINAPI wine_glutJoystickFunc(void *arg1, int arg2)
{
   glutJoystickFunc(arg1, arg2);
}

/***********************************************
 *	glutKeyboardFunc (glut32.@)
 */
extern void glutKeyboardFunc(void *arg);
void WINAPI wine_glutKeyboardFunc(void *arg)
{
   glutKeyboardFunc(arg);
}

/***********************************************
 *	glutKeyboardUpFunc (glut32.@)
 */
extern void glutKeyboardUpFunc(void *arg);
void WINAPI wine_glutKeyboardUpFunc(void *arg)
{
   glutKeyboardUpFunc(arg);
}

/***********************************************
 *	glutLeaveGameMode (glut32.@)
 */
extern void glutLeaveGameMode(void);
void WINAPI wine_glutLeaveGameMode(void)
{
   glutLeaveGameMode();
}

/***********************************************
 *	glutMainLoop (glut32.@)
 */
extern void glutMainLoop(void);
void WINAPI wine_glutMainLoop(void)
{
   glutMainLoop();
}

/***********************************************
 *	glutMenuStateFunc(glut32.@)
 */
extern void glutMenuStateFunc(void *arg);
void WINAPI wine_glutMenuStateFunc(void *arg)
{
   glutMenuStateFunc(arg);
}

/***********************************************
 *	glutMenuStatusFunc (glut32.@)
 */
extern void glutMenuStatusFunc(void *arg);
void WINAPI wine_glutMenuStatusFunc(void *arg)
{
   glutMenuStatusFunc(arg);
}

/***********************************************
 *	glutMotionFunc (glut32.@)
 */
extern void glutMotionFunc(void *arg);
void WINAPI wine_glutMotionFunc(void *arg)
{
   glutMotionFunc(arg);
}

/********************************************
 *	glutMouseFunc (glut32.@)
 */
extern void glutMouseFunc(void *arg);
void WINAPI wine_glutMouseFunc(void *arg)
{
   glutMouseFunc(arg);
}

/********************************************
 *	glutOverlayDisplayFunc (glut32.@)
 */
extern void glutOverlayDisplayFunc(void *arg);
void WINAPI wine_glutOverlayDisplayFunc(void *arg)
{
   glutOverlayDisplayFunc(arg);
}

/********************************************
 *      glutPassiveMotionFunc (glut32.@)
 */
extern void glutPassiveMotionFunc(void *arg);
void WINAPI wine_glutPassiveMotionFunc(void *arg)
{
   glutPassiveMotionFunc(arg);
}

/********************************************
 *      glutPopWindow (glut32.@)
 */
extern void glutPopWindow(void);
void WINAPI wine_glutPopWindow(void)
{
   glutPopWindow();
}

/********************************************
 *      glutPositionWindow (glut32.@)
 */
extern void glutPositionWindow(int arg1, int arg2);
void WINAPI wine_glutPositionWindow(int arg1, int arg2)
{
   glutPositionWindow(arg1, arg2);
}

/********************************************
 *      glutPostOverlayRedisplay (glut32.@)
 */
extern void glutPostOverlayRedisplay(void);
void WINAPI wine_glutPostOverlayRedisplay(void)
{
   glutPostOverlayRedisplay();
}

/********************************************
 *	glutPostRedisplay (glut32.@)
 */
extern void glutPostRedisplay(void);
void WINAPI wine_glutPostRedisplay(void)
{
   glutPostRedisplay();
}

/********************************************
 *	glutPostWindowOverlayRedisplay (glut32.@)
 */
extern void glutPostWindowOverlayRedisplay(int arg);
void WINAPI wine_glutPostWindowOverlayRedisplay(int arg)
{
   glutPostWindowOverlayRedisplay(arg);
}

/********************************************
 *      glutPostWindowRedisplay (glut32.@)
 */
extern void glutPostWindowRedisplay(int arg);
void WINAPI wine_glutPostWindowRedisplay(int arg)
{
   glutPostWindowRedisplay(arg);
}

/********************************************
 *      glutPushWindow (glut32.@)
 */
extern void glutPushWindow(void);
void WINAPI wine_glutPushWindow(void)
{
   glutPushWindow();
}

/********************************************
 *      glutRemoveMenuItem (glut32.@)
 */
extern void glutRemoveMenuItem(int arg);
void WINAPI wine_glutRemoveMenuItem(int arg)
{
   glutRemoveMenuItem(arg);
}

/********************************************
 *      glutRemoveOverlay (glut32.@)
 */
extern void glutRemoveOverlay(void);
void WINAPI wine_glutRemoveOverlay(void)
{
   glutRemoveOverlay();
}

/********************************************
 *      glutReportErrors (glut32.@)
 */
extern void glutReportErrors(void);
void WINAPI wine_glutReportErrors(void)
{
   glutReportErrors();
}

/********************************************
 *	glutReshapeFunc (glut32.@)
 */
extern void glutReshapeFunc(void *arg);
void WINAPI wine_glutReshapeFunc(void *arg)
{
   glutReshapeFunc(arg);
}

/********************************************
 *	glutReshapeWindow (glut32.@)
 */
extern void glutReshapeWindow(int arg1, int arg2);
void WINAPI wine_glutReshapeWindow(int arg1, int arg2)
{
   glutReshapeWindow(arg1, arg2);
}

/********************************************
 *	glutSetColor (glut32.@)
 */
extern void glutSetColor(float arg1, float arg2, float arg3);
void WINAPI wine_glutSetColor(float arg1, float arg2, float arg3)
{
   glutSetColor(arg1, arg2, arg3);
}

/********************************************
 *	glutSetCursor (glut32.@)
 */
extern void glutSetCursor(int arg);
void WINAPI wine_glutSetCursor(int arg)
{
   glutSetCursor(arg);
}

/********************************************
 *	glutSetIconTitle (glut32.@)
 */
extern void glutSetIconTitle(void *arg);
void WINAPI wine_glutSetIconTitle(void *arg)
{
   glutSetIconTitle(arg);
}

/********************************************
 *	glutSetKeyRepeat (glut32.@)
 */
extern void glutSetKeyRepeat(int arg);
void WINAPI wine_glutSetKeyRepeat(int arg)
{
   glutSetKeyRepeat(arg);
}

/********************************************
 *	glutSetMenu (glut32.@)
 */
extern void glutSetMenu(int arg);
void WINAPI wine_glutSetMenu(int arg)
{
   glutSetMenu(arg);
}

/********************************************
 *	glutSetupVideoResizing (glut32.@)
 */
extern void glutSetupVideoResizing(void);
void WINAPI wine_glutSetupVideoResizing(void)
{
/*   glutSetupVideoResizing(); */
}

/********************************************
 *	glutSetWindow (glut32.@)
 */
extern void glutSetWindow(int arg);
void WINAPI wine_glutSetWindow(int arg)
{
   glutSetWindow(arg);
}

/********************************************
 *	glutSetWindowTitle (glut32.@)
 */
extern void glutSetWindowTitle(void *arg);
void WINAPI wine_glutSetWindowTitle(void *arg)
{
   glutSetWindowTitle(arg);
}

/********************************************
 *	glutShowOverlay (glut32.@)
 */
extern void glutShowOverlay(void);
void WINAPI wine_glutShowOverlay(void)
{
   glutShowOverlay();
}

/********************************************
 *	glutShowWindow (glut32.@)
 */
extern void glutShowWindow(void);
void WINAPI wine_glutShowWindow(void)
{
   glutShowWindow();
}

/*********************************************
 * glutSolidCone (glut32.@)
 */
extern void glutSolidCone(double arg1, double arg2, int arg3, int arg4);
void WINAPI wine_glutSolidCone(double arg1, double arg2, int arg3, int arg4)
{
   glutSolidCone(arg1, arg2, arg3, arg4);
}

/**********************************************
 *	glutSolidCube (glut32.@)
 */
extern void glutSolidCube(double arg);
void WINAPI wine_glutSolidCube(double arg)
{
   glutSolidCube(arg);
}

/**********************************************
 *	glutSolidDodecahedron (glut32.@)
 */
extern void glutSolidDodecahedron(void);
void WINAPI wine_glutSolidDodecahedron(void)
{
   glutSolidDodecahedron();
}

/**********************************************
 *      glutSolidIcosahedron (glut32.@)
 */
extern void glutSolidIcosahedron(void);
void WINAPI wine_glutSolidIcosahedron(void)
{
   glutSolidIcosahedron();
}

/**********************************************
 *      glutSolidOctahedron (glut32.@)
 */
extern void glutSolidOctahedron(void);
void WINAPI wine_glutSolidOctahedron(void)
{
   glutSolidOctahedron();
}

/**********************************************
 *	glutSolidSphere (glut32.@)
 */
extern void glutSolidSphere(double arg1, int arg2, int arg3);
void WINAPI wine_glutSolidSphere(double arg1, int arg2, int arg3)
{
   glutSolidSphere(arg1, arg2, arg3);
}

/**********************************************
 *      glutSolidTeapot (glut32.@)
 */
extern void glutSolidTeapot(double arg);
void WINAPI wine_glutSolidTeapot(double arg)
{
   glutSolidTeapot(arg);
}

/**********************************************
 *      glutSolidTetrahedron (glut32.@)
 */
extern void glutSolidTetrahedron(void);
void WINAPI wine_glutSolidTetrahedron(void)
{
   glutSolidTetrahedron();
}

/**********************************************
 *      glutSolidTetrahedron (glut32.@)
 */
extern void glutSolidTorus(double arg1, double arg2,int arg3, int arg4);
void WINAPI wine_glutSolidTorus(double arg1, double arg2,int arg3, int arg4)
{
   glutSolidTorus(arg1, arg2, arg3, arg4);
}

/**********************************************
 *      glutSpaceballButtonFunc (glut32.@)
 */
extern void glutSpaceballButtonFunc(void *arg);
void WINAPI wine_glutSpaceballButtonFunc(void *arg)
{
   glutSpaceballButtonFunc(arg);
}

/**********************************************
 *      glutSpaceballMotionFunc (glut32.@)
 */
extern void glutSpaceballMotionFunc(void *arg);
void WINAPI wine_glutSpaceballMotionFunc(void *arg)
{
   glutSpaceballMotionFunc(arg);
}

/**********************************************
 *      glutSpaceballRotateFunc (glut32.@)
 */
extern void glutSpaceballRotateFunc(void *arg);
void WINAPI wine_glutSpaceballRotateFunc(void *arg)
{
   glutSpaceballRotateFunc(arg);
}

/**********************************************
 *	glutSpecialFunc (glut32.@)
 */
extern void glutSpecialFunc(void *arg);
void WINAPI wine_glutSpecialFunc(void *arg)
{
   glutSpecialFunc(arg);
}

/**********************************************
 *      glutSpecialUpFunc (glut32.@)
 */
extern void glutSpecialUpFunc(void *arg);
void WINAPI wine_glutSpecialUpFunc(void *arg)
{
   glutSpecialUpFunc(arg);
}

/**********************************************
 *      glutStopVideoResizing (glut32.@)
 */
extern void glutStopVideoResizing(void);
void WINAPI wine_glutStopVideoResizing(void)
{
   glutStopVideoResizing();
}

/**********************************************
 *      glutStrokeCharacter (glut32.@)
 */
extern void glutStrokeCharacter(void *arg1, int arg2);
void WINAPI wine_glutStrokeCharacter(void *arg1, int arg2)
{
   glutStrokeCharacter(arg1, arg2);
}

/**************************************************
 *	glutSwapBuffers (glut32.@)
 */
extern void glutSwapBuffers(void);
void WINAPI wine_glutSwapBuffers(void)
{
   glutSwapBuffers();
}

/**********************************************
 *      glutTabletButtonFunc (glut32.@)
 */
extern void glutTabletButtonFunc(void *arg);
void WINAPI wine_glutTabletButtonFunc(void *arg)
{
   glutTabletButtonFunc(arg);
}

/**********************************************
 *      glutTabletMotionFunc (glut32.@)
 */
extern void glutTabletMotionFunc(void *arg);
void WINAPI wine_glutTabletMotionFunc(void *arg)
{
   glutTabletMotionFunc(arg);
}

/**********************************************
 *      glutTimerFunc (glut32.@)
 */
extern void glutTimerFunc(int arg1, void *arg2, int arg3);
void WINAPI wine_glutTimerFunc(int arg1, void *arg2, int arg3)
{
   glutTimerFunc(arg1, arg2, arg3);
}

/**********************************************
 *      glutUseLayer (glut32.@)
 */
extern void glutUseLayer(int arg);
void WINAPI wine_glutUseLayer(int arg)
{
   glutUseLayer(arg);
}

/**********************************************
 *      glutVideoPan (glut32.@)
 */
extern void glutVideoPan(int arg1, int arg2, int arg3, int arg4);
void WINAPI wine_glutVideoPan(int arg1, int arg2, int arg3, int arg4)
{
   glutVideoPan(arg1, arg2, arg3, arg4);
}

/**********************************************
 *      glutVideoResize (glut32.@)
 */
extern void glutVideoResize(int arg1, int arg2, int arg3, int arg4);
void WINAPI wine_glutVideoResize(int arg1, int arg2, int arg3, int arg4)
{
   glutVideoResize(arg1, arg2, arg3, arg4);
}

/**********************************************
 *      glutVisibilityFunc (glut32.@)
 */
extern void glutVisibilityFunc(void *arg);
void WINAPI wine_glutVisibilityFunc(void *arg)
{
   glutVisibilityFunc(arg);
}

/**********************************************
 *      glutWarpPointer (glut32.@)
 */
extern void glutWarpPointer(int arg1, int arg2);
void WINAPI wine_glutWarpPointer(int arg1, int arg2)
{
   glutWarpPointer(arg1, arg2);
}

/**********************************************
 *      glutWindowStatusFunc (glut32.@)
 */
extern void glutWindowStatusFunc(void *arg);
void WINAPI wine_glutWindowStatusFunc(void *arg)
{
   glutWindowStatusFunc(arg);
}

/**********************************************
 *	glutWireCone (glut32.@)
 */
extern void glutWireCone(double arg1, double arg2, int arg3, int arg4);
void WINAPI wine_glutWireCone(double arg1, double arg2, int arg3,int arg4)
{
   glutWireCone(arg1, arg2, arg3, arg4);
}

/**********************************************
 *      glutWireCube (glut32.@)
 */
extern void glutWireCube(double arg);
void WINAPI wine_glutWireCube(double arg)
{
   glutWireCube(arg);
}

/**********************************************
 *      glutWireDodecahedron (glut32.@)
 */
extern void glutWireDodecahedron(void);
void WINAPI wine_glutWireDodecahedron(void)
{
   glutWireDodecahedron();
}

/**********************************************
 *      glutWireIcosahedron (glut32.@)
 */
extern void glutWireIcosahedron(void);
void WINAPI wine_glutWireIcosahedron(void)
{
   glutWireIcosahedron();
}

/**********************************************
 *      glutWireOctahedron (glut32.@)
 */
extern void glutWireOctahedron(void);
void WINAPI wine_glutWireOctahedron(void)
{
   glutWireOctahedron();
}

/**********************************************
 *      glutWireSphere (glut32.@)
 */
extern void glutWireSphere(double arg1, int arg2, int arg3);
void WINAPI wine_glutWireSphere(double arg1, int arg2, int arg3)
{
   glutWireSphere(arg1, arg2, arg3);
}

/**********************************************
 *      glutWireTeapot (glut32.@)
 */
extern void glutWireTeapot(double arg);
void WINAPI wine_glutWireTeapot(double arg)
{
   glutWireTeapot(arg);
}

/**********************************************
 *     glutWireTetrahedron (glut32.@)
 */
extern void glutWireTetrahedron(void);
void WINAPI wine_glutWireTetrahedron(void)
{
   glutWireTetrahedron();
}

/***********************************************
 *	glutWireTorus (glut32.@)
 */
extern void glutWireTorus(double arg1, double arg2, int arg3, int arg4);
void WINAPI wine_glutWireTorus(double arg1, double arg2, int arg3, int arg4)
{
   glutWireTorus(arg1, arg2, arg3, arg4);
}
