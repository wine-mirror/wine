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
#include <dlfcn.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>
#ifdef HAVE_CUPS_CUPS_H
#include <cups/cups.h>
#endif

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "wine/debug.h"

#include "localspl_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(localspl);

/* cups.h before version 1.7.0 doesn't have HTTP_STATUS_CONTINUE */
#define HTTP_STATUS_CONTINUE 100

#ifdef SONAME_LIBCUPS

static void *libcups_handle;

#define CUPS_FUNCS \
    DO_FUNC(cupsAddOption); \
    DO_FUNC(cupsCreateJob); \
    DO_FUNC(cupsFinishDocument); \
    DO_FUNC(cupsFreeDests); \
    DO_FUNC(cupsFreeOptions); \
    DO_FUNC(cupsGetOption); \
    DO_FUNC(cupsParseOptions); \
    DO_FUNC(cupsStartDocument); \
    DO_FUNC(cupsWriteRequestData)
#define CUPS_OPT_FUNCS \
    DO_FUNC(cupsGetNamedDest); \
    DO_FUNC(cupsLastErrorString)

#define DO_FUNC(f) static typeof(f) *p##f
CUPS_FUNCS;
#undef DO_FUNC
static cups_dest_t * (*pcupsGetNamedDest)(http_t *, const char *, const char *);
static const char *  (*pcupsLastErrorString)(void);

#endif /* SONAME_LIBCUPS */

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
        struct
        {
            int fd;
        } unixname;
#ifdef SONAME_LIBCUPS
        struct
        {
            char *queue;
            char *doc_title;
            enum
            {
                doc_parse_header = 0,
                doc_parse_options,
                doc_create_job,
                doc_initialized,
            } state;
            BOOL restore_ps_header;
            int num_options;
            cups_option_t *options;
            int buf_len;
            char buf[257]; /* DSC max of 256 + '\0' */
        } cups;
#endif
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

static BOOL unixname_write_doc(doc_t *doc, const BYTE *buf, unsigned int size)
{
    return write(doc->unixname.fd, buf, size) == size;
}

static BOOL unixname_end_doc(doc_t *doc)
{
    close(doc->unixname.fd);
    return TRUE;
}

static BOOL unixname_start_doc(doc_t *doc, const WCHAR *output)
{
    char *outputA;
    DWORD len;

    doc->write_doc = unixname_write_doc;
    doc->end_doc = unixname_end_doc;

    len = wcslen(output);
    outputA = malloc(len * 3 + 1);
    ntdll_wcstoumbs(output, len + 1, outputA, len * 3 + 1, FALSE);

    doc->unixname.fd = open(outputA, O_CREAT | O_TRUNC | O_WRONLY, 0666);
    free(outputA);

    return doc->unixname.fd != -1;
}

static BOOL lpr_start_doc(doc_t *doc, const WCHAR *printer_name)
{
    static const WCHAR lpr[] = { 'l','p','r',' ','-','P','\'' };
    static const WCHAR quote[] = { '\'',0 };
    int printer_len = wcslen(printer_name);
    WCHAR *cmd;
    BOOL ret;

    cmd = malloc(printer_len * sizeof(WCHAR) + sizeof(lpr) + sizeof(quote));
    memcpy(cmd, lpr, sizeof(lpr));
    memcpy(cmd + ARRAY_SIZE(lpr), printer_name, printer_len * sizeof(WCHAR));
    memcpy(cmd + ARRAY_SIZE(lpr) + printer_len, quote, sizeof(quote));
    ret = pipe_start_doc(doc, cmd);
    free(cmd);
    return ret;
}

#ifdef SONAME_LIBCUPS
static int get_cups_default_options(const char *printer, int num_options, cups_option_t **options)
{
    cups_dest_t *dest;
    int i;

    if (!pcupsGetNamedDest) return num_options;

    dest = pcupsGetNamedDest(NULL, printer, NULL);
    if (!dest) return num_options;

    for (i = 0; i < dest->num_options; i++)
    {
        if (!pcupsGetOption(dest->options[i].name, num_options, *options))
            num_options = pcupsAddOption(dest->options[i].name,
                    dest->options[i].value, num_options, options);
    }

    pcupsFreeDests(1, dest);
    return num_options;
}

static BOOL cups_gets(doc_t *doc, const BYTE **buf, unsigned int *size)
{
    BYTE b;

    while(doc->cups.buf_len < sizeof(doc->cups.buf) && *size)
    {
        b = (*buf)[0];
        doc->cups.buf[doc->cups.buf_len++] = b;
        (*buf)++;
        (*size)--;

        if (b == '\n')
            return TRUE;
    }
    return FALSE;
}

static BOOL cups_write(const char *buf, unsigned int size)
{
    if (!size)
        return TRUE;

    if (pcupsWriteRequestData(CUPS_HTTP_DEFAULT, buf, size) != HTTP_STATUS_CONTINUE)
    {
        if (pcupsLastErrorString)
            WARN("cupsWriteRequestData failed: %s\n", debugstr_a(pcupsLastErrorString()));
        return FALSE;
    }
    return TRUE;
}

static BOOL cups_write_doc(doc_t *doc, const BYTE *buf, unsigned int size)
{
    const char ps_adobe[] = "%!PS-Adobe-3.0\n";
    const char cups_job[] = "%cupsJobTicket:";

    if (doc->cups.state == doc_parse_header)
    {
        if (!cups_gets(doc, &buf, &size))
        {
            if (doc->cups.buf_len != sizeof(doc->cups.buf))
                return TRUE;
            doc->cups.state = doc_create_job;
        }
        else if (!strncmp(doc->cups.buf, ps_adobe, sizeof(ps_adobe) - 1))
        {
            doc->cups.restore_ps_header = TRUE;
            doc->cups.state = doc_parse_options;
            doc->cups.buf_len = 0;
        }
        else
        {
            doc->cups.state = doc_create_job;
        }
    }

     /* Explicitly set CUPS options based on any %cupsJobTicket lines.
      * The CUPS scheduler only looks for these in Print-File requests, and since
      * we use Create-Job / Send-Document, the ticket lines don't get parsed.
      */
    if (doc->cups.state == doc_parse_options)
    {
        while (1)
        {
            if (!cups_gets(doc, &buf, &size))
            {
                if (doc->cups.buf_len != sizeof(doc->cups.buf))
                    return TRUE;
                doc->cups.state =  doc_create_job;
                break;
            }
            else if (!strncmp(doc->cups.buf, cups_job, sizeof(cups_job) - 1))
            {
                doc->cups.buf[doc->cups.buf_len - 1] = 0;
                doc->cups.num_options = pcupsParseOptions(doc->cups.buf + sizeof(cups_job) - 1,
                        doc->cups.num_options, &doc->cups.options);
                doc->cups.buf_len = 0;
            }
            else
            {
                doc->cups.state = doc_create_job;
                break;
            }
        }
    }

    if (doc->cups.state == doc_create_job)
    {
        const char *format;
        int i, job_id;

        doc->cups.num_options = get_cups_default_options(doc->cups.queue,
                doc->cups.num_options, &doc->cups.options);

        TRACE("printing via cups with options:\n");
        for (i = 0; i < doc->cups.num_options; i++)
            TRACE("\t%d: %s = %s\n", i, doc->cups.options[i].name, doc->cups.options[i].value);

        if (pcupsGetOption("raw", doc->cups.num_options, doc->cups.options))
            format = CUPS_FORMAT_RAW;
        else if (!(format = pcupsGetOption("document-format", doc->cups.num_options, doc->cups.options)))
            format = CUPS_FORMAT_AUTO;

        job_id = pcupsCreateJob(CUPS_HTTP_DEFAULT, doc->cups.queue,
                doc->cups.doc_title, doc->cups.num_options, doc->cups.options);
        if (!job_id)
        {
            if (pcupsLastErrorString)
                WARN("cupsCreateJob failed: %s\n", debugstr_a(pcupsLastErrorString()));
            return FALSE;
        }

        if (pcupsStartDocument(CUPS_HTTP_DEFAULT, doc->cups.queue, job_id,
                    NULL, format, TRUE) != HTTP_STATUS_CONTINUE)
        {
            if (pcupsLastErrorString)
                WARN("cupsStartDocument failed: %s\n", debugstr_a(pcupsLastErrorString()));
            return FALSE;
        }

        doc->cups.state = doc_initialized;
    }

    if (doc->cups.restore_ps_header)
    {
        if (!cups_write(ps_adobe, sizeof(ps_adobe) - 1))
            return FALSE;
        doc->cups.restore_ps_header = FALSE;
    }

    if (doc->cups.buf_len)
    {
        if (!cups_write(doc->cups.buf, doc->cups.buf_len))
            return FALSE;
        doc->cups.buf_len = 0;
    }

    return cups_write((const char *)buf, size);
}

static BOOL cups_end_doc(doc_t *doc)
{
    if (doc->cups.buf_len)
    {
        if (doc->cups.state != doc_initialized)
            doc->cups.state = doc_create_job;
        cups_write_doc(doc, NULL, 0);
    }

    if (doc->cups.state == doc_initialized)
        pcupsFinishDocument(CUPS_HTTP_DEFAULT, doc->cups.queue);

    free(doc->cups.queue);
    free(doc->cups.doc_title);
    pcupsFreeOptions(doc->cups.num_options, doc->cups.options);
    return TRUE;
}
#endif

static BOOL cups_start_doc(doc_t *doc, const WCHAR *printer_name, const WCHAR *document_title)
{
#ifdef SONAME_LIBCUPS
    if (pcupsWriteRequestData)
    {
        int len;

        doc->write_doc = cups_write_doc;
        doc->end_doc = cups_end_doc;

        len = wcslen(printer_name);
        doc->cups.queue = malloc(len * 3 + 1);
        ntdll_wcstoumbs(printer_name, len + 1, doc->cups.queue, len * 3 + 1, FALSE);

        len = wcslen(document_title);
        doc->cups.doc_title = malloc(len * 3 + 1);
        ntdll_wcstoumbs(document_title, len + 1, doc->cups.doc_title, len + 3 + 1, FALSE);

        return TRUE;
    }
#endif

    return lpr_start_doc(doc, printer_name);
}

static NTSTATUS process_attach(void *args)
{
#ifdef SONAME_LIBCUPS
    libcups_handle = dlopen(SONAME_LIBCUPS, RTLD_NOW);
    TRACE("%p: %s loaded\n", libcups_handle, SONAME_LIBCUPS);
    if (!libcups_handle) return STATUS_DLL_NOT_FOUND;

#define DO_FUNC(x) \
    p##x = dlsym(libcups_handle, #x); \
    if (!p##x) \
    { \
        ERR("failed to load symbol %s\n", #x); \
        libcups_handle = NULL; \
        return STATUS_ENTRYPOINT_NOT_FOUND; \
    }
    CUPS_FUNCS;
#undef DO_FUNC
#define DO_FUNC(x) p##x = dlsym(libcups_handle, #x)
    CUPS_OPT_FUNCS;
#undef DO_FUNC
    return STATUS_SUCCESS;
#else /* SONAME_LIBCUPS */
    return STATUS_NOT_SUPPORTED;
#endif /* SONAME_LIBCUPS */
}

static NTSTATUS start_doc(void *args)
{
    const struct start_doc_params *params = args;
    doc_t *doc = calloc(1, sizeof(*doc));
    BOOL ret = FALSE;

    if (!doc) return STATUS_NO_MEMORY;

    if (params->type == PORT_IS_PIPE)
        ret = pipe_start_doc(doc, params->port + 1 /* strlen("|") */);
    else if (params->type == PORT_IS_UNIXNAME)
        ret = unixname_start_doc(doc, params->port);
    else if (params->type == PORT_IS_LPR)
        ret = lpr_start_doc(doc, params->port + 4 /* strlen("lpr:") */);
    else if (params->type == PORT_IS_CUPS)
        ret = cups_start_doc(doc, params->port + 5 /*strlen("cups:") */,
                params->document_title);

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
    process_attach,
    start_doc,
    write_doc,
    end_doc,
};

C_ASSERT( ARRAYSIZE(__wine_unix_call_funcs) == unix_funcs_count );

#ifdef _WIN64

typedef ULONG PTR32;

static NTSTATUS wow64_start_doc(void *args)
{
    struct
    {
        unsigned int type;
        PTR32 port;
        PTR32 document_title;
        PTR32 doc;
    } const *params32 = args;

    struct start_doc_params params =
    {
        params32->type,
        ULongToPtr(params32->port),
        ULongToPtr(params32->document_title),
        ULongToPtr(params32->doc),
    };

    return start_doc(&params);
}

static NTSTATUS wow64_write_doc(void *args)
{
    struct
    {
        INT64 doc;
        PTR32 buf;
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
    process_attach,
    wow64_start_doc,
    wow64_write_doc,
    end_doc,
};

C_ASSERT( ARRAYSIZE(__wine_unix_call_wow64_funcs) == unix_funcs_count );

#endif  /* _WIN64 */
