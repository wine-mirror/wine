/* 
 * Driver Environment functions
 *
 * Note: This has NOTHING to do with the task/process environment!
 *
 * Copyright 1997 Marcus Meissner
 * Copyright 1998 Andreas Mohr
 */
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <config.h>
#include <gdi.h>
#include <debug.h>

typedef struct {
        ATOM atom;
	HGLOBAL16 handle;
} ENVTABLE;

static ENVTABLE EnvTable[20];

static ENVTABLE *SearchEnvTable(ATOM atom)
{
    INT16 i;
    
    for (i = 20; i; i--) {
      if (EnvTable[i].atom == atom) return &EnvTable[i];
    }
    return NULL;
}

static ATOM GDI_GetNullPortAtom(void)
{
    static ATOM NullPortAtom = 0;
    if (!NullPortAtom)
    {
        char NullPort[256];
        
        GetProfileString32A( "windows", "nullport", "none",
                             NullPort, sizeof(NullPort) );
        NullPortAtom = AddAtom32A( NullPort );
    }
    return NullPortAtom;
}

static ATOM PortNameToAtom(LPCSTR lpPortName, BOOL16 add)
{
    char PortName[256];
    LPCSTR p;

    if (lpPortName[strlen(lpPortName) - 1] == ':') {
        strncpy(PortName, lpPortName, strlen(lpPortName) - 1);
        p = PortName;
    }
    else
        p = lpPortName;

    if (add)
        return AddAtom32A(p);
    else
        return FindAtom32A(p);
}


/***********************************************************************
 *           GetEnvironment   (GDI.134)
 */
INT16 WINAPI GetEnvironment(LPCSTR lpPortName, LPDEVMODE16 lpdev, UINT16 nMaxSize)
{
    ATOM atom;
    LPCSTR p;
    ENVTABLE *env;
    WORD size;

    TRACE(gdi, "('%s', %p, %d)\n", lpPortName, lpdev, nMaxSize);

    if (!(atom = PortNameToAtom(lpPortName, FALSE)))
	return 0;
    if (atom == GDI_GetNullPortAtom())
	if (!(atom = FindAtom32A((LPCSTR)lpdev)))
	    return 0;
    if (!(env = SearchEnvTable(atom)))
	return 0;
    size = GlobalSize16(env->handle);
    if (!lpdev) return 0;
    if (size < nMaxSize) nMaxSize = size;
    if (!(p = GlobalLock16(env->handle))) return 0;
    memcpy(lpdev, p, nMaxSize);
    GlobalUnlock16(env->handle);
    return nMaxSize;
}


/***********************************************************************
 *          SetEnvironment   (GDI.132)
 */
INT16 WINAPI SetEnvironment(LPCSTR lpPortName, LPDEVMODE16 lpdev, UINT16 nCount)
{
    ATOM atom; 
    BOOL16 nullport = FALSE;
    LPSTR p;
    ENVTABLE *env;
    HGLOBAL16 handle;

    TRACE(gdi, "('%s', %p, %d)\n", lpPortName, lpdev, nCount);

    if ((atom = PortNameToAtom(lpPortName, FALSE))) {
	if (atom == GDI_GetNullPortAtom()) {
	    nullport = TRUE;
	    atom = FindAtom32A((LPCSTR)lpdev);
	}
	env = SearchEnvTable(atom);
        GlobalFree16(env->handle);
        env->atom = 0;
    }
    if (nCount) { /* store DEVMODE struct */
	if (nullport)
	    p = (LPSTR)lpdev;
        else
	    p = (LPSTR)lpPortName;

        if ((atom = PortNameToAtom(p, TRUE))
	 && (env = SearchEnvTable(0))
	 && (handle = GlobalAlloc16(GMEM_SHARE|GMEM_MOVEABLE, nCount))) {
	    if (!(p = GlobalLock16(handle))) {
	        GlobalFree16(handle);
	        return 0;
	    }
	    env->atom = atom;
	    env->handle = handle;
            memcpy(p, lpdev, nCount);
            GlobalUnlock16(handle);
            return handle;
	}
	else return 0;
    }
    else return -1;
}
