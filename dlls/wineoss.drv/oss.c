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
#include <errno.h>
#include <sys/soundcard.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "winternl.h"
#include "audioclient.h"

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

/* dst must be large enough to hold devnode */
static void oss_clean_devnode(char *dest, const char *devnode)
{
    const char *dot, *slash;
    size_t len;

    strcpy(dest, devnode);
    dot = strrchr(dest, '.');
    if(!dot)
        return;

    slash = strrchr(dest, '/');
    if(slash && dot < slash)
        return;

    len = dot - dest;
    dest[len] = '\0';
}

static int open_device(const char *device, EDataFlow flow)
{
    int flags = ((flow == eRender) ? O_WRONLY : O_RDONLY) | O_NONBLOCK;

    return open(device, flags, 0);
}

static void get_default_device(EDataFlow flow, char device[OSS_DEVNODE_SIZE])
{
    int fd, err;
    oss_audioinfo ai;

    device[0] = '\0';
    fd = open_device("/dev/dsp", flow);
    if(fd < 0){
        WARN("Couldn't open default device!\n");
        return;
    }

    ai.dev = -1;
    if((err = ioctl(fd, SNDCTL_ENGINEINFO, &ai)) < 0){
        WARN("SNDCTL_ENGINEINFO failed: %d (%s)\n", err, strerror(errno));
        close(fd);
        return;
    }
    close(fd);

    TRACE("Default devnode: %s\n", ai.devnode);
    oss_clean_devnode(device, ai.devnode);
    return;
}

static NTSTATUS get_endpoint_ids(void *args)
{
    struct get_endpoint_ids_params *params = args;
    oss_sysinfo sysinfo;
    oss_audioinfo ai;
    static int print_once = 0;
    static const WCHAR outW[] = {'O','u','t',':',' ',0};
    static const WCHAR inW[] = {'I','n',':',' ',0};
    struct endpoint_info
    {
        WCHAR name[ARRAY_SIZE(ai.name) + ARRAY_SIZE(outW)];
        char device[OSS_DEVNODE_SIZE];
    } *info;
    unsigned int i, j, num, needed, name_len, device_len, default_idx = 0;
    char default_device[OSS_DEVNODE_SIZE];
    struct endpoint *endpoint;
    int mixer_fd;
    char *ptr;

    mixer_fd = open("/dev/mixer", O_RDONLY, 0);
    if(mixer_fd < 0){
        ERR("OSS /dev/mixer doesn't seem to exist\n");
        params->result = AUDCLNT_E_SERVICE_NOT_RUNNING;
        return STATUS_SUCCESS;
    }

    if(ioctl(mixer_fd, SNDCTL_SYSINFO, &sysinfo) < 0){
        close(mixer_fd);
        if(errno == EINVAL){
            ERR("OSS version too old, need at least OSSv4\n");
            params->result = AUDCLNT_E_SERVICE_NOT_RUNNING;
            return STATUS_SUCCESS;
        }

        ERR("Error getting SNDCTL_SYSINFO: %d (%s)\n", errno, strerror(errno));
        params->result = E_FAIL;
        return STATUS_SUCCESS;
    }

    if(!print_once){
        TRACE("OSS sysinfo:\n");
        TRACE("product: %s\n", sysinfo.product);
        TRACE("version: %s\n", sysinfo.version);
        TRACE("versionnum: %x\n", sysinfo.versionnum);
        TRACE("numaudios: %d\n", sysinfo.numaudios);
        TRACE("nummixers: %d\n", sysinfo.nummixers);
        TRACE("numcards: %d\n", sysinfo.numcards);
        TRACE("numaudioengines: %d\n", sysinfo.numaudioengines);
        print_once = 1;
    }

    if(sysinfo.numaudios <= 0){
        WARN("No audio devices!\n");
        close(mixer_fd);
        params->result = AUDCLNT_E_SERVICE_NOT_RUNNING;
        return STATUS_SUCCESS;
    }

    info = malloc(sysinfo.numaudios * sizeof(*info));
    if(!info){
        close(mixer_fd);
        params->result = E_OUTOFMEMORY;
        return STATUS_SUCCESS;
    }

    get_default_device(params->flow, default_device);

    num = 0;
    for(i = 0; i < sysinfo.numaudios; ++i){
        char devnode[OSS_DEVNODE_SIZE];
        int fd, prefix_len;
        const WCHAR *prefix;

        memset(&ai, 0, sizeof(ai));
        ai.dev = i;
        if(ioctl(mixer_fd, SNDCTL_AUDIOINFO, &ai) < 0){
            WARN("Error getting AUDIOINFO for dev %d: %d (%s)\n", i, errno,
                    strerror(errno));
            continue;
        }

        oss_clean_devnode(devnode, ai.devnode);

        /* check for duplicates */
        for(j = 0; j < num; j++)
            if(!strcmp(devnode, info[j].device))
                break;
        if(j < num)
            continue;

        fd = open_device(devnode, params->flow);
        if(fd < 0){
            WARN("Opening device \"%s\" failed, pretending it doesn't exist: %d (%s)\n",
                    devnode, errno, strerror(errno));
            continue;
        }
        close(fd);

        if((params->flow == eCapture && !(ai.caps & PCM_CAP_INPUT)) ||
           (params->flow == eRender && !(ai.caps & PCM_CAP_OUTPUT)))
            continue;

        strcpy(info[num].device, devnode);

        if(params->flow == eRender){
            prefix = outW;
            prefix_len = ARRAY_SIZE(outW) - 1;
        }else{
            prefix = inW;
            prefix_len = ARRAY_SIZE(inW) - 1;
        }
        memcpy(info[num].name, prefix, prefix_len * sizeof(WCHAR));
        ntdll_umbstowcs(ai.name, strlen(ai.name) + 1, info[num].name + prefix_len,
                        ARRAY_SIZE(info[num].name) - prefix_len);
        if(!strcmp(default_device, info[num].device))
            default_idx = num;
        num++;
    }
    close(mixer_fd);

    needed = num * sizeof(*params->endpoints);
    endpoint = params->endpoints;
    ptr = (char *)(endpoint + num);

    for(i = 0; i < num; i++){
        name_len = wcslen(info[i].name) + 1;
        device_len = strlen(info[i].device) + 1;
        needed += name_len * sizeof(WCHAR) + ((device_len + 1) & ~1);

        if(needed <= params->size){
            endpoint->name = (WCHAR *)ptr;
            memcpy(endpoint->name, info[i].name, name_len * sizeof(WCHAR));
            ptr += name_len * sizeof(WCHAR);
            endpoint->device = ptr;
            memcpy(endpoint->device, info[i].device, device_len);
            ptr += (device_len + 1) & ~1;
            endpoint++;
        }
    }
    free(info);

    params->num = num;
    params->default_idx = default_idx;

    if(needed > params->size){
        params->size = needed;
        params->result = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
    } else
        params->result = S_OK;

    return STATUS_SUCCESS;
}

unixlib_entry_t __wine_unix_call_funcs[] =
{
    test_connect,
    get_endpoint_ids,
};
