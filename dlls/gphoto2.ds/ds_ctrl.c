/*
 * Copyright 2000 Corel Corporation
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

#include "config.h"

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include "twain.h"
#include "gphoto2_i.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(twain);

static void
load_filesystem(const char *folder) {
#ifdef HAVE_GPHOTO2
    int		i, count, ret;
    CameraList	*list;

    ret = gp_list_new (&list);
    if (ret < GP_OK)
	return;
    ret = gp_camera_folder_list_files (activeDS.camera, folder, list, activeDS.context);
    if (ret < GP_OK) {
	gp_list_free (list);
	return;
    }
    count = gp_list_count (list);
    if (count < GP_OK) {
	gp_list_free (list);
	return;
    }
    for (i = 0; i < count; i++) {
	const char *name;
	struct gphoto2_file *gpfile;

	ret = gp_list_get_name (list, i, &name);
	if (ret < GP_OK)
	    continue;
	gpfile = malloc(sizeof(struct gphoto2_file));
	if (!gpfile)
	    continue;
	TRACE("adding %s/%s\n", folder, name);
	gpfile->folder = strdup(folder);
	gpfile->filename = strdup(name);
	gpfile->download = FALSE;
	list_add_tail( &activeDS.files, &gpfile->entry );
    }
    gp_list_reset (list);

    ret = gp_camera_folder_list_folders (activeDS.camera, folder, list, activeDS.context);
    if (ret < GP_OK) {
	FIXME("list_folders failed\n");
	gp_list_free (list);
	return;
    }
    count = gp_list_count (list);
    if (count < GP_OK) {
	FIXME("list_folders failed\n");
	gp_list_free (list);
	return;
    }
    for (i = 0; i < count; i++) {
	const char *name;
	char *newfolder;
	ret = gp_list_get_name (list, i, &name);
	if (ret < GP_OK)
	    continue;
	TRACE("recursing into %s\n", name);
	newfolder = malloc(strlen(folder)+1+strlen(name)+1);
	if (!strcmp(folder,"/"))
	    sprintf (newfolder, "/%s", name);
	else
	    sprintf (newfolder, "%s/%s", folder, name);
	load_filesystem (newfolder); /* recurse ... happily */
    }
    gp_list_free (list);
#endif
}

/* DG_CONTROL/DAT_CAPABILITY/MSG_GET */
TW_UINT16 GPHOTO2_CapabilityGet (pTW_IDENTITY pOrigin, TW_MEMREF pData)
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
TW_UINT16 GPHOTO2_CapabilityGetCurrent (pTW_IDENTITY pOrigin, TW_MEMREF pData)
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
TW_UINT16 GPHOTO2_CapabilityGetDefault (pTW_IDENTITY pOrigin, TW_MEMREF pData)
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
TW_UINT16 GPHOTO2_CapabilityQuerySupport (pTW_IDENTITY pOrigin,
                                        TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_CONTROL/DAT_CAPABILITY/MSG_RESET */
TW_UINT16 GPHOTO2_CapabilityReset (pTW_IDENTITY pOrigin, 
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
TW_UINT16 GPHOTO2_CapabilitySet (pTW_IDENTITY pOrigin, 
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
TW_UINT16 GPHOTO2_CustomDSDataGet (pTW_IDENTITY pOrigin, 
                                TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_CONTROL/DAT_CUSTOMDSDATA/MSG_SET */
TW_UINT16 GPHOTO2_CustomDSDataSet (pTW_IDENTITY pOrigin, 
                                 TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_CONTROL/DAT_FILESYSTEM/MSG_AUTOMATICCAPTUREDIRECTORY */
TW_UINT16 GPHOTO2_AutomaticCaptureDirectory (pTW_IDENTITY pOrigin,
                                           
                                           TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_CONTROL/DAT_FILESYSTEM/MSG_CHANGEDIRECTORY */
TW_UINT16 GPHOTO2_ChangeDirectory (pTW_IDENTITY pOrigin, 
                                 TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_CONTROL/DAT_FILESYSTEM/MSG_COPY */
TW_UINT16 GPHOTO2_FileSystemCopy (pTW_IDENTITY pOrigin, 
                                TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_CONTROL/DAT_FILESYSTEM/MSG_CREATEDIRECTORY */
TW_UINT16 GPHOTO2_CreateDirectory (pTW_IDENTITY pOrigin, 
                                 TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_CONTROL/DAT_FILESYSTEM/MSG_DELETE */
TW_UINT16 GPHOTO2_FileSystemDelete (pTW_IDENTITY pOrigin, 
                                  TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_CONTROL/DAT_FILESYSTEM/MSG_FORMATMEDIA */
TW_UINT16 GPHOTO2_FormatMedia (pTW_IDENTITY pOrigin, 
                             TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_CONTROL/DAT_FILESYSTEM/MSG_GETCLOSE */
TW_UINT16 GPHOTO2_FileSystemGetClose (pTW_IDENTITY pOrigin, 
                                    TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_CONTROL/DAT_FILESYSTEM/MSG_GETFIRSTFILE */
TW_UINT16 GPHOTO2_FileSystemGetFirstFile (pTW_IDENTITY pOrigin,
                                        
                                        TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_CONTROL/DAT_FILESYSTEM/MSG_GETINFO */
TW_UINT16 GPHOTO2_FileSystemGetInfo (pTW_IDENTITY pOrigin, 
                                   TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_CONTROL/DAT_FILESYSTEM/MSG_GETNEXTFILE */
TW_UINT16 GPHOTO2_FileSystemGetNextFile (pTW_IDENTITY pOrigin,
                                       
                                       TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_CONTROL/DAT_FILESYSTEM/MSG_RENAME */
TW_UINT16 GPHOTO2_FileSystemRename (pTW_IDENTITY pOrigin, 
                                  TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_CONTROL/DAT_EVENT/MSG_PROCESSEVENT */
TW_UINT16 GPHOTO2_ProcessEvent (pTW_IDENTITY pOrigin, 
                              TW_MEMREF pData)
{
    TW_UINT16 twRC = TWRC_SUCCESS;
    pTW_EVENT pEvent = (pTW_EVENT) pData;

    TRACE("DG_CONTROL/DAT_EVENT/MSG_PROCESSEVENT\n");

    if (activeDS.currentState < 5 || activeDS.currentState > 7) {
        activeDS.twCC = TWCC_SEQERROR;
        return TWRC_FAILURE;
    }

    if (activeDS.pendingEvent.TWMessage != MSG_NULL) {
        pEvent->TWMessage = activeDS.pendingEvent.TWMessage;
        activeDS.pendingEvent.TWMessage = MSG_NULL;
        twRC = TWRC_SUCCESS;
    } else {
        pEvent->TWMessage = MSG_NULL;  /* no message to the application */
        twRC = TWRC_NOTDSEVENT;
    }
    activeDS.twCC = TWCC_SUCCESS;
    return twRC;
}

/* DG_CONTROL/DAT_PASSTHRU/MSG_PASSTHRU */
TW_UINT16 GPHOTO2_PassThrough (pTW_IDENTITY pOrigin, 
                             TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_CONTROL/DAT_PENDINGXFERS/MSG_ENDXFER */
TW_UINT16 GPHOTO2_PendingXfersEndXfer (pTW_IDENTITY pOrigin, 
                                     TW_MEMREF pData)
{
    TW_UINT32 count;
    pTW_PENDINGXFERS pPendingXfers = (pTW_PENDINGXFERS) pData;
    struct gphoto2_file *file;

    TRACE("DG_CONTROL/DAT_PENDINGXFERS/MSG_ENDXFER\n");

    if (activeDS.currentState != 6 && activeDS.currentState != 7) {
        activeDS.twCC = TWCC_SEQERROR;
        return TWRC_FAILURE;
    }
    count = 0;
    LIST_FOR_EACH_ENTRY( file, &activeDS.files, struct gphoto2_file, entry ) {
	if (file->download)
	    count++;
    }
    TRACE("count = %ld\n", count);
    pPendingXfers->Count = count;
    if (pPendingXfers->Count != 0) {
        activeDS.currentState = 6;
    } else {
        activeDS.currentState = 5;
        /* Notify the application that it can close the data source */
        activeDS.pendingEvent.TWMessage = MSG_CLOSEDSREQ;
        /* close any Transferring dialog */
        TransferringDialogBox(activeDS.progressWnd,-1);
        activeDS.progressWnd = 0;
    }
    activeDS.twCC = TWCC_SUCCESS;
    return TWRC_SUCCESS;
}

/* DG_CONTROL/DAT_PENDINGXFERS/MSG_GET */
TW_UINT16 GPHOTO2_PendingXfersGet (pTW_IDENTITY pOrigin, 
                                 TW_MEMREF pData)
{
    TW_UINT32 count;
    pTW_PENDINGXFERS pPendingXfers = (pTW_PENDINGXFERS) pData;
    struct gphoto2_file *file;

    TRACE("DG_CONTROL/DAT_PENDINGXFERS/MSG_GET\n");

    if (activeDS.currentState < 4 || activeDS.currentState > 7) {
        activeDS.twCC = TWCC_SEQERROR;
        return TWRC_FAILURE;
    }

    count = 0;
    LIST_FOR_EACH_ENTRY( file, &activeDS.files, struct gphoto2_file, entry ) {
	if (file->download)
	    count++;
    }
    TRACE("count = %ld\n", count);
    pPendingXfers->Count = count;
    activeDS.twCC = TWCC_SUCCESS;
    return TWRC_SUCCESS;
}

/* DG_CONTROL/DAT_PENDINGXFERS/MSG_RESET */
TW_UINT16 GPHOTO2_PendingXfersReset (pTW_IDENTITY pOrigin, 
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

/* DG_CONTROL/DAT_PENDINGXFERS/MSG_STOPFEEDER */
TW_UINT16 GPHOTO2_PendingXfersStopFeeder (pTW_IDENTITY pOrigin,
                                        TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_CONTROL/DAT_SETUPFILEXFER/MSG_GET */
TW_UINT16 GPHOTO2_SetupFileXferGet (pTW_IDENTITY pOrigin, 
                                  TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_CONTROL/DAT_SETUPXFER/MSG_GETDEFAULT */
TW_UINT16 GPHOTO2_SetupFileXferGetDefault (pTW_IDENTITY pOrigin, 
                                         TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}


/* DG_CONTROL/DAT_SETUPFILEXFER/MSG_RESET */
TW_UINT16 GPHOTO2_SetupFileXferReset (pTW_IDENTITY pOrigin, 
                                    TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_CONTROL/DAT_SETUPFILEXFER/MSG_SET */
TW_UINT16 GPHOTO2_SetupFileXferSet (pTW_IDENTITY pOrigin, 
                                  TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_CONTROL/DAT_SETUPFILEXFER2/MSG_GET */
TW_UINT16 GPHOTO2_SetupFileXfer2Get (pTW_IDENTITY pOrigin, 
                                   TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_CONTROL/DAT_SETUPFILEXFER2/MSG_GETDEFAULT */
TW_UINT16 GPHOTO2_SetupFileXfer2GetDefault (pTW_IDENTITY pOrigin, 
                                         TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_CONTROL/DAT_SETUPFILEXFER2/MSG_RESET */
TW_UINT16 GPHOTO2_SetupFileXfer2Reset (pTW_IDENTITY pOrigin, 
                                  TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_CONTROL/DAT_SETUPFILEXFER2/MSG_SET */
TW_UINT16 GPHOTO2_SetupFileXfer2Set (pTW_IDENTITY pOrigin, 
                                  TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_CONTROL/DAT_SETUPMEMXFER/MSG_GET */
TW_UINT16 GPHOTO2_SetupMemXferGet (pTW_IDENTITY pOrigin, 
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
TW_UINT16 GPHOTO2_GetDSStatus (pTW_IDENTITY pOrigin, 
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
TW_UINT16 GPHOTO2_DisableDSUserInterface (pTW_IDENTITY pOrigin,
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
TW_UINT16 GPHOTO2_EnableDSUserInterface (pTW_IDENTITY pOrigin,
                                       TW_MEMREF pData)
{
    pTW_USERINTERFACE pUserInterface = (pTW_USERINTERFACE) pData;

    load_filesystem("/");

    TRACE ("DG_CONTROL/DAT_USERINTERFACE/MSG_ENABLEDS\n");
    if (activeDS.currentState != 4) {
	FIXME("Sequence error %d\n", activeDS.currentState);
        activeDS.twCC = TWCC_SEQERROR;
	return TWRC_FAILURE;
    }
    activeDS.hwndOwner = pUserInterface->hParent;
    if (pUserInterface->ShowUI)
    {
	BOOL rc;
	activeDS.currentState = 5; /* Transitions to state 5 */
	rc = DoCameraUI();
	if (!rc) {
	    activeDS.pendingEvent.TWMessage = MSG_CLOSEDSREQ;
	} else {
	    /* FIXME: The GUI should have marked the files to download... */
	    activeDS.pendingEvent.TWMessage = MSG_XFERREADY;
	    activeDS.currentState = 6; /* Transitions to state 6 directly */
	}
    } else {
	/* no UI will be displayed, so source is ready to transfer data */
	activeDS.pendingEvent.TWMessage = MSG_XFERREADY;
	activeDS.currentState = 6; /* Transitions to state 6 directly */
    }
    activeDS.hwndOwner = pUserInterface->hParent;
    activeDS.twCC = TWCC_SUCCESS;
    return TWRC_SUCCESS;
}

/* DG_CONTROL/DAT_USERINTERFACE/MSG_ENABLEDSUIONLY */
TW_UINT16 GPHOTO2_EnableDSUIOnly (pTW_IDENTITY pOrigin, 
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
TW_UINT16 GPHOTO2_XferGroupGet (pTW_IDENTITY pOrigin, 
                              TW_MEMREF pData)
{
    FIXME ("stub!\n");
    return TWRC_FAILURE;
}

/* DG_CONTROL/DAT_XFERGROUP/MSG_SET */
TW_UINT16 GPHOTO2_XferGroupSet (pTW_IDENTITY pOrigin, 
                                  TW_MEMREF pData)
{
    FIXME ("stub!\n");
    return TWRC_FAILURE;
}
