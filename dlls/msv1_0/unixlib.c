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
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/wait.h>
#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"
#include "winbase.h"
#include "sspi.h"

#include "wine/debug.h"
#include "unixlib.h"

WINE_DEFAULT_DEBUG_CHANNEL(ntlm);
WINE_DECLARE_DEBUG_CHANNEL(winediag);

#define INITIAL_BUFFER_SIZE 200

static SECURITY_STATUS read_line( struct ntlm_ctx *ctx, unsigned int *offset )
{
    char *newline;

    if (!ctx->com_buf)
    {
        if (!(ctx->com_buf = RtlAllocateHeap( GetProcessHeap(), 0, INITIAL_BUFFER_SIZE )))
            return SEC_E_INSUFFICIENT_MEMORY;
        ctx->com_buf_size = INITIAL_BUFFER_SIZE;
        ctx->com_buf_offset = 0;
    }

    do
    {
        ssize_t size;
        if (ctx->com_buf_offset + INITIAL_BUFFER_SIZE > ctx->com_buf_size)
        {
            char *buf = RtlReAllocateHeap( GetProcessHeap(), 0, ctx->com_buf, ctx->com_buf_size + INITIAL_BUFFER_SIZE );
            if (!buf) return SEC_E_INSUFFICIENT_MEMORY;
            ctx->com_buf_size += INITIAL_BUFFER_SIZE;
            ctx->com_buf = buf;
        }
        size = read( ctx->pipe_in,  ctx->com_buf +  ctx->com_buf_offset,  ctx->com_buf_size -  ctx->com_buf_offset );
        if (size <= 0) return SEC_E_INTERNAL_ERROR;

        ctx->com_buf_offset += size;
        newline = memchr( ctx->com_buf, '\n',  ctx->com_buf_offset );
    } while (!newline);

    /* if there's a newline character, and we read more than that newline, we have to store the offset so we can
       preserve the additional data */
    if (newline != ctx->com_buf + ctx->com_buf_offset) *offset = (ctx->com_buf + ctx->com_buf_offset) - (newline + 1);
    else *offset = 0;

    *newline = 0;
    return SEC_E_OK;
}

static SECURITY_STATUS CDECL ntlm_chat( struct ntlm_ctx *ctx, char *buf, unsigned int buflen, unsigned int *retlen )
{
    SECURITY_STATUS status = SEC_E_OK;
    unsigned int offset;

    write( ctx->pipe_out, buf, strlen(buf) );
    write( ctx->pipe_out, "\n", 1 );

    if ((status = read_line( ctx, &offset )) != SEC_E_OK) return status;
    *retlen = strlen( ctx->com_buf );

    if (*retlen > buflen) return SEC_E_BUFFER_TOO_SMALL;
    if (*retlen < 2) return SEC_E_ILLEGAL_MESSAGE;
    if (!strncmp( ctx->com_buf, "ERR", 3 )) return SEC_E_INVALID_TOKEN;

    memcpy( buf, ctx->com_buf, *retlen + 1 );

    if (!offset) ctx->com_buf_offset = 0;
    else
    {
        memmove( ctx->com_buf, ctx->com_buf + ctx->com_buf_offset, offset );
        ctx->com_buf_offset = offset;
    }

    return SEC_E_OK;
}

static void CDECL ntlm_cleanup( struct ntlm_ctx *ctx )
{
    if (!ctx || (ctx->mode != MODE_CLIENT && ctx->mode != MODE_SERVER)) return;
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

    RtlFreeHeap( GetProcessHeap(), 0, ctx->com_buf );
    RtlFreeHeap( GetProcessHeap(), 0, ctx );
}

static SECURITY_STATUS CDECL ntlm_fork( char **argv, struct ntlm_ctx **ret_ctx )
{
    int pipe_in[2], pipe_out[2];
    struct ntlm_ctx *ctx;

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

    if (!(ctx = RtlAllocateHeap( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*ctx) )))
    {
        close( pipe_in[0] );
        close( pipe_in[1] );
        close( pipe_out[0] );
        close( pipe_out[1] );
        return SEC_E_INSUFFICIENT_MEMORY;
    }

    if (!(ctx->pid = fork())) /* child */
    {
        dup2( pipe_out[0], 0 );
        close( pipe_out[0] );
        close( pipe_out[1] );

        dup2( pipe_in[1], 1 );
        close( pipe_in[0] );
        close( pipe_in[1] );

        execvp( argv[0], argv );

        write( 1, "BH\n", 3 );
        _exit( 1 );
    }
    else
    {
        ctx->pipe_in = pipe_in[0];
        close( pipe_in[1] );
        ctx->pipe_out = pipe_out[1];
        close( pipe_out[0] );
        *ret_ctx = ctx;
    }

    return SEC_E_OK;
}

#define NTLM_AUTH_MAJOR_VERSION 3
#define NTLM_AUTH_MINOR_VERSION 0
#define NTLM_AUTH_MICRO_VERSION 25

static BOOL check_version( void )
{
    struct ntlm_ctx *ctx;
    char *argv[3], buf[80];
    BOOL ret = FALSE;
    int len;

    argv[0] = (char *)"ntlm_auth";
    argv[1] = (char *)"--version";
    argv[2] = NULL;
    if (ntlm_fork( argv, &ctx ) != SEC_E_OK) return FALSE;

    if ((len = read( ctx->pipe_in, buf, sizeof(buf) - 1 )) > 8)
    {
        char *newline;
        int major = 0, minor = 0, micro = 0;

        if ((newline = memchr( buf, '\n', len ))) *newline = 0;
        else buf[len] = 0;

        if (sscanf( buf, "Version %d.%d.%d", &major, &minor, &micro ) == 3)
        {
            if (((major > NTLM_AUTH_MAJOR_VERSION) ||
                 (major == NTLM_AUTH_MAJOR_VERSION && minor > NTLM_AUTH_MINOR_VERSION) ||
                 (major == NTLM_AUTH_MAJOR_VERSION && minor == NTLM_AUTH_MINOR_VERSION &&
                  micro >= NTLM_AUTH_MICRO_VERSION)))
            {
                TRACE( "detected ntlm_auth version %d.%d.%d\n", major, minor, micro );
                ret = TRUE;
            }
        }
    }

    if (!ret) ERR_(winediag)( "ntlm_auth was not found or is outdated. "
                              "Make sure that ntlm_auth >= %d.%d.%d is in your path. "
                              "Usually, you can find it in the winbind package of your distribution.\n",
                              NTLM_AUTH_MAJOR_VERSION, NTLM_AUTH_MINOR_VERSION, NTLM_AUTH_MICRO_VERSION );
    ntlm_cleanup( ctx );
    return ret;
}

static const struct ntlm_funcs funcs =
{
    ntlm_chat,
    ntlm_cleanup,
    ntlm_fork,
};

NTSTATUS CDECL __wine_init_unix_lib( HMODULE module, DWORD reason, const void *ptr_in, void *ptr_out )
{
    if (reason != DLL_PROCESS_ATTACH) return STATUS_SUCCESS;
    if (!check_version()) return STATUS_DLL_NOT_FOUND;
    *(const struct ntlm_funcs **)ptr_out = &funcs;
    return STATUS_SUCCESS;
}
