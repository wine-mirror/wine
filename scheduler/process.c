/*
 * Win32 processes
 *
 * Copyright 1996 Alexandre Julliard
 */

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "process.h"
#include "file.h"
#include "heap.h"
#include "task.h"
#include "thread.h"
#include "winerror.h"

PDB32 *pCurrentProcess = NULL;

#define HTABLE_SIZE  0x30  /* Handle table initial size */
#define HTABLE_INC   0x10  /* Handle table increment */

#define BOOT_HTABLE_SIZE  5

static HANDLE_ENTRY boot_handles[BOOT_HTABLE_SIZE];

/***********************************************************************
 *           PROCESS_AllocHandleTable
 */
static HANDLE_TABLE *PROCESS_AllocHandleTable( PDB32 *process )
{
    HANDLE_TABLE *table = HeapAlloc( process->system_heap, HEAP_ZERO_MEMORY,
                                     sizeof(HANDLE_TABLE) +
                                     (HTABLE_SIZE-1) * sizeof(HANDLE_ENTRY) );
    if (!table) return NULL;
    table->count = HTABLE_SIZE;
    return table;
}


/***********************************************************************
 *           PROCESS_GrowHandleTable
 */
static BOOL32 PROCESS_GrowHandleTable( PDB32 *process )
{
    HANDLE_TABLE *table = process->handle_table;
    table = HeapReAlloc( process->system_heap, HEAP_ZERO_MEMORY, table,
                         sizeof(HANDLE_TABLE) +
                         (table->count+HTABLE_INC-1) * sizeof(HANDLE_ENTRY) );
    if (!table) return FALSE;
    table->count += HTABLE_INC;
    process->handle_table = table;
    return TRUE;
}


/***********************************************************************
 *           PROCESS_AllocBootHandle
 *
 * Allocate a handle from the boot table.
 */
static HANDLE32 PROCESS_AllocBootHandle( K32OBJ *ptr, DWORD flags )
{
    HANDLE32 h;
    for (h = 0; h < BOOT_HTABLE_SIZE; h++)
        if (!boot_handles[h].ptr) break;
    assert( h < BOOT_HTABLE_SIZE );
    K32OBJ_IncCount( ptr );
    boot_handles[h].flags = flags;
    boot_handles[h].ptr   = ptr;
    return h + 1;  /* Avoid handle 0 */
}


/***********************************************************************
 *           PROCESS_CloseBootHandle
 *
 * Close a handle from the boot table.
 */
static BOOL32 PROCESS_CloseBootHandle( HANDLE32 handle )
{
    HANDLE_ENTRY *entry = &boot_handles[handle - 1];
    assert( (handle > 0) && (handle <= BOOT_HTABLE_SIZE) );
    assert( entry->ptr );
    K32OBJ_DecCount( entry->ptr );
    entry->flags = 0;
    entry->ptr   = NULL;
    return TRUE;
}


/***********************************************************************
 *           PROCESS_GetBootObjPtr
 *
 * Get a handle ptr from the boot table.
 */
static K32OBJ *PROCESS_GetBootObjPtr( HANDLE32 handle, K32OBJ_TYPE type )
{
    K32OBJ *ptr;

    assert( (handle > 0) && (handle <= BOOT_HTABLE_SIZE) );
    ptr = boot_handles[handle - 1].ptr;
    assert (ptr && (ptr->type == type));
    K32OBJ_IncCount( ptr );
    return ptr;
}


/***********************************************************************
 *           PROCESS_SetBootObjPtr
 *
 * Set a handle ptr from the boot table.
 */
static BOOL32 PROCESS_SetBootObjPtr( HANDLE32 handle, K32OBJ *ptr, DWORD flags)
{
    K32OBJ *old_ptr;

    assert( (handle > 0) && (handle <= BOOT_HTABLE_SIZE) );
    K32OBJ_IncCount( ptr );
    if ((old_ptr = boot_handles[handle - 1].ptr)) K32OBJ_DecCount( old_ptr );
    boot_handles[handle - 1].flags = flags;
    boot_handles[handle - 1].ptr   = ptr;
    return TRUE;
}


/***********************************************************************
 *           PROCESS_AllocHandle
 *
 * Allocate a handle for a kernel object and increment its refcount.
 */
HANDLE32 PROCESS_AllocHandle( K32OBJ *ptr, DWORD flags )
{
    HANDLE32 h;
    HANDLE_ENTRY *entry;

    assert( ptr );
    if (!pCurrentProcess) return PROCESS_AllocBootHandle( ptr, flags );
    EnterCriticalSection( &pCurrentProcess->crit_section );
    K32OBJ_IncCount( ptr );
    entry = pCurrentProcess->handle_table->entries;
    for (h = 0; h < pCurrentProcess->handle_table->count; h++, entry++)
        if (!entry->ptr) break;
    if ((h < pCurrentProcess->handle_table->count) ||
        PROCESS_GrowHandleTable( pCurrentProcess ))
    {
        entry->flags = flags;
        entry->ptr   = ptr;
        LeaveCriticalSection( &pCurrentProcess->crit_section );
        return h + 1;  /* Avoid handle 0 */
    }
    LeaveCriticalSection( &pCurrentProcess->crit_section );
    SetLastError( ERROR_OUTOFMEMORY );
    K32OBJ_DecCount( ptr );
    return INVALID_HANDLE_VALUE32;
}


/***********************************************************************
 *           PROCESS_GetObjPtr
 *
 * Retrieve a pointer to a kernel object and increments its reference count.
 * The refcount must be decremented when the pointer is no longer used.
 */
K32OBJ *PROCESS_GetObjPtr( HANDLE32 handle, K32OBJ_TYPE type )
{
    K32OBJ *ptr = NULL;
    if (!pCurrentProcess) return PROCESS_GetBootObjPtr( handle, type );
    EnterCriticalSection( &pCurrentProcess->crit_section );

    if ((handle > 0) && (handle <= pCurrentProcess->handle_table->count))
        ptr = pCurrentProcess->handle_table->entries[handle - 1].ptr;
    else if (handle == 0x7fffffff) ptr = &pCurrentProcess->header;

    if (ptr && ((type == K32OBJ_UNKNOWN) || (ptr->type == type)))
        K32OBJ_IncCount( ptr );
    else ptr = NULL;

    LeaveCriticalSection( &pCurrentProcess->crit_section );
    if (!ptr) SetLastError( ERROR_INVALID_HANDLE );
    return ptr;
}


/***********************************************************************
 *           PROCESS_SetObjPtr
 *
 * Change the object pointer of a handle, and increment the refcount.
 * Use with caution!
 */
BOOL32 PROCESS_SetObjPtr( HANDLE32 handle, K32OBJ *ptr, DWORD flags )
{
    BOOL32 ret = TRUE;
    K32OBJ *old_ptr = NULL;

    if (!pCurrentProcess) return PROCESS_SetBootObjPtr( handle, ptr, flags );
    EnterCriticalSection( &pCurrentProcess->crit_section );
    if ((handle > 0) && (handle <= pCurrentProcess->handle_table->count))
    {
        HANDLE_ENTRY*entry = &pCurrentProcess->handle_table->entries[handle-1];
        old_ptr = entry->ptr;
        K32OBJ_IncCount( ptr );
        entry->flags = flags;
        entry->ptr   = ptr;
    }
    else
    {
        SetLastError( ERROR_INVALID_HANDLE );
        ret = FALSE;
    }
    LeaveCriticalSection( &pCurrentProcess->crit_section );
    if (old_ptr) K32OBJ_DecCount( old_ptr );
    return ret;
}


/*********************************************************************
 *           CloseHandle   (KERNEL32.23)
 */
BOOL32 CloseHandle( HANDLE32 handle )
{
    BOOL32 ret = FALSE;
    K32OBJ *ptr = NULL;

    if (!pCurrentProcess) return PROCESS_CloseBootHandle( handle );
    EnterCriticalSection( &pCurrentProcess->crit_section );
    if ((handle > 0) && (handle <= pCurrentProcess->handle_table->count))
    {
        HANDLE_ENTRY*entry = &pCurrentProcess->handle_table->entries[handle-1];
        if ((ptr = entry->ptr))
        {
            entry->flags = 0;
            entry->ptr   = NULL;
            ret = TRUE;
        }
    }
    LeaveCriticalSection( &pCurrentProcess->crit_section );
    if (!ret) SetLastError( ERROR_INVALID_HANDLE );
    if (ptr) K32OBJ_DecCount( ptr );
    return ret;
}


static int pstr_cmp( const void *ps1, const void *ps2 )
{
    return lstrcmpi32A( *(LPSTR *)ps1, *(LPSTR *)ps2 );
}

/***********************************************************************
 *           PROCESS_FillEnvDB
 */
static BOOL32 PROCESS_FillEnvDB( PDB32 *pdb, TDB *pTask )
{
    LPSTR p, env;
    INT32 count = 0;
    LPSTR *pp, *array = NULL;

    /* Copy the Win16 environment, sorting it in the process */

    env = p = GlobalLock16( pTask->pdb.environment );
    for (p = env; *p; p += strlen(p) + 1) count++;
    pdb->env_db->env_size = (p - env) + 1;
    pdb->env_db->environ = HeapAlloc( pdb->heap, 0, pdb->env_db->env_size );
    if (!pdb->env_db->environ) goto error;
    if (!(array = HeapAlloc( pdb->heap, 0, count * sizeof(array[0]) )))
        goto error;
    for (p = env, pp = array; *p; p += strlen(p) + 1) *pp++ = p;
    qsort( array, count, sizeof(LPSTR), pstr_cmp );
    p = pdb->env_db->environ;
    for (pp = array; count; count--, pp++)
    {
        strcpy( p, *pp );
        p += strlen(p) + 1;
    }
    *p = '\0';
    HeapFree( pdb->heap, 0, array );
    array = NULL;

    return TRUE;

error:
    if (array) HeapFree( pdb->heap, 0, array );
    if (pdb->env_db->environ) HeapFree( pdb->heap, 0, pdb->env_db->environ );
    return FALSE;
}


/***********************************************************************
 *           PROCESS_Create
 */
PDB32 *PROCESS_Create( TDB *pTask )
{
    PDB32 *pdb = HeapAlloc( SystemHeap, HEAP_ZERO_MEMORY, sizeof(PDB32) );
    if (!pdb) return NULL;
    pdb->header.type     = K32OBJ_PROCESS;
    pdb->header.refcount = 1;
    pdb->exit_code       = 0x103; /* STILL_ACTIVE */
    pdb->threads         = 1;
    pdb->running_threads = 1;
    pdb->ring0_threads   = 1;
    pdb->system_heap     = SystemHeap;
    pdb->parent          = pCurrentProcess;
    pdb->group           = pdb;
    pdb->priority        = 8;  /* Normal */
    pdb->heap_list       = pdb->heap;
    InitializeCriticalSection( &pdb->crit_section );
    if (!(pdb->heap = HeapCreate( HEAP_GROWABLE, 0x10000, 0 ))) goto error;
    if (!(pdb->env_db = HeapAlloc(pdb->heap, HEAP_ZERO_MEMORY, sizeof(ENVDB))))
        goto error;
    if (!(pdb->handle_table = PROCESS_AllocHandleTable( pdb ))) goto error;
    if (!PROCESS_FillEnvDB( pdb, pTask )) goto error;
    return pdb;

error:
    if (pdb->env_db) HeapFree( pdb->heap, 0, pdb->env_db );
    if (pdb->handle_table) HeapFree( pdb->system_heap, 0, pdb->handle_table );
    if (pdb->heap) HeapDestroy( pdb->heap );
    DeleteCriticalSection( &pdb->crit_section );
    HeapFree( SystemHeap, 0, pdb );
    return NULL;
}


/***********************************************************************
 *           PROCESS_Destroy
 */
void PROCESS_Destroy( K32OBJ *ptr )
{
    PDB32 *pdb = (PDB32 *)ptr;
    HANDLE32 handle;
    assert( ptr->type == K32OBJ_PROCESS );

    /* Close all handles */
    for (handle = 0; handle < pdb->handle_table->count; handle++)
        if (pdb->handle_table->entries[handle].ptr) CloseHandle( handle );

    /* Free everything */

    ptr->type = K32OBJ_UNKNOWN;
    HeapFree( pdb->heap, 0, pdb->env_db );
    HeapFree( pdb->system_heap, 0, pdb->handle_table );
    HeapDestroy( pdb->heap );
    DeleteCriticalSection( &pdb->crit_section );
    HeapFree( SystemHeap, 0, pdb );
}


/***********************************************************************
 *           ExitProcess   (KERNEL32.100)
 */
void ExitProcess( DWORD status )
{
    TASK_KillCurrentTask( status );
}


/***********************************************************************
 *           GetCurrentProcess   (KERNEL32.198)
 */
HANDLE32 GetCurrentProcess(void)
{
    return 0x7fffffff;
}


/***********************************************************************
 *           GetCurrentProcessId   (KERNEL32.199)
 */
DWORD GetCurrentProcessId(void)
{
    return (DWORD)pCurrentProcess;
}


/***********************************************************************
 *      GetEnvironmentStrings32A   (KERNEL32.210) (KERNEL32.211)
 */
LPSTR GetEnvironmentStrings32A(void)
{
    assert( pCurrentProcess );
    return pCurrentProcess->env_db->environ;
}


/***********************************************************************
 *      GetEnvironmentStrings32W   (KERNEL32.212)
 */
LPWSTR GetEnvironmentStrings32W(void)
{
    INT32 size;
    LPWSTR ret, pW;
    LPSTR pA;

    assert( pCurrentProcess );
    size = HeapSize( GetProcessHeap(), 0, pCurrentProcess->env_db->environ );
    if (!(ret = HeapAlloc( GetProcessHeap(), 0, size * sizeof(WCHAR) )))
        return NULL;
    pA = pCurrentProcess->env_db->environ;
    pW = ret;
    while (*pA)
    {
        lstrcpyAtoW( pW, pA );
        size = strlen(pA);
        pA += size + 1;
        pW += size + 1;
    }
    *pW = 0;
    return ret;
}


/***********************************************************************
 *           FreeEnvironmentStrings32A   (KERNEL32.141)
 */
BOOL32 FreeEnvironmentStrings32A( LPSTR ptr )
{
    assert( pCurrentProcess );
    if (ptr != pCurrentProcess->env_db->environ)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    return TRUE;
}


/***********************************************************************
 *           FreeEnvironmentStrings32W   (KERNEL32.142)
 */
BOOL32 FreeEnvironmentStrings32W( LPWSTR ptr )
{
    assert( pCurrentProcess );
    return HeapFree( GetProcessHeap(), 0, ptr );
}


/***********************************************************************
 *          GetEnvironmentVariable32A   (KERNEL32.213)
 */
DWORD GetEnvironmentVariable32A( LPCSTR name, LPSTR value, DWORD size )
{
    LPSTR p;
    INT32 len, res;

    assert( pCurrentProcess );
    p = pCurrentProcess->env_db->environ;
    if (!name || !*name)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }
    len = strlen(name);
    while (*p)
    {
        res = lstrncmpi32A( name, p, len );
        if (res < 0) goto not_found;
        if (!res && (p[len] == '=')) break;
        p += strlen(p) + 1;
    }
    if (!*p) goto not_found;
    if (value) lstrcpyn32A( value, p + len + 1, size );
    return strlen(p);
not_found:
    return 0;  /* FIXME: SetLastError */
}


/***********************************************************************
 *           GetEnvironmentVariable32W   (KERNEL32.214)
 */
DWORD GetEnvironmentVariable32W( LPCWSTR nameW, LPWSTR valW, DWORD size )
{
    LPSTR name = HEAP_strdupWtoA( GetProcessHeap(), 0, nameW );
    LPSTR val  = HeapAlloc( GetProcessHeap(), 0, size );
    DWORD res  = GetEnvironmentVariable32A( name, val, size );
    HeapFree( GetProcessHeap(), 0, name );
    if (valW) lstrcpynAtoW( valW, val, size );
    HeapFree( GetProcessHeap(), 0, val );
    return res;
}


/***********************************************************************
 *           SetEnvironmentVariable32A   (KERNEL32.484)
 */
BOOL32 SetEnvironmentVariable32A( LPCSTR name, LPCSTR value )
{
    INT32 size, len, res;
    LPSTR p, env, new_env;

    assert( pCurrentProcess );
    env = p = pCurrentProcess->env_db->environ;

    /* Find a place to insert the string */

    res = -1;
    len = strlen(name);
    while (*p)
    {
        res = lstrncmpi32A( name, p, len );
        if (res < 0) break;
        if (!res && (p[len] == '=')) break;
        res = 1;
        p += strlen(p) + 1;
    }
    if (!value && res)  /* Value to remove doesn't exist already */
        return FALSE;

    /* Realloc the buffer */

    len = value ? strlen(name) + strlen(value) + 2 : 0;
    if (!res) len -= strlen(p) + 1;  /* The name already exists */
    size = pCurrentProcess->env_db->env_size + len;
    if (!(new_env = HeapReAlloc( GetProcessHeap(), 0, env, size )))
        return FALSE;
    p = new_env + (p - env);

    /* Set the new string */

    memmove( p + len, p, pCurrentProcess->env_db->env_size - (p-new_env) );
    if (value)
    {
        strcpy( p, name );
        strcat( p, "=" );
        strcat( p, value );
    }
    pCurrentProcess->env_db->env_size = size;
    pCurrentProcess->env_db->environ  = new_env;
    return TRUE;
}


/***********************************************************************
 *           SetEnvironmentVariable32W   (KERNEL32.485)
 */
BOOL32 SetEnvironmentVariable32W( LPCWSTR name, LPCWSTR value )
{
    LPSTR nameA  = HEAP_strdupWtoA( GetProcessHeap(), 0, name );
    LPSTR valueA = HEAP_strdupWtoA( GetProcessHeap(), 0, value );
    BOOL32 ret = SetEnvironmentVariable32A( nameA, valueA );
    HeapFree( GetProcessHeap(), 0, nameA );
    HeapFree( GetProcessHeap(), 0, valueA );
    return ret;
}


/***********************************************************************
 *           ExpandEnvironmentVariablesA   (KERNEL32.103)
 */
DWORD ExpandEnvironmentStrings32A( LPCSTR src, LPSTR dst, DWORD len) {
	LPCSTR s;
        LPSTR d;
	HANDLE32	heap = GetProcessHeap();
	LPSTR		xdst = HeapAlloc(heap,0,10);
	DWORD		cursize = 10;
	DWORD		ret;

	fprintf(stderr,"ExpandEnvironmentStrings32A(%s)\n",src);
	s=src;
	d=xdst;
	memset(dst,'\0',len);
#define CHECK_FREE(n)  {				\
	DWORD	_needed = (n);				\
							\
	while (cursize-(d-xdst)<_needed) {		\
		DWORD	ind = d-xdst;			\
							\
		cursize+=100;				\
		xdst=(LPSTR)HeapReAlloc(heap,0,xdst,cursize);\
		d = xdst+ind;				\
	}						\
}

	while (*s) {
		if (*s=='%') {
			LPCSTR end;

			end = s;do { end++; } while (*end && *end!='%');
			if (*end=='%') {
				LPSTR	x = HeapAlloc(heap,0,end-s+1);
				char	buf[2];

				lstrcpyn32A(x,s+1,end-s-1);
				x[end-s-1]=0;

				/* put expanded variable directly into 
				 * destination string, so we don't have
				 * to use temporary buffers.
				 */
				ret = GetEnvironmentVariable32A(x,buf,2);
				CHECK_FREE(ret+2);
				ret = GetEnvironmentVariable32A(x,d,d-xdst);
				if (ret) {
					d+=strlen(d);
				} else {
					CHECK_FREE(strlen(x)+2);
					*d++='%';
					lstrcpy32A(d,x);
					d+=strlen(x);
					*d++='%';
				}
				HeapFree(heap,0,x);
			} else
				*d=*s;

			s++;d++;
		} else {
			CHECK_FREE(1);
			*d++=*s++;
		}
	}
	*d	= '\0';
	ret	= lstrlen32A(xdst)+1;
	if (d-xdst<len)
		lstrcpy32A(dst,xdst);
	HeapFree(heap,0,xdst);
	return ret;
}

/***********************************************************************
 *           ExpandEnvironmentVariablesA   (KERNEL32.104)
 */
DWORD ExpandEnvironmentStrings32W( LPCWSTR src, LPWSTR dst, DWORD len) {
	HANDLE32	heap = GetProcessHeap();
	LPSTR		srcA = HEAP_strdupWtoA(heap,0,src);
	LPSTR		dstA = HeapAlloc(heap,0,len);
	DWORD		ret = ExpandEnvironmentStrings32A(srcA,dstA,len);

	lstrcpyAtoW(dst,dstA);
	HeapFree(heap,0,dstA);
	HeapFree(heap,0,srcA);
	return ret;
}

/***********************************************************************
 *           GetProcessHeap    (KERNEL32.259)
 */
HANDLE32 GetProcessHeap(void)
{
    if (!pCurrentProcess) return SystemHeap;  /* For the boot-up code */
    return pCurrentProcess->heap;
}


/***********************************************************************
 *           GetThreadLocale    (KERNEL32.295)
 */
LCID GetThreadLocale(void)
{
    return pCurrentProcess->locale;
}


/***********************************************************************
 *           SetPriorityClass   (KERNEL32.503)
 */
BOOL32 SetPriorityClass( HANDLE32 hprocess, DWORD priorityclass )
{
    PDB32	*pdb;

    pdb = (PDB32*)PROCESS_GetObjPtr(hprocess,K32OBJ_PROCESS);
    if (!pdb) return FALSE;
    switch (priorityclass)
    {
    case NORMAL_PRIORITY_CLASS:
    	pdb->priority = 0x00000008;
	break;
    case IDLE_PRIORITY_CLASS:
    	pdb->priority = 0x00000004;
	break;
    case HIGH_PRIORITY_CLASS:
    	pdb->priority = 0x0000000d;
	break;
    case REALTIME_PRIORITY_CLASS:
    	pdb->priority = 0x00000018;
    	break;
    default:
    	fprintf(stderr,"SetPriorityClass: unknown priority class %ld\n",priorityclass);
	break;
    }
    K32OBJ_DecCount((K32OBJ*)pdb);
    return TRUE;
}


/***********************************************************************
 *           GetPriorityClass   (KERNEL32.250)
 */
DWORD GetPriorityClass(HANDLE32 hprocess)
{
    PDB32	*pdb;
    DWORD	ret;

    pdb = (PDB32*)PROCESS_GetObjPtr(hprocess,K32OBJ_PROCESS);
    ret = 0;
    if (pdb)
    {
    	switch (pdb->priority)
        {
	case 0x00000008:
	    ret = NORMAL_PRIORITY_CLASS;
	    break;
	case 0x00000004:
	    ret = IDLE_PRIORITY_CLASS;
	    break;
	case 0x0000000d:
	    ret = HIGH_PRIORITY_CLASS;
	    break;
	case 0x00000018:
	    ret = REALTIME_PRIORITY_CLASS;
	    break;
	default:
	    fprintf(stderr,"GetPriorityClass: unknown priority %ld\n",pdb->priority);
	}
	K32OBJ_DecCount((K32OBJ*)pdb);
    }
    return ret;
}


/***********************************************************************
 *           GetStdHandle    (KERNEL32.276)
 *
 * FIXME: These should be allocated when a console is created, or inherited
 *        from the parent.
 */
HANDLE32 GetStdHandle( DWORD std_handle )
{
    HFILE32 hFile;
    int fd;

    assert( pCurrentProcess );
    switch(std_handle)
    {
    case STD_INPUT_HANDLE:
        if (pCurrentProcess->env_db->hStdin)
            return pCurrentProcess->env_db->hStdin;
        fd = 0;
        break;
    case STD_OUTPUT_HANDLE:
        if (pCurrentProcess->env_db->hStdout)
            return pCurrentProcess->env_db->hStdout;
        fd = 1;
        break;
    case STD_ERROR_HANDLE:
        if (pCurrentProcess->env_db->hStderr)
            return pCurrentProcess->env_db->hStderr;
        fd = 2;
        break;
    default:
        SetLastError( ERROR_INVALID_PARAMETER );
        return INVALID_HANDLE_VALUE32;
    }
    hFile = FILE_DupUnixHandle( fd );
    if (hFile != HFILE_ERROR32)
    {
        FILE_SetFileType( hFile, FILE_TYPE_CHAR );
        switch(std_handle)
        {
        case STD_INPUT_HANDLE: pCurrentProcess->env_db->hStdin=hFile; break;
        case STD_OUTPUT_HANDLE: pCurrentProcess->env_db->hStdout=hFile; break;
        case STD_ERROR_HANDLE: pCurrentProcess->env_db->hStderr=hFile; break;
        }
    }
    return hFile;
}


/***********************************************************************
 *           SetStdHandle    (KERNEL32.506)
 */
BOOL32 SetStdHandle( DWORD std_handle, HANDLE32 handle )
{
    assert( pCurrentProcess );
    switch(std_handle)
    {
    case STD_INPUT_HANDLE:
        pCurrentProcess->env_db->hStdin = handle;
        return TRUE;
    case STD_OUTPUT_HANDLE:
        pCurrentProcess->env_db->hStdout = handle;
        return TRUE;
    case STD_ERROR_HANDLE:
        pCurrentProcess->env_db->hStderr = handle;
        return TRUE;
    }
    SetLastError( ERROR_INVALID_PARAMETER );
    return FALSE;
}

/***********************************************************************
 *           _KERNEL32_18    (KERNEL32.18,Win95)
 * 'Of course you cannot directly access Windows internal structures'
 */
extern THDB *pCurrentThread;
DWORD
_KERNEL32_18(DWORD processid,DWORD action) {
	PDB32	*process;
	TDB	*pTask;

	action+=56;
	fprintf(stderr,"_KERNEL32_18(%ld,%ld+0x38)\n",processid,action);
	if (action>56)
		return 0;
	if (!processid) {
		process=pCurrentProcess;
		/* check if valid process */
	} else
		process=(PDB32*)pCurrentProcess; /* decrypt too, if needed */
	switch (action) {
	case 0:	/* return app compat flags */
		pTask = (TDB*)GlobalLock16(process->task);
		if (!pTask)
			return 0;
		return pTask->compat_flags;
	case 4:	/* returns offset 0xb8 of process struct... dunno what it is */
		return 0;
	case 8:	/* return hinstance16 */
		pTask = (TDB*)GlobalLock16(process->task);
		if (!pTask)
			return 0;
		return pTask->hInstance;
	case 12:/* return expected windows version */
		pTask = (TDB*)GlobalLock16(process->task);
		if (!pTask)
			return 0;
		return pTask->version;
	case 16:/* return uncrypted pointer to current thread */
		return (DWORD)pCurrentThread;
	case 20:/* return uncrypted pointer to process */
		return (DWORD)process;
	case 24:/* return stdoutput handle from startupinfo */
		return (DWORD)(process->env_db->startup_info->hStdOutput);
	case 28:/* return stdinput handle from startupinfo */
		return (DWORD)(process->env_db->startup_info->hStdInput);
	case 32:/* get showwindow flag from startupinfo */
		return (DWORD)(process->env_db->startup_info->wShowWindow);
	case 36:{/* return startup x and y sizes */
		LPSTARTUPINFO32A si = process->env_db->startup_info;
		DWORD x,y;

		x=si->dwXSize;if (x==0x80000000) x=0x8000;
		y=si->dwYSize;if (y==0x80000000) y=0x8000;
		return (y<<16)+x;
	}
	case 40:{/* return startup x and y */
		LPSTARTUPINFO32A si = process->env_db->startup_info;
		DWORD x,y;

		x=si->dwX;if (x==0x80000000) x=0x8000;
		y=si->dwY;if (y==0x80000000) y=0x8000;
		return (y<<16)+x;
	}
	case 44:/* return startup flags */
		return process->env_db->startup_info->dwFlags;
	case 48:/* return uncrypted pointer to parent process (if any) */
		return (DWORD)process->parent;
	case 52:/* return process flags */
		return process->flags;
	case 56:/* unexplored */
		return 0;
	default:
		fprintf(stderr,"_KERNEL32_18:unknown offset (%ld)\n",action);
		return 0;
	}
	/* shouldn't come here */
}

VOID /* FIXME */
_KERNEL32_52(DWORD arg1,CONTEXT *regs) {
	SEGPTR *str = SEGPTR_STRDUP("ThkBuf");

	fprintf(stderr,"_KERNE32_52(arg1=%08lx,%08lx)\n",arg1,EDI_reg(regs));

	EAX_reg(regs) = GetProcAddress16(EDI_reg(regs),SEGPTR_GET(str));
	fprintf(stderr,"	GetProcAddress16(\"ThkBuf\") returns %08lx\n",
			EAX_reg(regs)
	);
	SEGPTR_FREE(str);
}

/***********************************************************************
 *           GetPWinLock    (KERNEL32) FIXME
 */
VOID
GetPWinLock(CRITICAL_SECTION **lock) {
	static CRITICAL_SECTION plock;
	fprintf(stderr,"GetPWinlock(%p)\n",lock);
	*lock = &plock;
}

/***********************************************************************
 *           GetProcessVersion    (KERNEL32)
 */
DWORD
GetProcessVersion(DWORD processid) {
	PDB32	*process;
	TDB	*pTask;

	if (!processid) {
		process=pCurrentProcess;
		/* check if valid process */
	} else
		process=(PDB32*)pCurrentProcess; /* decrypt too, if needed */
	pTask = (TDB*)GlobalLock16(process->task);
	if (!pTask)
		return 0;
	return (pTask->version&0xff) | (((pTask->version >>8) & 0xff)<<16);
}
