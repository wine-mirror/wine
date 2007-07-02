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

#include "windef.h"
#include "winbase.h"
#include "twain.h"
#include "sane_i.h"
#include "wine/debug.h"
#include "wine/library.h"

WINE_DEFAULT_DEBUG_CHANNEL(twain);

HINSTANCE SANE_instance;

#ifdef HAVE_SANE
#ifndef SONAME_LIBSANE
#define SONAME_LIBSANE "libsane" SONAME_EXT
#endif

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

#endif /* HAVE_SANE */

BOOL WINAPI DllMain (HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    TRACE("%p,%x,%p\n", hinstDLL, fdwReason, lpvReserved);

    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH: {
#ifdef HAVE_SANE
	    SANE_Status status;
	    SANE_Int version_code;

            libsane_handle = open_libsane();
            if (! libsane_handle)
                return FALSE;

	    status = psane_init (&version_code, NULL);
#endif
	    SANE_instance = hinstDLL;
            DisableThreadLibraryCalls(hinstDLL);
            break;
	}
        case DLL_PROCESS_DETACH:
#ifdef HAVE_SANE
            TRACE("calling sane_exit()\n");
	    psane_exit ();

            close_libsane(libsane_handle);
            libsane_handle = NULL;
#endif 
	    SANE_instance = NULL;
            break;
    }

    return TRUE;
}

#ifdef HAVE_SANE
static TW_UINT16 SANE_GetIdentity( pTW_IDENTITY, pTW_IDENTITY);
static TW_UINT16 SANE_OpenDS( pTW_IDENTITY, pTW_IDENTITY);
#endif

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
#ifdef HAVE_SANE
		     psane_close (activeDS.deviceHandle);
#endif
		     break;
		case MSG_OPENDS:
#ifdef HAVE_SANE
		     twRC = SANE_OpenDS( pOrigin, (pTW_IDENTITY)pData);
#else
		     twRC = TWRC_FAILURE;
#endif
		     break;
		case MSG_GET:
#ifdef HAVE_SANE
		     twRC = SANE_GetIdentity( pOrigin, (pTW_IDENTITY)pData);
#else
		     twRC = TWRC_FAILURE;
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
                    FIXME("unrecognized opertion triplet\n");
            }
            break;

        case DAT_CUSTOMDSDATA:
            switch (MSG)
            {
                case MSG_GET:
                    twRC = SANE_CustomDSDataGet (pOrigin, pData);
                    break;
                case MSG_SET:
                    twRC = SANE_CustomDSDataSet (pOrigin, pData);
                    break;
                default:
                    break;
            }
            break;

        case DAT_FILESYSTEM:
            switch (MSG)
            {
                /*case MSG_AUTOMATICCAPTUREDIRECTORY:
                    twRC = SANE_AutomaticCaptureDirectory
                               (pOrigin, pData);
                    break;*/
                case MSG_CHANGEDIRECTORY:
                    twRC = SANE_ChangeDirectory (pOrigin, pData);
                    break;
                /*case MSG_COPY:
                    twRC = SANE_FileSystemCopy (pOrigin, pData);
                    break;*/
                case MSG_CREATEDIRECTORY:
                    twRC = SANE_CreateDirectory (pOrigin, pData);
                    break;
                case MSG_DELETE:
                    twRC = SANE_FileSystemDelete (pOrigin, pData);
                    break;
                case MSG_FORMATMEDIA:
                    twRC = SANE_FormatMedia (pOrigin, pData);
                    break;
                case MSG_GETCLOSE:
                    twRC = SANE_FileSystemGetClose (pOrigin, pData);
                    break;
                case MSG_GETFIRSTFILE:
                    twRC = SANE_FileSystemGetFirstFile (pOrigin, pData);
                    break;
                case MSG_GETINFO:
                    twRC = SANE_FileSystemGetInfo (pOrigin, pData);
                    break;
                case MSG_GETNEXTFILE:
                    twRC = SANE_FileSystemGetNextFile (pOrigin, pData);
                    break;
                case MSG_RENAME:
                    twRC = SANE_FileSystemRename (pOrigin, pData);
                    break;
                default:
                    twRC = TWRC_FAILURE;
                    break;
            }
            break;

        case DAT_EVENT:
            if (MSG == MSG_PROCESSEVENT)
                twRC = SANE_ProcessEvent (pOrigin, pData);
            else
                twRC = TWRC_FAILURE;
            break;

        case DAT_PASSTHRU:
            if (MSG == MSG_PASSTHRU)
                twRC = SANE_PassThrough (pOrigin, pData);
            else
                twRC = TWRC_FAILURE;
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
                /*case MSG_STOPFEEDER:
                    twRC = SANE_PendingXfersStopFeeder (pOrigin, pData);
                    break;*/
                default:
                    twRC = TWRC_FAILURE;
            }
            break;

        case DAT_SETUPFILEXFER:
            switch (MSG)
            {
                case MSG_GET:
                    twRC = SANE_SetupFileXferGet (pOrigin, pData);
                    break;
                case MSG_GETDEFAULT:
                    twRC = SANE_SetupFileXferGetDefault (pOrigin, pData);
                    break;
                case MSG_RESET:
                    twRC = SANE_SetupFileXferReset (pOrigin, pData);
                    break;
                case MSG_SET:
                    twRC = SANE_SetupFileXferSet (pOrigin, pData);
                    break;
                default:
                    twRC = TWRC_FAILURE;
                    break;
            }
            break;

        /*case DAT_SETUPFILEXFER2:
            switch (MSG)
            {
                case MSG_GET:
                    twRC = SANE_SetupFileXfer2Get (pOrigin, pData);
                    break;
                case MSG_GETDEFAULT:
                    twRC = SANE_SetupFileXfer2GetDefault (pOrigin, pData);
                    break;
                case MSG_RESET:
                    twRC = SANE_SetupFileXfer2Reset (pOrigin, pData);
                    break;
                case MSG_SET:
                    twRC = SANE_SetupFileXfer2Set (pOrigin, pData);
                    break;
            }
            break;*/
        case DAT_SETUPMEMXFER:
            if (MSG == MSG_GET)
                twRC = SANE_SetupMemXferGet (pOrigin, pData);
            else
                twRC = TWRC_FAILURE;
            break;

        case DAT_STATUS:
            if (MSG == MSG_GET)
                twRC = SANE_GetDSStatus (pOrigin, pData);
            else
                twRC = TWRC_FAILURE;
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
                    twRC = TWRC_FAILURE;
                    break;
            }
            break;

        default:
	    FIXME("code unknown: %d\n", DAT);
            twRC = TWRC_FAILURE;
            break;
    }

    return twRC;
}


TW_UINT16 SANE_ImageGroupHandler (
           pTW_IDENTITY pOrigin,
           TW_UINT16    DAT,
           TW_UINT16    MSG,
           TW_MEMREF    pData)
{
    TW_UINT16 twRC = TWRC_SUCCESS;

    switch (DAT)
    {
        case DAT_CIECOLOR:
            if (MSG == MSG_GET)
                twRC = SANE_CIEColorGet (pOrigin, pData);
            else
                twRC = TWRC_FAILURE;
            break;

        case DAT_EXTIMAGEINFO:
            if (MSG == MSG_GET)
                twRC = SANE_ExtImageInfoGet (pOrigin, pData);
            else
                twRC = TWRC_FAILURE;
            break;

        case DAT_GRAYRESPONSE:
            switch (MSG)
            {
                case MSG_RESET:
                    twRC = SANE_GrayResponseReset (pOrigin, pData);
                    break;
                case MSG_SET:
                    twRC = SANE_GrayResponseSet (pOrigin, pData);
                    break;
                default:
                    twRC = TWRC_FAILURE;
                    activeDS.twCC = TWCC_BADPROTOCOL;
                    FIXME("unrecognized operation triplet\n");
                    break;
            }
            break;
        case DAT_IMAGEFILEXFER:
            if (MSG == MSG_GET)
                twRC = SANE_ImageFileXferGet (pOrigin, pData);
            else
                twRC = TWRC_FAILURE;
            break;

        case DAT_IMAGEINFO:
            if (MSG == MSG_GET)
                twRC = SANE_ImageInfoGet (pOrigin, pData);
            else
                twRC = TWRC_FAILURE;
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
                    activeDS.twCC = TWCC_BADPROTOCOL;
                    ERR("unrecognized operation triplet\n");
                    break;
            }
            break;

        case DAT_IMAGEMEMXFER:
            if (MSG == MSG_GET)
                twRC = SANE_ImageMemXferGet (pOrigin, pData);
            else
                twRC = TWRC_FAILURE;
            break;

        case DAT_IMAGENATIVEXFER:
            if (MSG == MSG_GET)
                twRC = SANE_ImageNativeXferGet (pOrigin, pData);
            else
                twRC = TWRC_FAILURE;
            break;

        case DAT_JPEGCOMPRESSION:
            switch (MSG)
            {
                case MSG_GET:
                    twRC = SANE_JPEGCompressionGet (pOrigin, pData);
                    break;
                case MSG_GETDEFAULT:
                    twRC = SANE_JPEGCompressionGetDefault (pOrigin, pData);
                    break;
                case MSG_RESET:
                    twRC = SANE_JPEGCompressionReset (pOrigin, pData);
                    break;
                case MSG_SET:
                    twRC = SANE_JPEGCompressionSet (pOrigin, pData);
                    break;
                default:
                    twRC = TWRC_FAILURE;
                    activeDS.twCC = TWCC_BADPROTOCOL;
                    WARN("unrecognized operation triplet\n");
                    break;
            }
            break;

        case DAT_PALETTE8:
            switch (MSG)
            {
                case MSG_GET:
                    twRC = SANE_Palette8Get (pOrigin, pData);
                    break;
                case MSG_GETDEFAULT:
                    twRC = SANE_Palette8GetDefault (pOrigin, pData);
                    break;
                case MSG_RESET:
                    twRC = SANE_Palette8Reset (pOrigin, pData);
                    break;
                case MSG_SET:
                    twRC = SANE_Palette8Set (pOrigin, pData);
                    break;
                default:
                    twRC = TWRC_FAILURE;
                    activeDS.twCC = TWCC_BADPROTOCOL;
                    WARN("unrecognized operation triplet\n");
            }
            break;

        case DAT_RGBRESPONSE:
            switch (MSG)
            {
                case MSG_RESET:
                    twRC = SANE_RGBResponseReset (pOrigin, pData);
                    break;
                case MSG_SET:
                    twRC = SANE_RGBResponseSet (pOrigin, pData);
                    break;
                default:
                    twRC = TWRC_FAILURE;
                    activeDS.twCC = TWCC_BADPROTOCOL;
                    WARN("unrecognized operation triplet\n");
                    break;
            }
            break;

        default:
            twRC = TWRC_FAILURE;
            activeDS.twCC = TWCC_BADPROTOCOL;
            FIXME("unrecognized DG type %d\n", DAT);
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

    TRACE("(DG=%ld DAT=%d MSG=%d)\n", DG, DAT, MSG);

    switch (DG)
    {
        case DG_CONTROL:
            twRC = SANE_SourceControlHandler (pOrigin,DAT,MSG,pData);
            break;
        case DG_IMAGE:
            twRC = SANE_ImageGroupHandler (pOrigin,DAT,MSG,pData);
            break;
        case DG_AUDIO:
            FIXME("Audio group of controls not implemented yet.\n");
        default:
            activeDS.twCC = TWCC_BADPROTOCOL;
            twRC = TWRC_FAILURE;
    }

    return twRC;
}

#ifdef HAVE_SANE
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
	if (strcmp(name, self->Manufacturer))
	    continue;
	lstrcpynA(name, sane_devlist[i]->model, sizeof(name)-1);
	if (strcmp(name, self->ProductFamily))
	    continue;
        copy_sane_short_name(sane_devlist[i]->name, name, sizeof(name) - 1);
	if (strcmp(name, self->ProductName))
	    continue;
	break;
    }
    if (!sane_devlist[i]) {
	FIXME("Scanner not found? Using first one!\n");
	i=0;
    }
    status = psane_open(sane_devlist[i]->name,&activeDS.deviceHandle);
    if (status == SANE_STATUS_GOOD) {
	activeDS.currentState = 4;
	activeDS.twCC = TWRC_SUCCESS;
	return TWRC_SUCCESS;
    }
    FIXME("sane_open(%s): %s\n", sane_devlist[i]->name, psane_strstatus (status));
    return TWRC_FAILURE;
}
#endif
