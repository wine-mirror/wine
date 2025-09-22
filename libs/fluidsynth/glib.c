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

#include <glib.h>

int g_vsnprintf( char *buffer, size_t size, const char *format, va_list args )
{
    int ret = _vsnprintf( buffer, size - 1, format, args );
    if (ret >= 0 && ret < size) buffer[ret] = 0;
    else buffer[0] = 0;
    return ret;
}

int g_snprintf( char *buffer, size_t size, const char *format, ... )
{
    va_list args;
    int ret;

    va_start( args, format );
    ret = g_vsnprintf( buffer, size, format, args );
    va_end( args );

    return ret;
}

double g_get_monotonic_time(void)
{
    LARGE_INTEGER counter, frequency;

    QueryPerformanceFrequency( &frequency );
    QueryPerformanceCounter( &counter );

    return counter.QuadPart * 1000000.0 / frequency.QuadPart; /* time in micros */
}

void g_usleep( unsigned int micros )
{
    Sleep( (micros + 999) / 1000 );
}

static DWORD CALLBACK g_thread_wrapper( void *args )
{
    GThread *thread = args;
    thread->result = thread->func( thread->data );
    g_thread_unref( thread );
    return 0;
}

GThread *g_thread_try_new( const char *name, GThreadFunc func, gpointer data, GError **err )
{
    GThread *thread;

    if (!(thread = calloc( 1, sizeof(*thread) ))) return NULL;
    thread->ref = 2;
    thread->func = func;
    thread->data = data;

    if (!(thread->handle = CreateThread( NULL, 0, g_thread_wrapper, thread, 0, NULL )))
    {
        free( thread );
        return NULL;
    }

    return thread;
}

void g_thread_unref( GThread *thread )
{
    if (!InterlockedDecrement( &thread->ref ))
    {
        CloseHandle( thread->handle );
        free( thread );
    }
}

gpointer g_thread_join( GThread *thread )
{
    gpointer result;

    WaitForSingleObject( thread->handle, INFINITE );
    result = thread->result;
    g_thread_unref( thread );
    return result;
}

void g_clear_error( GError **error )
{
    *error = NULL;
}

int g_file_test( const char *path, int test )
{
    DWORD attrs = GetFileAttributesA( path );
    if (attrs != INVALID_FILE_ATTRIBUTES)
    {
        if (test & G_FILE_TEST_EXISTS) return 1;
        if ((test & G_FILE_TEST_IS_REGULAR) && !(attrs & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_DEVICE))) return 1;
    }
    return 0;
}
