/*
 * Modules
 *
 * Copyright 1995 Alexandre Julliard
 */

#include <assert.h>
#include <fcntl.h>
#include <stdlib.h>
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
#include "debug.h"
#include "callback.h"

extern BOOL32 THREAD_InitDone;


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
        if (!strcmp(name,"DefResourceHandler"))
            return (FARPROC16)NE_DefResourceHandler;
        if (!strcmp(name,"LoadDIBIconHandler"))
            return (FARPROC16)LoadDIBIconHandler;
        if (!strcmp(name,"LoadDIBCursorHandler"))
            return (FARPROC16)LoadDIBCursorHandler;
        FIXME(module,"No mapping for %s(), add one in library/miscstubs.c\n",name);
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
	if (!strcasecmp( filename, xmodname)) {
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
 *	    MODULE_Load
 *
 * Implementation of LoadModule().
 *
 * cmd_line must contain the whole command-line, including argv[0] (and
 * without a preceding length byte).
 * If cmd_line is NULL, the module is loaded as a library even if it is a .exe
 */
HINSTANCE16 MODULE_Load( LPCSTR name, BOOL32 implicit,
                         LPCSTR cmd_line, LPCSTR env, UINT32 show_cmd )
{
    HMODULE16 hModule;
    HINSTANCE16 hInstance, hPrevInstance;
    NE_MODULE *pModule;

    if (__winelib)
    {
        OFSTRUCT ofs;
        lstrcpyn32A( ofs.szPathName, name, sizeof(ofs.szPathName) );
        if ((hModule = MODULE_CreateDummyModule( &ofs )) < 32) return hModule;
        pModule = (NE_MODULE *)GlobalLock16( hModule );
        hInstance = NE_CreateInstance( pModule, &hPrevInstance,
                                       (cmd_line == NULL) );
    }
    else
    {
        hInstance = NE_LoadModule( name, &hPrevInstance, implicit,
                                 (cmd_line == NULL) );
        if ((hInstance == 21) && cmd_line)
            return PE_LoadModule( name, cmd_line, env, show_cmd );
    }

    /* Create a task for this instance */

    if (hInstance < 32) return hInstance;
    pModule = NE_GetPtr( hInstance );
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
    LOADPARAMS *params;
    LPSTR cmd_line, new_cmd_line;
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
    hInstance = MODULE_Load( name, FALSE, new_cmd_line, env, show_cmd );
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
  return MODULE_Load( name, FALSE, params->lpCmdLine, params->lpEnvAddress, 
                        *((UINT16 *)params->lpCmdShow + 1) );
#endif
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


/***********************************************************************
 *              GetModuleFileName32A      (KERNEL32.235)
 */
DWORD WINAPI GetModuleFileName32A( 
	HMODULE32 hModule,	/* [in] module handle (32bit) */
	LPSTR lpFileName,	/* [out] filenamebuffer */
        DWORD size		/* [in] size of filenamebuffer */
) {                   
    WINE_MODREF *wm = MODULE32_LookupHMODULE(PROCESS_Current(),hModule);

    if (!wm) /* can happen on start up or the like */
    	return 0;

    /* FIXME: we should probably get a real long name, but wm->longname
     * is currently a UNIX filename!
     */
    lstrcpyn32A( lpFileName, wm->shortname, size );
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
    FIXME(module,"(%08x): stub\n", hLibModule);
    return TRUE;  /* FIXME */
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
            handle = MODULE_Load( filename, FALSE, lpCmdLine, NULL, nCmdShow );
	    if (handle == 2)  /* file not found */
	    {
		/* Check that the original file name did not have a suffix */
		p = strrchr(filename, '.');
		/* if there is a '.', check if either \ OR / follow */
		if (!p || strchr(p, '/') || strchr(p, '\\'))
		{
		    p = filename + strlen(filename);
		    strcpy( p, ".exe" );
                    handle = MODULE_Load( filename, FALSE, lpCmdLine,
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
FARPROC16 WINAPI WIN32_GetProcAddress16( HMODULE32 hModule, LPCSTR name )
{
    WORD	ordinal;
    FARPROC16	ret;

    if (!hModule) {
    	WARN(module,"hModule may not be 0!\n");
	return (FARPROC16)0;
    }
    if (HIWORD(hModule))
    {
    	WARN( module, "hModule is Win32 handle (%08x)\n", hModule );
	return (FARPROC16)0;
    }
    hModule = GetExePtr( hModule );
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
