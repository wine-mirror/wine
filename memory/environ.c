/*
 * Process environment management
 *
 * Copyright 1996, 1998 Alexandre Julliard
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "process.h"
#include "heap.h"
#include "selectors.h"
#include "winerror.h"

/* Format of an environment block:
 * ASCIIZ   string 1 (xx=yy format)
 * ...
 * ASCIIZ   string n
 * BYTE     0
 * WORD     1
 * ASCIIZ   program name (e.g. C:\WINDOWS\SYSTEM\KRNL386.EXE)
 *
 * Notes:
 * - contrary to Microsoft docs, the environment strings do not appear
 *   to be sorted on Win95 (although they are on NT); so we don't bother
 *   to sort them either.
 */

static const char ENV_program_name[] = "C:\\WINDOWS\\SYSTEM\\KRNL386.EXE";

/* Maximum length of an environment string (including NULL) */
#define MAX_STR_LEN  128

/* Extra bytes to reserve at the end of an environment */
#define EXTRA_ENV_SIZE (sizeof(BYTE) + sizeof(WORD) + sizeof(ENV_program_name))

/* Fill the extra bytes with the program name and stuff */
#define FILL_EXTRA_ENV(p) \
    *(p) = '\0'; \
    PUT_WORD( (p) + 1, 1 ); \
    strcpy( (p) + 3, ENV_program_name );


/***********************************************************************
 *           ENV_FindVariable
 *
 * Find a variable in the environment and return a pointer to the value.
 * Helper function for GetEnvironmentVariable and ExpandEnvironmentStrings.
 */
static LPCSTR ENV_FindVariable( LPCSTR env, LPCSTR name, INT32 len )
{
    while (*env)
    {
        if (!lstrncmpi32A( name, env, len ) && (env[len] == '='))
            return env + len + 1;
        env += strlen(env) + 1;
    }
    return NULL;
}


/***********************************************************************
 *           ENV_BuildEnvironment
 *
 * Build the environment for the initial process
 */
BOOL32 ENV_BuildEnvironment( PDB32 *pdb )
{
    extern char **environ;
    LPSTR p, *e;
    int size, len;

    /* Compute the total size of the Unix environment */

    size = EXTRA_ENV_SIZE;
    for (e = environ; *e; e++)
    {
        len = strlen(*e) + 1;
        size += MIN( len, MAX_STR_LEN );
    }

    /* Now allocate the environment */

    if (!(p = HeapAlloc( SystemHeap, 0, size ))) return FALSE;
    pdb->env_db->environ = p;
    pdb->env_db->env_sel = SELECTOR_AllocBlock( p, 0x10000, SEGMENT_DATA,
                                                FALSE, FALSE );

    /* And fill it with the Unix environment */

    for (e = environ; *e; e++)
    {
        lstrcpyn32A( p, *e, MAX_STR_LEN );
        p += strlen(p) + 1;
    }

    /* Now add the program name */

    FILL_EXTRA_ENV( p );
    return TRUE;
}


/***********************************************************************
 *           ENV_InheritEnvironment
 *
 * Make a process inherit the environment from its parent or from an
 * explicit environment.
 */
BOOL32 ENV_InheritEnvironment( PDB32 *pdb, LPCSTR env )
{
    DWORD size;
    LPCSTR p;

    /* FIXME: should lock the parent environment */
    if (!env) env = pdb->parent->env_db->environ;

    /* Compute the environment size */

    p = env;
    while (*p) p += strlen(p) + 1;
    size = (p - env);

    /* Copy the environment */

    if (!(pdb->env_db->environ = HeapAlloc( pdb->heap, 0,
                                            size + EXTRA_ENV_SIZE )))
        return FALSE;
    pdb->env_db->env_sel = SELECTOR_AllocBlock( pdb->env_db->environ,
                                                0x10000, SEGMENT_DATA,
                                                FALSE, FALSE );
    memcpy( pdb->env_db->environ, env, size );
    FILL_EXTRA_ENV( pdb->env_db->environ + size );
    return TRUE;
}


/***********************************************************************
 *           ENV_FreeEnvironment
 *
 * Free a process environment.
 */
void ENV_FreeEnvironment( PDB32 *pdb )
{
    if (!pdb->env_db) return;
    if (pdb->env_db->env_sel) SELECTOR_FreeBlock( pdb->env_db->env_sel, 1 );
    DeleteCriticalSection( &pdb->env_db->section );
    HeapFree( pdb->heap, 0, pdb->env_db );
}


/***********************************************************************
 *           GetCommandLine32A      (KERNEL32.289)
 */
LPCSTR WINAPI GetCommandLine32A(void)
{
    return PROCESS_Current()->env_db->cmd_line;
}

/***********************************************************************
 *           GetCommandLine32W      (KERNEL32.290)
 */
LPCWSTR WINAPI GetCommandLine32W(void)
{
    PDB32 *pdb = PROCESS_Current();
    EnterCriticalSection( &pdb->env_db->section );
    if (!pdb->env_db->cmd_lineW)
        pdb->env_db->cmd_lineW = HEAP_strdupAtoW( pdb->heap, 0,
                                                  pdb->env_db->cmd_line );
    LeaveCriticalSection( &pdb->env_db->section );
    return pdb->env_db->cmd_lineW;
}


/***********************************************************************
 *           GetEnvironmentStrings32A   (KERNEL32.319) (KERNEL32.320)
 */
LPSTR WINAPI GetEnvironmentStrings32A(void)
{
    PDB32 *pdb = PROCESS_Current();
    return pdb->env_db->environ;
}


/***********************************************************************
 *           GetEnvironmentStrings32W   (KERNEL32.321)
 */
LPWSTR WINAPI GetEnvironmentStrings32W(void)
{
    INT32 size;
    LPWSTR ret;
    PDB32 *pdb = PROCESS_Current();

    EnterCriticalSection( &pdb->env_db->section );
    size = HeapSize( pdb->heap, 0, pdb->env_db->environ );
    if ((ret = HeapAlloc( pdb->heap, 0, size * sizeof(WCHAR) )) != NULL)
    {
        LPSTR pA = pdb->env_db->environ;
        LPWSTR pW = ret;
        while (size--) *pW++ = (WCHAR)(BYTE)*pA++;
    }
    LeaveCriticalSection( &pdb->env_db->section );
    return ret;
}


/***********************************************************************
 *           FreeEnvironmentStrings32A   (KERNEL32.268)
 */
BOOL32 WINAPI FreeEnvironmentStrings32A( LPSTR ptr )
{
    PDB32 *pdb = PROCESS_Current();
    if (ptr != pdb->env_db->environ)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    return TRUE;
}


/***********************************************************************
 *           FreeEnvironmentStrings32W   (KERNEL32.269)
 */
BOOL32 WINAPI FreeEnvironmentStrings32W( LPWSTR ptr )
{
    return HeapFree( GetProcessHeap(), 0, ptr );
}


/***********************************************************************
 *           GetEnvironmentVariable32A   (KERNEL32.322)
 */
DWORD WINAPI GetEnvironmentVariable32A( LPCSTR name, LPSTR value, DWORD size )
{
    LPCSTR p;
    INT32 ret = 0;
    PDB32 *pdb = PROCESS_Current();

    if (!name || !*name)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }
    EnterCriticalSection( &pdb->env_db->section );
    if ((p = ENV_FindVariable( pdb->env_db->environ, name, strlen(name) )))
    {
        ret = strlen(p);
        if (size <= ret)
        {
            /* If not enough room, include the terminating null
             * in the returned size and return an empty string */
            ret++;
            if (value) *value = '\0';
        }
        else if (value) strcpy( value, p );
    }
    LeaveCriticalSection( &pdb->env_db->section );
    return ret;  /* FIXME: SetLastError */
}


/***********************************************************************
 *           GetEnvironmentVariable32W   (KERNEL32.323)
 */
DWORD WINAPI GetEnvironmentVariable32W( LPCWSTR nameW, LPWSTR valW, DWORD size)
{
    LPSTR name = HEAP_strdupWtoA( GetProcessHeap(), 0, nameW );
    LPSTR val  = valW ? HeapAlloc( GetProcessHeap(), 0, size ) : NULL;
    DWORD res  = GetEnvironmentVariable32A( name, val, size );
    HeapFree( GetProcessHeap(), 0, name );
    if (val)
    {
        lstrcpynAtoW( valW, val, size );
        HeapFree( GetProcessHeap(), 0, val );
    }
    return res;
}


/***********************************************************************
 *           SetEnvironmentVariable32A   (KERNEL32.641)
 */
BOOL32 WINAPI SetEnvironmentVariable32A( LPCSTR name, LPCSTR value )
{
    INT32 old_size, len, res;
    LPSTR p, env, new_env;
    BOOL32 ret = FALSE;
    PDB32 *pdb = PROCESS_Current();

    EnterCriticalSection( &pdb->env_db->section );
    env = p = pdb->env_db->environ;

    /* Find a place to insert the string */

    res = -1;
    len = strlen(name);
    while (*p)
    {
        if (!lstrncmpi32A( name, p, len ) && (p[len] == '=')) break;
        p += strlen(p) + 1;
    }
    if (!value && !*p) goto done;  /* Value to remove doesn't exist */

    /* Realloc the buffer */

    len = value ? strlen(name) + strlen(value) + 2 : 0;
    if (*p) len -= strlen(p) + 1;  /* The name already exists */
    old_size = HeapSize( pdb->heap, 0, env );
    if (len < 0)
    {
        LPSTR next = p + strlen(p) + 1;  /* We know there is a next one */
        memmove( next + len, next, old_size - (next - env) );
    }
    if (!(new_env = HeapReAlloc( pdb->heap, 0, env, old_size + len )))
        goto done;
    SELECTOR_MoveBlock( pdb->env_db->env_sel, new_env );
    p = new_env + (p - env);
    if (len > 0) memmove( p + len, p, old_size - (p - new_env) );

    /* Set the new string */

    if (value)
    {
        strcpy( p, name );
        strcat( p, "=" );
        strcat( p, value );
    }
    pdb->env_db->environ = new_env;
    ret = TRUE;

done:
    LeaveCriticalSection( &pdb->env_db->section );
    return ret;
}


/***********************************************************************
 *           SetEnvironmentVariable32W   (KERNEL32.642)
 */
BOOL32 WINAPI SetEnvironmentVariable32W( LPCWSTR name, LPCWSTR value )
{
    LPSTR nameA  = HEAP_strdupWtoA( GetProcessHeap(), 0, name );
    LPSTR valueA = HEAP_strdupWtoA( GetProcessHeap(), 0, value );
    BOOL32 ret = SetEnvironmentVariable32A( nameA, valueA );
    HeapFree( GetProcessHeap(), 0, nameA );
    HeapFree( GetProcessHeap(), 0, valueA );
    return ret;
}


/***********************************************************************
 *           ExpandEnvironmentStrings32A   (KERNEL32.216)
 *
 * Note: overlapping buffers are not supported; this is how it should be.
 */
DWORD WINAPI ExpandEnvironmentStrings32A( LPCSTR src, LPSTR dst, DWORD count )
{
    DWORD len, total_size = 1;  /* 1 for terminating '\0' */
    LPCSTR p, var;
    PDB32 *pdb = PROCESS_Current();

    if (!count) dst = NULL;
    EnterCriticalSection( &pdb->env_db->section );

    while (*src)
    {
        if (*src != '%')
        {
            if ((p = strchr( src, '%' ))) len = p - src;
            else len = strlen(src);
            var = src;
            src += len;
        }
        else  /* we are at the start of a variable */
        {
            if ((p = strchr( src + 1, '%' )))
            {
                len = p - src - 1;  /* Length of the variable name */
                if ((var = ENV_FindVariable( pdb->env_db->environ,
                                             src + 1, len )))
                {
                    src += len + 2;  /* Skip the variable name */
                    len = strlen(var);
                }
                else
                {
                    var = src;  /* Copy original name instead */
                    len += 2;
                    src += len;
                }
            }
            else  /* unfinished variable name, ignore it */
            {
                var = src;
                len = strlen(src);  /* Copy whole string */
                src += len;
            }
        }
        total_size += len;
        if (dst)
        {
            if (count < len) len = count;
            memcpy( dst, var, len );
            dst += len;
            count -= len;
        }
    }
    LeaveCriticalSection( &pdb->env_db->section );

    /* Null-terminate the string */
    if (dst)
    {
        if (!count) dst--;
        *dst = '\0';
    }
    return total_size;
}


/***********************************************************************
 *           ExpandEnvironmentStrings32W   (KERNEL32.217)
 */
DWORD WINAPI ExpandEnvironmentStrings32W( LPCWSTR src, LPWSTR dst, DWORD len )
{
    LPSTR srcA = HEAP_strdupWtoA( GetProcessHeap(), 0, src );
    LPSTR dstA = dst ? HeapAlloc( GetProcessHeap(), 0, len ) : NULL;
    DWORD ret  = ExpandEnvironmentStrings32A( srcA, dstA, len );
    if (dstA)
    {
        lstrcpyAtoW( dst, dstA );
        HeapFree( GetProcessHeap(), 0, dstA );
    }
    HeapFree( GetProcessHeap(), 0, srcA );
    return ret;
}

