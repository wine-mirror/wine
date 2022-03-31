/*
 * OSS driver (unixlib)
 *
 * Copyright 2011 Andrew Eikum for CodeWeavers
 *           2022 Huw Davies
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
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/soundcard.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "winternl.h"

#include "wine/debug.h"
#include "wine/unixlib.h"

#include "unixlib.h"

WINE_DEFAULT_DEBUG_CHANNEL(oss);

static NTSTATUS test_connect(void *args)
{
    struct test_connect_params *params = args;
    int mixer_fd;
    oss_sysinfo sysinfo;

    /* Attempt to determine if we are running on OSS or ALSA's OSS
     * compatibility layer. There is no official way to do that, so just check
     * for validity as best as possible, without rejecting valid OSS
     * implementations. */

    mixer_fd = open("/dev/mixer", O_RDONLY, 0);
    if(mixer_fd < 0){
        TRACE("Priority_Unavailable: open failed\n");
        params->priority = Priority_Unavailable;
        return STATUS_SUCCESS;
    }

    sysinfo.version[0] = 0xFF;
    sysinfo.versionnum = ~0;
    if(ioctl(mixer_fd, SNDCTL_SYSINFO, &sysinfo) < 0){
        TRACE("Priority_Unavailable: ioctl failed\n");
        close(mixer_fd);
        params->priority = Priority_Unavailable;
        return STATUS_SUCCESS;
    }

    close(mixer_fd);

    if(sysinfo.version[0] < '4' || sysinfo.version[0] > '9'){
        TRACE("Priority_Low: sysinfo.version[0]: %x\n", sysinfo.version[0]);
        params->priority = Priority_Low;
        return STATUS_SUCCESS;
    }
    if(sysinfo.versionnum & 0x80000000){
        TRACE("Priority_Low: sysinfo.versionnum: %x\n", sysinfo.versionnum);
        params->priority = Priority_Low;
        return STATUS_SUCCESS;
    }

    TRACE("Priority_Preferred: Seems like valid OSS!\n");

    params->priority = Priority_Preferred;
    return STATUS_SUCCESS;
}

unixlib_entry_t __wine_unix_call_funcs[] =
{
    test_connect,
};
