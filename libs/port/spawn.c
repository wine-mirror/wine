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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"
#include "wine/port.h"

#include <errno.h>
#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif
#include <sys/stat.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

int spawnvp(int mode, const char *cmdname, char *const argv[])
{
#ifndef HAVE__SPAWNVP
    int pid = 0, status, wret;

    if (mode != _P_OVERLAY) pid = fork();
    if (pid == 0) pid = execvp(argv[0], argv);
    if (pid < 0) return -1;

    if (mode != _P_WAIT) return pid;

    while (pid != (wret = waitpid(pid, &status, 0)))
        if (wret == -1 && errno != EINTR) break;

    if (pid == wret && WIFEXITED(status)) return WEXITSTATUS(status);

    return 255; /* abnormal exit with an abort or an interrupt */
#else   /* HAVE__SPAWNVP */
    return _spawnvp(mode, cmdname, argv);
#endif  /* HAVE__SPAWNVP */
}
