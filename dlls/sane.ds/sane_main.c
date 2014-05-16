/*
 * SANE.DS functions
 *
 * Copyright 2000 Shi Quan He <shiquan@cyberdude.com>
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

#include <stdarg.h>
#include <stdio.h>

#include "sane_i.h"
#include "wine/debug.h"
#include "wine/library.h"

WINE_DEFAULT_DEBUG_CHANNEL(twain);

#ifdef SONAME_LIBSANE

HINSTANCE SANE_instance;

static void *libsane_handle;

static void close_libsane(void *h)
{
    if (h)
        wine_dlclose(h, NULL, 0);
}

static void *open_libsane(void)
{
    void *h;

    h = wine_dlopen(SONAME_LIBSANE, RTLD_GLOBAL | RTLD_NOW, NULL, 0);
    if (!h)
    {
        WARN("dlopen(%s) failed\n", SONAME_LIBSANE);
        return NULL;
    }

#define LOAD_FUNCPTR(f) \
    if((p##f = wine_dlsym(h, #f, NULL, 0)) == NULL) { \
        close_libsane(h); \
        ERR("Could not dlsym %s\n", #f); \
        return NULL; \
    }

    LOAD_FUNCPTR(sane_init)
    LOAD_FUNCPTR(sane_exit)
    LOAD_FUNCPTR(sane_get_devices)
    LOAD_FUNCPTR(sane_open)
    LOAD_FUNCPTR(sane_close)
    LOAD_FUNCPTR(sane_get_option_descriptor)
    LOAD_FUNCPTR(sane_control_option)
    LOAD_FUNCPTR(sane_get_parameters)
    LOAD_FUNCPTR(sane_start)
    LOAD_FUNCPTR(sane_read)
    LOAD_FUNCPTR(sane_cancel)
    LOAD_FUNCPTR(sane_set_io_mode)
    LOAD_FUNCPTR(sane_get_select_fd)
    LOAD_FUNCPTR(sane_strstatus)
#undef LOAD_FUNCPTR

    return h;
}

BOOL WINAPI DllMain (HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    TRACE("%p,%x,%p\n", hinstDLL, fdwReason, lpvReserved);

    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH: {
	    SANE_Int version_code;

            libsane_handle = open_libsane();
            if (! libsane_handle)
                return FALSE;

	    psane_init (&version_code, NULL);
	    SANE_instance = hinstDLL;
            DisableThreadLibraryCalls(hinstDLL);
            break;
	}
        case DLL_PROCESS_DETACH:
            if (lpvReserved) break;
            TRACE("calling sane_exit()\n");
	    psane_exit ();
            close_libsane(libsane_handle);
            break;
    }

    return TRUE;
}

static TW_UINT16 SANE_GetIdentity( pTW_IDENTITY, pTW_IDENTITY);
static TW_UINT16 SANE_OpenDS( pTW_IDENTITY, pTW_IDENTITY);

#endif /* SONAME_LIBSANE */

static TW_UINT16 SANE_SourceControlHandler (
           pTW_IDENTITY pOrigin,
           TW_UINT16    DAT,
           TW_UINT16    MSG,
           TW_MEMREF    pData)
{
    TW_UINT16 twRC = TWRC_SUCCESS;

    switch (DAT)
    {
	case DAT_IDENTITY:
	    switch (MSG)
	    {
		case MSG_CLOSEDS:
#ifdef SONAME_LIBSANE
		     psane_close (activeDS.deviceHandle);
#else
		     twRC = TWRC_FAILURE;
                     activeDS.twCC = TWCC_CAPUNSUPPORTED;
#endif
		     break;
		case MSG_OPENDS:
#ifdef SONAME_LIBSANE
		     twRC = SANE_OpenDS( pOrigin, (pTW_IDENTITY)pData);
#else
		     twRC = TWRC_FAILURE;
                     activeDS.twCC = TWCC_CAPUNSUPPORTED;
#endif
		     break;
		case MSG_GET:
#ifdef SONAME_LIBSANE
		     twRC = SANE_GetIdentity( pOrigin, (pTW_IDENTITY)pData);
#else
		     twRC = TWRC_FAILURE;
                     activeDS.twCC = TWCC_CAPUNSUPPORTED;
#endif
		     break;
	    }
	    break;
        case DAT_CAPABILITY:
            switch (MSG)
            {
                case MSG_GET:
                    twRC = SANE_CapabilityGet (pOrigin, pData);
                    break;
                case MSG_GETCURRENT:
                    twRC = SANE_CapabilityGetCurrent (pOrigin, pData);
                    break;
                case MSG_GETDEFAULT:
                    twRC = SANE_CapabilityGetDefault (pOrigin, pData);
                    break;
                case MSG_QUERYSUPPORT:
                    twRC = SANE_CapabilityQuerySupport (pOrigin, pData);
                    break;
                case MSG_RESET:
                    twRC = SANE_CapabilityReset (pOrigin, pData);
                    break;
                case MSG_SET:
                    twRC = SANE_CapabilitySet (pOrigin, pData);
                    break;
                default:
                    twRC = TWRC_FAILURE;
                    activeDS.twCC = TWCC_CAPBADOPERATION;
                    FIXME("unrecognized opertion triplet\n");
                    break;
            }
            break;

        case DAT_EVENT:
            if (MSG == MSG_PROCESSEVENT)
                twRC = SANE_ProcessEvent (pOrigin, pData);
            else
            {
                activeDS.twCC = TWCC_CAPBADOPERATION;
                twRC = TWRC_FAILURE;
            }
            break;

        case DAT_PENDINGXFERS:
            switch (MSG)
            {
                case MSG_ENDXFER:
                    twRC = SANE_PendingXfersEndXfer (pOrigin, pData);
                    break;
                case MSG_GET:
                    twRC = SANE_PendingXfersGet (pOrigin, pData);
                    break;
                case MSG_RESET:
                    twRC = SANE_PendingXfersReset (pOrigin, pData);
                    break;
                default:
                    activeDS.twCC = TWCC_CAPBADOPERATION;
                    twRC = TWRC_FAILURE;
            }
            break;

        case DAT_SETUPMEMXFER:
            if (MSG == MSG_GET)
                twRC = SANE_SetupMemXferGet (pOrigin, pData);
            else
            {
                activeDS.twCC = TWCC_CAPBADOPERATION;
                twRC = TWRC_FAILURE;
            }
            break;

        case DAT_STATUS:
            if (MSG == MSG_GET)
                twRC = SANE_GetDSStatus (pOrigin, pData);
            else
            {
                activeDS.twCC = TWCC_CAPBADOPERATION;
                twRC = TWRC_FAILURE;
            }
            break;

        case DAT_USERINTERFACE:
            switch (MSG)
            {
                case MSG_DISABLEDS:
                    twRC = SANE_DisableDSUserInterface (pOrigin, pData);
                    break;
                case MSG_ENABLEDS:
                    twRC = SANE_EnableDSUserInterface (pOrigin, pData);
                    break;
                case MSG_ENABLEDSUIONLY:
                    twRC = SANE_EnableDSUIOnly (pOrigin, pData);
                    break;
                default:
                    activeDS.twCC = TWCC_CAPBADOPERATION;
                    twRC = TWRC_FAILURE;
                    break;
            }
            break;

        case DAT_XFERGROUP:
            switch (MSG)
            {
                case MSG_GET:
                    twRC = SANE_XferGroupGet (pOrigin, pData);
                    break;
                case MSG_SET:
                    twRC = SANE_XferGroupSet (pOrigin, pData);
                    break;
                default:
                    activeDS.twCC = TWCC_CAPBADOPERATION;
                    twRC = TWRC_FAILURE;
                    break;
            }
            break;

        default:
	    WARN("code unsupported: %d\n", DAT);
            activeDS.twCC = TWCC_CAPUNSUPPORTED;
            twRC = TWRC_FAILURE;
            break;
    }

    return twRC;
}


static TW_UINT16 SANE_ImageGroupHandler (
           pTW_IDENTITY pOrigin,
           TW_UINT16    DAT,
           TW_UINT16    MSG,
           TW_MEMREF    pData)
{
    TW_UINT16 twRC = TWRC_SUCCESS;

    switch (DAT)
    {
        case DAT_IMAGEINFO:
            if (MSG == MSG_GET)
                twRC = SANE_ImageInfoGet (pOrigin, pData);
            else
            {
                activeDS.twCC = TWCC_CAPBADOPERATION;
                twRC = TWRC_FAILURE;
            }
            break;

        case DAT_IMAGELAYOUT:
            switch (MSG)
            {
                case MSG_GET:
                    twRC = SANE_ImageLayoutGet (pOrigin, pData);
                    break;
                case MSG_GETDEFAULT:
                    twRC = SANE_ImageLayoutGetDefault (pOrigin, pData);
                    break;
                case MSG_RESET:
                    twRC = SANE_ImageLayoutReset (pOrigin, pData);
                    break;
                case MSG_SET:
                    twRC = SANE_ImageLayoutSet (pOrigin, pData);
                    break;
                default:
                    twRC = TWRC_FAILURE;
                    activeDS.twCC = TWCC_CAPBADOPERATION;
                    ERR("unrecognized operation triplet\n");
                    break;
            }
            break;

        case DAT_IMAGEMEMXFER:
            if (MSG == MSG_GET)
                twRC = SANE_ImageMemXferGet (pOrigin, pData);
            else
            {
                activeDS.twCC = TWCC_CAPBADOPERATION;
                twRC = TWRC_FAILURE;
            }
            break;

        case DAT_IMAGENATIVEXFER:
            if (MSG == MSG_GET)
                twRC = SANE_ImageNativeXferGet (pOrigin, pData);
            else
            {
                activeDS.twCC = TWCC_CAPBADOPERATION;
                twRC = TWRC_FAILURE;
            }
            break;

        default:
            twRC = TWRC_FAILURE;
            activeDS.twCC = TWCC_CAPUNSUPPORTED;
            WARN("unsupported DG type %d\n", DAT);
            break;
    }
    return twRC;
}

/* Main entry point for the TWAIN library */
TW_UINT16 WINAPI
DS_Entry ( pTW_IDENTITY pOrigin,
           TW_UINT32    DG,
           TW_UINT16    DAT,
           TW_UINT16    MSG,
           TW_MEMREF    pData)
{
    TW_UINT16 twRC = TWRC_SUCCESS;  /* Return Code */

    TRACE("(DG=%d DAT=%d MSG=%d)\n", DG, DAT, MSG);

    switch (DG)
    {
        case DG_CONTROL:
            twRC = SANE_SourceControlHandler (pOrigin,DAT,MSG,pData);
            break;
        case DG_IMAGE:
            twRC = SANE_ImageGroupHandler (pOrigin,DAT,MSG,pData);
            break;
        case DG_AUDIO:
            WARN("Audio group of controls not implemented yet.\n");
            twRC = TWRC_FAILURE;
            activeDS.twCC = TWCC_CAPUNSUPPORTED;
            break;
        default:
            activeDS.twCC = TWCC_BADPROTOCOL;
            twRC = TWRC_FAILURE;
    }

    return twRC;
}

#ifdef SONAME_LIBSANE
/* Sane returns device names that are longer than the 32 bytes allowed
   by TWAIN.  However, it colon separates them, and the last bit is
   the most interesting.  So we use the last bit, and add a signature
   to ensure uniqueness */
static void copy_sane_short_name(const char *in, char *out, size_t outsize)
{
    const char *p;
    int  signature = 0;

    if (strlen(in) <= outsize - 1)
    {
        strcpy(out, in);
        return;
    }

    for (p = in; *p; p++)
        signature += *p;

    p = strrchr(in, ':');
    if (!p)
        p = in;
    else
        p++;

    if (strlen(p) > outsize - 7 - 1)
        p += strlen(p) - (outsize - 7 - 1);

    strcpy(out, p);
    sprintf(out + strlen(out), "(%04X)", signature % 0x10000);

}

static const SANE_Device **sane_devlist;

static void
detect_sane_devices(void) {
    if (sane_devlist && sane_devlist[0]) return;
    TRACE("detecting sane...\n");
    if (psane_get_devices (&sane_devlist, SANE_FALSE) != SANE_STATUS_GOOD)
	return;
}

static TW_UINT16
SANE_GetIdentity( pTW_IDENTITY pOrigin, pTW_IDENTITY self) {
    static int cursanedev = 0;

    detect_sane_devices();
    if (!sane_devlist[cursanedev])
	return TWRC_FAILURE;
    self->ProtocolMajor = TWON_PROTOCOLMAJOR;
    self->ProtocolMinor = TWON_PROTOCOLMINOR;
    self->SupportedGroups = DG_CONTROL | DG_IMAGE;
    copy_sane_short_name(sane_devlist[cursanedev]->name, self->ProductName, sizeof(self->ProductName) - 1);
    lstrcpynA (self->Manufacturer, sane_devlist[cursanedev]->vendor, sizeof(self->Manufacturer) - 1);
    lstrcpynA (self->ProductFamily, sane_devlist[cursanedev]->model, sizeof(self->ProductFamily) - 1);
    cursanedev++;

    if (!sane_devlist[cursanedev] 		||
	!sane_devlist[cursanedev]->model	||
	!sane_devlist[cursanedev]->vendor	||
	!sane_devlist[cursanedev]->name
    )
	cursanedev = 0; /* wrap to begin */
    return TWRC_SUCCESS;
}

static TW_UINT16 SANE_OpenDS( pTW_IDENTITY pOrigin, pTW_IDENTITY self) {
    SANE_Status status;
    int i;

    detect_sane_devices();
    if (!sane_devlist[0]) {
	ERR("No scanners? We should not get to OpenDS?\n");
	return TWRC_FAILURE;
    }

    for (i=0; sane_devlist[i] && sane_devlist[i]->model; i++) {
	TW_STR32 name;

	/* To make string as short as above */
	lstrcpynA(name, sane_devlist[i]->vendor, sizeof(name)-1);
	if (*self->Manufacturer && strcmp(name, self->Manufacturer))
	    continue;
	lstrcpynA(name, sane_devlist[i]->model, sizeof(name)-1);
	if (*self->ProductFamily && strcmp(name, self->ProductFamily))
	    continue;
        copy_sane_short_name(sane_devlist[i]->name, name, sizeof(name) - 1);
	if (*self->ProductName && strcmp(name, self->ProductName))
	    continue;
	break;
    }
    if (!sane_devlist[i]) {
	WARN("Scanner not found.\n");
	return TWRC_FAILURE;
    }
    status = psane_open(sane_devlist[i]->name,&activeDS.deviceHandle);
    if (status == SANE_STATUS_GOOD) {
        activeDS.twCC = SANE_SaneSetDefaults();
        if (activeDS.twCC == TWCC_SUCCESS) {
	    activeDS.currentState = 4;
	    return TWRC_SUCCESS;
        }
        else
            psane_close(activeDS.deviceHandle);
    }
    else
        ERR("sane_open(%s): %s\n", sane_devlist[i]->name, psane_strstatus (status));
    return TWRC_FAILURE;
}
#endif
