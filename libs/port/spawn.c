/*
 * spawnvp function
 *
 * Copyright 2003 Dimitrie O. Paun
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

#if !defined(HAVE__SPAWNVP) && (!defined(_WIN32) || defined(__CYGWIN__))

#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif
#include <sys/stat.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

int _spawnvp(int mode, const char *cmdname, const char *const argv[])
{
    int pid, status, wret;

    if (mode == _P_OVERLAY)
    {
        execvp(cmdname, (char **)argv);
        /* if we get here it failed */
#ifdef ENOTSUP
        if (errno != ENOTSUP)  /* exec fails on MacOS if the process has multiple threads */
#endif
            return -1;
    }

    pid = fork();
    if (pid == 0)
    {
        /* in child */
        if (mode == _P_DETACH)
        {
            pid = fork();
            if (pid == -1) _exit(1);
            else if (pid > 0) _exit(0);
            /* else in grandchild */
        }

        signal( SIGPIPE, SIG_DFL );
        execvp(cmdname, (char **)argv);
        _exit(1);
    }

    if (pid == -1)
        return -1;

    if (mode == _P_OVERLAY) exit(0);

    if (mode == _P_WAIT || mode == _P_DETACH)
    {
        while (pid != (wret = waitpid(pid, &status, 0)))
            if (wret == -1 && errno != EINTR) break;

        if (pid == wret && WIFEXITED(status))
        {
            if (mode == _P_WAIT)
                pid = WEXITSTATUS(status);
            else /* mode == _P_DETACH */
                if (WEXITSTATUS(status) != 0) /* child couldn't fork grandchild */
                    pid = -1;
        }
        else
        {
            if (mode == _P_WAIT)
                pid = 255; /* abnormal exit with an abort or an interrupt */
            else /* mode == _P_DETACH */
                pid = -1;
        }
    }

    return pid;
}

#endif  /* HAVE__SPAWNVP */
