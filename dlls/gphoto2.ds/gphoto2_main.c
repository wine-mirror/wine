/*
 * SANE.DS functions
 *
 * Copyright 2000 Shi Quan He <shiquan@cyberdude.com>
 * Copyright 2006 Marcus Meissner
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


#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "gphoto2_i.h"
#include "unixlib.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(twain);

struct tagActiveDS activeDS;

static DSMENTRYPROC GPHOTO2_dsmentry;

/* DG_CONTROL/DAT_CAPABILITY/MSG_GET */
static TW_UINT16 GPHOTO2_CapabilityGet (pTW_IDENTITY pOrigin, TW_MEMREF pData)
{
    pTW_CAPABILITY pCapability = (pTW_CAPABILITY) pData;

    TRACE("DG_CONTROL/DAT_CAPABILITY/MSG_GET\n");
    if (activeDS.currentState < 4 || activeDS.currentState > 7) {
        activeDS.twCC = TWCC_SEQERROR;
        return TWRC_FAILURE;
    }
    activeDS.twCC = GPHOTO2_SaneCapability (pCapability, MSG_GET);
    return (activeDS.twCC == TWCC_SUCCESS)?TWRC_SUCCESS:TWRC_FAILURE;
}

/* DG_CONTROL/DAT_CAPABILITY/MSG_GETCURRENT */
static TW_UINT16 GPHOTO2_CapabilityGetCurrent (pTW_IDENTITY pOrigin, TW_MEMREF pData)
{
    pTW_CAPABILITY pCapability = (pTW_CAPABILITY) pData;

    TRACE("DG_CONTROL/DAT_CAPABILITY/MSG_GETCURRENT\n");

    if (activeDS.currentState < 4 || activeDS.currentState > 7) {
        activeDS.twCC = TWCC_SEQERROR;
        return TWRC_FAILURE;
    }
    activeDS.twCC = GPHOTO2_SaneCapability (pCapability, MSG_GETCURRENT);
    return (activeDS.twCC == TWCC_SUCCESS)?TWRC_SUCCESS:TWRC_FAILURE;
}

/* DG_CONTROL/DAT_CAPABILITY/MSG_GETDEFAULT */
static TW_UINT16 GPHOTO2_CapabilityGetDefault (pTW_IDENTITY pOrigin, TW_MEMREF pData)
{
    pTW_CAPABILITY pCapability = (pTW_CAPABILITY) pData;

    TRACE("DG_CONTROL/DAT_CAPABILITY/MSG_GETDEFAULT\n");
    if (activeDS.currentState < 4 || activeDS.currentState > 7) {
        activeDS.twCC = TWCC_SEQERROR;
        return TWRC_FAILURE;
    }
    activeDS.twCC = GPHOTO2_SaneCapability (pCapability, MSG_GETDEFAULT);
    return (activeDS.twCC == TWCC_SUCCESS)?TWRC_SUCCESS:TWRC_FAILURE;
}

/* DG_CONTROL/DAT_CAPABILITY/MSG_QUERYSUPPORT */
static TW_UINT16 GPHOTO2_CapabilityQuerySupport (pTW_IDENTITY pOrigin,
                                        TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_CONTROL/DAT_CAPABILITY/MSG_RESET */
static TW_UINT16 GPHOTO2_CapabilityReset (pTW_IDENTITY pOrigin,
                                 TW_MEMREF pData)
{
    pTW_CAPABILITY pCapability = (pTW_CAPABILITY) pData;

    TRACE("DG_CONTROL/DAT_CAPABILITY/MSG_RESET\n");

    if (activeDS.currentState < 4 || activeDS.currentState > 7) {
        activeDS.twCC = TWCC_SEQERROR;
        return TWRC_FAILURE;
    }
    activeDS.twCC = GPHOTO2_SaneCapability (pCapability, MSG_RESET);
    return (activeDS.twCC == TWCC_SUCCESS)?TWRC_SUCCESS:TWRC_FAILURE;
}

/* DG_CONTROL/DAT_CAPABILITY/MSG_SET */
static TW_UINT16 GPHOTO2_CapabilitySet (pTW_IDENTITY pOrigin,
                               TW_MEMREF pData)
{
    pTW_CAPABILITY pCapability = (pTW_CAPABILITY) pData;

    TRACE ("DG_CONTROL/DAT_CAPABILITY/MSG_SET\n");

    if (activeDS.currentState != 4) {
        activeDS.twCC = TWCC_SEQERROR;
        return TWRC_FAILURE;
    }
    activeDS.twCC = GPHOTO2_SaneCapability (pCapability, MSG_SET);
    return (activeDS.twCC == TWCC_SUCCESS)?TWRC_SUCCESS:TWRC_FAILURE;
}

/* DG_CONTROL/DAT_CUSTOMDSDATA/MSG_GET */
static TW_UINT16 GPHOTO2_CustomDSDataGet (pTW_IDENTITY pOrigin,
                                TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_CONTROL/DAT_CUSTOMDSDATA/MSG_SET */
static TW_UINT16 GPHOTO2_CustomDSDataSet (pTW_IDENTITY pOrigin,
                                 TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_CONTROL/DAT_FILESYSTEM/MSG_CHANGEDIRECTORY */
static TW_UINT16 GPHOTO2_ChangeDirectory (pTW_IDENTITY pOrigin,
                                 TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_CONTROL/DAT_FILESYSTEM/MSG_CREATEDIRECTORY */
static TW_UINT16 GPHOTO2_CreateDirectory (pTW_IDENTITY pOrigin,
                                 TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_CONTROL/DAT_FILESYSTEM/MSG_DELETE */
static TW_UINT16 GPHOTO2_FileSystemDelete (pTW_IDENTITY pOrigin,
                                  TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_CONTROL/DAT_FILESYSTEM/MSG_FORMATMEDIA */
static TW_UINT16 GPHOTO2_FormatMedia (pTW_IDENTITY pOrigin,
                             TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_CONTROL/DAT_FILESYSTEM/MSG_GETCLOSE */
static TW_UINT16 GPHOTO2_FileSystemGetClose (pTW_IDENTITY pOrigin,
                                    TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_CONTROL/DAT_FILESYSTEM/MSG_GETFIRSTFILE */
static TW_UINT16 GPHOTO2_FileSystemGetFirstFile (pTW_IDENTITY pOrigin,

                                        TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_CONTROL/DAT_FILESYSTEM/MSG_GETINFO */
static TW_UINT16 GPHOTO2_FileSystemGetInfo (pTW_IDENTITY pOrigin,
                                   TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_CONTROL/DAT_FILESYSTEM/MSG_GETNEXTFILE */
static TW_UINT16 GPHOTO2_FileSystemGetNextFile (pTW_IDENTITY pOrigin,

                                       TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_CONTROL/DAT_FILESYSTEM/MSG_RENAME */
static TW_UINT16 GPHOTO2_FileSystemRename (pTW_IDENTITY pOrigin,
                                  TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

static void GPHOTO2_Notify (TW_UINT16 message)
{
    GPHOTO2_dsmentry (&activeDS.identity, &activeDS.appIdentity, DG_CONTROL, DAT_NULL, message, NULL);
}

/* DG_CONTROL/DAT_ENTRYPOINT/MSG_SET */
static TW_UINT16 GPHOTO2_SetEntryPoint (pTW_IDENTITY pOrigin, TW_MEMREF pData)
{
    TW_ENTRYPOINT *entry = (TW_ENTRYPOINT*)pData;

    GPHOTO2_dsmentry = entry->DSM_Entry;

    return TWRC_SUCCESS;
}

/* DG_CONTROL/DAT_EVENT/MSG_PROCESSEVENT */
static TW_UINT16 GPHOTO2_ProcessEvent (pTW_IDENTITY pOrigin,
                              TW_MEMREF pData)
{
    pTW_EVENT pEvent = (pTW_EVENT) pData;

    TRACE("DG_CONTROL/DAT_EVENT/MSG_PROCESSEVENT\n");

    if (activeDS.currentState < 5 || activeDS.currentState > 7) {
        activeDS.twCC = TWCC_SEQERROR;
        return TWRC_FAILURE;
    }

    pEvent->TWMessage = MSG_NULL;  /* no message to the application */
    activeDS.twCC = TWCC_SUCCESS;
    return TWRC_NOTDSEVENT;
}

/* DG_CONTROL/DAT_PASSTHRU/MSG_PASSTHRU */
static TW_UINT16 GPHOTO2_PassThrough (pTW_IDENTITY pOrigin,
                             TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_CONTROL/DAT_PENDINGXFERS/MSG_ENDXFER */
static TW_UINT16 GPHOTO2_PendingXfersEndXfer (pTW_IDENTITY pOrigin,
                                     TW_MEMREF pData)
{
    pTW_PENDINGXFERS pPendingXfers = (pTW_PENDINGXFERS) pData;

    TRACE("DG_CONTROL/DAT_PENDINGXFERS/MSG_ENDXFER\n");

    if (activeDS.currentState != 6 && activeDS.currentState != 7) {
        activeDS.twCC = TWCC_SEQERROR;
        return TWRC_FAILURE;
    }
    TRACE("count = %d\n", activeDS.download_count);
    pPendingXfers->Count = activeDS.download_count;
    if (pPendingXfers->Count != 0) {
        activeDS.currentState = 6;
    } else {
        activeDS.currentState = 5;
        /* Notify the application that it can close the data source */
        GPHOTO2_Notify(MSG_CLOSEDSREQ);
        /* close any Transferring dialog */
        TransferringDialogBox(activeDS.progressWnd,-1);
        activeDS.progressWnd = 0;
    }
    activeDS.twCC = TWCC_SUCCESS;
    return TWRC_SUCCESS;
}

/* DG_CONTROL/DAT_PENDINGXFERS/MSG_GET */
static TW_UINT16 GPHOTO2_PendingXfersGet (pTW_IDENTITY pOrigin,
                                 TW_MEMREF pData)
{
    pTW_PENDINGXFERS pPendingXfers = (pTW_PENDINGXFERS) pData;

    TRACE("DG_CONTROL/DAT_PENDINGXFERS/MSG_GET\n");

    if (activeDS.currentState < 4 || activeDS.currentState > 7) {
        activeDS.twCC = TWCC_SEQERROR;
        return TWRC_FAILURE;
    }

    TRACE("count = %d\n", activeDS.download_count);
    pPendingXfers->Count = activeDS.download_count;
    activeDS.twCC = TWCC_SUCCESS;
    return TWRC_SUCCESS;
}

/* DG_CONTROL/DAT_PENDINGXFERS/MSG_RESET */
static TW_UINT16 GPHOTO2_PendingXfersReset (pTW_IDENTITY pOrigin,
                                   TW_MEMREF pData)
{
    pTW_PENDINGXFERS pPendingXfers = (pTW_PENDINGXFERS) pData;

    TRACE("DG_CONTROL/DAT_PENDINGXFERS/MSG_RESET\n");

    if (activeDS.currentState != 6) {
        activeDS.twCC = TWCC_SEQERROR;
        return TWRC_FAILURE;
    }
    pPendingXfers->Count = 0;
    activeDS.currentState = 5;
    activeDS.twCC = TWCC_SUCCESS;
    return TWRC_SUCCESS;
}

/* DG_CONTROL/DAT_SETUPFILEXFER/MSG_GET */
static TW_UINT16 GPHOTO2_SetupFileXferGet (pTW_IDENTITY pOrigin,
                                  TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_CONTROL/DAT_SETUPXFER/MSG_GETDEFAULT */
static TW_UINT16 GPHOTO2_SetupFileXferGetDefault (pTW_IDENTITY pOrigin,
                                         TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}


/* DG_CONTROL/DAT_SETUPFILEXFER/MSG_RESET */
static TW_UINT16 GPHOTO2_SetupFileXferReset (pTW_IDENTITY pOrigin,
                                    TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_CONTROL/DAT_SETUPFILEXFER/MSG_SET */
static TW_UINT16 GPHOTO2_SetupFileXferSet (pTW_IDENTITY pOrigin,
                                  TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_CONTROL/DAT_SETUPMEMXFER/MSG_GET */
static TW_UINT16 GPHOTO2_SetupMemXferGet (pTW_IDENTITY pOrigin,
                                  TW_MEMREF pData)
{
    pTW_SETUPMEMXFER  pSetupMemXfer = (pTW_SETUPMEMXFER)pData;

    TRACE("DG_CONTROL/DAT_SETUPMEMXFER/MSG_GET\n");
    /* Guessing */
    pSetupMemXfer->MinBufSize = 20000;
    pSetupMemXfer->MaxBufSize = 80000;
    pSetupMemXfer->Preferred = 40000;
    return TWRC_SUCCESS;
}

/* DG_CONTROL/DAT_STATUS/MSG_GET */
static TW_UINT16 GPHOTO2_GetDSStatus (pTW_IDENTITY pOrigin,
                             TW_MEMREF pData)
{
    pTW_STATUS pSourceStatus = (pTW_STATUS) pData;

    TRACE ("DG_CONTROL/DAT_STATUS/MSG_GET\n");
    pSourceStatus->ConditionCode = activeDS.twCC;
    /* Reset the condition code */
    activeDS.twCC = TWCC_SUCCESS;
    return TWRC_SUCCESS;
}

/* DG_CONTROL/DAT_USERINTERFACE/MSG_DISABLEDS */
static TW_UINT16 GPHOTO2_DisableDSUserInterface (pTW_IDENTITY pOrigin,
                                        TW_MEMREF pData)
{
    TRACE ("DG_CONTROL/DAT_USERINTERFACE/MSG_DISABLEDS\n");

    if (activeDS.currentState != 5) {
        activeDS.twCC = TWCC_SEQERROR;
        return TWRC_FAILURE;
    }
    activeDS.currentState = 4;
    activeDS.twCC = TWCC_SUCCESS;
    return TWRC_SUCCESS;
}

/* DG_CONTROL/DAT_USERINTERFACE/MSG_ENABLEDS */
static TW_UINT16 GPHOTO2_EnableDSUserInterface (pTW_IDENTITY pOrigin,
                                       TW_MEMREF pData)
{
    pTW_USERINTERFACE pUserInterface = (pTW_USERINTERFACE) pData;
    struct load_file_list_params params = { "/", &activeDS.file_count };

    GPHOTO2_CALL( load_file_list, &params );
    activeDS.download_flags = calloc( activeDS.file_count, sizeof(*activeDS.download_flags) );
    activeDS.download_count = 0;

    TRACE ("DG_CONTROL/DAT_USERINTERFACE/MSG_ENABLEDS\n");
    if (activeDS.currentState != 4) {
	FIXME("Sequence error %d\n", activeDS.currentState);
        activeDS.twCC = TWCC_SEQERROR;
	return TWRC_FAILURE;
    }
    if (pUserInterface->ShowUI)
    {
	BOOL rc;
	activeDS.currentState = 5; /* Transitions to state 5 */
	rc = DoCameraUI();
	if (!rc) {
	    GPHOTO2_Notify(MSG_CLOSEDSREQ);
	} else {
	    /* FIXME: The GUI should have marked the files to download... */
	    GPHOTO2_Notify(MSG_XFERREADY);
	    activeDS.currentState = 6; /* Transitions to state 6 directly */
	}
    } else {
	/* no UI will be displayed, so source is ready to transfer data */
	GPHOTO2_Notify(MSG_XFERREADY);
	activeDS.currentState = 6; /* Transitions to state 6 directly */
    }
    activeDS.twCC = TWCC_SUCCESS;
    return TWRC_SUCCESS;
}

/* DG_CONTROL/DAT_USERINTERFACE/MSG_ENABLEDSUIONLY */
static TW_UINT16 GPHOTO2_EnableDSUIOnly (pTW_IDENTITY pOrigin,
                                TW_MEMREF pData)
{
    TRACE("DG_CONTROL/DAT_USERINTERFACE/MSG_ENABLEDSUIONLY\n");

    if (activeDS.currentState != 4) {
        activeDS.twCC = TWCC_SEQERROR;
        return TWRC_FAILURE;
    }
    /* FIXME: we should replace xscanimage with our own UI */
    FIXME ("not implemented!\n");
    activeDS.currentState = 5;
    activeDS.twCC = TWCC_SUCCESS;
    return TWRC_SUCCESS;
}

/* DG_CONTROL/DAT_XFERGROUP/MSG_GET */
static TW_UINT16 GPHOTO2_XferGroupGet (pTW_IDENTITY pOrigin,
                              TW_MEMREF pData)
{
    FIXME ("stub!\n");
    return TWRC_FAILURE;
}

/* DG_CONTROL/DAT_XFERGROUP/MSG_SET */
static TW_UINT16 GPHOTO2_XferGroupSet (pTW_IDENTITY pOrigin,
                                  TW_MEMREF pData)
{
    FIXME ("stub!\n");
    return TWRC_FAILURE;
}

HINSTANCE GPHOTO2_instance = 0;

BOOL WINAPI DllMain (HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    TRACE("%p,%lx,%p\n", hinstDLL, fdwReason, lpvReserved);

    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
	    GPHOTO2_instance = hinstDLL;
            DisableThreadLibraryCalls(hinstDLL);
            __wine_init_unix_call();
            break;
    }

    return TRUE;
}

static TW_UINT16 GPHOTO2_OpenDS( pTW_IDENTITY pOrigin, pTW_IDENTITY self )
{
    if (GPHOTO2_dsmentry == NULL)
    {
        HMODULE moddsm = GetModuleHandleW(L"twain_32");

        if (moddsm)
            GPHOTO2_dsmentry = (void*)GetProcAddress(moddsm, "DSM_Entry");

        if (!GPHOTO2_dsmentry)
        {
            ERR("can't find DSM entry point\n");
            return TWRC_FAILURE;
        }
    }

    if (GPHOTO2_CALL( open_ds, self )) return TWRC_FAILURE;

    activeDS.file_count = 0;
    activeDS.file_handle = 0;
    activeDS.download_count = 0;
    activeDS.currentState = 4;
    activeDS.twCC 		= TWRC_SUCCESS;
    activeDS.pixelflavor	= TWPF_CHOCOLATE;
    activeDS.pixeltype		= TWPT_RGB;
    activeDS.capXferMech	= TWSX_MEMORY;
    activeDS.identity.Id = self->Id;
    activeDS.appIdentity = *pOrigin;
    TRACE("OK!\n");
    return TWRC_SUCCESS;
}

static TW_UINT16 GPHOTO2_SourceControlHandler (
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
                    GPHOTO2_CALL( close_ds, NULL );
                    break;
		case MSG_GET:
                    if (GPHOTO2_CALL( get_identity, pData )) twRC = TWRC_FAILURE;
                    break;
		case MSG_OPENDS:
		     twRC = GPHOTO2_OpenDS(pOrigin,(pTW_IDENTITY)pData);
		     break;
	    }
	    break;
        case DAT_CAPABILITY:
            switch (MSG)
            {
                case MSG_GET:
                    twRC = GPHOTO2_CapabilityGet (pOrigin, pData);
                    break;
                case MSG_GETCURRENT:
                    twRC = GPHOTO2_CapabilityGetCurrent (pOrigin, pData);
                    break;
                case MSG_GETDEFAULT:
                    twRC = GPHOTO2_CapabilityGetDefault (pOrigin, pData);
                    break;
                case MSG_QUERYSUPPORT:
                    twRC = GPHOTO2_CapabilityQuerySupport (pOrigin, pData);
                    break;
                case MSG_RESET:
                    twRC = GPHOTO2_CapabilityReset (pOrigin, pData);
                    break;
                case MSG_SET:
                    twRC = GPHOTO2_CapabilitySet (pOrigin, pData);
                    break;
                default:
                    twRC = TWRC_FAILURE;
                    FIXME("unrecognized operation triplet\n");
            }
            break;

        case DAT_CUSTOMDSDATA:
            switch (MSG)
            {
                case MSG_GET:
                    twRC = GPHOTO2_CustomDSDataGet (pOrigin, pData);
                    break;
                case MSG_SET:
                    twRC = GPHOTO2_CustomDSDataSet (pOrigin, pData);
                    break;
                default:
                    break;
            }
            break;

        case DAT_FILESYSTEM:
            switch (MSG)
            {
                /*case MSG_AUTOMATICCAPTUREDIRECTORY:
                    twRC = GPHOTO2_AutomaticCaptureDirectory
                               (pOrigin, pData);
                    break;*/
                case MSG_CHANGEDIRECTORY:
                    twRC = GPHOTO2_ChangeDirectory (pOrigin, pData);
                    break;
                /*case MSG_COPY:
                    twRC = GPHOTO2_FileSystemCopy (pOrigin, pData);
                    break;*/
                case MSG_CREATEDIRECTORY:
                    twRC = GPHOTO2_CreateDirectory (pOrigin, pData);
                    break;
                case MSG_DELETE:
                    twRC = GPHOTO2_FileSystemDelete (pOrigin, pData);
                    break;
                case MSG_FORMATMEDIA:
                    twRC = GPHOTO2_FormatMedia (pOrigin, pData);
                    break;
                case MSG_GETCLOSE:
                    twRC = GPHOTO2_FileSystemGetClose (pOrigin, pData);
                    break;
                case MSG_GETFIRSTFILE:
                    twRC = GPHOTO2_FileSystemGetFirstFile (pOrigin, pData);
                    break;
                case MSG_GETINFO:
                    twRC = GPHOTO2_FileSystemGetInfo (pOrigin, pData);
                    break;
                case MSG_GETNEXTFILE:
                    twRC = GPHOTO2_FileSystemGetNextFile (pOrigin, pData);
                    break;
                case MSG_RENAME:
                    twRC = GPHOTO2_FileSystemRename (pOrigin, pData);
                    break;
                default:
                    twRC = TWRC_FAILURE;
                    break;
            }
            break;

        case DAT_ENTRYPOINT:
            if (MSG == MSG_SET)
                twRC = GPHOTO2_SetEntryPoint (pOrigin, pData);
            else
                twRC = TWRC_FAILURE;
            break;

        case DAT_EVENT:
            if (MSG == MSG_PROCESSEVENT)
                twRC = GPHOTO2_ProcessEvent (pOrigin, pData);
            else
                twRC = TWRC_FAILURE;
            break;

        case DAT_PASSTHRU:
            if (MSG == MSG_PASSTHRU)
                twRC = GPHOTO2_PassThrough (pOrigin, pData);
            else
                twRC = TWRC_FAILURE;
            break;

        case DAT_PENDINGXFERS:
            switch (MSG)
            {
                case MSG_ENDXFER:
                    twRC = GPHOTO2_PendingXfersEndXfer (pOrigin, pData);
                    break;
                case MSG_GET:
                    twRC = GPHOTO2_PendingXfersGet (pOrigin, pData);
                    break;
                case MSG_RESET:
                    twRC = GPHOTO2_PendingXfersReset (pOrigin, pData);
                    break;
                /*case MSG_STOPFEEDER:
                    twRC = GPHOTO2_PendingXfersStopFeeder (pOrigin, pData);
                    break;*/
                default:
                    twRC = TWRC_FAILURE;
            }
            break;

        case DAT_SETUPFILEXFER:
            switch (MSG)
            {
                case MSG_GET:
                    twRC = GPHOTO2_SetupFileXferGet (pOrigin, pData);
                    break;
                case MSG_GETDEFAULT:
                    twRC = GPHOTO2_SetupFileXferGetDefault (pOrigin, pData);
                    break;
                case MSG_RESET:
                    twRC = GPHOTO2_SetupFileXferReset (pOrigin, pData);
                    break;
                case MSG_SET:
                    twRC = GPHOTO2_SetupFileXferSet (pOrigin, pData);
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
                    twRC = GPHOTO2_SetupFileXfer2Get (pOrigin, pData);
                    break;
                case MSG_GETDEFAULT:
                    twRC = GPHOTO2_SetupFileXfer2GetDefault (pOrigin, pData);
                    break;
                case MSG_RESET:
                    twRC = GPHOTO2_SetupFileXfer2Reset (pOrigin, pData);
                    break;
                case MSG_SET:
                    twRC = GPHOTO2_SetupFileXfer2Set (pOrigin, pData);
                    break;
            }
            break;*/
        case DAT_SETUPMEMXFER:
            if (MSG == MSG_GET)
                twRC = GPHOTO2_SetupMemXferGet (pOrigin, pData);
            else
                twRC = TWRC_FAILURE;
            break;

        case DAT_STATUS:
            if (MSG == MSG_GET)
                twRC = GPHOTO2_GetDSStatus (pOrigin, pData);
            else
                twRC = TWRC_FAILURE;
            break;

        case DAT_USERINTERFACE:
            switch (MSG)
            {
                case MSG_DISABLEDS:
                    twRC = GPHOTO2_DisableDSUserInterface (pOrigin, pData);
                    break;
                case MSG_ENABLEDS:
                    twRC = GPHOTO2_EnableDSUserInterface (pOrigin, pData);
                    break;
                case MSG_ENABLEDSUIONLY:
                    twRC = GPHOTO2_EnableDSUIOnly (pOrigin, pData);
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
                    twRC = GPHOTO2_XferGroupGet (pOrigin, pData);
                    break;
                case MSG_SET:
                    twRC = GPHOTO2_XferGroupSet (pOrigin, pData);
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


static TW_UINT16 GPHOTO2_ImageGroupHandler (
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
                twRC = GPHOTO2_CIEColorGet (pOrigin, pData);
            else
                twRC = TWRC_FAILURE;
            break;

        case DAT_EXTIMAGEINFO:
            if (MSG == MSG_GET)
                twRC = GPHOTO2_ExtImageInfoGet (pOrigin, pData);
            else
                twRC = TWRC_FAILURE;
            break;

        case DAT_GRAYRESPONSE:
            switch (MSG)
            {
                case MSG_RESET:
                    twRC = GPHOTO2_GrayResponseReset (pOrigin, pData);
                    break;
                case MSG_SET:
                    twRC = GPHOTO2_GrayResponseSet (pOrigin, pData);
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
                twRC = GPHOTO2_ImageFileXferGet (pOrigin, pData);
            else
                twRC = TWRC_FAILURE;
            break;

        case DAT_IMAGEINFO:
            if (MSG == MSG_GET)
                twRC = GPHOTO2_ImageInfoGet (pOrigin, pData);
            else
                twRC = TWRC_FAILURE;
            break;

        case DAT_IMAGELAYOUT:
            switch (MSG)
            {
                case MSG_GET:
                    twRC = GPHOTO2_ImageLayoutGet (pOrigin, pData);
                    break;
                case MSG_GETDEFAULT:
                    twRC = GPHOTO2_ImageLayoutGetDefault (pOrigin, pData);
                    break;
                case MSG_RESET:
                    twRC = GPHOTO2_ImageLayoutReset (pOrigin, pData);
                    break;
                case MSG_SET:
                    twRC = GPHOTO2_ImageLayoutSet (pOrigin, pData);
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
                twRC = GPHOTO2_ImageMemXferGet (pOrigin, pData);
            else
                twRC = TWRC_FAILURE;
            break;

        case DAT_IMAGENATIVEXFER:
            if (MSG == MSG_GET)
                twRC = GPHOTO2_ImageNativeXferGet (pOrigin, pData);
            else
                twRC = TWRC_FAILURE;
            break;

        case DAT_JPEGCOMPRESSION:
            switch (MSG)
            {
                case MSG_GET:
                    twRC = GPHOTO2_JPEGCompressionGet (pOrigin, pData);
                    break;
                case MSG_GETDEFAULT:
                    twRC = GPHOTO2_JPEGCompressionGetDefault (pOrigin, pData);
                    break;
                case MSG_RESET:
                    twRC = GPHOTO2_JPEGCompressionReset (pOrigin, pData);
                    break;
                case MSG_SET:
                    twRC = GPHOTO2_JPEGCompressionSet (pOrigin, pData);
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
                    twRC = GPHOTO2_Palette8Get (pOrigin, pData);
                    break;
                case MSG_GETDEFAULT:
                    twRC = GPHOTO2_Palette8GetDefault (pOrigin, pData);
                    break;
                case MSG_RESET:
                    twRC = GPHOTO2_Palette8Reset (pOrigin, pData);
                    break;
                case MSG_SET:
                    twRC = GPHOTO2_Palette8Set (pOrigin, pData);
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
                    twRC = GPHOTO2_RGBResponseReset (pOrigin, pData);
                    break;
                case MSG_SET:
                    twRC = GPHOTO2_RGBResponseSet (pOrigin, pData);
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
            twRC = GPHOTO2_SourceControlHandler (pOrigin,DAT,MSG,pData);
            break;
        case DG_IMAGE:
            twRC = GPHOTO2_ImageGroupHandler (pOrigin,DAT,MSG,pData);
            break;
        case DG_AUDIO:
            FIXME("The audio group of entry codes is not implemented.\n");
        default:
            activeDS.twCC = TWCC_BADPROTOCOL;
            twRC = TWRC_FAILURE;
    }

    return twRC;
}
