/*
 * Modules
 *
 * Copyright 1995 Alexandre Julliard
 */

#include <assert.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include "windows.h"
#include "class.h"
#include "file.h"
#include "global.h"
#include "heap.h"
#include "module.h"
#include "neexe.h"
#include "pe_image.h"
#include "process.h"
#include "thread.h"
#include "resource.h"
#include "selectors.h"
#include "stackframe.h"
#include "task.h"
#include "toolhelp.h"
#include "debug.h"
#include "callback.h"

extern BOOL32 THREAD_InitDone;

extern HMODULE16 hFirstModule;  /* FIXME */
static HMODULE16 hCachedModule = 0;  /* Module cached by MODULE_OpenFile */

static HMODULE32 MODULE_LoadModule(LPCSTR name,BOOL32 force) { return 0; }
HMODULE32 (*fnBUILTIN_LoadModule)(LPCSTR name,BOOL32 force) = MODULE_LoadModule;


/*************************************************************************
 *		MODULE32_LookupHMODULE
 * looks for the referenced HMODULE in the current process
 */
WINE_MODREF*
MODULE32_LookupHMODULE(PDB32 *process,HMODULE32 hmod) {
    WINE_MODREF	*wm;

    if (!hmod) 
    	return process->exe_modref;
    if (!HIWORD(hmod)) {
    	ERR(module,"tried to lookup 0x%04x in win32 module handler!\n",hmod);
	return NULL;
    }
    for (wm = process->modref_list;wm;wm=wm->next)
	if (wm->module == hmod)
	    return wm;
    return NULL;
}

/***********************************************************************
 *           MODULE_GetPtr16
 */
NE_MODULE *MODULE_GetPtr16( HMODULE16 hModule )
{
    return (NE_MODULE*)GlobalLock16( GetExePtr(hModule) );
}


/***********************************************************************
 *           MODULE_GetPtr32
 */
NE_MODULE *MODULE_GetPtr32( HMODULE32 hModule )
{
    return (NE_MODULE*)GlobalLock16( MODULE_HANDLEtoHMODULE16(hModule) );
}

/***********************************************************************
 *           MODULE_HANDLEtoHMODULE16
 */
HMODULE16
MODULE_HANDLEtoHMODULE16(HANDLE32 handle) {
    NE_MODULE	*pModule;

    if (HIWORD(handle))
    {
        WARN(module,"looking up 0x%08x in win16 function!\n",handle);
	/* this is a HMODULE32 */

        /* walk the list looking for the correct startaddress */
    	pModule = (NE_MODULE *)GlobalLock16( hFirstModule );
	while (pModule)
        {
            if (pModule->module32 == handle) return pModule->self;
            pModule = (NE_MODULE*)GlobalLock16(pModule->next);
	}
	return 0;
    }
    return GetExePtr(handle);
}

/***********************************************************************
 *           MODULE_OpenFile
 */
int MODULE_OpenFile( HMODULE32 hModule )
{
    NE_MODULE *pModule;
    DOS_FULL_NAME full_name;
    char *name;

    static int cachedfd = -1;

    hModule = MODULE_HANDLEtoHMODULE16(hModule);
    TRACE(module, "(%04x) cache: mod=%04x fd=%d\n",
                    hModule, hCachedModule, cachedfd );
    if (!(pModule = MODULE_GetPtr32( hModule ))) return -1;
    if (hCachedModule == hModule) return cachedfd;
    close( cachedfd );
    hCachedModule = hModule;
    name = NE_MODULE_NAME( pModule );
    if (!DOSFS_GetFullName( name, TRUE, &full_name ) ||
        (cachedfd = open( full_name.long_name, O_RDONLY )) == -1)
        WARN( module, "Can't open file '%s' for module %04x\n",
                 name, hModule );
    TRACE(module, "opened '%s' -> %d\n",
                    name, cachedfd );
    return cachedfd;
}


/***********************************************************************
 *           MODULE_CreateInstance
 *
 * If lib_only is TRUE, handle the module like a library even if it is a .EXE
 */
HINSTANCE16 MODULE_CreateInstance( HMODULE16 hModule, HINSTANCE16 *prev,
                                   BOOL32 lib_only )
{
    SEGTABLEENTRY *pSegment;
    NE_MODULE *pModule;
    int minsize;
    HINSTANCE16 hNewInstance;

    if (!(pModule = MODULE_GetPtr16( hModule ))) return 0;
    if (pModule->dgroup == 0)
    {
        if (prev) *prev = hModule;
        return hModule;
    }

    pSegment = NE_SEG_TABLE( pModule ) + pModule->dgroup - 1;
    if (prev) *prev = pSegment->selector;

      /* if it's a library, create a new instance only the first time */
    if (pSegment->selector)
    {
        if (pModule->flags & NE_FFLAGS_LIBMODULE) return pSegment->selector;
        if (lib_only) return pSegment->selector;
    }

    minsize = pSegment->minsize ? pSegment->minsize : 0x10000;
    if (pModule->ss == pModule->dgroup) minsize += pModule->stack_size;
    minsize += pModule->heap_size;
    hNewInstance = GLOBAL_Alloc( GMEM_ZEROINIT | GMEM_FIXED,
                                 minsize, hModule, FALSE, FALSE, FALSE );
    if (!hNewInstance) return 0;
    pSegment->selector = hNewInstance;
    return hNewInstance;
}


/***********************************************************************
 *           MODULE_CreateDummyModule
 *
 * Create a dummy NE module for Win32 or Winelib.
 */
HMODULE32 MODULE_CreateDummyModule( const OFSTRUCT *ofs )
{
    HMODULE32 hModule;
    NE_MODULE *pModule;
    SEGTABLEENTRY *pSegment;
    char *pStr,*s;
    int len;
    const char* basename;

    INT32 of_size = sizeof(OFSTRUCT) - sizeof(ofs->szPathName)
                    + strlen(ofs->szPathName) + 1;
    INT32 size = sizeof(NE_MODULE) +
                 /* loaded file info */
                 of_size +
                 /* segment table: DS,CS */
                 2 * sizeof(SEGTABLEENTRY) +
                 /* name table */
                 9 +
                 /* several empty tables */
                 8;

    hModule = GlobalAlloc16( GMEM_MOVEABLE | GMEM_ZEROINIT, size );
    if (!hModule) return (HMODULE32)11;  /* invalid exe */

    FarSetOwner( hModule, hModule );
    pModule = (NE_MODULE *)GlobalLock16( hModule );

    /* Set all used entries */
    pModule->magic            = IMAGE_OS2_SIGNATURE;
    pModule->count            = 1;
    pModule->next             = 0;
    pModule->flags            = 0;
    pModule->dgroup           = 1;
    pModule->ss               = 1;
    pModule->cs               = 2;
    pModule->heap_size        = 0xe000;
    pModule->stack_size       = 0x1000;
    pModule->seg_count        = 2;
    pModule->modref_count     = 0;
    pModule->nrname_size      = 0;
    pModule->fileinfo         = sizeof(NE_MODULE);
    pModule->os_flags         = NE_OSFLAGS_WINDOWS;
    pModule->expected_version = 0x030a;
    pModule->self             = hModule;

    /* Set loaded file information */
    memcpy( pModule + 1, ofs, of_size );
    ((OFSTRUCT *)(pModule+1))->cBytes = of_size - 1;

    pSegment = (SEGTABLEENTRY*)((char*)(pModule + 1) + of_size);
    pModule->seg_table = pModule->dgroup_entry = (int)pSegment - (int)pModule;
    /* Data segment */
    pSegment->size    = 0;
    pSegment->flags   = NE_SEGFLAGS_DATA;
    pSegment->minsize = 0x1000;
    pSegment++;
    /* Code segment */
    pSegment->flags   = 0;
    pSegment++;

    /* Module name */
    pStr = (char *)pSegment;
    pModule->name_table = (int)pStr - (int)pModule;
    basename = strrchr(ofs->szPathName,'\\');
    if (!basename) basename = ofs->szPathName;
    else basename++;
    len = strlen(basename);
    if ((s = strchr(basename,'.'))) len = s - basename;
    if (len > 8) len = 8;
    *pStr = len;
    strncpy( pStr+1, basename, len );
    if (len < 8) pStr[len+1] = 0;
    pStr += 9;

    /* All tables zero terminated */
    pModule->res_table = pModule->import_table = pModule->entry_table =
		(int)pStr - (int)pModule;

    NE_RegisterModule( pModule );
    return hModule;
}


/***********************************************************************
 *           MODULE_GetWndProcEntry16  (not a Windows API function)
 *
 * Return an entry point from the WPROCS dll.
 */
FARPROC16 MODULE_GetWndProcEntry16( LPCSTR name )
{
    FARPROC16 ret = NULL;

    if (__winelib)
    {
        /* FIXME: hack for Winelib */
        extern LRESULT ColorDlgProc(HWND16,UINT16,WPARAM16,LPARAM);
        extern LRESULT FileOpenDlgProc(HWND16,UINT16,WPARAM16,LPARAM);
        extern LRESULT FileSaveDlgProc(HWND16,UINT16,WPARAM16,LPARAM);
        extern LRESULT FindTextDlgProc16(HWND16,UINT16,WPARAM16,LPARAM);
        extern LRESULT PrintDlgProc(HWND16,UINT16,WPARAM16,LPARAM);
        extern LRESULT PrintSetupDlgProc(HWND16,UINT16,WPARAM16,LPARAM);
        extern LRESULT ReplaceTextDlgProc16(HWND16,UINT16,WPARAM16,LPARAM);

        if (!strcmp(name,"ColorDlgProc"))
            return (FARPROC16)ColorDlgProc;
        if (!strcmp(name,"FileOpenDlgProc"))
            return (FARPROC16)FileOpenDlgProc;
        if (!strcmp(name,"FileSaveDlgProc"))
            return (FARPROC16)FileSaveDlgProc;
        if (!strcmp(name,"FindTextDlgProc"))
            return (FARPROC16)FindTextDlgProc16;
        if (!strcmp(name,"PrintDlgProc"))
            return (FARPROC16)PrintDlgProc;
        if (!strcmp(name,"PrintSetupDlgProc"))
            return (FARPROC16)PrintSetupDlgProc;
        if (!strcmp(name,"ReplaceTextDlgProc"))
            return (FARPROC16)ReplaceTextDlgProc16;
        WARN(module,"No mapping for %s(), add one in library/miscstubs.c\n",name);
        assert( FALSE );
        return NULL;
    }
    else
    {
        WORD ordinal;
        static HMODULE32 hModule = 0;

        if (!hModule) hModule = GetModuleHandle16( "WPROCS" );
        ordinal = NE_GetOrdinal( hModule, name );
        if (!(ret = NE_GetEntryPoint( hModule, ordinal )))
        {            
            WARN( module, "%s not found\n", name );
            assert( FALSE );
        }
    }
    return ret;
}


/***********************************************************************
 *           MODULE_GetModuleName
 */
LPSTR MODULE_GetModuleName( HMODULE32 hModule )
{
    NE_MODULE *pModule;
    BYTE *p, len;
    static char buffer[10];

    if (!(pModule = MODULE_GetPtr32( hModule ))) return NULL;
    p = (BYTE *)pModule + pModule->name_table;
    len = MIN( *p, 8 );
    memcpy( buffer, p + 1, len );
    buffer[len] = '\0';
    return buffer;
}


/**********************************************************************
 *	    MODULE_FindModule
 *
 * Find a module from a path name.
 * RETURNS
 *	the win16 module handle if found
 * 	0 if not
 */
HMODULE32 MODULE_FindModule16(
	LPCSTR path	/* [in] path of the module to be found */
) {
    HMODULE32 hModule = hFirstModule;
    LPCSTR	filename, dotptr, modulepath, modulename;
    BYTE	len, *name_table;

    if (!(filename = strrchr( path, '\\' ))) filename = path;
    else filename++;
    if ((dotptr = strrchr( filename, '.' )) != NULL)
        len = (BYTE)(dotptr - filename);
    else len = strlen( filename );

    while(hModule)
    {
        NE_MODULE *pModule = MODULE_GetPtr16( hModule );
        if (!pModule) break;
        modulepath = NE_MODULE_NAME(pModule);
        if (!(modulename = strrchr( modulepath, '\\' )))
            modulename = modulepath;
        else modulename++;
        if (!lstrcmpi32A( modulename, filename )) return hModule;

        name_table = (BYTE *)pModule + pModule->name_table;
        if ((*name_table == len) && !lstrncmpi32A(filename, name_table+1, len))
            return hModule;
        hModule = pModule->next;
    }
    return 0;
}

/**********************************************************************
 *	    MODULE_FindModule32
 *
 * Find a (loaded) win32 module depending on path
 * The handling of '.' is a bit weird, but we need it that way, 
 * for sometimes the programs use '<name>.exe' and '<name>.dll' and
 * this is the only way to differentiate. (mainly hypertrm.exe)
 *
 * RETURNS
 *	the module handle if found
 * 	0 if not
 */
HMODULE32 MODULE_FindModule32(
	PDB32* process,	/* [in] process in which to find the library */
	LPCSTR path	/* [in] pathname of module/library to be found */
) {
    LPSTR	filename;
    LPSTR	dotptr;
    WINE_MODREF	*wm;

    if (!(filename = strrchr( path, '\\' )))
    	filename = HEAP_strdupA(process->heap,0,path);
    else 
    	filename = HEAP_strdupA(process->heap,0,filename+1);
    dotptr=strrchr(filename,'.');

    if (!process) {
    	HeapFree(process->heap,0,filename);
    	return 0;
    }
    for (wm=process->modref_list;wm;wm=wm->next) {
    	LPSTR	xmodname,xdotptr;

	assert (wm->modname);
	xmodname = HEAP_strdupA(process->heap,0,wm->modname);
	xdotptr=strrchr(xmodname,'.');
	if (	(xdotptr && !dotptr) ||
		(!xdotptr && dotptr)
	) {
	    if (dotptr)	*dotptr		= '\0';
	    if (xdotptr) *xdotptr	= '\0';
	}
	if (!lstrcmpi32A( filename, xmodname)) {
	    HeapFree(process->heap,0,filename);
	    HeapFree(process->heap,0,xmodname);
	    return wm->module;
	}
	if (dotptr) *dotptr='.';
	/* FIXME: add paths, shortname */
	HeapFree(process->heap,0,xmodname);
    }
    HeapFree(process->heap,0,filename);
    return 0;
}



/**********************************************************************
 *	    MODULE_CallWEP
 *
 * Call a DLL's WEP, allowing it to shut down.
 * FIXME: we always pass the WEP WEP_FREE_DLL, never WEP_SYSTEM_EXIT
 */
static BOOL16 MODULE_CallWEP( HMODULE16 hModule )
{
    FARPROC16 WEP = (FARPROC16)0;
    WORD ordinal = NE_GetOrdinal( hModule, "WEP" );

    if (ordinal) WEP = NE_GetEntryPoint( hModule, ordinal );
    if (!WEP)
    {
	WARN(module, "module %04x doesn't have a WEP\n", hModule );
	return FALSE;
    }
    return Callbacks->CallWindowsExitProc( WEP, WEP_FREE_DLL );
}


/**********************************************************************
 *	    MODULE_FreeModule
 *
 * Remove a module from memory.
 */
BOOL16 MODULE_FreeModule( HMODULE32 hModule, TDB* pTaskContext )
{
    HMODULE16 *hPrevModule;
    NE_MODULE *pModule;
    SEGTABLEENTRY *pSegment;
    HMODULE16 *pModRef;
    int i;

    if (!(pModule = MODULE_GetPtr32( hModule ))) return FALSE;
    hModule = pModule->self;

    if (((INT16)(--pModule->count)) > 0 ) return TRUE;
    else pModule->count = 0;

    if (pModule->flags & NE_FFLAGS_BUILTIN)
        return FALSE;  /* Can't free built-in module */

    if (pModule->flags & NE_FFLAGS_LIBMODULE) 
    {
	MODULE_CallWEP( hModule );

	/* Free the objects owned by the DLL module */

	if( pTaskContext && pTaskContext->userhandler )
	{
            pTaskContext->userhandler( hModule, USIG_DLL_UNLOAD, 0,
                                       pTaskContext->hInstance,
                                       pTaskContext->hQueue );
	}
    }
    /* Clear magic number just in case */

    pModule->magic = pModule->self = 0;

      /* Remove it from the linked list */

    hPrevModule = &hFirstModule;
    while (*hPrevModule && (*hPrevModule != hModule))
    {
        hPrevModule = &(MODULE_GetPtr16( *hPrevModule ))->next;
    }
    if (*hPrevModule) *hPrevModule = pModule->next;

      /* Free all the segments */

    pSegment = NE_SEG_TABLE( pModule );
    for (i = 1; i <= pModule->seg_count; i++, pSegment++)
    {
        GlobalFree16( pSegment->selector );
    }

      /* Free the referenced modules */

    pModRef = (HMODULE16*)NE_MODULE_TABLE( pModule );
    for (i = 0; i < pModule->modref_count; i++, pModRef++)
    {
        FreeModule16( *pModRef );
    }

      /* Free the module storage */

    if (pModule->nrname_handle) GlobalFree16( pModule->nrname_handle );
    if (pModule->dlls_to_init) GlobalFree16( pModule->dlls_to_init );
    GlobalFree16( hModule );

      /* Remove module from cache */

    if (hCachedModule == hModule) hCachedModule = 0;

    return TRUE;
}


/**********************************************************************
 *	    MODULE_Load
 *
 * Implementation of LoadModule().
 *
 * cmd_line must contain the whole command-line, including argv[0] (and
 * without a preceding length byte).
 * If cmd_line is NULL, the module is loaded as a library even if it is a .exe
 */
HINSTANCE16 MODULE_Load( LPCSTR name, UINT16 uFlags,
                         LPCSTR cmd_line, LPCSTR env, UINT32 show_cmd )
{
    HMODULE32 hModule;
    HINSTANCE16 hInstance, hPrevInstance;
    NE_MODULE *pModule;
    OFSTRUCT ofs;
    HFILE32 hFile;

    if (__winelib)
    {
        lstrcpyn32A( ofs.szPathName, name, sizeof(ofs.szPathName) );
        if ((hModule = MODULE_CreateDummyModule( &ofs )) < 32) return hModule;
        pModule = (NE_MODULE *)GlobalLock16( hModule );
        hInstance = MODULE_CreateInstance( hModule, &hPrevInstance,
                                           (cmd_line == NULL) );
    }
    else
    {
        hModule = MODULE_FindModule16( name );

        if (!hModule)  /* We have to load the module */
        {
            /* Try to load the built-in first if not disabled */
            if ((hModule = fnBUILTIN_LoadModule( name, FALSE )))
                return MODULE_HANDLEtoHMODULE16( hModule );
            
            if ((hFile = OpenFile32( name, &ofs, OF_READ )) == HFILE_ERROR32)
            {
                /* Now try the built-in even if disabled */
                if ((hModule = fnBUILTIN_LoadModule( name, TRUE )))
                {
                    WARN(module, "Could not load Windows DLL '%s', using built-in module.\n", name );
                    return MODULE_HANDLEtoHMODULE16( hModule );
                }
                return 2;  /* File not found */
            }

            /* Create the module structure */

            hModule = NE_LoadModule( hFile, &ofs, uFlags, cmd_line,
                                     env, show_cmd );
            if (hModule < 32)
            {
                if ((hModule == 21) && cmd_line)
                    hModule = PE_LoadModule( hFile, &ofs, cmd_line,
                                             env, show_cmd );
            }

            if (hModule < 32)
                fprintf( stderr, "LoadModule: can't load '%s', error=%d\n",
                         name, hModule );
            _lclose32( hFile );
            return hModule;
        }
        else /* module is already loaded, just create a new data segment if it's a task */
        {
            pModule = MODULE_GetPtr32( hModule );
            hInstance = MODULE_CreateInstance( hModule, &hPrevInstance,
                                               (cmd_line == NULL) );
            if (hInstance != hPrevInstance)  /* not a library */
                NE_LoadSegment( pModule, pModule->dgroup );
            pModule->count++;
        }
    } /* !winelib */

    /* Create a task for this instance */

    if (cmd_line && !(pModule->flags & NE_FFLAGS_LIBMODULE))
    {
        PDB32 *pdb;

	pModule->flags |= NE_FFLAGS_GUI;

        pdb = PROCESS_Create( pModule, cmd_line, env, hInstance,
                              hPrevInstance, show_cmd );
        if (pdb && (GetNumTasks() > 1)) Yield16();
    }

    return hInstance;
}


/**********************************************************************
 *	    LoadModule16    (KERNEL.45)
 */
HINSTANCE16 LoadModule16( LPCSTR name, LPVOID paramBlock )
{
    LOADPARAMS *params = (LOADPARAMS *)paramBlock;
    LPSTR cmd_line = (LPSTR)PTR_SEG_TO_LIN( params->cmdLine );
    LPSTR new_cmd_line;
    UINT16 show_cmd = 0;
    LPCVOID env = NULL;
    HINSTANCE16 hInstance;

    if (!paramBlock || (paramBlock == (LPVOID)-1))
        return LoadLibrary16( name );

    params = (LOADPARAMS *)paramBlock;
    cmd_line = (LPSTR)PTR_SEG_TO_LIN( params->cmdLine );
    /* PowerPoint passes NULL as showCmd */
    if (params->showCmd)
        show_cmd = *((UINT16 *)PTR_SEG_TO_LIN(params->showCmd)+1);

    if (!cmd_line) cmd_line = "";
    else if (*cmd_line) cmd_line++;  /* skip the length byte */

    if (!(new_cmd_line = HeapAlloc( GetProcessHeap(), 0,
                                    strlen(cmd_line) + strlen(name) + 2 )))
        return 0;
    strcpy( new_cmd_line, name );
    strcat( new_cmd_line, " " );
    strcat( new_cmd_line, cmd_line );

    if (params->hEnvironment) env = GlobalLock16( params->hEnvironment );
    hInstance = MODULE_Load( name, 0, new_cmd_line, env, show_cmd );
    if (params->hEnvironment) GlobalUnlock16( params->hEnvironment );
    HeapFree( GetProcessHeap(), 0, new_cmd_line );
    return hInstance;
}

/**********************************************************************
 *	    LoadModule32    (KERNEL32.499)
 *
 * FIXME
 *
 *  This should get implemented via CreateProcess -- MODULE_Load
 *  is resolutely 16-bit.
 */
DWORD LoadModule32( LPCSTR name, LPVOID paramBlock ) 
{
    LOADPARAMS32 *params = (LOADPARAMS32 *)paramBlock;
#if 0
  STARTUPINFO st;
  PROCESSINFORMATION pi;
  st.cb = sizeof(STARTUPINFO);
  st.wShowWindow = p->lpCmdShow[2] ; WRONG

  BOOL32 ret = CreateProcess32A( name, p->lpCmdLine, 
				 NULL, NULL, FALSE, 0, p->lpEnvAddress,
				 NULL, &st, &pi);
  if (!ret) {
    /*    handle errors appropriately */
  }
  CloseHandle32(pi.hProcess);
  CloseHandle32(pi.hThread); 

#else
  return MODULE_Load( name, 0, params->lpCmdLine, params->lpEnvAddress, 
                        *((UINT16 *)params->lpCmdShow + 1) );
#endif
}


/**********************************************************************
 *	    FreeModule16    (KERNEL.46)
 */
BOOL16 WINAPI FreeModule16( HMODULE16 hModule )
{
    NE_MODULE *pModule;

    if (!(pModule = MODULE_GetPtr16( hModule ))) return FALSE;
    TRACE(module, "%s count %d\n", 
		    MODULE_GetModuleName(hModule), pModule->count );

    return MODULE_FreeModule( hModule, GlobalLock16(GetCurrentTask()) );
}


/**********************************************************************
 *	    GetModuleHandle16    (KERNEL.47)
 */
HMODULE16 WINAPI WIN16_GetModuleHandle( SEGPTR name )
{
    if (HIWORD(name) == 0) return GetExePtr( (HINSTANCE16)name );
    return MODULE_FindModule16( PTR_SEG_TO_LIN(name) );
}

HMODULE16 WINAPI GetModuleHandle16( LPCSTR name )
{
    return MODULE_FindModule16( name );
}

/***********************************************************************
 *              GetModuleHandle         (KERNEL32.237)
 */
HMODULE32 WINAPI GetModuleHandle32A(LPCSTR module)
{

    TRACE(win32, "%s\n", module ? module : "NULL");
    if (module == NULL)
    	return PROCESS_Current()->exe_modref->module;
    else
	return MODULE_FindModule32(PROCESS_Current(),module);
}

HMODULE32 WINAPI GetModuleHandle32W(LPCWSTR module)
{
    HMODULE32 hModule;
    LPSTR modulea = HEAP_strdupWtoA( GetProcessHeap(), 0, module );
    hModule = GetModuleHandle32A( modulea );
    HeapFree( GetProcessHeap(), 0, modulea );
    return hModule;
}


/**********************************************************************
 *	    GetModuleUsage    (KERNEL.48)
 */
INT16 WINAPI GetModuleUsage( HINSTANCE16 hModule )
{
    NE_MODULE *pModule;

    if (!(pModule = MODULE_GetPtr16( hModule ))) return 0;
    TRACE(module, "(%04x): returning %d\n",
                    hModule, pModule->count );
    return pModule->count;
}


/**********************************************************************
 *	    GetModuleFileName16    (KERNEL.49)
 */
INT16 WINAPI GetModuleFileName16( HINSTANCE16 hModule, LPSTR lpFileName,
                                  INT16 nSize )
{
    NE_MODULE *pModule;

    if (!hModule) hModule = GetCurrentTask();
    if (!(pModule = MODULE_GetPtr16( hModule ))) return 0;
    lstrcpyn32A( lpFileName, NE_MODULE_NAME(pModule), nSize );
    TRACE(module, "%s\n", lpFileName );
    return strlen(lpFileName);
}


/***********************************************************************
 *              GetModuleFileName32A      (KERNEL32.235)
 * FIXME FIXME
 */
DWORD WINAPI GetModuleFileName32A( HMODULE32 hModule, LPSTR lpFileName,
                                   DWORD size )
{                   
    NE_MODULE *pModule;
           
    if (!hModule)
    {
        TDB *pTask = (TDB *)GlobalLock16( GetCurrentTask() );
        hModule = pTask->hInstance;
    }
    if (!(pModule = MODULE_GetPtr32( hModule ))) return 0;
    lstrcpyn32A( lpFileName, NE_MODULE_NAME(pModule), size );
    TRACE(module, "%s\n", lpFileName );
    return strlen(lpFileName);
}                   
 

/***********************************************************************
 *              GetModuleFileName32W      (KERNEL32.236)
 */
DWORD WINAPI GetModuleFileName32W( HMODULE32 hModule, LPWSTR lpFileName,
                                   DWORD size )
{
    LPSTR fnA = (char*)HeapAlloc( GetProcessHeap(), 0, size );
    DWORD res = GetModuleFileName32A( hModule, fnA, size );
    lstrcpynAtoW( lpFileName, fnA, size );
    HeapFree( GetProcessHeap(), 0, fnA );
    return res;
}


/**********************************************************************
 *	    GetModuleName    (KERNEL.27)
 */
BOOL16 WINAPI GetModuleName( HINSTANCE16 hinst, LPSTR buf, INT16 nSize )
{
    LPSTR name = MODULE_GetModuleName(hinst);

    if (!name) return FALSE;
    lstrcpyn32A( buf, name, nSize );
    return TRUE;
}


/***********************************************************************
 *           LoadLibraryEx32W   (KERNEL.513)
 * FIXME
 */
HMODULE32 WINAPI LoadLibraryEx32W16( LPCSTR libname, HANDLE16 hf,
                                       DWORD flags )
{
    TRACE(module,"(%s,%d,%08lx)\n",libname,hf,flags);
    return LoadLibraryEx32A(libname, hf,flags);
}

/***********************************************************************
 *           LoadLibraryEx32A   (KERNEL32)
 */
HMODULE32 WINAPI LoadLibraryEx32A(LPCSTR libname,HFILE32 hfile,DWORD flags)
{
    HMODULE32 hmod;
    
    hmod = PE_LoadLibraryEx32A(libname,PROCESS_Current(),hfile,flags);
    if (hmod < 32) {
	char buffer[256];

	strcpy( buffer, libname );
	strcat( buffer, ".dll" );
	hmod = PE_LoadLibraryEx32A(buffer,PROCESS_Current(),hfile,flags);
    }
    /* initialize all DLLs, which haven't been initialized yet. */
    if (hmod >= 32)
        PE_InitializeDLLs( PROCESS_Current(), DLL_PROCESS_ATTACH, NULL);
    return hmod;
}

/***********************************************************************
 *           LoadLibraryA         (KERNEL32)
 */
HMODULE32 WINAPI LoadLibrary32A(LPCSTR libname) {
	return LoadLibraryEx32A(libname,0,0);
}

/***********************************************************************
 *           LoadLibraryW         (KERNEL32)
 */
HMODULE32 WINAPI LoadLibrary32W(LPCWSTR libnameW)
{
    return LoadLibraryEx32W(libnameW,0,0);
}

/***********************************************************************
 *           LoadLibraryExW       (KERNEL32)
 */
HMODULE32 WINAPI LoadLibraryEx32W(LPCWSTR libnameW,HFILE32 hfile,DWORD flags)
{
    LPSTR libnameA = HEAP_strdupWtoA( GetProcessHeap(), 0, libnameW );
    HMODULE32 ret = LoadLibraryEx32A( libnameA , hfile, flags );

    HeapFree( GetProcessHeap(), 0, libnameA );
    return ret;
}

/***********************************************************************
 *           FreeLibrary
 */
BOOL32 WINAPI FreeLibrary32(HINSTANCE32 hLibModule)
{
    TRACE(module,"hLibModule=%08x\n", hLibModule);
    return MODULE_FreeModule(hLibModule,GlobalLock16(GetCurrentTask()) );
}


/***********************************************************************
 *           LoadLibrary   (KERNEL.95)
 */
HINSTANCE16 WINAPI LoadLibrary16( LPCSTR libname )
{
    HINSTANCE16 handle;

    TRACE(module, "(%08x) %s\n", (int)libname, libname);

    handle = MODULE_Load( libname, 0, NULL, NULL, 0 );
    if (handle == (HINSTANCE16)2)  /* file not found */
    {
        char buffer[256];
        lstrcpyn32A( buffer, libname, 252 );
        strcat( buffer, ".dll" );
        handle = MODULE_Load( buffer, 0, NULL, NULL, 0 );
    }
    return handle;
}


/***********************************************************************
 *           PrivateLoadLibrary       (KERNEL32)
 *
 * FIXME: rough guesswork, don't know what "Private" means
 */
HINSTANCE32 WINAPI PrivateLoadLibrary(LPCSTR libname)
{
        return (HINSTANCE32)LoadLibrary16(libname);
}


/***********************************************************************
 *           FreeLibrary16   (KERNEL.96)
 */
void WINAPI FreeLibrary16( HINSTANCE16 handle )
{
    TRACE(module,"%04x\n", handle );
    FreeModule16( handle );
}


/***********************************************************************
 *           PrivateFreeLibrary       (KERNEL32)
 *
 * FIXME: rough guesswork, don't know what "Private" means
 */
void WINAPI PrivateFreeLibrary(HINSTANCE32 handle)
{
	FreeLibrary16((HINSTANCE16)handle);
}


/***********************************************************************
 *           WinExec16   (KERNEL.166)
 */
HINSTANCE16 WINAPI WinExec16( LPCSTR lpCmdLine, UINT16 nCmdShow )
{
    return WinExec32( lpCmdLine, nCmdShow );
}


/***********************************************************************
 *           WinExec32   (KERNEL32.566)
 */
HINSTANCE32 WINAPI WinExec32( LPCSTR lpCmdLine, UINT32 nCmdShow )
{
    HINSTANCE32 handle = 2;
    char *p, filename[256];
    static int use_load_module = 1;
    int  spacelimit = 0, exhausted = 0;

    if (!lpCmdLine)
        return 2;  /* File not found */

    /* Keep trying to load a file by trying different filenames; e.g.,
       for the cmdline "abcd efg hij", try "abcd" with args "efg hij",
       then "abcd efg" with arg "hij", and finally "abcd efg hij" with
       no args */

    while(!exhausted && handle == 2) {
	int spacecount = 0;

	/* Build the filename and command-line */

	lstrcpyn32A(filename, lpCmdLine,
		    sizeof(filename) - 4 /* for extension */);

	/* Keep grabbing characters until end-of-string, tab, or until the
	   number of spaces is greater than the spacelimit */

	for (p = filename; ; p++) {
	    if(*p == ' ') {
		++spacecount;
		if(spacecount > spacelimit) {
		    ++spacelimit;
		    break;
		}
	    }

	    if(*p == '\0' || *p == '\t') {
		exhausted = 1;
		break;
	    }
	}

	*p = '\0';

	/* Now load the executable file */

	if (use_load_module)
	{
	    /* Winelib: Use LoadModule() only for the program itself */
	    if (__winelib) use_load_module = 0;
            handle = MODULE_Load( filename, 0, lpCmdLine, NULL, nCmdShow );
	    if (handle == 2)  /* file not found */
	    {
		/* Check that the original file name did not have a suffix */
		p = strrchr(filename, '.');
		/* if there is a '.', check if either \ OR / follow */
		if (!p || strchr(p, '/') || strchr(p, '\\'))
		{
		    p = filename + strlen(filename);
		    strcpy( p, ".exe" );
                    handle = MODULE_Load( filename, 0, lpCmdLine,
                                          NULL, nCmdShow );
                    *p = '\0';  /* Remove extension */
		}
	    }
	}
	else
	    handle = 2; /* file not found */

	if (handle < 32)
	{
	    /* Try to start it as a unix program */
	    if (!fork())
	    {
		/* Child process */
		DOS_FULL_NAME full_name;
		const char *unixfilename = NULL;
		const char *argv[256], **argptr;
		int iconic = (nCmdShow == SW_SHOWMINIMIZED ||
			      nCmdShow == SW_SHOWMINNOACTIVE);

		THREAD_InitDone = FALSE; /* we didn't init this process */
		/* get unixfilename */
		if (strchr(filename, '/') ||
		    strchr(filename, ':') ||
		    strchr(filename, '\\'))
		{
		    if (DOSFS_GetFullName( filename, TRUE, &full_name ))
			unixfilename = full_name.long_name;
		}
		else unixfilename = filename;

		if (unixfilename)
		{
		    /* build argv */
		    argptr = argv;
		    if (iconic) *argptr++ = "-iconic";
		    *argptr++ = unixfilename;
		    p = strdup(lpCmdLine);
		    while (1)
		    {
			while (*p && (*p == ' ' || *p == '\t')) *p++ = '\0';
			if (!*p) break;
			*argptr++ = p;
			while (*p && *p != ' ' && *p != '\t') p++;
		    }
		    *argptr++ = 0;

		    /* Execute */
		    execvp(argv[0], (char**)argv);
		}

		/* Failed ! */

		if (__winelib)
		{
		    /* build argv */
		    argptr = argv;
		    *argptr++ = "wine";
		    if (iconic) *argptr++ = "-iconic";
		    *argptr++ = lpCmdLine;
		    *argptr++ = 0;

		    /* Execute */
		    execvp(argv[0] , (char**)argv);

		    /* Failed ! */
		    MSG("WinExec: can't exec 'wine %s'\n",
			    lpCmdLine);
		}
		exit(1);
	    }
	}
    } /* while (!exhausted && handle < 32) */

    return handle;
}


/***********************************************************************
 *           WIN32_GetProcAddress16   (KERNEL32.36)
 * Get procaddress in 16bit module from win32... (kernel32 undoc. ordinal func)
 */
FARPROC16 WINAPI WIN32_GetProcAddress16( HMODULE32 hModule, LPSTR name )
{
    WORD	ordinal;
    FARPROC16	ret;

    if (!hModule) {
    	WARN(module,"hModule may not be 0!\n");
	return (FARPROC16)0;
    }
    hModule = MODULE_HANDLEtoHMODULE16(hModule);
    if (HIWORD(name)) {
        ordinal = NE_GetOrdinal( hModule, name );
        TRACE(module, "%04x '%s'\n",
                        hModule, name );
    } else {
        ordinal = LOWORD(name);
        TRACE(module, "%04x %04x\n",
                        hModule, ordinal );
    }
    if (!ordinal) return (FARPROC16)0;
    ret = NE_GetEntryPoint( hModule, ordinal );
    TRACE(module,"returning %08x\n",(UINT32)ret);
    return ret;
}

/***********************************************************************
 *           GetProcAddress16   (KERNEL.50)
 */
FARPROC16 WINAPI GetProcAddress16( HMODULE16 hModule, SEGPTR name )
{
    WORD ordinal;
    FARPROC16 ret;

    if (!hModule) hModule = GetCurrentTask();
    hModule = GetExePtr( hModule );

    if (HIWORD(name) != 0)
    {
        ordinal = NE_GetOrdinal( hModule, (LPSTR)PTR_SEG_TO_LIN(name) );
        TRACE(module, "%04x '%s'\n",
                        hModule, (LPSTR)PTR_SEG_TO_LIN(name) );
    }
    else
    {
        ordinal = LOWORD(name);
        TRACE(module, "%04x %04x\n",
                        hModule, ordinal );
    }
    if (!ordinal) return (FARPROC16)0;

    ret = NE_GetEntryPoint( hModule, ordinal );

    TRACE(module, "returning %08x\n", (UINT32)ret );
    return ret;
}


/***********************************************************************
 *           GetProcAddress32   		(KERNEL32.257)
 */
FARPROC32 WINAPI GetProcAddress32( HMODULE32 hModule, LPCSTR function )
{
    return MODULE_GetProcAddress32( PROCESS_Current(), hModule, function );
}


/***********************************************************************
 *           MODULE_GetProcAddress32   		(internal)
 */
FARPROC32 MODULE_GetProcAddress32( 
	PDB32 *process,		/* [in] process context */
	HMODULE32 hModule, 	/* [in] current module handle */
	LPCSTR function )	/* [in] function to be looked up */
{
    WINE_MODREF	*wm = MODULE32_LookupHMODULE(process,hModule);

    if (HIWORD(function))
	TRACE(win32,"(%08lx,%s)\n",(DWORD)hModule,function);
    else
	TRACE(win32,"(%08lx,%p)\n",(DWORD)hModule,function);
    if (!wm)
        return (FARPROC32)0;
    switch (wm->type)
    {
    case MODULE32_PE:
    	return PE_FindExportedFunction( process, wm, function);
    default:
    	ERR(module,"wine_modref type %d not handled.\n",wm->type);
    	return (FARPROC32)0;
    }
}


/***********************************************************************
 *           RtlImageNtHeaders   (NTDLL)
 */
LPIMAGE_NT_HEADERS WINAPI RtlImageNtHeader(HMODULE32 hModule)
{
    /* basically:
     * return  hModule+(((IMAGE_DOS_HEADER*)hModule)->e_lfanew); 
     * but we could get HMODULE16 or the like (think builtin modules)
     */

    WINE_MODREF	*wm = MODULE32_LookupHMODULE( PROCESS_Current(), hModule );
    if (!wm || (wm->type != MODULE32_PE)) return (LPIMAGE_NT_HEADERS)0;
    return PE_HEADER(wm->module);
}


/**********************************************************************
 *	    GetExpWinVer    (KERNEL.167)
 */
WORD WINAPI GetExpWinVer( HMODULE16 hModule )
{
    NE_MODULE *pModule = MODULE_GetPtr16( hModule );
    return pModule ? pModule->expected_version : 0;
}


/**********************************************************************
 *	    IsSharedSelector    (KERNEL.345)
 */
BOOL16 WINAPI IsSharedSelector( HANDLE16 selector )
{
    /* Check whether the selector belongs to a DLL */
    NE_MODULE *pModule = MODULE_GetPtr16( selector );
    if (!pModule) return FALSE;
    return (pModule->flags & NE_FFLAGS_LIBMODULE) != 0;
}


/**********************************************************************
 *	    ModuleFirst    (TOOLHELP.59)
 */
BOOL16 WINAPI ModuleFirst( MODULEENTRY *lpme )
{
    lpme->wNext = hFirstModule;
    return ModuleNext( lpme );
}


/**********************************************************************
 *	    ModuleNext    (TOOLHELP.60)
 */
BOOL16 WINAPI ModuleNext( MODULEENTRY *lpme )
{
    NE_MODULE *pModule;
    char *name;

    if (!lpme->wNext) return FALSE;
    if (!(pModule = MODULE_GetPtr16( lpme->wNext ))) return FALSE;
    name = (char *)pModule + pModule->name_table;
    memcpy( lpme->szModule, name + 1, *name );
    lpme->szModule[(BYTE)*name] = '\0';
    lpme->hModule = lpme->wNext;
    lpme->wcUsage = pModule->count;
    strncpy( lpme->szExePath, NE_MODULE_NAME(pModule), MAX_PATH );
    lpme->szExePath[MAX_PATH] = '\0';
    lpme->wNext = pModule->next;
    return TRUE;
}


/**********************************************************************
 *	    ModuleFindName    (TOOLHELP.61)
 */
BOOL16 WINAPI ModuleFindName( MODULEENTRY *lpme, LPCSTR name )
{
    lpme->wNext = GetModuleHandle16( name );
    return ModuleNext( lpme );
}


/**********************************************************************
 *	    ModuleFindHandle    (TOOLHELP.62)
 */
BOOL16 WINAPI ModuleFindHandle( MODULEENTRY *lpme, HMODULE16 hModule )
{
    hModule = GetExePtr( hModule );
    lpme->wNext = hModule;
    return ModuleNext( lpme );
}
