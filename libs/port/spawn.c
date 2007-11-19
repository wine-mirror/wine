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

#ifndef HAVE_SPAWNVP
int spawnvp(int mode, const char *cmdname, const char *const argv[])
{
#ifndef HAVE__SPAWNVP
    int pid = 0, status, wret;
    struct sigaction dfl_act, old_act;

    if (mode == _P_OVERLAY)
    {
        execvp(cmdname, (char **)argv);
        /* if we get here it failed */
        if (errno != ENOTSUP) return -1;  /* exec fails on MacOS if the process has multiple threads */
    }

    dfl_act.sa_handler = SIG_DFL;
    dfl_act.sa_flags = 0;
    sigemptyset( &dfl_act.sa_mask );

    if (mode == _P_WAIT) sigaction( SIGCHLD, &dfl_act, &old_act );

    pid = fork();
    if (pid == 0)
    {
        sigaction( SIGPIPE, &dfl_act, NULL );
        execvp(cmdname, (char **)argv);
        _exit(1);
    }

    if (pid != -1 && mode == _P_OVERLAY) exit(0);

    if (pid != -1 && mode == _P_WAIT)
    {
        while (pid != (wret = waitpid(pid, &status, 0)))
            if (wret == -1 && errno != EINTR) break;

        if (pid == wret && WIFEXITED(status)) pid = WEXITSTATUS(status);
        else pid = 255; /* abnormal exit with an abort or an interrupt */
    }

    if (mode == _P_WAIT) sigaction( SIGCHLD, &old_act, NULL );
    return pid;
#else   /* HAVE__SPAWNVP */
    return _spawnvp(mode, cmdname, argv);
#endif  /* HAVE__SPAWNVP */
}
#endif  /* HAVE_SPAWNVP */
