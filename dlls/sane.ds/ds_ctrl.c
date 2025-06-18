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

#include <stdlib.h>
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
    TW_UINT16 twRC = TWRC_SUCCESS, twCC = TWCC_SUCCESS;
    pTW_CAPABILITY pCapability = (pTW_CAPABILITY) pData;

    TRACE("DG_CONTROL/DAT_CAPABILITY/MSG_QUERYSUPPORT\n");

    if (activeDS.currentState < 4 || activeDS.currentState > 7)
    {
        twRC = TWRC_FAILURE;
        activeDS.twCC = TWCC_SEQERROR;
    }
    else
    {
        twCC = SANE_SaneCapability (pCapability, MSG_QUERYSUPPORT);
        twRC = (twCC == TWCC_SUCCESS)?TWRC_SUCCESS:TWRC_FAILURE;
        activeDS.twCC = twCC;
    }

    return twRC;
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
        if (twCC == TWCC_CHECKSTATUS)
        {
            twCC = TWCC_SUCCESS;
            twRC = TWRC_CHECKSTATUS;
        }
        else
            twRC = (twCC == TWCC_SUCCESS)?TWRC_SUCCESS:TWRC_FAILURE;
        activeDS.twCC = twCC;
    }
    return twRC;
}

/* DG_CONTROL/DAT_EVENT/MSG_PROCESSEVENT */
TW_UINT16 SANE_ProcessEvent (pTW_IDENTITY pOrigin, 
                              TW_MEMREF pData)
{
    TW_UINT16 twRC = TWRC_NOTDSEVENT;
    pTW_EVENT pEvent = (pTW_EVENT) pData;
    MSG *pMsg = pEvent->pEvent;

    TRACE("DG_CONTROL/DAT_EVENT/MSG_PROCESSEVENT  msg 0x%x, wParam 0x%Ix\n", pMsg->message, pMsg->wParam);

    activeDS.twCC = TWCC_SUCCESS;
    pEvent->TWMessage = MSG_NULL;  /* no message to the application */

    if (activeDS.currentState < 5 || activeDS.currentState > 7)
    {
        twRC = TWRC_FAILURE;
        activeDS.twCC = TWCC_SEQERROR;
    }

    return twRC;
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
        pPendingXfers->Count = -1;
        activeDS.currentState = 6;
        if (SANE_CALL( start_device, NULL ))
        {
            pPendingXfers->Count = 0;
            activeDS.currentState = 5;
            /* Notify the application that it can close the data source */
            SANE_Notify(MSG_CLOSEDSREQ);
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
        pPendingXfers->Count = -1;
        if (SANE_CALL( start_device, NULL ))
            pPendingXfers->Count = 0;
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
        SANE_CALL( cancel_device, NULL );
    }

    return twRC;
}

/* DG_CONTROL/DAT_SETUPMEMXFER/MSG_GET */
TW_UINT16 SANE_SetupMemXferGet (pTW_IDENTITY pOrigin, 
                                  TW_MEMREF pData)
{
    pTW_SETUPMEMXFER  pSetupMemXfer = (pTW_SETUPMEMXFER)pData;

    TRACE("DG_CONTROL/DAT_SETUPMEMXFER/MSG_GET\n");
    if (activeDS.frame_params.bytes_per_line)
    {
        pSetupMemXfer->MinBufSize = activeDS.frame_params.bytes_per_line;
        pSetupMemXfer->MaxBufSize = activeDS.frame_params.bytes_per_line * 8;
        pSetupMemXfer->Preferred = activeDS.frame_params.bytes_per_line * 2;
    }
    else
    {
        /* Guessing */
        pSetupMemXfer->MinBufSize = 2000;
        pSetupMemXfer->MaxBufSize = 8000;
        pSetupMemXfer->Preferred = 4000;
    }

    return TWRC_SUCCESS;
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
	WARN("sequence error %d\n", activeDS.currentState);
    }
    else
    {
        activeDS.hwndOwner = pUserInterface->hParent;
        if (pUserInterface->ShowUI)
        {
            BOOL rc;
            activeDS.currentState = 5; /* Transitions to state 5 */
            rc = DoScannerUI();
            pUserInterface->ModalUI = TRUE;
            if (!rc)
            {
                SANE_Notify(MSG_CLOSEDSREQ);
            }
            else
            {
                get_sane_params( &activeDS.frame_params );
            }
        }
        else
        {
            /* no UI will be displayed, so source is ready to transfer data */
            activeDS.currentState = 6; /* Transitions to state 6 directly */
            SANE_Notify(MSG_XFERREADY);
        }

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
