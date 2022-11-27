/*
 * CUPS functions
 *
 * Copyright 2021 Huw Davies
 * Copyright 2022 Piotr Caban
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

#include "config.h"

#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "wine/debug.h"

#include "localspl_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(localspl);

typedef struct _doc_t
{
    BOOL (*write_doc)(struct _doc_t *, const BYTE *buf, unsigned int size);
    BOOL (*end_doc)(struct _doc_t *);

    union
    {
        struct
        {
            pid_t pid;
            int fd;
        } pipe;
    };
} doc_t;

static BOOL pipe_write_doc(doc_t *doc, const BYTE *buf, unsigned int size)
{
    return write(doc->pipe.fd, buf, size) == size;
}

static BOOL pipe_end_doc(doc_t *doc)
{
    pid_t wret;
    int status;

    close(doc->pipe.fd);

    do {
        wret = waitpid(doc->pipe.pid, &status, 0);
    } while (wret < 0 && errno == EINTR);
    if (wret < 0)
    {
        ERR("waitpid() failed!\n");
        return FALSE;
    }
    if (!WIFEXITED(status) || WEXITSTATUS(status))
    {
        ERR("child process failed! %d\n", status);
        return FALSE;
    }

    return TRUE;
}

static BOOL pipe_start_doc(doc_t *doc, const WCHAR *cmd)
{
    char *cmdA;
    int fds[2];
    DWORD len;

    doc->write_doc = pipe_write_doc;
    doc->end_doc = pipe_end_doc;

    len = wcslen(cmd);
    cmdA = malloc(len * 3 + 1);
    ntdll_wcstoumbs(cmd, len + 1, cmdA, len * 3 + 1, FALSE);

    TRACE("printing with: %s\n", cmdA);

    if (pipe(fds))
    {
        ERR("pipe() failed!\n");
        free(cmdA);
        return FALSE;
    }

    if ((doc->pipe.pid = fork()) == 0)
    {
        close(0);
        dup2(fds[0], 0);
        close(fds[1]);

        /* reset signals that we previously set to SIG_IGN */
        signal(SIGPIPE, SIG_DFL);

        execl("/bin/sh", "/bin/sh", "-c", cmdA, NULL);
        _exit(1);
    }
    close(fds[0]);
    free(cmdA);
    if (doc->pipe.pid == -1)
    {
        ERR("fork() failed!\n");
        close(fds[1]);
        return FALSE;
    }

    doc->pipe.fd = fds[1];
    return TRUE;
}

static NTSTATUS start_doc(void *args)
{
    const struct start_doc_params *params = args;
    doc_t *doc = malloc(sizeof(*doc));
    BOOL ret = FALSE;

    if (!doc) return STATUS_NO_MEMORY;

    if (params->type == PORT_IS_PIPE)
        ret = pipe_start_doc(doc, params->port + 1 /* strlen("|") */);

    if (ret)
        *params->doc = (size_t)doc;
    else
        free(doc);
    return ret;
}

static NTSTATUS write_doc(void *args)
{
    const struct write_doc_params *params = args;
    doc_t *doc  = (doc_t *)(size_t)params->doc;

    return doc->write_doc(doc, params->buf, params->size);
}

static NTSTATUS end_doc(void *args)
{
    const struct end_doc_params *params = args;
    doc_t *doc = (doc_t *)(size_t)params->doc;
    NTSTATUS ret;

    ret = doc->end_doc(doc);
    free(doc);
    return ret;
}

const unixlib_entry_t __wine_unix_call_funcs[] =
{
    start_doc,
    write_doc,
    end_doc,
};

#ifdef _WIN64

typedef ULONG PTR32;

static NTSTATUS wow64_start_doc(void *args)
{
    struct
    {
        unsigned int type;
        const PTR32 port;
        INT64 *doc;
    } const *params32 = args;

    struct start_doc_params params =
    {
        params32->type,
        ULongToPtr(params32->port),
        params32->doc,
    };

    return start_doc(&params);
}

static NTSTATUS wow64_write_doc(void *args)
{
    struct
    {
        INT64 doc;
        const PTR32 buf;
        unsigned int size;
    } const *params32 = args;

    struct write_doc_params params =
    {
        params32->doc,
        ULongToPtr(params32->buf),
        params32->size,
    };

    return write_doc(&params);
}

const unixlib_entry_t __wine_unix_call_wow64_funcs[] =
{
    wow64_start_doc,
    wow64_write_doc,
    end_doc,
};

#endif  /* _WIN64 */
