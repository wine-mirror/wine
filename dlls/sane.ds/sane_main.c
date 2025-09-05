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

#include <stdarg.h>
#include <stdio.h>

#include "sane_i.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(twain);

struct tagActiveDS activeDS;

DSMENTRYPROC SANE_dsmentry;
HINSTANCE SANE_instance;

BOOL WINAPI DllMain (HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    TRACE("%p,%lx,%p\n", hinstDLL, fdwReason, lpvReserved);

    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH: {
	    SANE_instance = hinstDLL;
            DisableThreadLibraryCalls(hinstDLL);
            if (__wine_init_unix_call()) return FALSE;
            SANE_CALL( process_attach, NULL );
            break;
	}
        case DLL_PROCESS_DETACH:
            if (lpvReserved) break;
            SANE_CALL( process_detach, NULL );
            break;
    }

    return TRUE;
}

static TW_UINT16 SANE_OpenDS( pTW_IDENTITY pOrigin, pTW_IDENTITY self)
{
    if (SANE_dsmentry == NULL)
    {
        HMODULE moddsm = GetModuleHandleW(L"twain_32");

        if (moddsm)
            SANE_dsmentry = (void*)GetProcAddress(moddsm, "DSM_Entry");

        if (!SANE_dsmentry)
        {
            ERR("can't find DSM entry point\n");
            return TWRC_FAILURE;
        }
    }

    if (SANE_CALL( open_ds, self )) return TWRC_FAILURE;

    activeDS.twCC = SANE_SaneSetDefaults();
    if (activeDS.twCC == TWCC_SUCCESS)
    {
        strcpy(activeDS.identity.Manufacturer, self->Manufacturer);
        strcpy(activeDS.identity.ProductFamily, self->ProductFamily);
        strcpy(activeDS.identity.ProductName, self->ProductName);
        activeDS.currentState = 4;
        activeDS.identity.Id = self->Id;
        activeDS.appIdentity = *pOrigin;
        return TWRC_SUCCESS;
    }
    SANE_CALL( close_ds, NULL );
    return TWRC_FAILURE;
}

static TW_UINT16 SANE_SetEntryPoint (pTW_IDENTITY pOrigin, TW_MEMREF pData);

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
                    SANE_CALL( close_ds, NULL );
                    break;
		case MSG_OPENDS:
		     twRC = SANE_OpenDS( pOrigin, (pTW_IDENTITY)pData);
		     break;
		case MSG_GET:
                {
                    if (SANE_CALL( get_identity, pData ))
                    {
                        activeDS.twCC = TWCC_CAPUNSUPPORTED;
                        twRC = TWRC_FAILURE;
                    }
                    break;
                }
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
                    FIXME("unrecognized operation triplet\n");
                    break;
            }
            break;

        case DAT_ENTRYPOINT:
            if (MSG == MSG_SET)
                twRC = SANE_SetEntryPoint (pOrigin, pData);
            else
            {
                twRC = TWRC_FAILURE;
                activeDS.twCC = TWCC_CAPBADOPERATION;
                FIXME("unrecognized operation triplet\n");
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

void SANE_Notify (TW_UINT16 message)
{
    SANE_dsmentry (&activeDS.identity, &activeDS.appIdentity, DG_CONTROL, DAT_NULL, message, NULL);
}

/* DG_CONTROL/DAT_ENTRYPOINT/MSG_SET */
TW_UINT16 SANE_SetEntryPoint (pTW_IDENTITY pOrigin, TW_MEMREF pData)
{
    TW_ENTRYPOINT *entry = (TW_ENTRYPOINT*)pData;

    SANE_dsmentry = entry->DSM_Entry;

    return TWRC_SUCCESS;
}
