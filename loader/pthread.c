/*
 * Wine threading routines using the pthread library
 *
 * Copyright 2003 Alexandre Julliard
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

#ifdef HAVE_PTHREAD_H

#include <assert.h>
#include <stdlib.h>
#include <signal.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <string.h>
#include <sys/types.h>
#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif

#include "wine/library.h"
#include "wine/pthread.h"

/***********************************************************************
 *           wine_pthread_init_process
 *
 * Initialization for a newly created process.
 */
void wine_pthread_init_process( const struct wine_pthread_functions *functions )
{
}


/***********************************************************************
 *           wine_pthread_init_thread
 *
 * Initialization for a newly created thread.
 */
void wine_pthread_init_thread(void)
{
}


/***********************************************************************
 *           wine_pthread_create_thread
 */
int wine_pthread_create_thread( struct wine_pthread_thread_info *info )
{
    pthread_t id;
    pthread_attr_t attr;

    if (!info->stack_base)
    {
        info->stack_base = wine_anon_mmap( NULL, info->stack_size,
                                           PROT_READ | PROT_WRITE | PROT_EXEC, 0 );
        if (info->stack_base == (void *)-1) return -1;
    }
    pthread_attr_init( &attr );
    pthread_attr_setstack( &attr, info->stack_base, info->stack_size );
    if (pthread_create( &id, &attr, (void * (*)(void *))info->entry, info )) return -1;
    return 0;
}


/***********************************************************************
 *           wine_pthread_exit_thread
 */
void wine_pthread_exit_thread( struct wine_pthread_thread_info *info )
{
    struct cleanup_info
    {
        pthread_t self;
        struct wine_pthread_thread_info thread_info;
    };

    static struct cleanup_info *previous_info;
    struct cleanup_info *cleanup_info, *free_info;
    void *ptr;

    /* store it at the end of the TEB structure */
    cleanup_info = (struct cleanup_info *)((char *)info->teb_base + info->teb_size) - 1;
    cleanup_info->self = pthread_self();
    cleanup_info->thread_info = *info;

    if ((free_info = interlocked_xchg_ptr( (void **)&previous_info, cleanup_info )) != NULL)
    {
        pthread_join( free_info->self, &ptr );
        wine_ldt_free_fs( free_info->thread_info.teb_sel );
        munmap( free_info->thread_info.stack_base, free_info->thread_info.stack_size );
        munmap( free_info->thread_info.teb_base, free_info->thread_info.teb_size );
    }
    pthread_exit( (void *)info->exit_status );
}


/***********************************************************************
 *           wine_pthread_abort_thread
 */
void wine_pthread_abort_thread( int status )
{
    pthread_exit( (void *)status );
}

#endif  /* HAVE_PTHREAD_H */
