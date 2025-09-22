/*
 * Copyright 2023 Rémi Bernon for CodeWeavers
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

#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>

#include <windef.h>
#include <winbase.h>
#include <winsock.h>

#include <stdint.h>
#include <stdlib.h>
#include <math.h>

#define GLIB_CHECK_VERSION(x, y, z) ((x) < GLIB_MAJOR_VERSION || ((x) == GLIB_MAJOR_VERSION && (y) <= GLIB_MINOR_VERSION))
#define GLIB_MAJOR_VERSION 2
#define GLIB_MINOR_VERSION 54

#define G_BYTE_ORDER 1234
#define G_BIG_ENDIAN 4321
#define GINT32_FROM_LE(val) ((INT32)(val))
#define GINT16_FROM_LE(val) ((INT16)(val))

#define G_UNLIKELY(expr) (expr)
#define G_LIKELY(expr) (expr)

#define g_return_val_if_fail( cond, val ) do { if (!(cond)) return (val); } while (0)

typedef void *gpointer;
typedef gpointer (*GThreadFunc)( gpointer data );

typedef struct _stat64 GStatBuf;
#define g_stat _stat64

typedef struct
{
    const char *message;
} GError;

typedef struct
{
    LONG ref;
    HANDLE handle;
    GThreadFunc func;
    gpointer data;
} GThread;

extern int g_vsnprintf( char *buffer, size_t size, const char *format, va_list args ) __WINE_CRT_PRINTF_ATTR(3, 0);
extern int g_snprintf( char *buffer, size_t size, const char *format, ... ) __WINE_CRT_PRINTF_ATTR(3, 4);

extern double g_get_monotonic_time(void);
extern void g_usleep( unsigned int micros );

extern GThread *g_thread_try_new( const char *name, GThreadFunc func, gpointer data, GError **err );
extern void g_thread_unref( GThread *thread );
extern void g_thread_join( GThread *thread );
extern void g_clear_error( GError **error );

#define G_FILE_TEST_EXISTS 1
#define G_FILE_TEST_IS_REGULAR 2

extern int g_file_test( const char *path, int test );

#define g_new( type, count ) calloc( (count), sizeof(type) )
static inline void g_free( void *ptr ) { free( ptr ); }

typedef SRWLOCK GMutex;
static inline void g_mutex_init( GMutex *mutex ) { InitializeSRWLock( mutex ); }
static inline void g_mutex_clear( GMutex *mutex ) {}
static inline void g_mutex_lock( GMutex *mutex ) { AcquireSRWLockExclusive( mutex ); }
static inline void g_mutex_unlock( GMutex *mutex ) { ReleaseSRWLockExclusive( mutex ); }

typedef CRITICAL_SECTION GRecMutex;
static inline void g_rec_mutex_init( GRecMutex *mutex ) { InitializeCriticalSection( mutex ); }
static inline void g_rec_mutex_clear( GRecMutex *mutex ) { DeleteCriticalSection( mutex ); }
static inline void g_rec_mutex_lock( GRecMutex *mutex ) { EnterCriticalSection( mutex ); }
static inline void g_rec_mutex_unlock( GRecMutex *mutex ) { LeaveCriticalSection( mutex ); }

typedef CONDITION_VARIABLE GCond;
static inline void g_cond_init( GCond *cond ) { InitializeConditionVariable( cond ); }
static inline void g_cond_clear( GCond *cond ) {}
static inline void g_cond_signal( GCond *cond ) { WakeConditionVariable( cond ); }
static inline void g_cond_broadcast( GCond *cond ) { WakeAllConditionVariable( cond ); }
static inline void g_cond_wait( GCond *cond, GMutex *mutex ) { SleepConditionVariableSRW( cond, mutex, INFINITE, 0 ); }

static inline void g_atomic_int_inc( int *ptr ) { InterlockedIncrement( (LONG *)ptr ); }
static inline int g_atomic_int_add( int *ptr, int val ) { return InterlockedAdd( (LONG *)ptr, val ) - val; }
static inline int g_atomic_int_get( int *ptr ) { int value = ReadNoFence( (LONG *)ptr ); MemoryBarrier(); return value; }
static inline void g_atomic_int_set( int *ptr, int val ) { InterlockedExchange( (LONG *)ptr, val ); }
static inline int g_atomic_int_dec_and_test( int *ptr ) { return !InterlockedAdd( (LONG *)ptr, -1 ); }
static inline int g_atomic_int_compare_and_exchange( int *ptr, int cmp, int val ) { return InterlockedCompareExchange( (LONG *)ptr, val, cmp ) == cmp; }
