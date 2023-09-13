/*
 * Unix interface for ntlm_auth
 *
 * Copyright 2005, 2006 Kai Blin
 * Copyright 2021 Hans Leidekker for CodeWeavers
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

#if 0
#pragma makedep unix
#endif

#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <spawn.h>
#include <sys/wait.h>
#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"
#include "winbase.h"
#include "sspi.h"

#include "wine/debug.h"
#include "unixlib.h"

extern char **environ;

WINE_DEFAULT_DEBUG_CHANNEL(ntlm);
WINE_DECLARE_DEBUG_CHANNEL(winediag);

#define INITIAL_BUFFER_SIZE 200

struct com_buf
{
    char        *buffer;
    unsigned int size;
    unsigned int offset;
};

static SECURITY_STATUS read_line( struct ntlm_ctx *ctx, unsigned int *offset )
{
    char *newline;
    struct com_buf *com_buf = (struct com_buf *)(ULONG_PTR)ctx->com_buf;

    if (!com_buf)
    {
        if (!(com_buf = malloc( sizeof(*com_buf) ))) return SEC_E_INSUFFICIENT_MEMORY;
        if (!(com_buf->buffer = malloc( INITIAL_BUFFER_SIZE )))
        {
            free( com_buf );
            return SEC_E_INSUFFICIENT_MEMORY;
        }
        com_buf->size = INITIAL_BUFFER_SIZE;
        com_buf->offset = 0;
        ctx->com_buf = (ULONG_PTR)com_buf;
    }

    do
    {
        ssize_t size;
        if (com_buf->offset + INITIAL_BUFFER_SIZE > com_buf->size)
        {
            char *buf = realloc( com_buf->buffer, com_buf->size + INITIAL_BUFFER_SIZE );
            if (!buf) return SEC_E_INSUFFICIENT_MEMORY;
            com_buf->size += INITIAL_BUFFER_SIZE;
            com_buf->buffer = buf;
        }
        size = read( ctx->pipe_in, com_buf->buffer + com_buf->offset, com_buf->size - com_buf->offset );
        if (size <= 0) return SEC_E_INTERNAL_ERROR;

        com_buf->offset += size;
        newline = memchr( com_buf->buffer, '\n',  com_buf->offset );
    } while (!newline);

    /* if there's a newline character, and we read more than that newline, we have to store the offset so we can
       preserve the additional data */
    if (newline != com_buf->buffer + com_buf->offset) *offset = (com_buf->buffer + com_buf->offset) - (newline + 1);
    else *offset = 0;

    *newline = 0;
    return SEC_E_OK;
}

static NTSTATUS ntlm_chat( void *args )
{
    const struct chat_params *params = args;
    struct ntlm_ctx *ctx = params->ctx;
    struct com_buf *com_buf;
    SECURITY_STATUS status = SEC_E_OK;
    unsigned int offset;

    write( ctx->pipe_out, params->buf, strlen(params->buf) );
    write( ctx->pipe_out, "\n", 1 );

    if ((status = read_line( ctx, &offset )) != SEC_E_OK) return status;
    com_buf = (struct com_buf *)(ULONG_PTR)ctx->com_buf;
    *params->retlen = strlen( com_buf->buffer );

    if (*params->retlen > params->buflen) return SEC_E_BUFFER_TOO_SMALL;
    if (*params->retlen < 2) return SEC_E_ILLEGAL_MESSAGE;
    if (!strncmp( com_buf->buffer, "ERR", 3 )) return SEC_E_INVALID_TOKEN;

    memcpy( params->buf, com_buf->buffer, *params->retlen + 1 );

    if (!offset) com_buf->offset = 0;
    else
    {
        memmove( com_buf->buffer, com_buf->buffer + com_buf->offset, offset );
        com_buf->offset = offset;
    }

    return SEC_E_OK;
}

static NTSTATUS ntlm_cleanup( void *args )
{
    struct ntlm_ctx *ctx = args;
    struct com_buf *com_buf = (struct com_buf *)(ULONG_PTR)ctx->com_buf;

    if (!ctx || (ctx->mode != MODE_CLIENT && ctx->mode != MODE_SERVER)) return STATUS_INVALID_HANDLE;
    ctx->mode = MODE_INVALID;

    /* closing stdin will terminate ntlm_auth */
    close( ctx->pipe_out );
    close( ctx->pipe_in );

    if (ctx->pid > 0) /* reap child */
    {
        pid_t ret;
        do {
            ret = waitpid( ctx->pid, NULL, 0 );
        } while (ret < 0 && errno == EINTR);
    }

    if (com_buf) free( com_buf->buffer );
    free( com_buf );
    return STATUS_SUCCESS;
}

static NTSTATUS ntlm_fork( void *args )
{
    const struct fork_params *params = args;
    struct ntlm_ctx *ctx = params->ctx;
    posix_spawn_file_actions_t file_actions;
    int pipe_in[2], pipe_out[2];

#ifdef HAVE_PIPE2
    if (pipe2( pipe_in, O_CLOEXEC ) < 0)
#endif
    {
        if (pipe( pipe_in ) < 0 ) return SEC_E_INTERNAL_ERROR;
        fcntl( pipe_in[0], F_SETFD, FD_CLOEXEC );
        fcntl( pipe_in[1], F_SETFD, FD_CLOEXEC );
    }
#ifdef HAVE_PIPE2
    if (pipe2( pipe_out, O_CLOEXEC ) < 0)
#endif
    {
        if (pipe( pipe_out ) < 0)
        {
            close( pipe_in[0] );
            close( pipe_in[1] );
            return SEC_E_INTERNAL_ERROR;
        }
        fcntl( pipe_out[0], F_SETFD, FD_CLOEXEC );
        fcntl( pipe_out[1], F_SETFD, FD_CLOEXEC );
    }

    posix_spawn_file_actions_init( &file_actions );

    posix_spawn_file_actions_adddup2( &file_actions, pipe_out[0], 0 );
    posix_spawn_file_actions_addclose( &file_actions, pipe_out[0] );
    posix_spawn_file_actions_addclose( &file_actions, pipe_out[1] );

    posix_spawn_file_actions_adddup2( &file_actions, pipe_in[1], 1 );
    posix_spawn_file_actions_addclose( &file_actions, pipe_in[0] );
    posix_spawn_file_actions_addclose( &file_actions, pipe_in[1] );

    if (posix_spawnp( &ctx->pid, params->argv[0], &file_actions, NULL, params->argv, environ ))
    {
        ctx->pid = -1;
        write( pipe_in[1], "BH\n", 3 );
    }

    ctx->pipe_in = pipe_in[0];
    close( pipe_in[1] );
    ctx->pipe_out = pipe_out[1];
    close( pipe_out[0] );

    posix_spawn_file_actions_destroy( &file_actions );

    return SEC_E_OK;
}

static NTSTATUS ntlm_check_version( void *args )
{
    struct ntlm_ctx ctx = { 0 };
    char *argv[3], buf[80];
    NTSTATUS status = STATUS_DLL_NOT_FOUND;
    struct fork_params params = { &ctx, argv };
    int len;

    argv[0] = (char *)"ntlm_auth";
    argv[1] = (char *)"--version";
    argv[2] = NULL;
    if (ntlm_fork( &params ) != SEC_E_OK) return status;

    if ((len = read( ctx.pipe_in, buf, sizeof(buf) - 1 )) > 8)
    {
        char *newline;

        if ((newline = memchr( buf, '\n', len ))) *newline = 0;
        else buf[len] = 0;

        TRACE( "detected ntlm_auth version %s\n", debugstr_a(buf) );
        status = STATUS_SUCCESS;
    }

    if (status) ERR_(winediag)( "ntlm_auth was not found. Make sure that ntlm_auth >= 3.0.25 is in your path. "
                                "Usually, you can find it in the winbind package of your distribution.\n" );
    ntlm_cleanup( &ctx );
    return status;
}

const unixlib_entry_t __wine_unix_call_funcs[] =
{
    ntlm_chat,
    ntlm_cleanup,
    ntlm_fork,
    ntlm_check_version,
};

C_ASSERT( ARRAYSIZE(__wine_unix_call_funcs) == unix_funcs_count );

#ifdef _WIN64

typedef ULONG PTR32;

static NTSTATUS wow64_ntlm_chat( void *args )
{
    struct
    {
        PTR32 ctx;
        PTR32 buf;
        UINT  buflen;
        PTR32 retlen;
    } const *params32 = args;

    struct chat_params params =
    {
        ULongToPtr(params32->ctx),
        ULongToPtr(params32->buf),
        params32->buflen,
        ULongToPtr(params32->retlen)
    };

    return ntlm_chat( &params );
}

static NTSTATUS wow64_ntlm_fork( void *args )
{
    struct
    {
        PTR32 ctx;
        PTR32 argv;
    } const *params32 = args;

    struct fork_params params;
    PTR32 *argv32 = ULongToPtr(params32->argv);
    char **argv;
    NTSTATUS ret;
    int i, argc = 0;

    while (argv32[argc]) argc++;
    argv = malloc( (argc + 1) * sizeof(*argv) );
    for (i = 0; i <= argc; i++) argv[i] = ULongToPtr( argv32[i] );

    params.ctx = ULongToPtr(params32->ctx);
    params.argv = argv;
    ret = ntlm_fork( &params );
    free( argv );
    return ret;
}

const unixlib_entry_t __wine_unix_call_wow64_funcs[] =
{
    wow64_ntlm_chat,
    ntlm_cleanup,
    wow64_ntlm_fork,
    ntlm_check_version,
};

C_ASSERT( ARRAYSIZE(__wine_unix_call_wow64_funcs) == unix_funcs_count );

#endif  /* _WIN64 */
