/*
 * Wine portability routines
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"
#include "wine/port.h"

#include <assert.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#ifdef HAVE_STDINT_H
# include <stdint.h>
#endif

#include "wine/library.h"
#include "wine/pthread.h"

/* Note: the wine_pthread functions are just placeholders,
 * they will be overridden by the pthread support code.
 */

/***********************************************************************
 *           wine_pthread_init_process
 */
void wine_pthread_init_process( const struct wine_pthread_functions *functions )
{
}

/***********************************************************************
 *           wine_pthread_init_thread
 */
void wine_pthread_init_thread( struct wine_pthread_thread_info *info )
{
}

/***********************************************************************
 *           wine_pthread_create_thread
 */
int wine_pthread_create_thread( struct wine_pthread_thread_info *info )
{
    return -1;
}

/***********************************************************************
 *           wine_pthread_init_current_teb
 */
void wine_pthread_init_current_teb( struct wine_pthread_thread_info *info )
{
}

/***********************************************************************
 *           wine_pthread_get_current_teb
 */
void *wine_pthread_get_current_teb(void)
{
    return NULL;
}

/***********************************************************************
 *           wine_pthread_exit_thread
 */
void wine_pthread_exit_thread( struct wine_pthread_thread_info *info )
{
    exit( info->exit_status );
}

/***********************************************************************
 *           wine_pthread_abort_thread
 */
void wine_pthread_abort_thread( int status )
{
    exit( status );
}


/***********************************************************************
 *           wine_switch_to_stack
 *
 * Switch to the specified stack and call the function.
 */
void DECLSPEC_NORETURN wine_switch_to_stack( void (*func)(void *), void *arg, void *stack );
#if defined(__i386__) && defined(__GNUC__)
__ASM_GLOBAL_FUNC( wine_switch_to_stack,
                   "movl 4(%esp),%ecx\n\t"  /* func */
                   "movl 8(%esp),%edx\n\t"  /* arg */
                   "movl 12(%esp),%esp\n\t"  /* stack */
                   "pushl %edx\n\t"
                   "xorl %ebp,%ebp\n\t"
                   "call *%ecx\n\t"
                   "int $3" /* we never return here */ );
#elif defined(__i386__) && defined(_MSC_VER)
__declspec(naked) void wine_switch_to_stack( void (*func)(void *), void *arg, void *stack )
{
  __asm mov ecx, 4[esp];
  __asm mov edx, 8[esp];
  __asm mov esp, 12[esp];
  __asm push edx;
  __asm xor ebp, ebp;
  __asm call [ecx];
  __asm int 3;
}
#elif defined(__sparc__) && defined(__GNUC__)
__ASM_GLOBAL_FUNC( wine_switch_to_stack,
                   "mov %o0, %l0\n\t" /* store first argument */
                   "mov %o1, %l1\n\t" /* store second argument */
                   "mov %o2, %sp\n\t" /* store stack */
                   "call %l0, 0\n\t" /* call func */
                   "mov %l1, %o0\n\t" /* delay slot:  arg for func */
                   "ta 0x01"); /* breakpoint - we never get here */
#elif defined(__powerpc__) && defined(__APPLE__)
__ASM_GLOBAL_FUNC( wine_switch_to_stack,
                   "mtctr r3\n\t" /* func -> ctr */
                   "mr r3,r4\n\t" /* args -> function param 1 (r3) */
                   "mr r1,r5\n\t" /* stack */
                   "bctr\n" /* call ctr */
                   "1:\tb 1b"); /* loop */
#elif defined(__powerpc__) && defined(__GNUC__)
__ASM_GLOBAL_FUNC( wine_switch_to_stack,
                   "mtctr 3\n\t" /* func -> ctr */
                   "mr 3,4\n\t" /* args -> function param 1 (r3) */
                   "mr 1,5\n\t" /* stack */
                   "bctr\n\t" /* call ctr */
                   "1:\tb 1b"); /* loop */
#else
#error You must implement wine_switch_to_stack for your platform
#endif


static char *pe_area;
static size_t pe_area_size;

/***********************************************************************
 *		wine_set_pe_load_area
 *
 * Define the reserved area to use for loading the main PE binary.
 */
void wine_set_pe_load_area( void *base, size_t size )
{
    unsigned int page_mask = getpagesize() - 1;
    char *end = (char *)base + size;

    pe_area = (char *)(((unsigned long)base + page_mask) & ~page_mask);
    pe_area_size = (end - pe_area) & ~page_mask;
}


/***********************************************************************
 *		wine_free_pe_load_area
 *
 * Free the reserved area to use for loading the main PE binary.
 */
void wine_free_pe_load_area(void)
{
#ifdef HAVE_MMAP
    if (pe_area) munmap( pe_area, pe_area_size );
#endif
    pe_area = NULL;
}


#if (defined(__svr4__) || defined(__NetBSD__)) && !defined(MAP_TRYFIXED)
/***********************************************************************
 *             try_mmap_fixed
 *
 * The purpose of this routine is to emulate the behaviour of
 * the Linux mmap() routine if a non-NULL address is passed,
 * but the MAP_FIXED flag is not set.  Linux in this case tries
 * to place the mapping at the specified address, *unless* the
 * range is already in use.  Solaris, however, completely ignores
 * the address argument in this case.
 *
 * As Wine code occasionally relies on the Linux behaviour, e.g. to
 * be able to map non-relocateable PE executables to their proper
 * start addresses, or to map the DOS memory to 0, this routine
 * emulates the Linux behaviour by checking whether the desired
 * address range is still available, and placing the mapping there
 * using MAP_FIXED if so.
 */
static int try_mmap_fixed (void *addr, size_t len, int prot, int flags,
                           int fildes, off_t off)
{
    char * volatile result = NULL;
    int pagesize = getpagesize();
    pid_t pid;

    /* We only try to map to a fixed address if
       addr is non-NULL and properly aligned,
       and MAP_FIXED isn't already specified. */

    if ( !addr )
        return 0;
    if ( (uintptr_t)addr & (pagesize-1) )
        return 0;
    if ( flags & MAP_FIXED )
        return 0;

    /* We use vfork() to freeze all threads of the
       current process.  This allows us to check without
       race condition whether the desired memory range is
       already in use.  Note that because vfork() shares
       the address spaces between parent and child, we
       can actually perform the mapping in the child. */

    if ( (pid = vfork()) == -1 )
    {
        perror("try_mmap_fixed: vfork");
        exit(1);
    }
    if ( pid == 0 )
    {
        int i;
        char vec;

        /* We call mincore() for every page in the desired range.
           If any of these calls succeeds, the page is already
           mapped and we must fail. */
        for ( i = 0; i < len; i += pagesize )
            if ( mincore( (caddr_t)addr + i, pagesize, &vec ) != -1 )
               _exit(1);

        /* Perform the mapping with MAP_FIXED set.  This is safe
           now, as none of the pages is currently in use. */
        result = mmap( addr, len, prot, flags | MAP_FIXED, fildes, off );
        if ( result == addr )
            _exit(0);

        if ( result != (void *) -1 ) /* This should never happen ... */
            munmap( result, len );

       _exit(1);
    }

    /* vfork() lets the parent continue only after the child
       has exited.  Furthermore, Wine sets SIGCHLD to SIG_IGN,
       so we don't need to wait for the child. */

    return result == addr;
}
#endif  /* (__svr4__ || __NetBSD__) && !MAP_TRYFIXED */


/***********************************************************************
 *		wine_anon_mmap
 *
 * Portable wrapper for anonymous mmaps
 */
void *wine_anon_mmap( void *start, size_t size, int prot, int flags )
{
#ifdef HAVE_MMAP
    static int fdzero = -1;

#ifdef MAP_ANON
    flags |= MAP_ANON;
#else
    if (fdzero == -1)
    {
        if ((fdzero = open( "/dev/zero", O_RDONLY )) == -1)
        {
            perror( "/dev/zero: open" );
            exit(1);
        }
    }
#endif  /* MAP_ANON */

#ifdef MAP_SHARED
    flags &= ~MAP_SHARED;
#endif

    /* Linux EINVAL's on us if we don't pass MAP_PRIVATE to an anon mmap */
#ifdef MAP_PRIVATE
    flags |= MAP_PRIVATE;
#endif

    if (pe_area && start &&
        (char *)start >= pe_area &&
        (char *)start + size <= pe_area + pe_area_size)
    {
        wine_free_pe_load_area();
        flags |= MAP_FIXED;
    }

    if (!(flags & MAP_FIXED))
    {
#ifdef MAP_TRYFIXED
        /* If available, this will attempt a fixed mapping in-kernel */
        flags |= MAP_TRYFIXED;
#elif defined(__svr4__) || defined(__NetBSD__)
        if ( try_mmap_fixed( start, size, prot, flags, fdzero, 0 ) )
            return start;
#endif
    }
    return mmap( start, size, prot, flags, fdzero, 0 );
#else
    return (void *)-1;
#endif
}
