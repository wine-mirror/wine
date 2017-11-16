/*
 * Emulator initialisation code
 *
 * Copyright 2000 Alexandre Julliard
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

#include "config.h"
#include "wine/port.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_SYS_MMAN_H
# include <sys/mman.h>
#endif
#ifdef HAVE_SYS_RESOURCE_H
# include <sys/resource.h>
#endif
#ifdef HAVE_SYS_SYSCALL_H
# include <sys/syscall.h>
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <pthread.h>

#include "wine/library.h"
#include "main.h"

#ifdef __APPLE__

#ifndef __clang__
__asm__(".zerofill WINE_DOS, WINE_DOS, ___wine_dos, 0x40000000");
__asm__(".zerofill WINE_SHAREDHEAP, WINE_SHAREDHEAP, ___wine_shared_heap, 0x03000000");
extern char __wine_dos[0x40000000], __wine_shared_heap[0x03000000];
#else
__asm__(".zerofill WINE_DOS, WINE_DOS");
__asm__(".zerofill WINE_SHAREDHEAP, WINE_SHAREDHEAP");
static char __wine_dos[0x40000000] __attribute__((section("WINE_DOS, WINE_DOS")));
static char __wine_shared_heap[0x03000000] __attribute__((section("WINE_SHAREDHEAP, WINE_SHAREDHEAP")));
#endif

static const struct wine_preload_info wine_main_preload_info[] =
{
    { __wine_dos,         sizeof(__wine_dos) },          /* DOS area + PE exe */
    { __wine_shared_heap, sizeof(__wine_shared_heap) },  /* shared user data + shared heap */
    { 0, 0 }  /* end of list */
};

static inline void reserve_area( void *addr, size_t size )
{
    wine_anon_mmap( addr, size, PROT_NONE, MAP_FIXED | MAP_NORESERVE );
    wine_mmap_add_reserved_area( addr, size );
}

#else  /* __APPLE__ */

/* the preloader will set this variable */
const struct wine_preload_info *wine_main_preload_info = NULL;

static inline void reserve_area( void *addr, size_t size )
{
    wine_mmap_add_reserved_area( addr, size );
}

#endif  /* __APPLE__ */

/***********************************************************************
 *           check_command_line
 *
 * Check if command line is one that needs to be handled specially.
 */
static void check_command_line( int argc, char *argv[] )
{
    static const char usage[] =
        "Usage: wine PROGRAM [ARGUMENTS...]   Run the specified program\n"
        "       wine --help                   Display this help and exit\n"
        "       wine --version                Output version information and exit";

    if (argc <= 1)
    {
        fprintf( stderr, "%s\n", usage );
        exit(1);
    }
    if (!strcmp( argv[1], "--help" ))
    {
        printf( "%s\n", usage );
        exit(0);
    }
    if (!strcmp( argv[1], "--version" ))
    {
        printf( "%s\n", wine_get_build_id() );
        exit(0);
    }
}


#ifdef __ANDROID__

static int pre_exec(void)
{
#if defined(__i386__) || defined(__x86_64__)
    return 1;  /* we have a preloader */
#else
    return 0;  /* no exec needed */
#endif
}

#elif defined(__linux__) && (defined(__i386__) || defined(__arm__))

#ifdef __i386__
/* separate thread to check for NPTL and TLS features */
static void *needs_pthread( void *arg )
{
    pid_t tid = syscall( 224 /* SYS_gettid */ );
    /* check for NPTL */
    if (tid != -1 && tid != getpid()) return (void *)1;
    /* check for TLS glibc */
    if (wine_get_gs() != 0) return (void *)1;
    /* check for exported epoll_create to detect new glibc versions without TLS */
    if (wine_dlsym( RTLD_DEFAULT, "epoll_create", NULL, 0 ))
        fprintf( stderr,
                 "wine: glibc >= 2.3 without NPTL or TLS is not a supported combination.\n"
                 "      Please upgrade to a glibc with NPTL support.\n" );
    else
        fprintf( stderr,
                 "wine: Your C library is too old. You need at least glibc 2.3 with NPTL support.\n" );
    return 0;
}

/* check if we support the glibc threading model */
static void check_threading(void)
{
    pthread_t id;
    void *ret;

    pthread_create( &id, NULL, needs_pthread, NULL );
    pthread_join( id, &ret );
    if (!ret) exit(1);
}
#else
static void check_threading(void)
{
}
#endif

static void check_vmsplit( void *stack )
{
    if (stack < (void *)0x80000000)
    {
        /* if the stack is below 0x80000000, assume we can safely try a munmap there */
        if (munmap( (void *)0x80000000, 1 ) == -1 && errno == EINVAL)
            fprintf( stderr,
                     "Warning: memory above 0x80000000 doesn't seem to be accessible.\n"
                     "Wine requires a 3G/1G user/kernel memory split to work properly.\n" );
    }
}

static void set_max_limit( int limit )
{
    struct rlimit rlimit;

    if (!getrlimit( limit, &rlimit ))
    {
        rlimit.rlim_cur = rlimit.rlim_max;
        setrlimit( limit, &rlimit );
    }
}

static int pre_exec(void)
{
    int temp;

    check_threading();
    check_vmsplit( &temp );
    set_max_limit( RLIMIT_AS );
#ifdef __i386__
    return 1;  /* we have a preloader on x86 */
#else
    return 0;
#endif
}

#elif defined(__linux__) && (defined(__x86_64__) || defined(__aarch64__))

static int pre_exec(void)
{
    return 1;  /* we have a preloader on x86-64/arm64 */
}

#elif (defined(__FreeBSD__) || defined (__FreeBSD_kernel__) || defined(__DragonFly__))

static int pre_exec(void)
{
    struct rlimit rl;

    rl.rlim_cur = 0x02000000;
    rl.rlim_max = 0x02000000;
    setrlimit( RLIMIT_DATA, &rl );
    return 1;
}

#else

static int pre_exec(void)
{
    return 0;  /* no exec needed */
}

#endif


/**********************************************************************
 *           main
 */
int main( int argc, char *argv[] )
{
    char error[1024];
    int i;

    if (!getenv( "WINELOADERNOEXEC" ))  /* first time around */
    {
        static char noexec[] = "WINELOADERNOEXEC=1";

        putenv( noexec );
        check_command_line( argc, argv );
        if (pre_exec())
        {
            wine_init_argv0_path( argv[0] );
            wine_exec_wine_binary( NULL, argv, getenv( "WINELOADER" ));
            fprintf( stderr, "wine: could not exec the wine loader\n" );
            exit(1);
        }
    }

#ifndef __APPLE__
    if (wine_main_preload_info)
#endif
    {
        for (i = 0; wine_main_preload_info[i].size; i++)
            reserve_area( wine_main_preload_info[i].addr, wine_main_preload_info[i].size );
    }

    wine_init( argc, argv, error, sizeof(error) );
    fprintf( stderr, "wine: failed to initialize: %s\n", error );
    exit(1);
}
