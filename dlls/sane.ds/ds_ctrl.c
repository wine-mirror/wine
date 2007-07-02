/*
 * Copyright 2000 Corel Corporation
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

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <stdlib.h>
#include "twain.h"
#include "sane_i.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(twain);

/* DG_CONTROL/DAT_CAPABILITY/MSG_GET */
TW_UINT16 SANE_CapabilityGet (pTW_IDENTITY pOrigin, TW_MEMREF pData)
{
    TW_UINT16 twRC = TWRC_SUCCESS, twCC = TWCC_SUCCESS;
    pTW_CAPABILITY pCapability = (pTW_CAPABILITY) pData;

    TRACE("DG_CONTROL/DAT_CAPABILITY/MSG_GET\n");

    if (activeDS.currentState < 4 || activeDS.currentState > 7)
    {
        twRC = TWRC_FAILURE;
        activeDS.twCC = TWCC_SEQERROR;
    }
    else
    {
        twCC = SANE_SaneCapability (pCapability, MSG_GET);
        twRC = (twCC == TWCC_SUCCESS)?TWRC_SUCCESS:TWRC_FAILURE;
        activeDS.twCC = twCC;
    }

    return twRC;
}

/* DG_CONTROL/DAT_CAPABILITY/MSG_GETCURRENT */
TW_UINT16 SANE_CapabilityGetCurrent (pTW_IDENTITY pOrigin, TW_MEMREF pData)
{
    TW_UINT16 twRC = TWRC_SUCCESS, twCC = TWCC_SUCCESS;
    pTW_CAPABILITY pCapability = (pTW_CAPABILITY) pData;

    TRACE("DG_CONTROL/DAT_CAPABILITY/MSG_GETCURRENT\n");

    if (activeDS.currentState < 4 || activeDS.currentState > 7)
    {
        twRC = TWRC_FAILURE;
        activeDS.twCC = TWCC_SEQERROR;
    }
    else
    {
        twCC = SANE_SaneCapability (pCapability, MSG_GETCURRENT);
        twRC = (twCC == TWCC_SUCCESS)?TWRC_SUCCESS:TWRC_FAILURE;
        activeDS.twCC = twCC;
    }

    return twRC;
}

/* DG_CONTROL/DAT_CAPABILITY/MSG_GETDEFAULT */
TW_UINT16 SANE_CapabilityGetDefault (pTW_IDENTITY pOrigin, TW_MEMREF pData)
{
    TW_UINT16 twRC = TWRC_SUCCESS, twCC = TWCC_SUCCESS;
    pTW_CAPABILITY pCapability = (pTW_CAPABILITY) pData;

    TRACE("DG_CONTROL/DAT_CAPABILITY/MSG_GETDEFAULT\n");

    if (activeDS.currentState < 4 || activeDS.currentState > 7)
    {
        twRC = TWRC_FAILURE;
        activeDS.twCC = TWCC_SEQERROR;
    }
    else
    {
        twCC = SANE_SaneCapability (pCapability, MSG_GETDEFAULT);
        twRC = (twCC == TWCC_SUCCESS)?TWRC_SUCCESS:TWRC_FAILURE;
        activeDS.twCC = twCC;
    }

    return twRC;
}

/* DG_CONTROL/DAT_CAPABILITY/MSG_QUERYSUPPORT */
TW_UINT16 SANE_CapabilityQuerySupport (pTW_IDENTITY pOrigin,
                                        TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_CONTROL/DAT_CAPABILITY/MSG_RESET */
TW_UINT16 SANE_CapabilityReset (pTW_IDENTITY pOrigin, 
                                 TW_MEMREF pData)
{
    TW_UINT16 twRC = TWRC_SUCCESS, twCC = TWCC_SUCCESS;
    pTW_CAPABILITY pCapability = (pTW_CAPABILITY) pData;

    TRACE("DG_CONTROL/DAT_CAPABILITY/MSG_RESET\n");

    if (activeDS.currentState < 4 || activeDS.currentState > 7)
    {
        twRC = TWRC_FAILURE;
        activeDS.twCC = TWCC_SEQERROR;
    }
    else
    {
        twCC = SANE_SaneCapability (pCapability, MSG_RESET);
        twRC = (twCC == TWCC_SUCCESS)?TWRC_SUCCESS:TWRC_FAILURE;
        activeDS.twCC = twCC;
    }

    return twRC;
}

/* DG_CONTROL/DAT_CAPABILITY/MSG_SET */
TW_UINT16 SANE_CapabilitySet (pTW_IDENTITY pOrigin, 
                               TW_MEMREF pData)
{
    TW_UINT16 twRC = TWRC_SUCCESS, twCC = TWCC_SUCCESS;
    pTW_CAPABILITY pCapability = (pTW_CAPABILITY) pData;

    TRACE ("DG_CONTROL/DAT_CAPABILITY/MSG_SET\n");

    if (activeDS.currentState != 4)
    {
        twRC = TWRC_FAILURE;
        activeDS.twCC = TWCC_SEQERROR;
    }
    else
    {
        twCC = SANE_SaneCapability (pCapability, MSG_SET);
        twRC = (twCC == TWCC_SUCCESS)?TWRC_SUCCESS:TWRC_FAILURE;
        activeDS.twCC = twCC;
    }
    return twRC;
}

/* DG_CONTROL/DAT_CUSTOMDSDATA/MSG_GET */
TW_UINT16 SANE_CustomDSDataGet (pTW_IDENTITY pOrigin, 
                                TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_CONTROL/DAT_CUSTOMDSDATA/MSG_SET */
TW_UINT16 SANE_CustomDSDataSet (pTW_IDENTITY pOrigin, 
                                 TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_CONTROL/DAT_FILESYSTEM/MSG_AUTOMATICCAPTUREDIRECTORY */
TW_UINT16 SANE_AutomaticCaptureDirectory (pTW_IDENTITY pOrigin,
                                           
                                           TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_CONTROL/DAT_FILESYSTEM/MSG_CHANGEDIRECTORY */
TW_UINT16 SANE_ChangeDirectory (pTW_IDENTITY pOrigin, 
                                 TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_CONTROL/DAT_FILESYSTEM/MSG_COPY */
TW_UINT16 SANE_FileSystemCopy (pTW_IDENTITY pOrigin, 
                                TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_CONTROL/DAT_FILESYSTEM/MSG_CREATEDIRECTORY */
TW_UINT16 SANE_CreateDirectory (pTW_IDENTITY pOrigin, 
                                 TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_CONTROL/DAT_FILESYSTEM/MSG_DELETE */
TW_UINT16 SANE_FileSystemDelete (pTW_IDENTITY pOrigin, 
                                  TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_CONTROL/DAT_FILESYSTEM/MSG_FORMATMEDIA */
TW_UINT16 SANE_FormatMedia (pTW_IDENTITY pOrigin, 
                             TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_CONTROL/DAT_FILESYSTEM/MSG_GETCLOSE */
TW_UINT16 SANE_FileSystemGetClose (pTW_IDENTITY pOrigin, 
                                    TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_CONTROL/DAT_FILESYSTEM/MSG_GETFIRSTFILE */
TW_UINT16 SANE_FileSystemGetFirstFile (pTW_IDENTITY pOrigin,
                                        
                                        TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_CONTROL/DAT_FILESYSTEM/MSG_GETINFO */
TW_UINT16 SANE_FileSystemGetInfo (pTW_IDENTITY pOrigin, 
                                   TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_CONTROL/DAT_FILESYSTEM/MSG_GETNEXTFILE */
TW_UINT16 SANE_FileSystemGetNextFile (pTW_IDENTITY pOrigin,
                                       
                                       TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_CONTROL/DAT_FILESYSTEM/MSG_RENAME */
TW_UINT16 SANE_FileSystemRename (pTW_IDENTITY pOrigin, 
                                  TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_CONTROL/DAT_EVENT/MSG_PROCESSEVENT */
TW_UINT16 SANE_ProcessEvent (pTW_IDENTITY pOrigin, 
                              TW_MEMREF pData)
{
    TW_UINT16 twRC = TWRC_SUCCESS;
    pTW_EVENT pEvent = (pTW_EVENT) pData;

    TRACE("DG_CONTROL/DAT_EVENT/MSG_PROCESSEVENT\n");

    if (activeDS.currentState < 5 || activeDS.currentState > 7)
    {
        twRC = TWRC_FAILURE;
        activeDS.twCC = TWCC_SEQERROR;
    }
    else
    {
        if (activeDS.pendingEvent.TWMessage != MSG_NULL)
        {
            pEvent->TWMessage = activeDS.pendingEvent.TWMessage;
            activeDS.pendingEvent.TWMessage = MSG_NULL;
            twRC = TWRC_NOTDSEVENT;
        }
        else
        {
            pEvent->TWMessage = MSG_NULL;  /* no message to the application */
            twRC = TWRC_NOTDSEVENT;
        }
        activeDS.twCC = TWCC_SUCCESS;
    }

    return twRC;
}

/* DG_CONTROL/DAT_PASSTHRU/MSG_PASSTHRU */
TW_UINT16 SANE_PassThrough (pTW_IDENTITY pOrigin, 
                             TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_CONTROL/DAT_PENDINGXFERS/MSG_ENDXFER */
TW_UINT16 SANE_PendingXfersEndXfer (pTW_IDENTITY pOrigin, 
                                     TW_MEMREF pData)
{
    TW_UINT16 twRC = TWRC_SUCCESS;
    pTW_PENDINGXFERS pPendingXfers = (pTW_PENDINGXFERS) pData;

    TRACE("DG_CONTROL/DAT_PENDINGXFERS/MSG_ENDXFER\n");

    if (activeDS.currentState != 6 && activeDS.currentState != 7)
    {
        twRC = TWRC_FAILURE;
        activeDS.twCC = TWCC_SEQERROR;
    }
    else
    {
        if (pPendingXfers->Count != 0)
        {
            pPendingXfers->Count --;
            activeDS.currentState = 6;
        }
        else
        {
            activeDS.currentState = 5;
            /* Notify the application that it can close the data source */
            activeDS.pendingEvent.TWMessage = MSG_CLOSEDSREQ;
        }
        twRC = TWRC_SUCCESS;
        activeDS.twCC = TWCC_SUCCESS;
    }

    return twRC;
}

/* DG_CONTROL/DAT_PENDINGXFERS/MSG_GET */
TW_UINT16 SANE_PendingXfersGet (pTW_IDENTITY pOrigin, 
                                 TW_MEMREF pData)
{
    TW_UINT16 twRC = TWRC_SUCCESS;
    pTW_PENDINGXFERS pPendingXfers = (pTW_PENDINGXFERS) pData;

    TRACE("DG_CONTROL/DAT_PENDINGXFERS/MSG_GET\n");

    if (activeDS.currentState < 4 || activeDS.currentState > 7)
    {
        twRC = TWRC_FAILURE;
        activeDS.twCC = TWCC_SEQERROR;
    }
    else
    {
        /* FIXME: we shouldn't return 1 here */
        pPendingXfers->Count = 1;
        twRC = TWRC_SUCCESS;
        activeDS.twCC = TWCC_SUCCESS;
    }

    return twRC;
}

/* DG_CONTROL/DAT_PENDINGXFERS/MSG_RESET */
TW_UINT16 SANE_PendingXfersReset (pTW_IDENTITY pOrigin, 
                                   TW_MEMREF pData)
{
    TW_UINT16 twRC = TWRC_SUCCESS;
    pTW_PENDINGXFERS pPendingXfers = (pTW_PENDINGXFERS) pData;

    TRACE("DG_CONTROL/DAT_PENDINGXFERS/MSG_RESET\n");

    if (activeDS.currentState != 6)
    {
        twRC = TWRC_FAILURE;
        activeDS.twCC = TWCC_SEQERROR;
    }
    else
    {
        pPendingXfers->Count = 0;
        activeDS.currentState = 5;
        twRC = TWRC_SUCCESS;
        activeDS.twCC = TWCC_SUCCESS;
    }

    return twRC;
}

/* DG_CONTROL/DAT_PENDINGXFERS/MSG_STOPFEEDER */
TW_UINT16 SANE_PendingXfersStopFeeder (pTW_IDENTITY pOrigin,
                                        TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_CONTROL/DAT_SETUPFILEXFER/MSG_GET */
TW_UINT16 SANE_SetupFileXferGet (pTW_IDENTITY pOrigin, 
                                  TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_CONTROL/DAT_SETUPXFER/MSG_GETDEFAULT */
TW_UINT16 SANE_SetupFileXferGetDefault (pTW_IDENTITY pOrigin, 
                                         TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}


/* DG_CONTROL/DAT_SETUPFILEXFER/MSG_RESET */
TW_UINT16 SANE_SetupFileXferReset (pTW_IDENTITY pOrigin, 
                                    TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_CONTROL/DAT_SETUPFILEXFER/MSG_SET */
TW_UINT16 SANE_SetupFileXferSet (pTW_IDENTITY pOrigin, 
                                  TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_CONTROL/DAT_SETUPFILEXFER2/MSG_GET */
TW_UINT16 SANE_SetupFileXfer2Get (pTW_IDENTITY pOrigin, 
                                   TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_CONTROL/DAT_SETUPFILEXFER2/MSG_GETDEFAULT */
TW_UINT16 SANE_SetupFileXfer2GetDefault (pTW_IDENTITY pOrigin, 
                                         TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_CONTROL/DAT_SETUPFILEXFER2/MSG_RESET */
TW_UINT16 SANE_SetupFileXfer2Reset (pTW_IDENTITY pOrigin, 
                                  TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_CONTROL/DAT_SETUPFILEXFER2/MSG_SET */
TW_UINT16 SANE_SetupFileXfer2Set (pTW_IDENTITY pOrigin, 
                                  TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_CONTROL/DAT_SETUPMEMXFER/MSG_GET */
TW_UINT16 SANE_SetupMemXferGet (pTW_IDENTITY pOrigin, 
                                  TW_MEMREF pData)
{
#ifndef SONAME_LIBSANE
    return TWRC_FAILURE;
#else
    pTW_SETUPMEMXFER  pSetupMemXfer = (pTW_SETUPMEMXFER)pData;

    TRACE("DG_CONTROL/DAT_SETUPMEMXFER/MSG_GET\n");
    if (activeDS.sane_param_valid)
    {
        pSetupMemXfer->MinBufSize = activeDS.sane_param.bytes_per_line;
        pSetupMemXfer->MaxBufSize = activeDS.sane_param.bytes_per_line * 8;
        pSetupMemXfer->Preferred = activeDS.sane_param.bytes_per_line * 2;
    }
    else
    {
        /* Guessing */
        pSetupMemXfer->MinBufSize = 2000;
        pSetupMemXfer->MaxBufSize = 8000;
        pSetupMemXfer->Preferred = 4000;
    }

    return TWRC_SUCCESS;
#endif
}

/* DG_CONTROL/DAT_STATUS/MSG_GET */
TW_UINT16 SANE_GetDSStatus (pTW_IDENTITY pOrigin, 
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
TW_UINT16 SANE_DisableDSUserInterface (pTW_IDENTITY pOrigin,
                                        TW_MEMREF pData)
{
    TW_UINT16 twRC = TWRC_SUCCESS;

    TRACE ("DG_CONTROL/DAT_USERINTERFACE/MSG_DISABLEDS\n");

    if (activeDS.currentState != 5)
    {
        twRC = TWRC_FAILURE;
        activeDS.twCC = TWCC_SEQERROR;
    }
    else
    {
        activeDS.currentState = 4;
        twRC = TWRC_SUCCESS;
        activeDS.twCC = TWCC_SUCCESS;
    }

    return twRC;
}

/* DG_CONTROL/DAT_USERINTERFACE/MSG_ENABLEDS */
TW_UINT16 SANE_EnableDSUserInterface (pTW_IDENTITY pOrigin,
                                       TW_MEMREF pData)
{
    TW_UINT16 twRC = TWRC_SUCCESS;
    pTW_USERINTERFACE pUserInterface = (pTW_USERINTERFACE) pData;

    TRACE ("DG_CONTROL/DAT_USERINTERFACE/MSG_ENABLEDS\n");

    if (activeDS.currentState != 4)
    {
        twRC = TWRC_FAILURE;
        activeDS.twCC = TWCC_SEQERROR;
	FIXME("sequence error %d\n", activeDS.currentState);
    }
    else
    {
        activeDS.hwndOwner = pUserInterface->hParent;
        if (pUserInterface->ShowUI)
        {
            BOOL rc;
            activeDS.currentState = 5; /* Transitions to state 5 */
		FIXME("showing UI\n");
            rc = DoScannerUI();
            if (!rc)
            {
                activeDS.pendingEvent.TWMessage = MSG_CLOSEDSREQ;
            }
#ifdef SONAME_LIBSANE
            else
            {
                psane_get_parameters (activeDS.deviceHandle, &activeDS.sane_param);
                activeDS.sane_param_valid = TRUE;
            }
#endif
        }
        else
        {
            /* no UI will be displayed, so source is ready to transfer data */
            activeDS.pendingEvent.TWMessage = MSG_XFERREADY;
            activeDS.currentState = 6; /* Transitions to state 6 directly */
        }

        activeDS.hwndOwner = pUserInterface->hParent;
        twRC = TWRC_SUCCESS;
        activeDS.twCC = TWCC_SUCCESS;
    }

    return twRC;
}

/* DG_CONTROL/DAT_USERINTERFACE/MSG_ENABLEDSUIONLY */
TW_UINT16 SANE_EnableDSUIOnly (pTW_IDENTITY pOrigin, 
                                TW_MEMREF pData)
{
    TW_UINT16 twRC = TWRC_SUCCESS;

    TRACE("DG_CONTROL/DAT_USERINTERFACE/MSG_ENABLEDSUIONLY\n");

    if (activeDS.currentState != 4)
    {
        twRC = TWRC_FAILURE;
        activeDS.twCC = TWCC_SEQERROR;
    }
    else
    {
        /* FIXME: we should replace xscanimage with our own UI */
        system ("xscanimage");
        activeDS.currentState = 5;
        twRC = TWRC_SUCCESS;
        activeDS.twCC = TWCC_SUCCESS;
    }

    return twRC;
}

/* DG_CONTROL/DAT_XFERGROUP/MSG_GET */
TW_UINT16 SANE_XferGroupGet (pTW_IDENTITY pOrigin, 
                              TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_CONTROL/DAT_XFERGROUP/MSG_SET */
TW_UINT16 SANE_XferGroupSet (pTW_IDENTITY pOrigin, 
                                  TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}
