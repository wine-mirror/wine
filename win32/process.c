/*
 * Win32 kernel functions
 *
 * Copyright 1995 Martin von Loewis
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/times.h>
#include "windows.h"
#include "winerror.h"
#include "heap.h"
#include "thread.h"
#include "handle32.h"
#include "pe_image.h"
#include "file.h"
#include "stddebug.h"
#include "debug.h"

typedef struct {
	K32OBJ	header;
	DWORD	maxcount;
	DWORD	count;
	DWORD	initial;
} K32SEMAPHORE;

typedef struct {
	K32OBJ	header;
	THDB	*owning_thread; /* we are locked by this thread */
} K32MUTEX;

typedef struct {
	K32OBJ	header;
} K32EVENT;

/***********************************************************************
 *           CreateMutexA    (KERNEL32.52)
 */
HANDLE32 CreateMutex32A(SECURITY_ATTRIBUTES *sa,BOOL32 on,LPCSTR name)
{
	K32MUTEX	*mut;
	HANDLE32	handle;
	K32OBJ 		*obj = K32OBJ_FindName( name );

	if (obj) {
		if (obj->type == K32OBJ_MUTEX) {
			SetLastError( ERROR_ALREADY_EXISTS );
			return PROCESS_AllocHandle( obj,0 );
		}
		SetLastError( ERROR_DUP_NAME );
		return 0;
	}
	mut = (K32MUTEX*)HeapAlloc(GetProcessHeap(),0,sizeof(K32MUTEX));
	mut->header.type = K32OBJ_MUTEX;
	mut->header.refcount  = 1;
	mut->owning_thread = NULL;
	if (name)
		K32OBJ_AddName(&(mut->header),name);
	handle = PROCESS_AllocHandle(&(mut->header),0);
	if (handle != INVALID_HANDLE_VALUE32) {
		if (on)
			mut->owning_thread = (THDB *)GetCurrentThreadId();
		return handle;
	}
	K32OBJ_DecCount(&(mut->header)); /* also frees name */
	HeapFree(GetProcessHeap(),0,mut);
	return 0;
}

/***********************************************************************
 *           CreateMutexW    (KERNEL32.53)
 */
HANDLE32 CreateMutex32W(SECURITY_ATTRIBUTES *sa, BOOL32 on, LPCWSTR a)
{
	LPSTR		name = a?HEAP_strdupWtoA(GetProcessHeap(),0,a):NULL;
	HANDLE32	ret;

	ret = CreateMutex32A(sa,on,name);
	if (name) HeapFree(GetProcessHeap(),0,name);
	return ret;
}

/***********************************************************************
 *           CreateSemaphoreA    (KERNEL32.60)
 */
HANDLE32 CreateSemaphore32A(
	LPSECURITY_ATTRIBUTES sa,LONG initial,LONG max,LPCSTR name
) {
	K32SEMAPHORE	*sem;
	HANDLE32	handle;
	K32OBJ 		*obj = K32OBJ_FindName( name );

	if (obj) {
		if (obj->type == K32OBJ_SEMAPHORE) {
			SetLastError( ERROR_ALREADY_EXISTS );
			return PROCESS_AllocHandle( obj,0 );
		}
		SetLastError( ERROR_DUP_NAME );
		return 0;
	}
	sem = (K32SEMAPHORE*)HeapAlloc(GetProcessHeap(),0,sizeof(K32SEMAPHORE));
	sem->header.type = K32OBJ_SEMAPHORE;
	sem->header.refcount  = 1;

	sem->maxcount 	= max;
	sem->count	= initial;
	sem->initial	= initial;
	if (name)
		K32OBJ_AddName(&(sem->header),name);
	handle = PROCESS_AllocHandle(&(sem->header),0);
	if (handle != INVALID_HANDLE_VALUE32)
		return handle;
	K32OBJ_DecCount(&(sem->header)); /* also frees name */
	HeapFree(GetProcessHeap(),0,sem);
	return 0;
}

/***********************************************************************
 *           CreateSemaphoreW    (KERNEL32.61)
 */
HANDLE32 CreateSemaphore32W(SECURITY_ATTRIBUTES *sa,LONG initial,LONG max,LPCWSTR a)
{
	LPSTR		name =a?HEAP_strdupWtoA(GetProcessHeap(),0,a):NULL;
	HANDLE32	ret;

	ret = CreateSemaphore32A(sa,initial,max,name);
	if (a) HeapFree(GetProcessHeap(),0,name);
	return ret;
}


/***********************************************************************
 *           OpenSemaphoreA    (KERNEL32.403)
 */
HANDLE32 OpenSemaphore32A(DWORD desired,BOOL32 inherit,LPCSTR name)
{
	K32OBJ 		*obj = K32OBJ_FindName( name );

	if (obj) {
		if (obj->type == K32OBJ_SEMAPHORE)
			return PROCESS_AllocHandle( obj,0 );
		SetLastError( ERROR_DUP_NAME );
		return 0;
	}
	return 0;
}

/***********************************************************************
 *           OpenSemaphoreA    (KERNEL32.404)
 */
HANDLE32 OpenSemaphore32W(DWORD desired,BOOL32 inherit,LPCWSTR name)
{
	LPSTR	nameA = name?HEAP_strdupWtoA(GetProcessHeap(),0,name):NULL;
	HANDLE32	ret = OpenSemaphore32A(desired,inherit,nameA);

	if (name) HeapFree(GetProcessHeap(),0,nameA);
	return ret;
}

/***********************************************************************
 *           ReleaseSemaphore    (KERNEL32.403)
 */
BOOL32 ReleaseSemaphore(HANDLE32 hSemaphore,LONG lReleaseCount,LPLONG lpPreviousCount) {
	K32SEMAPHORE	*sem;

	sem = (K32SEMAPHORE*)PROCESS_GetObjPtr(hSemaphore,K32OBJ_SEMAPHORE);
	if (!sem)
		return FALSE;
	if (lpPreviousCount) *lpPreviousCount = sem->count;
	sem->count += lReleaseCount;
	if (sem->count>sem->maxcount) {
		fprintf(stderr,"ReleaseSemaphore(%d,%ld,.), released more then possible??\n",hSemaphore,lReleaseCount);
		sem->count = sem->maxcount;
	}

	/* FIXME: wake up all threads blocked on that semaphore */

	K32OBJ_DecCount(&(sem->header));
	return TRUE;
}

/***********************************************************************
 *           OpenMutexA    (KERNEL32.399)
 */
HANDLE32 OpenMutex32A(DWORD desiredaccess, BOOL32 inherithandle, LPCSTR name)
{
	K32OBJ 		*obj = K32OBJ_FindName( name );

	if (obj) {
		if (obj->type == K32OBJ_MUTEX)
			return PROCESS_AllocHandle( obj,0 );
		SetLastError( ERROR_DUP_NAME );
		return 0;
	}
	return 0;
}

/***********************************************************************
 *           OpenMutexW    (KERNEL32.400)
 */
HANDLE32 OpenMutex32W(DWORD desiredaccess, BOOL32 inherithandle, LPCWSTR name)
{
	LPSTR		nameA=name?HEAP_strdupWtoA(GetProcessHeap(),0,name):NULL;
	HANDLE32	ret = OpenMutex32A(desiredaccess,inherithandle,nameA);

	if (name) HeapFree(GetProcessHeap(),0,nameA);
	return ret;
}

/***********************************************************************
 *           ReleaseMutex    (KERNEL32.435)
 */
BOOL32 ReleaseMutex (HANDLE32 h)
{
	K32MUTEX	*mut = (K32MUTEX*)PROCESS_GetObjPtr(h,K32OBJ_MUTEX);

	if (!mut)
		return 0;
	if (mut->owning_thread != (THDB *)GetCurrentThreadId()) {
		/* set error ... */
		K32OBJ_DecCount(&(mut->header));
		return 0;
	}
	mut->owning_thread = NULL;

	/* FIXME: wake up all threads blocked on this mutex */

	K32OBJ_DecCount(&(mut->header));
	return TRUE;
}

/***********************************************************************
 *           CreateEventA    (KERNEL32.43)
 */
HANDLE32 CreateEvent32A(SECURITY_ATTRIBUTES *sa,BOOL32 au,BOOL32 on,LPCSTR name)
{
	K32EVENT	*evt;
	HANDLE32	handle;
	K32OBJ 		*obj = K32OBJ_FindName( name );

	if (obj) {
		if (obj->type == K32OBJ_EVENT) {
			SetLastError( ERROR_ALREADY_EXISTS );
			return PROCESS_AllocHandle( obj,0 );
		}
		SetLastError( ERROR_DUP_NAME );
		return 0;
	}
	evt = (K32EVENT*)HeapAlloc(GetProcessHeap(),0,sizeof(K32EVENT));
	evt->header.type = K32OBJ_EVENT;
	evt->header.refcount  = 1;
	if (name)
		K32OBJ_AddName(&(evt->header),name);
	handle = PROCESS_AllocHandle(&(evt->header),0);
	if (handle != INVALID_HANDLE_VALUE32)
		return handle;
	K32OBJ_DecCount(&(evt->header)); /* also frees name */
	HeapFree(GetProcessHeap(),0,evt);
	return 0;
}

/***********************************************************************
 *           CreateEventW    (KERNEL32.43)
 */
HANDLE32 CreateEvent32W(SECURITY_ATTRIBUTES *sa, BOOL32 au, BOOL32 on,LPCWSTR name)
{
	LPSTR nameA=name?HEAP_strdupWtoA(GetProcessHeap(),0,name):NULL;
	HANDLE32 ret = CreateEvent32A(sa,au,on,nameA);

	if (name) HeapFree(GetProcessHeap(),0,nameA);
	return ret;
}

/***********************************************************************
 *           OpenEventA    (KERNEL32.394)
 */
HANDLE32 OpenEvent32A(DWORD desiredaccess,BOOL32 inherithandle,LPCSTR name) {
	K32OBJ 		*obj = K32OBJ_FindName( name );

	if (obj) {
		if (obj->type == K32OBJ_EVENT)
			return PROCESS_AllocHandle( obj,0 );
		SetLastError( ERROR_DUP_NAME );
		return 0;
	}
	return 0;
}

/***********************************************************************
 *           OpenEventW    (KERNEL32.395)
 */
HANDLE32 OpenEvent32W(DWORD desiredaccess,BOOL32 inherithandle,LPCWSTR name) {
	LPSTR	nameA = name?HEAP_strdupWtoA(GetProcessHeap(),0,name):NULL;
	HANDLE32	ret = OpenEvent32A(desiredaccess,inherithandle,nameA);

	if (name) HeapFree(GetProcessHeap(),0,nameA);
	return ret;
}

/***********************************************************************
 *           SetEvent    (KERNEL32.487)
 */
BOOL32 SetEvent (HANDLE32 h)
{
	fprintf(stderr,"SetEvent(%d) stub\n",h);
	return 0;
}
/***********************************************************************
 *           ResetEvent    (KERNEL32.439)
 */
BOOL32 ResetEvent (HANDLE32 h)
{
	fprintf(stderr,"ResetEvent(%d) stub\n",h);
	return 0;
}

/***********************************************************************
 *           WaitForSingleObject    (KERNEL32.561)
 */
DWORD WaitForSingleObject(HANDLE32 h, DWORD timeout)
{
	fprintf(stderr,"WaitForSingleObject(%d,%ld) stub\n",h,timeout);
	return 0;
}

/***********************************************************************
 *           WaitForSingleObject    (USER32.399)
 */
DWORD MsgWaitForMultipleObjects(
	DWORD nCount,HANDLE32 *pHandles,BOOL32 fWaitAll,DWORD dwMilliseconds,
	DWORD dwWakeMask
) {
	int	i;
	fprintf(stderr,"MsgWaitForMultipleObjects(%ld,[",nCount);
	for (i=0;i<nCount;i++)
		fprintf(stderr,"%ld,",(DWORD)pHandles[i]);
	fprintf(stderr,"],%d,%ld,0x%08lx)\n",fWaitAll,dwMilliseconds,dwWakeMask);
	return 0;
}
/***********************************************************************
 *           DuplicateHandle    (KERNEL32.78)
 */
BOOL32 DuplicateHandle(HANDLE32 a, HANDLE32 b, HANDLE32 c, HANDLE32 * d, DWORD e, BOOL32 f, DWORD g)
{
	fprintf(stderr,"DuplicateHandle(%d,%d,%d,%p,%ld,%d,%ld) stub\n",a,b,c,d,e,f,g);
	*d = b;
	return TRUE;
}


/***********************************************************************
 *           LoadLibraryA         (KERNEL32.365)
 * copied from LoadLibrary
 * This does not currently support built-in libraries
 */
HINSTANCE32 LoadLibrary32A(LPCSTR libname)
{
	HINSTANCE32 handle;
	dprintf_module( stddeb, "LoadLibrary: (%08x) %s\n", (int)libname, libname);
	handle = LoadModule16( libname, (LPVOID)-1 );
	if (handle == (HINSTANCE32) -1)
	{
		char buffer[256];
		strcpy( buffer, libname );
		strcat( buffer, ".dll" );
		handle = LoadModule16( buffer, (LPVOID)-1 );
	}
	/* Obtain module handle and call initialization function */
#ifndef WINELIB
	if (handle >= (HINSTANCE32)32) PE_InitializeDLLs( GetExePtr(handle), DLL_PROCESS_ATTACH, NULL);
#endif
	return handle;
}

/***********************************************************************
 *           LoadLibrary32W         (KERNEL32.368)
 */
HINSTANCE32 LoadLibrary32W(LPCWSTR libnameW)
{
    LPSTR libnameA = HEAP_strdupWtoA( GetProcessHeap(), 0, libnameW );
    HINSTANCE32 ret = LoadLibrary32A( libnameA );
    HeapFree( GetProcessHeap(), 0, libnameA );
    return ret;
}

/***********************************************************************
 *           FreeLibrary
 */
BOOL32 FreeLibrary32(HINSTANCE32 hLibModule)
{
	fprintf(stderr,"FreeLibrary: empty stub\n");
	return TRUE;
}

/**********************************************************************
 *          GetProcessAffinityMask
 */
BOOL32 GetProcessAffinityMask(HANDLE32 hProcess, LPDWORD lpProcessAffinityMask,
  LPDWORD lpSystemAffinityMask)
{
	dprintf_task(stddeb,"GetProcessAffinityMask(%x,%lx,%lx)\n",
		hProcess,(lpProcessAffinityMask?*lpProcessAffinityMask:0),
		(lpSystemAffinityMask?*lpSystemAffinityMask:0));
	/* It is definitely important for a process to know on what processor
	   it is running :-) */
	if(lpProcessAffinityMask)
		*lpProcessAffinityMask=1;
	if(lpSystemAffinityMask)
		*lpSystemAffinityMask=1;
	return TRUE;
}

/**********************************************************************
 *           SetThreadAffinityMask
 * Works now like the Windows95 (no MP support) version
 */
BOOL32 SetThreadAffinityMask(HANDLE32 hThread, DWORD dwThreadAffinityMask)
{
	THDB	*thdb = (THDB*)PROCESS_GetObjPtr(hThread,K32OBJ_THREAD);

	if (!thdb) 
		return FALSE;
	if (dwThreadAffinityMask!=1) {
		fprintf(stderr,"SetThreadAffinityMask(%d,%ld), only 1 processor supported.\n",(int)hThread,dwThreadAffinityMask);
		K32OBJ_DecCount((K32OBJ*)thdb);
		return FALSE;
	}
	K32OBJ_DecCount((K32OBJ*)thdb);
	return TRUE;
}

BOOL32
CreateProcess32A(
	LPCSTR appname,LPSTR cmdline,LPSECURITY_ATTRIBUTES processattributes,
        LPSECURITY_ATTRIBUTES threadattributes,BOOL32 inherithandles,
	DWORD creationflags,LPVOID env,LPCSTR curdir,
	LPSTARTUPINFO32A startupinfo,LPPROCESS_INFORMATION processinfo
) {
	fprintf(stderr,"CreateProcess(%s,%s,%p,%p,%d,%08lx,%p,%s,%p,%p)\n",
		appname,cmdline,processattributes,threadattributes,
		inherithandles,creationflags,env,curdir,startupinfo,processinfo
	);
	return TRUE;
}

BOOL32
ContinueDebugEvent(DWORD pid,DWORD tid,DWORD contstatus) {
	fprintf(stderr,"ContinueDebugEvent(%ld,%ld,%ld), stub\n",pid,tid,contstatus);
	return TRUE;
}

/*********************************************************************
 *	GetProcessTimes				[KERNEL32.262]
 *
 * FIXME: implement this better ...
 */
BOOL32
GetProcessTimes(
	HANDLE32 hprocess,LPFILETIME lpCreationTime,LPFILETIME lpExitTime,
	LPFILETIME lpKernelTime, LPFILETIME lpUserTime
) {
	struct tms tms;

	times(&tms);
	DOSFS_UnixTimeToFileTime(tms.tms_utime,lpUserTime,0);
	DOSFS_UnixTimeToFileTime(tms.tms_stime,lpKernelTime,0);
	return TRUE;
}

