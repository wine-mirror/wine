/*
 * PURPOSE: Load a DLL and run an entry point with the specified parameters
 *
 * Copyright 2002 Alberto Massari
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/*
 *
 *  rundll32 dllname,entrypoint [arguments]
 *
 *  Documentation for this utility found on KB Q164787
 *
 */

/**
 * FIXME - currently receives command-line parameters in ASCII only and later
 * converts to Unicode. Ideally the function should have wWinMain entry point
 * and then work in Unicode only, but it seems Wine does not have necessary
 * support.
 */

#include "config.h"
#include "wine/port.h"

#include <stdio.h>
#include <string.h>
#include <windows.h>

typedef void (*EntryPointW) (HWND hWnd, HINSTANCE hInst, LPWSTR lpszCmdLine, int nCmdShow);
typedef void (*EntryPointA) (HWND hWnd, HINSTANCE hInst, LPSTR lpszCmdLine, int nCmdShow);

/**
 * Loads procedure.
 *
 * Parameters:
 * strDll - name of the dll.
 * procName - name of the procedure to load from dll
 * pDllHanlde - output variable receives handle of the loaded dll.
 */
static FARPROC LoadProc(char* strDll, char* procName, HMODULE* DllHandle)
{
    FARPROC proc;

    *DllHandle = LoadLibrary(strDll);
    if(!*DllHandle)
    {
        exit(-1);
    }
    proc = GetProcAddress(*DllHandle, procName);
    if(!proc)
    {
        FreeLibrary(*DllHandle);
        return NULL;
    }
    return proc;
}

int main(int argc, char* argv[])
{
    char szDllName[MAX_PATH],szEntryPoint[64],szCmdLine[2048];
    char* comma;
    EntryPointW pfEntryPointW;
    EntryPointA pfEntryPointA;
    HMODULE DllHandle=NULL;

    if(argc<2)
        return 0;
    comma=strchr(argv[1],',');
    if(comma==NULL)
        return 0;
    /* Extract the name of the DLL */
    memset(szDllName,0,MAX_PATH);
    strncpy(szDllName,argv[1],(comma-argv[1]));
    /* Merge the other paramters into one big command line */
    memset(szCmdLine,0,2048);
    if(argc>2)
    {
        int i;
        for(i=2;i<argc;i++)
        {
            strcat(szCmdLine,argv[i]);
            if(i+1<argc) strcat(szCmdLine," ");
        }
    }

    /* try loading the UNICODE version first */
    strcpy(szEntryPoint,comma+1);
    strcat(szEntryPoint,"W");
    pfEntryPointW=(EntryPointW)LoadProc(szDllName, szEntryPoint, &DllHandle);
    if(pfEntryPointW!=NULL)
    {
        WCHAR wszCmdLine[2048];
        MultiByteToWideChar(CP_ACP,0,szCmdLine,-1,wszCmdLine,2048);
        pfEntryPointW(NULL,DllHandle,wszCmdLine,SW_HIDE);
    }
    else
    {
        strcpy(szEntryPoint,comma+1);
        strcat(szEntryPoint,"A");
        pfEntryPointA=(EntryPointA)LoadProc(szDllName, szEntryPoint, &DllHandle);
        if(pfEntryPointA==NULL)
        {
            strcpy(szEntryPoint,comma+1);
            pfEntryPointA=(EntryPointA)LoadProc(szDllName, szEntryPoint, &DllHandle);
            if(pfEntryPointA==NULL)
                return 0;
        }
        pfEntryPointA(NULL,DllHandle,szCmdLine,SW_HIDE);
    }
    if(DllHandle)
        FreeLibrary(DllHandle);
    return 0;
}
