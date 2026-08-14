/* FAudio - XAudio Reimplementation for FNA
 *
 * Copyright (c) 2011-2024 Ethan Lee, Luigi Auriemma, and the MonoGame Team
 *
 * This software is provided 'as-is', without any express or implied warranty.
 * In no event will the authors be held liable for any damages arising from
 * the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 * claim that you wrote the original software. If you use this software in a
 * product, an acknowledgment in the product documentation would be
 * appreciated but is not required.
 *
 * 2. Altered source versions must be plainly marked as such, and must not be
 * misrepresented as being the original software.
 *
 * 3. This notice may not be removed or altered from any source distribution.
 *
 * Ethan "flibitijibibo" Lee <flibitijibibo@flibitijibibo.com>
 *
 */

#include "FAudioFX.h"
#include "FACT_internal.h"

/* AudioEngine implementation */

static void remove_single_notification(FACTAudioEngine *engine, size_t index)
{
	--engine->notification_count;
	if (index < engine->notification_count)
		FAudio_memmove(&engine->notifications[index], &engine->notifications[index + 1],
				(engine->notification_count - index) * sizeof(FACTNotificationDescription));
}

static void send_wavebank_notification(FACTAudioEngine *engine, uint8_t type, FACTWaveBank *wavebank)
{
	FACTNotification notification;

	notification.type = type;
	notification.waveBank.pWaveBank = wavebank;

	for (ptrdiff_t i = engine->notification_count - 1; i >= 0; --i)
	{
		FACTNotificationDescription *desc = &engine->notifications[i];

		if (desc->type == type && (!desc->pWaveBank || desc->pWaveBank == wavebank))
		{
			notification.pvContext = desc->pvContext;
			if (!(desc->flags & FACT_FLAG_NOTIFICATION_PERSIST))
				remove_single_notification(engine, i);
			engine->notificationCallback(&notification);
			break;
		}
	}
}

static void send_wave_notification(FACTAudioEngine *engine, uint8_t type, const FACTNotificationWave *wave)
{
	FACTNotification notification;

	notification.type = type;
	notification.wave = *wave;

	for (ptrdiff_t i = engine->notification_count - 1; i >= 0; --i)
	{
		FACTNotificationDescription *desc = &engine->notifications[i];

		if (desc->type == type && (!desc->pWave || desc->pWave == wave->pWave))
		{
			notification.pvContext = desc->pvContext;
			if (!(desc->flags & FACT_FLAG_NOTIFICATION_PERSIST))
				remove_single_notification(engine, i);
			engine->notificationCallback(&notification);
			break;
		}
	}
}

static void send_soundbank_notification(FACTAudioEngine *engine, FACTSoundBank *soundbank)
{
	FACTNotification notification;

	notification.type = FACTNOTIFICATIONTYPE_SOUNDBANKDESTROYED;
	notification.soundBank.pSoundBank = soundbank;

	for (ptrdiff_t i = engine->notification_count - 1; i >= 0; --i)
	{
		FACTNotificationDescription *desc = &engine->notifications[i];

		if (desc->type == FACTNOTIFICATIONTYPE_SOUNDBANKDESTROYED && (!desc->pSoundBank || desc->pSoundBank == soundbank))
		{
			notification.pvContext = desc->pvContext;
			if (!(desc->flags & FACT_FLAG_NOTIFICATION_PERSIST))
				remove_single_notification(engine, i);
			engine->notificationCallback(&notification);
			break;
		}
	}
}

void FACT_INTERNAL_SendCueNotification(FACTCue *cue, uint8_t type)
{
	FACTAudioEngine *engine = cue->parentBank->parentEngine;
	FACTNotification notification;

	notification.type = type;
	notification.cue.cueIndex = cue->index;
	notification.cue.pSoundBank = cue->parentBank;
	notification.cue.pCue = cue;

	for (ptrdiff_t i = engine->notification_count - 1; i >= 0; --i)
	{
		FACTNotificationDescription *desc = &engine->notifications[i];

		if (desc->type == type && (!desc->pCue || desc->pCue == cue))
		{
			notification.pvContext = desc->pvContext;
			if (!(desc->flags & FACT_FLAG_NOTIFICATION_PERSIST))
				remove_single_notification(engine, i);
			engine->notificationCallback(&notification);
			break;
		}
	}
}

uint32_t FACTCreateEngine(
	uint32_t dwCreationFlags,
	FACTAudioEngine **ppEngine
) {
	return FACTCreateEngineWithCustomAllocatorEXT(
		dwCreationFlags,
		ppEngine,
		FAudio_malloc,
		FAudio_free,
		FAudio_realloc
	);
}

uint32_t FACTCreateEngineWithCustomAllocatorEXT(
	uint32_t dwCreationFlags,
	FACTAudioEngine **ppEngine,
	FAudioMallocFunc customMalloc,
	FAudioFreeFunc customFree,
	FAudioReallocFunc customRealloc
) {
	/* TODO: Anything fun with dwCreationFlags? */
	FAudio_PlatformAddRef();
	*ppEngine = (FACTAudioEngine*) customMalloc(sizeof(FACTAudioEngine));
	if (*ppEngine == NULL)
	{
		return FAUDIO_E_OUT_OF_MEMORY;
	}
	FAudio_zero(*ppEngine, sizeof(FACTAudioEngine));
	(*ppEngine)->sbLock = FAudio_PlatformCreateMutex();
	(*ppEngine)->wbLock = FAudio_PlatformCreateMutex();
	(*ppEngine)->apiLock = FAudio_PlatformCreateMutex();
	(*ppEngine)->pMalloc = customMalloc;
	(*ppEngine)->pFree = customFree;
	(*ppEngine)->pRealloc = customRealloc;
	(*ppEngine)->refcount = 1;
	return FAUDIO_OK;
}

uint32_t FACTAudioEngine_AddRef(FACTAudioEngine *pEngine)
{
	FAudio_PlatformLockMutex(pEngine->apiLock);
	pEngine->refcount += 1;
	FAudio_PlatformUnlockMutex(pEngine->apiLock);
	return pEngine->refcount;
}

uint32_t FACTAudioEngine_Release(FACTAudioEngine *pEngine)
{
	FAudio_PlatformLockMutex(pEngine->apiLock);
	pEngine->refcount -= 1;
	if (pEngine->refcount > 0)
	{
		FAudio_PlatformUnlockMutex(pEngine->apiLock);
		return pEngine->refcount;
	}
	FACTAudioEngine_ShutDown(pEngine);
	FAudio_PlatformDestroyMutex(pEngine->sbLock);
	FAudio_PlatformDestroyMutex(pEngine->wbLock);
	FAudio_PlatformUnlockMutex(pEngine->apiLock);
	FAudio_PlatformDestroyMutex(pEngine->apiLock);
	if (pEngine->settings != NULL)
	{
		pEngine->pFree(pEngine->settings);
	}
	pEngine->pFree(pEngine);
	FAudio_PlatformRelease();
	return 0;
}

uint32_t FACTAudioEngine_GetRendererCount(
	FACTAudioEngine *pEngine,
	uint16_t *pnRendererCount
) {
	FAudio_PlatformLockMutex(pEngine->apiLock);
	*pnRendererCount = (uint16_t) FAudio_PlatformGetDeviceCount();
	FAudio_PlatformUnlockMutex(pEngine->apiLock);
	return FAUDIO_OK;
}

uint32_t FACTAudioEngine_GetRendererDetails(
	FACTAudioEngine *pEngine,
	uint16_t nRendererIndex,
	FACTRendererDetails *pRendererDetails
) {
	FAudioDeviceDetails deviceDetails;

	FAudio_PlatformLockMutex(pEngine->apiLock);

	FAudio_PlatformGetDeviceDetails(
		nRendererIndex,
		&deviceDetails
	);
	FAudio_memcpy(
		pRendererDetails->rendererID,
		deviceDetails.DeviceID,
		sizeof(int16_t) * 0xFF
	);
	FAudio_memcpy(
		pRendererDetails->displayName,
		deviceDetails.DisplayName,
		sizeof(int16_t) * 0xFF
	);
	/* FIXME: Which defaults does it care about...? */
	pRendererDetails->defaultDevice = (deviceDetails.Role & (
		FAudioGlobalDefaultDevice |
		FAudioDefaultGameDevice
	)) != 0;

	FAudio_PlatformUnlockMutex(pEngine->apiLock);
	return FAUDIO_OK;
}

uint32_t FACTAudioEngine_GetFinalMixFormat(
	FACTAudioEngine *pEngine,
	FAudioWaveFormatExtensible *pFinalMixFormat
) {
	*pFinalMixFormat = pEngine->output_format;
	return FAUDIO_OK;
}

uint32_t FACTAudioEngine_Initialize(
	FACTAudioEngine *pEngine,
	const FACTRuntimeParameters *pParams
) {
	FAudioDeviceDetails device_details;
	uint32_t parseRet;
	uint32_t deviceIndex;
	FAudioVoiceDetails masterDetails;
	FAudioEffectDescriptor reverbDesc;
	FAudioEffectChain reverbChain;

	FAudio_PlatformLockMutex(pEngine->apiLock);

	if (!pParams->pGlobalSettingsBuffer || pParams->globalSettingsBufferSize == 0)
	{
		/* No file? Just go with a safe default. (Also why are you using XACT) */
		pEngine->categoryCount = 3;
		pEngine->variableCount = 0;
		pEngine->rpcCount = 0;
		pEngine->dspPresetCount = 0;
		pEngine->dspParameterCount = 0;

		pEngine->categories = (FACTAudioCategory*) pEngine->pMalloc(
			sizeof(FACTAudioCategory) * pEngine->categoryCount
		);
		pEngine->categoryNames = (char**) pEngine->pMalloc(
			sizeof(char*) * pEngine->categoryCount
		);

		pEngine->categoryNames[0] = pEngine->pMalloc(7);
		FAudio_strlcpy(pEngine->categoryNames[0], "Global", 7);
		pEngine->categories[0].instanceLimit = 255;
		pEngine->categories[0].fadeInMS = 0;
		pEngine->categories[0].fadeOutMS = 0;
		pEngine->categories[0].maxInstanceBehavior = MAX_INSTANCE_BEHAVIOR_FAIL;
		pEngine->categories[0].parentCategory = -1;
		pEngine->categories[0].volume = 1.0f;
		pEngine->categories[0].visibility = 1;
		pEngine->categories[0].instanceCount = 0;
		pEngine->categories[0].currentVolume = 1.0f;

		pEngine->categoryNames[1] = pEngine->pMalloc(8);
		FAudio_strlcpy(pEngine->categoryNames[1], "Default", 8);
		pEngine->categories[1].instanceLimit = 255;
		pEngine->categories[1].fadeInMS = 0;
		pEngine->categories[1].fadeOutMS = 0;
		pEngine->categories[1].maxInstanceBehavior = MAX_INSTANCE_BEHAVIOR_FAIL;
		pEngine->categories[1].parentCategory = 0;
		pEngine->categories[1].volume = 1.0f;
		pEngine->categories[1].visibility = 1;
		pEngine->categories[1].instanceCount = 0;
		pEngine->categories[1].currentVolume = 1.0f;

		pEngine->categoryNames[2] = pEngine->pMalloc(6);
		FAudio_strlcpy(pEngine->categoryNames[2], "Music", 6);
		pEngine->categories[2].instanceLimit = 255;
		pEngine->categories[2].fadeInMS = 0;
		pEngine->categories[2].fadeOutMS = 0;
		pEngine->categories[2].maxInstanceBehavior = MAX_INSTANCE_BEHAVIOR_FAIL;
		pEngine->categories[2].parentCategory = 0;
		pEngine->categories[2].volume = 1.0f;
		pEngine->categories[2].visibility = 1;
		pEngine->categories[2].instanceCount = 0;
		pEngine->categories[2].currentVolume = 1.0f;

		pEngine->variables = NULL;
		pEngine->variableNames = NULL;
		pEngine->globalVariableValues = NULL;
		pEngine->rpcs = NULL;
		pEngine->dspPresets = NULL;
	}
	else
	{
		/* Parse the file */
		parseRet = FACT_INTERNAL_ParseAudioEngine(pEngine, pParams);
		if (parseRet != 0)
		{
			FAudio_PlatformUnlockMutex(pEngine->apiLock);
			return parseRet;
		}
	}

	pEngine->notifications = NULL;
	pEngine->notification_count = 0;
	pEngine->notifications_capacity = 0;

	/* Assign the callbacks */
	pEngine->notificationCallback = pParams->fnNotificationCallback;
	pEngine->pReadFile = pParams->fileIOCallbacks.readFileCallback;
	pEngine->pGetOverlappedResult = pParams->fileIOCallbacks.getOverlappedResultCallback;
	if (pEngine->pReadFile == NULL)
	{
		pEngine->pReadFile = FACT_INTERNAL_DefaultReadFile;
	}
	if (pEngine->pGetOverlappedResult == NULL)
	{
		pEngine->pGetOverlappedResult = FACT_INTERNAL_DefaultGetOverlappedResult;
	}

	/* Init the FAudio subsystem */
	pEngine->audio = pParams->pXAudio2;
	if (pEngine->audio == NULL)
	{
		FAudio_assert(pParams->pMasteringVoice == NULL);
		FAudioCreate(&pEngine->audio, 0, FAUDIO_DEFAULT_PROCESSOR);
	}

	if (!pParams->pRendererID || !pParams->pRendererID[0])
	{
		deviceIndex = 0;
	}
	else
	{
		deviceIndex = pParams->pRendererID[0] - L'0';
		if (deviceIndex > FAudio_PlatformGetDeviceCount())
			deviceIndex = 0;
	}

	FAudio_GetDeviceDetails(pEngine->audio, deviceIndex, &device_details);
	pEngine->output_format = device_details.OutputFormat;

	/* Create the audio device */
	pEngine->master = pParams->pMasteringVoice;
	if (pEngine->master == NULL)
	{
		if (FAudio_CreateMasteringVoice(
			pEngine->audio,
			&pEngine->master,
			FAUDIO_DEFAULT_CHANNELS,
			FAUDIO_DEFAULT_SAMPLERATE,
			0,
			deviceIndex,
			NULL
		) != 0) {
			FAudio_Release(pEngine->audio);
			FAudio_PlatformUnlockMutex(pEngine->apiLock);
			return FAUDIO_E_INVALID_CALL;
		}
	}

	/* Create the reverb effect, if applicable */
	if (pEngine->dspPresetCount > 0) /* Never more than 1...? */
	{
		FAudioVoice_GetVoiceDetails(pEngine->master, &masterDetails);

		/* Reverb effect chain... */
		FAudioCreateReverb(&reverbDesc.pEffect, 0);
		reverbDesc.InitialState = 1;
		reverbDesc.OutputChannels = (masterDetails.InputChannels == 6) ? 6 : 1;
		reverbChain.EffectCount = 1;
		reverbChain.pEffectDescriptors = &reverbDesc;

		/* Reverb submix voice... */
		FAudio_CreateSubmixVoice(
			pEngine->audio,
			&pEngine->reverbVoice,
			1, /* Reverb will be omnidirectional */
			masterDetails.InputSampleRate,
			0,
			0,
			NULL,
			&reverbChain
		);

		/* We can release now, the submix owns this! */
		FAPOBase_Release((FAPOBase*) reverbDesc.pEffect);
	}

	pEngine->initialized = true;
	pEngine->apiThread = FAudio_PlatformCreateThread(
		FACT_INTERNAL_APIThread,
		"FACT Thread",
		pEngine
	);

	FAudio_PlatformUnlockMutex(pEngine->apiLock);
	return FAUDIO_OK;
}

static void send_queued_wavebank_notifications(FACTAudioEngine *engine)
{
	for (size_t i = 0; i < engine->prepared_wavebank_count; ++i)
		send_wavebank_notification(engine, FACTNOTIFICATIONTYPE_WAVEBANKPREPARED, engine->prepared_wavebanks[i]);
	engine->prepared_wavebank_count = 0;
}

uint32_t FACTAudioEngine_ShutDown(FACTAudioEngine *pEngine)
{
	uint32_t i, refcount;
	FAudioMutex mutex;
	FAudioMallocFunc pMalloc;
	FAudioFreeFunc pFree;
	FAudioReallocFunc pRealloc;

	/* Close thread, then lock ASAP */
	pEngine->initialized = false;
	FAudio_PlatformWaitThread(pEngine->apiThread, NULL);
	FAudio_PlatformLockMutex(pEngine->apiLock);

	/* Stop the platform stream before freeing stuff! */
	if (pEngine->audio != NULL)
	{
		FAudio_StopEngine(pEngine->audio);
	}

	send_queued_wavebank_notifications(pEngine);
	pEngine->pFree(pEngine->prepared_wavebanks);

	/* This method destroys all existing cues, sound banks, and wave banks.
	 * It blocks until all cues are destroyed.
	 */
	while (pEngine->wbList != NULL)
	{
		FACTWaveBank_Destroy((FACTWaveBank*) pEngine->wbList->entry);
	}
	while (pEngine->sbList != NULL)
	{
		FACTSoundBank_Destroy((FACTSoundBank*) pEngine->sbList->entry);
	}

	pEngine->pFree(pEngine->notifications);
	pEngine->notification_count = 0;
	pEngine->notifications_capacity = 0;

	/* Category data */
	for (i = 0; i < pEngine->categoryCount; i += 1)
	{
		pEngine->pFree(pEngine->categoryNames[i]);
	}
	pEngine->pFree(pEngine->categoryNames);
	pEngine->pFree(pEngine->categories);

	/* Variable data */
	for (i = 0; i < pEngine->variableCount; i += 1)
	{
		pEngine->pFree(pEngine->variableNames[i]);
	}
	pEngine->pFree(pEngine->variableNames);
	pEngine->pFree(pEngine->variables);
	pEngine->pFree(pEngine->globalVariableValues);

	/* RPC data */
	for (i = 0; i < pEngine->rpcCount; i += 1)
	{
		pEngine->pFree(pEngine->rpcs[i].points);
	}
	pEngine->pFree(pEngine->rpcs);
	pEngine->pFree(pEngine->rpcCodes);

	/* DSP data */
	for (i = 0; i < pEngine->dspPresetCount; i += 1)
	{
		pEngine->pFree(pEngine->dspPresets[i].parameters);
	}
	pEngine->pFree(pEngine->dspPresets);
	pEngine->pFree(pEngine->dspPresetCodes);

	/* Audio resources */
	if (pEngine->reverbVoice != NULL)
	{
		FAudioVoice_DestroyVoice(pEngine->reverbVoice);
	}
	if (pEngine->master != NULL)
	{
		FAudioVoice_DestroyVoice(pEngine->master);
	}
	if (pEngine->audio != NULL)
	{
		FAudio_Release(pEngine->audio);
	}

	/* Finally. */
	refcount = pEngine->refcount;
	mutex = pEngine->apiLock;
	pMalloc = pEngine->pMalloc;
	pFree = pEngine->pFree;
	pRealloc = pEngine->pRealloc;
	FAudio_zero(pEngine, sizeof(FACTAudioEngine));
	pEngine->pMalloc = pMalloc;
	pEngine->pFree = pFree;
	pEngine->pRealloc = pRealloc;
	pEngine->refcount = refcount;
	pEngine->apiLock = mutex;

	FAudio_PlatformUnlockMutex(pEngine->apiLock);
	return FAUDIO_OK;
}

uint32_t FACTAudioEngine_DoWork(FACTAudioEngine *pEngine)
{
	uint8_t i;
	FACTCue *cue;
	LinkedList *list;
	FACTNotification *note;

	FAudio_PlatformLockMutex(pEngine->apiLock);

	send_queued_wavebank_notifications(pEngine);

	list = pEngine->sbList;
	while (list != NULL)
	{
		cue = ((FACTSoundBank*) list->entry)->cueList;
		while (cue != NULL)
		{
			if (cue->playingSound != NULL)
			for (i = 0; i < cue->playingSound->sound->trackCount; i += 1)
			{
				if (	cue->playingSound->tracks[i].upcomingWave.wave == NULL &&
					cue->playingSound->tracks[i].waveEvtInst->loopCount > 0	)
				{
					FACT_INTERNAL_GetNextWave(
						cue,
						cue->playingSound->sound,
						&cue->playingSound->sound->tracks[i],
						&cue->playingSound->tracks[i],
						cue->playingSound->tracks[i].waveEvt,
						cue->playingSound->tracks[i].waveEvtInst
					);
				}
			}
			cue = cue->next;
		}
		list = list->next;
	}

	FAudio_PlatformUnlockMutex(pEngine->apiLock);
	return FAUDIO_OK;
}

uint32_t FACTAudioEngine_CreateSoundBank(
	FACTAudioEngine *pEngine,
	const void *pvBuffer,
	uint32_t dwSize,
	uint32_t dwFlags,
	uint32_t dwAllocAttributes,
	FACTSoundBank **ppSoundBank
) {
	uint32_t retval;
	FAudio_PlatformLockMutex(pEngine->apiLock);
	retval = FACT_INTERNAL_ParseSoundBank(
		pEngine,
		pvBuffer,
		dwSize,
		ppSoundBank
	);
	FAudio_PlatformUnlockMutex(pEngine->apiLock);
	return retval;
}

uint32_t FACTAudioEngine_CreateInMemoryWaveBank(
	FACTAudioEngine *pEngine,
	const void *pvBuffer,
	uint32_t dwSize,
	uint32_t dwFlags,
	uint32_t dwAllocAttributes,
	FACTWaveBank **ppWaveBank
) {
	uint32_t retval;
	FAudio_PlatformLockMutex(pEngine->apiLock);
	retval = FACT_INTERNAL_ParseWaveBank(
		pEngine,
		FAudio_memopen((void*) pvBuffer, dwSize),
		0,
		0,
		FACT_INTERNAL_DefaultReadFile,
		FACT_INTERNAL_DefaultGetOverlappedResult,
		false,
		ppWaveBank
	);
	if (pEngine->prepared_wavebank_count == pEngine->prepared_wavebanks_capacity)
	{
		pEngine->prepared_wavebanks_capacity = FAudio_max(pEngine->prepared_wavebanks_capacity * 2, 8);
		pEngine->prepared_wavebanks = pEngine->pRealloc(pEngine->prepared_wavebanks,
			pEngine->prepared_wavebanks_capacity * sizeof(FACTWaveBank *));
	}
	pEngine->prepared_wavebanks[pEngine->prepared_wavebank_count++] = *ppWaveBank;
	FAudio_PlatformUnlockMutex(pEngine->apiLock);
	return retval;
}

uint32_t FACTAudioEngine_CreateStreamingWaveBank(
	FACTAudioEngine *pEngine,
	const FACTStreamingParameters *pParms,
	FACTWaveBank **ppWaveBank
) {
	FACTNotification *note;
	uint32_t retval, packetSize;
	FAudio_PlatformLockMutex(pEngine->apiLock);
	if (	pEngine->pReadFile == FACT_INTERNAL_DefaultReadFile &&
		pEngine->pGetOverlappedResult == FACT_INTERNAL_DefaultGetOverlappedResult	)
	{
		/* Our I/O doesn't care about packets, set to 0 as an optimization */
		packetSize = 0;
	}
	else
	{
		packetSize = pParms->packetSize * 2048;
	}
	retval = FACT_INTERNAL_ParseWaveBank(
		pEngine,
		pParms->file,
		pParms->offset,
		packetSize,
		pEngine->pReadFile,
		pEngine->pGetOverlappedResult,
		true,
		ppWaveBank
	);
	if (pEngine->prepared_wavebank_count == pEngine->prepared_wavebanks_capacity)
	{
		pEngine->prepared_wavebanks_capacity = FAudio_max(pEngine->prepared_wavebanks_capacity * 2, 8);
		pEngine->prepared_wavebanks = pEngine->pRealloc(pEngine->prepared_wavebanks,
			pEngine->prepared_wavebanks_capacity * sizeof(FACTWaveBank *));
	}
	pEngine->prepared_wavebanks[pEngine->prepared_wavebank_count++] = *ppWaveBank;
	FAudio_PlatformUnlockMutex(pEngine->apiLock);
	return retval;
}

uint32_t FACTAudioEngine_PrepareWave(
	FACTAudioEngine *pEngine,
	uint32_t dwFlags,
	const char *szWavePath,
	uint32_t wStreamingPacketSize,
	uint32_t dwAlignment,
	uint32_t dwPlayOffset,
	uint8_t nLoopCount,
	FACTWave **ppWave
) {
	/* TODO: FACTWave */
	return FAUDIO_OK;
}

uint32_t FACTAudioEngine_PrepareInMemoryWave(
	FACTAudioEngine *pEngine,
	uint32_t dwFlags,
	FACTWaveBankEntry entry,
	uint32_t *pdwSeekTable, /* Optional! */
	uint8_t *pbWaveData,
	uint32_t dwPlayOffset,
	uint8_t nLoopCount,
	FACTWave **ppWave
) {
	/* TODO: FACTWave */
	return FAUDIO_OK;
}

uint32_t FACTAudioEngine_PrepareStreamingWave(
	FACTAudioEngine *pEngine,
	uint32_t dwFlags,
	FACTWaveBankEntry entry,
	FACTStreamingParameters streamingParams,
	uint32_t dwAlignment,
	uint32_t *pdwSeekTable, /* Optional! */
	uint8_t *pbWaveData, /* ABI bug, do not use! */
	uint32_t dwPlayOffset,
	uint8_t nLoopCount,
	FACTWave **ppWave
) {
	/* TODO: FACTWave */
	return FAUDIO_OK;
}

uint32_t FACTAudioEngine_RegisterNotification(FACTAudioEngine *engine, const FACTNotificationDescription *desc)
{
	if (!desc)
		return FAUDIO_E_INVALID_ARG;

	FAudio_assert(engine != NULL);

	if (!engine->notificationCallback)
		return FACTENGINE_E_NONOTIFICATIONCALLBACK;

	if (desc->type == 0 || desc->type > FACTNOTIFICATIONTYPE_WAVEBANKSTREAMING_INVALIDCONTENT)
		return FAUDIO_E_INVALID_ARG;

	FAudio_PlatformLockMutex(engine->apiLock);

	if (engine->notification_count == engine->notifications_capacity)
	{
		engine->notifications_capacity = FAudio_max(engine->notifications_capacity * 2, 8);
		engine->notifications = engine->pRealloc(engine->notifications,
			engine->notifications_capacity * sizeof(FACTNotification));
	}
	engine->notifications[engine->notification_count++] = *desc;

	FAudio_PlatformUnlockMutex(engine->apiLock);
	return FAUDIO_OK;
}

static void unregister_notification(FACTAudioEngine *engine, const FACTNotificationDescription *desc)
{
	if (!desc)
	{
		/* Unregister all notifications. This behaviour is not documented. */
		engine->notification_count = 0;
		return;
	}

	for (size_t i = 0; i < engine->notification_count; ++i)
	{
		if (!memcmp(desc, &engine->notifications[i], sizeof(*desc)))
		{
			remove_single_notification(engine, i);
			return;
		}
	}

	FAudio_Log("No matching notification found.\n");
}

uint32_t FACTAudioEngine_UnRegisterNotification(FACTAudioEngine *engine, const FACTNotificationDescription *desc)
{
	FAudio_assert(engine != NULL);

	if (!engine->notificationCallback)
		return FACTENGINE_E_NONOTIFICATIONCALLBACK;

	FAudio_PlatformLockMutex(engine->apiLock);
	unregister_notification(engine, desc);
	FAudio_PlatformUnlockMutex(engine->apiLock);
	return FAUDIO_OK;
}

uint16_t FACTAudioEngine_GetCategory(
	FACTAudioEngine *pEngine,
	const char *szFriendlyName
) {
	uint16_t i;
	FAudio_PlatformLockMutex(pEngine->apiLock);
	for (i = 0; i < pEngine->categoryCount; i += 1)
	{
		if (FAudio_strcmp(szFriendlyName, pEngine->categoryNames[i]) == 0)
		{
			FAudio_PlatformUnlockMutex(pEngine->apiLock);
			return i;
		}
	}
	FAudio_PlatformUnlockMutex(pEngine->apiLock);
	return FACTCATEGORY_INVALID;
}

static bool FACT_INTERNAL_IsInCategory(
	FACTAudioEngine *engine,
	uint16_t target,
	uint16_t category
) {
	FACTAudioCategory *cat;

	/* Same category, no need to go on a crazy hunt */
	if (category == target)
	{
		return true;
	}

	/* Right, on with the crazy hunt */
	cat = &engine->categories[category];
	while (cat->parentCategory != -1)
	{
		if (cat->parentCategory == target)
		{
			return true;
		}
		cat = &engine->categories[cat->parentCategory];
	}
	return false;
}

uint32_t FACTAudioEngine_Stop(
	FACTAudioEngine *pEngine,
	uint16_t nCategory,
	uint32_t dwFlags
) {
	FACTCue *cue, *backup;
	LinkedList *list;

	FAudio_PlatformLockMutex(pEngine->apiLock);
	list = pEngine->sbList;
	while (list != NULL)
	{
		cue = ((FACTSoundBank*) list->entry)->cueList;
		while (cue != NULL)
		{
			if (	cue->playingSound != NULL &&
				FACT_INTERNAL_IsInCategory(
					pEngine,
					nCategory,
					cue->playingSound->sound->category
				)	)
			{
				if (	dwFlags == FACT_FLAG_STOP_IMMEDIATE &&
					cue->managed	)
				{
					/* Just blow this up now */
					backup = cue->next;
					FACTCue_Destroy(cue);
					cue = backup;
				}
				else
				{
					/* If managed, the mixer will destroy for us */
					FACTCue_Stop(cue, dwFlags);
					cue = cue->next;
				}
			}
			else
			{
				cue = cue->next;
			}
		}
		list = list->next;
	}
	FAudio_PlatformUnlockMutex(pEngine->apiLock);
	return FAUDIO_OK;
}

uint32_t FACTAudioEngine_SetVolume(
	FACTAudioEngine *pEngine,
	uint16_t nCategory,
	float volume
) {
	uint16_t i;
	FAudio_PlatformLockMutex(pEngine->apiLock);
	pEngine->categories[nCategory].currentVolume = (
		pEngine->categories[nCategory].volume *
		volume
	);
	for (i = 0; i < pEngine->categoryCount; i += 1)
	{
		if (pEngine->categories[i].parentCategory == nCategory)
		{
			FACTAudioEngine_SetVolume(
				pEngine,
				i,
				pEngine->categories[i].currentVolume
			);
		}
	}
	FAudio_PlatformUnlockMutex(pEngine->apiLock);
	return FAUDIO_OK;
}

uint32_t FACTAudioEngine_Pause(
	FACTAudioEngine *pEngine,
	uint16_t nCategory,
	int32_t fPause
) {
	FACTCue *cue;
	LinkedList *list;

	FAudio_PlatformLockMutex(pEngine->apiLock);
	list = pEngine->sbList;
	while (list != NULL)
	{
		cue = ((FACTSoundBank*) list->entry)->cueList;
		while (cue != NULL)
		{
			if (	cue->playingSound != NULL &&
				FACT_INTERNAL_IsInCategory(
					pEngine,
					nCategory,
					cue->playingSound->sound->category
				)	)
			{
				FACTCue_Pause(cue, fPause);
			}
			cue = cue->next;
		}
		list = list->next;
	}
	FAudio_PlatformUnlockMutex(pEngine->apiLock);
	return FAUDIO_OK;
}

uint16_t FACTAudioEngine_GetGlobalVariableIndex(
	FACTAudioEngine *pEngine,
	const char *szFriendlyName
) {
	uint16_t i;
	for (i = 0; i < pEngine->variableCount; i += 1)
	{
		if (!FAudio_strcmp(szFriendlyName, pEngine->variableNames[i]) &&
			!(pEngine->variables[i].accessibility & ACCESSIBILITY_CUE) &&
			(pEngine->variables[i].accessibility & ACCESSIBILITY_PUBLIC))
			return i;
	}
	return FACTVARIABLEINDEX_INVALID;
}

uint32_t FACTAudioEngine_SetGlobalVariable(
	FACTAudioEngine *pEngine,
	uint16_t nIndex,
	float nValue
) {
	FACTVariable *var;

	if (nIndex >= pEngine->variableCount)
		return FACTENGINE_E_INVALIDVARIABLEINDEX;
	var = &pEngine->variables[nIndex];
	if (!(var->accessibility & ACCESSIBILITY_PUBLIC) || (var->accessibility & ACCESSIBILITY_CUE)
			|| (var->accessibility & ACCESSIBILITY_READONLY))
		return FACTENGINE_E_INVALIDVARIABLEINDEX;

	FAudio_PlatformLockMutex(pEngine->apiLock);
	pEngine->globalVariableValues[nIndex] = FAudio_clamp(
		nValue,
		var->minValue,
		var->maxValue
	);
	FAudio_PlatformUnlockMutex(pEngine->apiLock);
	return FAUDIO_OK;
}

uint32_t FACTAudioEngine_GetGlobalVariable(
	FACTAudioEngine *pEngine,
	uint16_t nIndex,
	float *pnValue
) {
	FACTVariable *var;

	if (nIndex >= pEngine->variableCount)
		return FACTENGINE_E_INVALIDVARIABLEINDEX;
	var = &pEngine->variables[nIndex];
	if (!(var->accessibility & ACCESSIBILITY_PUBLIC) || (var->accessibility & ACCESSIBILITY_CUE))
		return FACTENGINE_E_INVALIDVARIABLEINDEX;

	FAudio_PlatformLockMutex(pEngine->apiLock);
	*pnValue = pEngine->globalVariableValues[nIndex];
	FAudio_PlatformUnlockMutex(pEngine->apiLock);
	return FAUDIO_OK;
}

/* SoundBank implementation */

uint16_t FACTSoundBank_GetCueIndex(
	FACTSoundBank *pSoundBank,
	const char *szFriendlyName
) {
	uint16_t i;
	if (pSoundBank == NULL)
	{
		return FACTINDEX_INVALID;
	}

	FAudio_PlatformLockMutex(pSoundBank->parentEngine->apiLock);
	if (pSoundBank->cueNames != NULL)
	for (i = 0; i < pSoundBank->cueCount; i += 1)
	{
		if (FAudio_strcmp(szFriendlyName, pSoundBank->cueNames[i]) == 0)
		{
			FAudio_PlatformUnlockMutex(
				pSoundBank->parentEngine->apiLock
			);
			return i;
		}
	}
	FAudio_PlatformUnlockMutex(pSoundBank->parentEngine->apiLock);
	return FACTINDEX_INVALID;
}

uint32_t FACTSoundBank_GetNumCues(
	FACTSoundBank *pSoundBank,
	uint16_t *pnNumCues
) {
	if (pSoundBank == NULL)
	{
		*pnNumCues = 0;
		return FAUDIO_OK;
	}

	FAudio_PlatformLockMutex(pSoundBank->parentEngine->apiLock);
	*pnNumCues = pSoundBank->cueCount;
	FAudio_PlatformUnlockMutex(pSoundBank->parentEngine->apiLock);
	return FAUDIO_OK;
}

uint32_t FACTSoundBank_GetCueProperties(
	FACTSoundBank *pSoundBank,
	uint16_t nCueIndex,
	FACTCueProperties *pProperties
) {
	uint16_t i;
	if (pSoundBank == NULL)
	{
		return 1;
	}

	FAudio_PlatformLockMutex(pSoundBank->parentEngine->apiLock);

	if (pSoundBank->cueNames == NULL)
	{
		FAudio_zero(pProperties->friendlyName, 0xFF);
	}
	else
	{
		FAudio_strlcpy(
			pProperties->friendlyName,
			pSoundBank->cueNames[nCueIndex],
			0xFF
		);
	}
	if (!(pSoundBank->cues[nCueIndex].flags & CUE_FLAG_SINGLE_SOUND))
	{
		for (i = 0; i < pSoundBank->variationCount; i += 1)
		{
			if (pSoundBank->variations[i].code == pSoundBank->cues[nCueIndex].sbCode)
			{
				break;
			}
		}

		FAudio_assert(i < pSoundBank->variationCount && "Variation table not found!");

		if (pSoundBank->variations[i].type == VARIATION_TABLE_TYPE_INTERACTIVE)
		{
			pProperties->interactive = 1;
			pProperties->iaVariableIndex = pSoundBank->variations[i].variable;
		}
		else
		{
			pProperties->interactive = 0;
			pProperties->iaVariableIndex = FACTINDEX_INVALID;
		}
		pProperties->numVariations = pSoundBank->variations[i].entryCount;
	}
	else
	{
		pProperties->interactive = 0;
		pProperties->iaVariableIndex = FACTINDEX_INVALID;
		pProperties->numVariations = 1;
	}
	pProperties->maxInstances = pSoundBank->cues[nCueIndex].instanceLimit;
	pProperties->currentInstances = pSoundBank->cues[nCueIndex].instanceCount;

	FAudio_PlatformUnlockMutex(pSoundBank->parentEngine->apiLock);
	return FAUDIO_OK;
}

uint32_t FACTSoundBank_Prepare(
	FACTSoundBank *pSoundBank,
	uint16_t nCueIndex,
	uint32_t dwFlags,
	int32_t timeOffset,
	FACTCue** ppCue
) {
	bool interactive = false;
	uint16_t i;
	FACTCue *latest;

	if (pSoundBank == NULL)
	{
		*ppCue = NULL;
		return 1;
	}

	if (nCueIndex >= pSoundBank->cueCount)
		return FACTENGINE_E_INVALIDVARIABLEINDEX;

	*ppCue = (FACTCue*) pSoundBank->parentEngine->pMalloc(sizeof(FACTCue));
	FAudio_zero(*ppCue, sizeof(FACTCue));

	FAudio_PlatformLockMutex(pSoundBank->parentEngine->apiLock);

	/* Engine references */
	(*ppCue)->parentBank = pSoundBank;
	(*ppCue)->index = nCueIndex;

	/* Sound data */
	(*ppCue)->data = &pSoundBank->cues[nCueIndex];
	if ((*ppCue)->data->flags & CUE_FLAG_SINGLE_SOUND)
	{
		for (i = 0; i < pSoundBank->soundCount; i += 1)
		{
			if ((*ppCue)->data->sbCode == pSoundBank->soundCodes[i])
			{
				(*ppCue)->sound = &pSoundBank->sounds[i];
				break;
			}
		}
	}
	else
	{
		for (i = 0; i < pSoundBank->variationCount; i += 1)
		{
			if ((*ppCue)->data->sbCode == pSoundBank->variations[i].code)
			{
				(*ppCue)->variation = &pSoundBank->variations[i];
				break;
			}
		}
		if ((*ppCue)->variation && (*ppCue)->variation->type == VARIATION_TABLE_TYPE_INTERACTIVE)
		{
			interactive = true;
			(*ppCue)->interactive = pSoundBank->parentEngine->variables[
				(*ppCue)->variation->variable
			].initialValue;
		}
	}

	/* Instance data */
	(*ppCue)->variableValues = (float*) pSoundBank->parentEngine->pMalloc(
		sizeof(float) * pSoundBank->parentEngine->variableCount
	);
	for (i = 0; i < pSoundBank->parentEngine->variableCount; i += 1)
	{
		(*ppCue)->variableValues[i] =
			pSoundBank->parentEngine->variables[i].initialValue;
	}

	/* Playback */
	(*ppCue)->state = FACT_STATE_PREPARED;

	/* Add to the SoundBank Cue list */
	if (pSoundBank->cueList == NULL)
	{
		pSoundBank->cueList = *ppCue;
	}
	else
	{
		latest = pSoundBank->cueList;
		while (latest->next != NULL)
		{
			latest = latest->next;
		}
		latest->next = *ppCue;
	}

	if (!interactive)
		create_sound(*ppCue);

	FAudio_PlatformUnlockMutex(pSoundBank->parentEngine->apiLock);
	return FAUDIO_OK;
}

uint32_t FACTSoundBank_Play(
	FACTSoundBank *pSoundBank,
	uint16_t nCueIndex,
	uint32_t dwFlags,
	int32_t timeOffset,
	FACTCue** ppCue /* Optional! */
) {
	FACTCue *result;
	if (pSoundBank == NULL)
	{
		if (ppCue != NULL)
		{
			*ppCue = NULL;
		}
		return 1;
	}

	FAudio_PlatformLockMutex(pSoundBank->parentEngine->apiLock);

	FACTSoundBank_Prepare(
		pSoundBank,
		nCueIndex,
		dwFlags,
		timeOffset,
		&result
	);
	if (ppCue != NULL)
	{
		*ppCue = result;
	}
	else
	{
		/* AKA we get to Destroy() this ourselves */
		result->managed = true;
	}
	FACTCue_Play(result);

	FAudio_PlatformUnlockMutex(pSoundBank->parentEngine->apiLock);
	return FAUDIO_OK;
}

uint32_t FACTSoundBank_Play3D(
	FACTSoundBank *pSoundBank,
	uint16_t nCueIndex,
	uint32_t dwFlags,
	int32_t timeOffset,
	F3DAUDIO_DSP_SETTINGS *pDSPSettings,
	FACTCue** ppCue /* Optional! */
) {
	FACTCue *result;
	if (pSoundBank == NULL)
	{
		if (ppCue != NULL)
		{
			*ppCue = NULL;
		}
		return 1;
	}

	FAudio_PlatformLockMutex(pSoundBank->parentEngine->apiLock);

	FACTSoundBank_Prepare(
		pSoundBank,
		nCueIndex,
		dwFlags,
		timeOffset,
		&result
	);
	if (ppCue != NULL)
	{
		*ppCue = result;
	}
	else
	{
		/* AKA we get to Destroy() this ourselves */
		result->managed = true;
	}
	FACT3DApply(pDSPSettings, result);
	FACTCue_Play(result);

	FAudio_PlatformUnlockMutex(pSoundBank->parentEngine->apiLock);
	return FAUDIO_OK;
}

uint32_t FACTSoundBank_Stop(
	FACTSoundBank *pSoundBank,
	uint16_t nCueIndex,
	uint32_t dwFlags
) {
	FACTCue *backup, *cue;
	if (pSoundBank == NULL)
	{
		return 1;
	}

	FAudio_PlatformLockMutex(pSoundBank->parentEngine->apiLock);
	cue = pSoundBank->cueList;
	while (cue != NULL)
	{
		if (cue->index == nCueIndex)
		{
			if (	dwFlags == FACT_FLAG_STOP_IMMEDIATE &&
				cue->managed	)
			{
				/* Just blow this up now */
				backup = cue->next;
				FACTCue_Destroy(cue);
				cue = backup;
			}
			else
			{
				/* If managed, the mixer will destroy for us */
				FACTCue_Stop(cue, dwFlags);
				cue = cue->next;
			}
		}
		else
		{
			cue = cue->next;
		}
	}
	FAudio_PlatformUnlockMutex(pSoundBank->parentEngine->apiLock);
	return FAUDIO_OK;
}

uint32_t FACTSoundBank_Destroy(FACTSoundBank *pSoundBank)
{
	uint16_t i, j, k;
	FAudioMutex mutex;

	if (pSoundBank == NULL)
	{
		return 1;
	}

	FAudio_PlatformLockMutex(pSoundBank->parentEngine->apiLock);

	/* Synchronously destroys all cues that are associated */
	while (pSoundBank->cueList != NULL)
	{
		FACTCue_Destroy(pSoundBank->cueList);
	}

	/* Remove this SoundBank from the Engine list */
	LinkedList_RemoveEntry(
		&pSoundBank->parentEngine->sbList,
		pSoundBank,
		pSoundBank->parentEngine->sbLock,
		pSoundBank->parentEngine->pFree
	);

	/* SoundBank Name */
	pSoundBank->parentEngine->pFree(pSoundBank->name);

	/* Cue data */
	pSoundBank->parentEngine->pFree(pSoundBank->cues);

	/* WaveBank Name data */
	for (i = 0; i < pSoundBank->wavebankCount; i += 1)
	{
		pSoundBank->parentEngine->pFree(pSoundBank->wavebankNames[i]);
	}
	pSoundBank->parentEngine->pFree(pSoundBank->wavebankNames);

	/* Sound data */
	for (i = 0; i < pSoundBank->soundCount; i += 1)
	{
		for (j = 0; j < pSoundBank->sounds[i].trackCount; j += 1)
		{
			for (k = 0; k < pSoundBank->sounds[i].tracks[j].eventCount; k += 1)
			{
				#define MATCH(t) \
					pSoundBank->sounds[i].tracks[j].events[k].type == t
				if (	MATCH(FACTEVENT_PLAYWAVE) ||
					MATCH(FACTEVENT_PLAYWAVETRACKVARIATION) ||
					MATCH(FACTEVENT_PLAYWAVEEFFECTVARIATION) ||
					MATCH(FACTEVENT_PLAYWAVETRACKEFFECTVARIATION)	)
				{
					if (pSoundBank->sounds[i].tracks[j].events[k].wave.isComplex)
					{
						pSoundBank->parentEngine->pFree(
							pSoundBank->sounds[i].tracks[j].events[k].wave.complex.wave_indices
						);
						pSoundBank->parentEngine->pFree(
							pSoundBank->sounds[i].tracks[j].events[k].wave.complex.wavebanks
						);
						pSoundBank->parentEngine->pFree(
							pSoundBank->sounds[i].tracks[j].events[k].wave.complex.weights
						);
					}
				}
				#undef MATCH
			}
			pSoundBank->parentEngine->pFree((void *)pSoundBank->sounds[i].tracks[j].events);
		}
		pSoundBank->parentEngine->pFree((void *)pSoundBank->sounds[i].tracks);
		pSoundBank->parentEngine->pFree((void *)pSoundBank->sounds[i].rpc_codes.codes);
		pSoundBank->parentEngine->pFree(pSoundBank->sounds[i].dspCodes);
	}
	pSoundBank->parentEngine->pFree((void *)pSoundBank->sounds);
	pSoundBank->parentEngine->pFree(pSoundBank->soundCodes);

	/* Variation data */
	for (i = 0; i < pSoundBank->variationCount; i += 1)
	{
		pSoundBank->parentEngine->pFree(
			pSoundBank->variations[i].entries
		);
	}
	pSoundBank->parentEngine->pFree(pSoundBank->variations);

	/* Transition data */
	for (i = 0; i < pSoundBank->transitionCount; i += 1)
	{
		pSoundBank->parentEngine->pFree(
			pSoundBank->transitions[i].entries
		);
	}
	pSoundBank->parentEngine->pFree(pSoundBank->transitions);
	pSoundBank->parentEngine->pFree(pSoundBank->transitionCodes);

	/* Cue Name data */
	if (pSoundBank->cueNames != NULL)
	{
		for (i = 0; i < pSoundBank->cueCount; i += 1)
		{
			pSoundBank->parentEngine->pFree(pSoundBank->cueNames[i]);
		}
		pSoundBank->parentEngine->pFree(pSoundBank->cueNames);
	}

	send_soundbank_notification(pSoundBank->parentEngine, pSoundBank);

	mutex = pSoundBank->parentEngine->apiLock;
	pSoundBank->parentEngine->pFree(pSoundBank);
	FAudio_PlatformUnlockMutex(mutex);
	return FAUDIO_OK;
}

uint32_t FACTSoundBank_GetState(
	FACTSoundBank *pSoundBank,
	uint32_t *pdwState
) {
	uint16_t i;
	if (pSoundBank == NULL)
	{
		*pdwState = 0;
		return 1;
	}

	FAudio_PlatformLockMutex(pSoundBank->parentEngine->apiLock);

	*pdwState = 0;
	for (i = 0; i < pSoundBank->cueCount; i += 1)
	{
		if (pSoundBank->cues[i].instanceCount > 0)
		{
			*pdwState |= FACT_STATE_INUSE;
			FAudio_PlatformUnlockMutex(
				pSoundBank->parentEngine->apiLock
			);
			return FAUDIO_OK;
		}
	}

	FAudio_PlatformUnlockMutex(pSoundBank->parentEngine->apiLock);
	return FAUDIO_OK;
}

/* WaveBank implementation */

uint32_t FACTWaveBank_Destroy(FACTWaveBank *pWaveBank)
{
	uint32_t i;
	FACTWave *wave;
	FAudioMutex mutex;
	FACTNotification note;
	if (pWaveBank == NULL)
	{
		return 1;
	}

	FAudio_PlatformLockMutex(pWaveBank->parentEngine->apiLock);

	/* Synchronously destroys any cues that are using the wavebank */
	while (pWaveBank->waveList != NULL)
	{
		wave = (FACTWave*) pWaveBank->waveList->entry;
		if (wave->parentCue != NULL)
		{
			/* Destroying this Cue destroys the Wave */
			FACTCue_Destroy(wave->parentCue);
		}
		else
		{
			FACTWave_Destroy(wave);
		}
	}

	/* Remove this WaveBank from the Engine list */
	LinkedList_RemoveEntry(
		&pWaveBank->parentEngine->wbList,
		pWaveBank,
		pWaveBank->parentEngine->wbLock,
		pWaveBank->parentEngine->pFree
	);

	/* Free everything, finally. */
	pWaveBank->parentEngine->pFree(pWaveBank->name);
	pWaveBank->parentEngine->pFree(pWaveBank->entries);
	pWaveBank->parentEngine->pFree(pWaveBank->entryRefs);
	if (pWaveBank->seekTables != NULL)
	{
		for (i = 0; i < pWaveBank->entryCount; i += 1)
		{
			if (pWaveBank->seekTables[i].entries != NULL)
			{
				pWaveBank->parentEngine->pFree(
					pWaveBank->seekTables[i].entries
				);
			}
		}
		pWaveBank->parentEngine->pFree(pWaveBank->seekTables);
	}

	if (!pWaveBank->streaming)
	{
		FAudio_close(pWaveBank->io);
	}

	if (pWaveBank->packetBuffer != NULL)
	{
		pWaveBank->parentEngine->pFree(pWaveBank->packetBuffer);
	}
	send_wavebank_notification(pWaveBank->parentEngine, FACTNOTIFICATIONTYPE_WAVEBANKDESTROYED, pWaveBank);
	FAudio_PlatformDestroyMutex(pWaveBank->waveLock);

	if (pWaveBank->waveBankNames != NULL)
	{
		pWaveBank->parentEngine->pFree(pWaveBank->waveBankNames);
	}

	mutex = pWaveBank->parentEngine->apiLock;
	pWaveBank->parentEngine->pFree(pWaveBank);
	FAudio_PlatformUnlockMutex(mutex);
	return FAUDIO_OK;
}

uint32_t FACTWaveBank_GetState(
	FACTWaveBank *pWaveBank,
	uint32_t *pdwState
) {
	uint32_t i;
	if (pWaveBank == NULL)
	{
		*pdwState = 0;
		return 1;
	}

	FAudio_PlatformLockMutex(pWaveBank->parentEngine->apiLock);

	*pdwState = FACT_STATE_PREPARED;
	for (i = 0; i < pWaveBank->entryCount; i += 1)
	{
		if (pWaveBank->entryRefs[i] > 0)
		{
			*pdwState |= FACT_STATE_INUSE;
			FAudio_PlatformUnlockMutex(
				pWaveBank->parentEngine->apiLock
			);
			return FAUDIO_OK;
		}
	}

	FAudio_PlatformUnlockMutex(pWaveBank->parentEngine->apiLock);
	return FAUDIO_OK;
}

uint32_t FACTWaveBank_GetNumWaves(
	FACTWaveBank *pWaveBank,
	uint16_t *pnNumWaves
) {
	if (pWaveBank == NULL)
	{
		*pnNumWaves = 0;
		return 1;
	}
	FAudio_PlatformLockMutex(pWaveBank->parentEngine->apiLock);
	*pnNumWaves = pWaveBank->entryCount;
	FAudio_PlatformUnlockMutex(pWaveBank->parentEngine->apiLock);
	return FAUDIO_OK;
}

uint16_t FACTWaveBank_GetWaveIndex(
	FACTWaveBank *pWaveBank,
	const char *szFriendlyName
) {
	uint16_t i;
	char *curName;
	if (pWaveBank == NULL || pWaveBank->waveBankNames == NULL)
	{
		return FACTINDEX_INVALID;
	}

	FAudio_PlatformLockMutex(pWaveBank->parentEngine->apiLock);
	curName = pWaveBank->waveBankNames;
	for (i = 0; i < pWaveBank->entryCount; i += 1, curName += 64)
	{
		if (FAudio_strncmp(szFriendlyName, curName, 64) == 0)
		{
			FAudio_PlatformUnlockMutex(pWaveBank->parentEngine->apiLock);
			return i;
		}
	}
	FAudio_PlatformUnlockMutex(pWaveBank->parentEngine->apiLock);

	return FACTINDEX_INVALID;
}

uint32_t FACTWaveBank_GetWaveProperties(
	FACTWaveBank *pWaveBank,
	uint16_t nWaveIndex,
	FACTWaveProperties *pWaveProperties
) {
	FACTWaveBankEntry *entry;
	if (pWaveBank == NULL)
	{
		return 1;
	}

	FAudio_PlatformLockMutex(pWaveBank->parentEngine->apiLock);

	entry = &pWaveBank->entries[nWaveIndex];

	if (pWaveBank->waveBankNames)
	{
		FAudio_memcpy(
			pWaveProperties->friendlyName,
			&pWaveBank->waveBankNames[nWaveIndex * 64],
			sizeof(pWaveProperties->friendlyName)
		);
	}
	else
	{
		FAudio_zero(
			pWaveProperties->friendlyName,
			sizeof(pWaveProperties->friendlyName)
		);
	}

	pWaveProperties->format = entry->Format;
	pWaveProperties->durationInSamples = entry->PlayRegion.dwLength;
	if (entry->Format.wFormatTag == FACT_WAVEBANKMINIFORMAT_TAG_PCM)
	{
		pWaveProperties->durationInSamples /= (8 << entry->Format.wBitsPerSample) / 8;
		pWaveProperties->durationInSamples /= entry->Format.nChannels;
	}
	else if (entry->Format.wFormatTag == FACT_WAVEBANKMINIFORMAT_TAG_ADPCM)
	{
		pWaveProperties->durationInSamples = (
			pWaveProperties->durationInSamples /
			((entry->Format.wBlockAlign + 22) * entry->Format.nChannels) *
			((entry->Format.wBlockAlign + 16) * 2)
		);
	}
	else
	{
		FAudio_assert(0 && "Unrecognized wFormatTag!");
	}

	pWaveProperties->loopRegion = entry->LoopRegion;
	pWaveProperties->streaming = pWaveBank->streaming;

	FAudio_PlatformUnlockMutex(pWaveBank->parentEngine->apiLock);
	return FAUDIO_OK;
}

uint32_t FACTWaveBank_Prepare(
	FACTWaveBank *pWaveBank,
	uint16_t nWaveIndex,
	uint32_t dwFlags,
	uint32_t dwPlayOffset,
	uint8_t nLoopCount,
	FACTWave **ppWave
) {
	FAudioBuffer buffer;
	FAudioBufferWMA bufferWMA;
	FAudioVoiceSends sends;
	FAudioSendDescriptor send;
	union
	{
		FAudioWaveFormatEx pcm;
		FAudioADPCMWaveFormat adpcm;
		FAudioXMA2WaveFormat xma2;
	} format;
	FACTWaveBankEntry *entry;
	FACTSeekTable *seek;
	if (pWaveBank == NULL)
	{
		*ppWave = NULL;
		return 1;
	}

	*ppWave = (FACTWave*) pWaveBank->parentEngine->pMalloc(sizeof(FACTWave));

	FAudio_PlatformLockMutex(pWaveBank->parentEngine->apiLock);

	entry = &pWaveBank->entries[nWaveIndex];

	/* Engine references */
	(*ppWave)->parentBank = pWaveBank;
	(*ppWave)->parentCue = NULL;
	(*ppWave)->index = nWaveIndex;

	(*ppWave)->background_music = (dwFlags & FACT_FLAG_BACKGROUND_MUSIC);

	/* Playback */
	(*ppWave)->state = FACT_STATE_PREPARED;
	(*ppWave)->volume = 1.0f;
	(*ppWave)->pitch = 0;
	(*ppWave)->loopCount = nLoopCount;

	if (dwPlayOffset)
		FAudio_Log("Unhandled play offset.\n");
#if 0
	if (dwFlags & FACT_FLAG_UNITS_MS)
	{
		dwPlayOffset = (uint32_t) (
			( /* Samples per millisecond... */
				(float) entry->Format.nSamplesPerSec /
				1000.0f
			) * (float) dwPlayOffset
		);
	}
#endif

	/* Create the voice */
	send.Flags = 0;
	send.pOutputVoice = pWaveBank->parentEngine->master;
	sends.SendCount = 1;
	sends.pSends = &send;
	format.pcm.nChannels = entry->Format.nChannels;
	format.pcm.nSamplesPerSec = entry->Format.nSamplesPerSec;
	if (entry->Format.wFormatTag == FACT_WAVEBANKMINIFORMAT_TAG_PCM)
	{
		format.pcm.wFormatTag = FAUDIO_FORMAT_PCM;
		format.pcm.wBitsPerSample = 8 << entry->Format.wBitsPerSample;
		format.pcm.nBlockAlign = format.pcm.nChannels * format.pcm.wBitsPerSample / 8;
		format.pcm.nAvgBytesPerSec = format.pcm.nBlockAlign * format.pcm.nSamplesPerSec;
		format.pcm.cbSize = 0;
	}
	else if (entry->Format.wFormatTag == FACT_WAVEBANKMINIFORMAT_TAG_XMA)
	{
		/* XMA2 is quite similar to WMA Pro... is what everyone thought.
		 * What a great way to start this comment.
		 *
		 * Let's reconstruct the extra data because who knows what decoder we're dealing with in <present year>.
		 * It's also a good exercise in understanding XMA2 metadata and feeding blocks into the decoder properly.
		 * At the time of writing this patch, it's FFmpeg via gstreamer which doesn't even respect most of this.
		 * ... which means: good luck to whoever ends up finding inaccuracies here in the future!
		 *
		 * dwLoopLength seems to match dwPlayLength in everything I've seen that had bLoopCount == 0.
		 * dwLoopBegin can be > 0 even with bLoopCount == 0 because why not. Let's ignore that.
		 *
		 * dwSamplesEncoded is usually close to dwPlayLength but not always (if ever?) equal. Let's assume equality.
		 * The XMA2 seek table uses sample indices as opposed to WMA's byte index seek table.
		 *
		 * nBlockAlign uses aWMABlockAlign given the entire WMA Pro thing BUT it's expected to be the block size for decoding.
		 * The XMA2 block size MUST be a multiple of 2048 BUT entry->PlayRegion.dwLength / seek->entryCount doesn't respect that.
		 * And even when correctly guesstimating the block size, we sometimes end up with block sizes >= 64k BYTES. nBlockAlign IS 16-BIT!
		 * Scrap nBlockAlign. I've given up and made all FAudio gstreamer functions use dwBytesPerBlock if available.
		 * Still though, if we don't want FAudio_INTERNAL_DecodeGSTREAMER to hang, the total data length must match (see SoundEffect.cs in FNA).
		 * As such, we round up when guessing the block size, feed GStreamer with zeroes^Wundersized blocks and hope for the best.
		 *
		 * This is FUN.
		 * -ade
		 */
		FAudio_assert(entry->Format.wBitsPerSample != 0);

		seek = &pWaveBank->seekTables[nWaveIndex];
		format.pcm.wFormatTag = FAUDIO_FORMAT_XMAUDIO2;
		format.pcm.wBitsPerSample = 16;
		format.pcm.nAvgBytesPerSec = aWMAAvgBytesPerSec[entry->Format.wBlockAlign >> 5];
		format.pcm.nBlockAlign = aWMABlockAlign[entry->Format.wBlockAlign & 0x1F];
		format.pcm.cbSize = (
			sizeof(FAudioXMA2WaveFormat) -
			sizeof(FAudioWaveFormatEx)
		);
		format.xma2.wNumStreams = (format.pcm.nChannels + 1) / 2;
		format.xma2.dwChannelMask = format.pcm.nChannels > 1 ? 0xFFFFFFFF >> (32 - format.pcm.nChannels) : 0;
		format.xma2.dwSamplesEncoded = seek->entries[seek->entryCount - 1];
		format.xma2.dwBytesPerBlock = (uint16_t) FAudio_ceil(
			(double) entry->PlayRegion.dwLength /
			(double) seek->entryCount /
			2048.0
		) * 2048;
		format.xma2.dwPlayBegin = format.xma2.dwLoopBegin = 0;
		format.xma2.dwPlayLength = format.xma2.dwLoopLength = format.xma2.dwSamplesEncoded;
		format.xma2.bLoopCount = 0;
		format.xma2.bEncoderVersion = 4;
		format.xma2.wBlockCount = seek->entryCount;
	}
	else if (entry->Format.wFormatTag == FACT_WAVEBANKMINIFORMAT_TAG_ADPCM)
	{
		format.pcm.wFormatTag = FAUDIO_FORMAT_MSADPCM;
		format.pcm.nBlockAlign = (entry->Format.wBlockAlign + 22) * format.pcm.nChannels;
		format.pcm.wBitsPerSample = 16;
		format.pcm.cbSize = (
			sizeof(FAudioADPCMWaveFormat) -
			sizeof(FAudioWaveFormatEx)
		);
		format.adpcm.wSamplesPerBlock = (
			((format.pcm.nBlockAlign / format.pcm.nChannels) - 6) * 2
		);
	}
	else if (entry->Format.wFormatTag == FACT_WAVEBANKMINIFORMAT_TAG_WMA)
	{
		/* Apparently this is used to detect WMA Pro...? */
		FAudio_assert(entry->Format.wBitsPerSample == 0);

		format.pcm.wFormatTag = FAUDIO_FORMAT_WMAUDIO2;
		format.pcm.nAvgBytesPerSec = aWMAAvgBytesPerSec[entry->Format.wBlockAlign >> 5];
		format.pcm.nBlockAlign = aWMABlockAlign[entry->Format.wBlockAlign & 0x1F];
		format.pcm.wBitsPerSample = 16;
		format.pcm.cbSize = 0;
	}
	(*ppWave)->callback.callback.OnBufferEnd = pWaveBank->streaming ?
		FACT_INTERNAL_OnBufferEnd :
		NULL;
	(*ppWave)->callback.callback.OnBufferStart = NULL;
	(*ppWave)->callback.callback.OnLoopEnd = NULL;
	(*ppWave)->callback.callback.OnStreamEnd = FACT_INTERNAL_OnStreamEnd;
	(*ppWave)->callback.callback.OnVoiceError = NULL;
	(*ppWave)->callback.callback.OnVoiceProcessingPassEnd = NULL;
	(*ppWave)->callback.callback.OnVoiceProcessingPassStart = NULL;
	(*ppWave)->callback.wave = *ppWave;
	(*ppWave)->srcChannels = format.pcm.nChannels;
	FAudio_CreateSourceVoice(
		pWaveBank->parentEngine->audio,
		&(*ppWave)->voice,
		&format.pcm,
		FAUDIO_VOICE_USEFILTER, /* FIXME: Can this be optional? */
		4.0f,
		(FAudioVoiceCallback*) &(*ppWave)->callback,
		&sends,
		NULL
	);
	if (pWaveBank->streaming)
	{
		/* Init stream cache info */
		if (format.pcm.wFormatTag == FAUDIO_FORMAT_PCM)
		{
			(*ppWave)->streamSize = (
				format.pcm.nSamplesPerSec *
				format.pcm.nBlockAlign
			);
		}
		else if (format.pcm.wFormatTag == FAUDIO_FORMAT_MSADPCM)
		{
			(*ppWave)->streamSize = (
				format.pcm.nSamplesPerSec /
				format.adpcm.wSamplesPerBlock *
				format.pcm.nBlockAlign
			);
		}
		else
		{
			/* Screw it, load the whole thing */
			(*ppWave)->streamSize = entry->PlayRegion.dwLength;

			/* XACT does NOT support loop subregions for these formats */
			FAudio_assert(entry->LoopRegion.dwStartSample == 0);
			FAudio_assert(entry->LoopRegion.dwTotalSamples == 0 || entry->LoopRegion.dwTotalSamples == entry->Duration);
		}
		(*ppWave)->streamCache = (uint8_t*) pWaveBank->parentEngine->pMalloc(
			(*ppWave)->streamSize
		);
		(*ppWave)->streamOffset = entry->PlayRegion.dwOffset;

		/* Read and submit first buffer from the WaveBank */
		FACT_INTERNAL_OnBufferEnd(&(*ppWave)->callback.callback, NULL);
	}
	else
	{
		(*ppWave)->streamCache = NULL;

		buffer.Flags = FAUDIO_END_OF_STREAM;
		buffer.AudioBytes = entry->PlayRegion.dwLength;
		buffer.pAudioData = FAudio_memptr(
			pWaveBank->io,
			entry->PlayRegion.dwOffset
		);
		buffer.PlayBegin = 0;
		buffer.PlayLength = entry->Duration;
		if (nLoopCount == 0)
		{
			buffer.LoopBegin = 0;
			buffer.LoopLength = 0;
			buffer.LoopCount = 0;
		}
		else
		{
			buffer.LoopBegin = entry->LoopRegion.dwStartSample;
			buffer.LoopLength = entry->LoopRegion.dwTotalSamples;
			buffer.LoopCount = nLoopCount;
		}
		buffer.pContext = NULL;
		if (format.pcm.wFormatTag == FAUDIO_FORMAT_WMAUDIO2)
		{
			bufferWMA.pDecodedPacketCumulativeBytes =
				pWaveBank->seekTables[nWaveIndex].entries;
			bufferWMA.PacketCount =
				pWaveBank->seekTables[nWaveIndex].entryCount;
			FAudioSourceVoice_SubmitSourceBuffer(
				(*ppWave)->voice,
				&buffer,
				&bufferWMA
			);
		}
		else
		{
			FAudioSourceVoice_SubmitSourceBuffer(
				(*ppWave)->voice,
				&buffer,
				NULL
			);
		}
	}

	/* Add to the WaveBank Wave list */
	LinkedList_AddEntry(
		&pWaveBank->waveList,
		*ppWave,
		pWaveBank->waveLock,
		pWaveBank->parentEngine->pMalloc
	);

	FAudio_PlatformUnlockMutex(pWaveBank->parentEngine->apiLock);
	return FAUDIO_OK;
}

uint32_t FACTWaveBank_Play(
	FACTWaveBank *pWaveBank,
	uint16_t nWaveIndex,
	uint32_t dwFlags,
	uint32_t dwPlayOffset,
	uint8_t nLoopCount,
	FACTWave **ppWave
) {
	if (pWaveBank == NULL)
	{
		*ppWave = NULL;
		return 1;
	}
	FAudio_PlatformLockMutex(pWaveBank->parentEngine->apiLock);
	FACTWaveBank_Prepare(
		pWaveBank,
		nWaveIndex,
		dwFlags,
		dwPlayOffset,
		nLoopCount,
		ppWave
	);
	FACTWave_Play(*ppWave);
	FAudio_PlatformUnlockMutex(pWaveBank->parentEngine->apiLock);
	return FAUDIO_OK;
}

uint32_t FACTWaveBank_Stop(
	FACTWaveBank *pWaveBank,
	uint16_t nWaveIndex,
	uint32_t dwFlags
) {
	FACTWave *wave;
	LinkedList *list;
	if (pWaveBank == NULL)
	{
		return 1;
	}
	FAudio_PlatformLockMutex(pWaveBank->parentEngine->apiLock);
	list = pWaveBank->waveList;
	while (list != NULL)
	{
		wave = (FACTWave*) list->entry;
		if (wave->index == nWaveIndex)
		{
			FACTWave_Stop(wave, dwFlags);
		}
		list = list->next;
	}
	FAudio_PlatformUnlockMutex(pWaveBank->parentEngine->apiLock);
	return FAUDIO_OK;
}

/* Wave implementation */

uint32_t FACTWave_Destroy(FACTWave *pWave)
{
	FACTNotificationWave notification;
	FAudioMutex mutex;

	if (pWave == NULL)
	{
		return 1;
	}

	FAudio_PlatformLockMutex(pWave->parentBank->parentEngine->apiLock);

	/* Stop before we start deleting everything */
	FACTWave_Stop(pWave, FACT_FLAG_STOP_IMMEDIATE);

	LinkedList_RemoveEntry(
		&pWave->parentBank->waveList,
		pWave,
		pWave->parentBank->waveLock,
		pWave->parentBank->parentEngine->pFree
	);

	FAudioVoice_DestroyVoice(pWave->voice);
	if (pWave->streamCache != NULL)
	{
		pWave->parentBank->parentEngine->pFree(pWave->streamCache);
	}
	notification.pWave = pWave;
	send_wave_notification(pWave->parentBank->parentEngine, FACTNOTIFICATIONTYPE_WAVEDESTROYED, &notification);

	mutex = pWave->parentBank->parentEngine->apiLock;
	pWave->parentBank->parentEngine->pFree(pWave);
	FAudio_PlatformUnlockMutex(mutex);
	return FAUDIO_OK;
}

uint32_t FACTWave_Play(FACTWave *pWave)
{
	if (pWave == NULL)
	{
		return 1;
	}
	FAudio_PlatformLockMutex(pWave->parentBank->parentEngine->apiLock);

	if (pWave->state & (FACT_STATE_PLAYING | FACT_STATE_STOPPING | FACT_STATE_STOPPED))
	{
		FAudio_PlatformUnlockMutex(pWave->parentBank->parentEngine->apiLock);
		return FACTENGINE_E_INVALIDUSAGE;
	}

	if (!(pWave->state & FACT_STATE_PAUSED))
	{
		pWave->state |= FACT_STATE_PLAYING;
		pWave->state &= ~FACT_STATE_PREPARED;
		FAudioSourceVoice_Start(pWave->voice, 0, 0);
	}

	FAudio_PlatformUnlockMutex(pWave->parentBank->parentEngine->apiLock);
	return FAUDIO_OK;
}

uint32_t FACTWave_Stop(FACTWave *pWave, uint32_t dwFlags)
{
	FACTNotificationWave notification;

	if (pWave == NULL)
	{
		return 1;
	}
	FAudio_PlatformLockMutex(pWave->parentBank->parentEngine->apiLock);

	/* There are two ways that a Wave might be stopped immediately:
	 * 1. The program explicitly asks for it
	 * 2. The Wave is paused and therefore we can't do fade/release effects
	 */
	if (	dwFlags & FACT_FLAG_STOP_IMMEDIATE ||
		pWave->state & FACT_STATE_PAUSED	)
	{
		pWave->state |= FACT_STATE_STOPPED;
		pWave->state &= ~(
			FACT_STATE_PLAYING |
			FACT_STATE_STOPPING |
			FACT_STATE_PAUSED
		);
		FAudioSourceVoice_Stop(pWave->voice, 0, 0);
		FAudioSourceVoice_FlushSourceBuffers(pWave->voice);
	}
	else
	{
		pWave->state |= FACT_STATE_STOPPING;
		FAudioSourceVoice_ExitLoop(pWave->voice, 0);
	}

	notification.pCue = pWave->parentCue;
	if (pWave->parentCue != NULL)
	{
		notification.cueIndex = pWave->parentCue->index;
		notification.pSoundBank = pWave->parentCue->parentBank;
	}
	else
	{
		notification.cueIndex = FACTINDEX_INVALID;
		notification.pSoundBank = NULL;
	}
	notification.pWave = pWave;
	notification.pWaveBank = pWave->parentBank;
	send_wave_notification(pWave->parentBank->parentEngine, FACTNOTIFICATIONTYPE_WAVESTOP, &notification);

	FAudio_PlatformUnlockMutex(pWave->parentBank->parentEngine->apiLock);
	return FAUDIO_OK;
}

uint32_t FACTWave_Pause(FACTWave *pWave, int32_t fPause)
{
	if (pWave == NULL)
	{
		return 1;
	}
	FAudio_PlatformLockMutex(pWave->parentBank->parentEngine->apiLock);

	/* FIXME: Does the Cue STOPPING/STOPPED rule apply here too? */
	if (pWave->state & (FACT_STATE_STOPPING | FACT_STATE_STOPPED))
	{
		FAudio_PlatformUnlockMutex(
			pWave->parentBank->parentEngine->apiLock
		);
		return FAUDIO_OK;
	}

	if (fPause)
	{
		pWave->state |= FACT_STATE_PAUSED;
		FAudioSourceVoice_Stop(pWave->voice, 0, 0);
	}
	else
	{
		pWave->state &= ~FACT_STATE_PAUSED;
		FAudioSourceVoice_Start(pWave->voice, 0, 0);
	}

	FAudio_PlatformUnlockMutex(pWave->parentBank->parentEngine->apiLock);
	return FAUDIO_OK;
}

uint32_t FACTWave_GetState(FACTWave *pWave, uint32_t *pdwState)
{
	if (pWave == NULL)
	{
		*pdwState = 0;
		return 1;
	}
	FAudio_PlatformLockMutex(pWave->parentBank->parentEngine->apiLock);
	*pdwState = pWave->state;
	FAudio_PlatformUnlockMutex(pWave->parentBank->parentEngine->apiLock);
	return FAUDIO_OK;
}

uint32_t FACTWave_SetPitch(FACTWave *pWave, int16_t pitch)
{
	if (pWave == NULL)
	{
		return 1;
	}
	FAudio_PlatformLockMutex(pWave->parentBank->parentEngine->apiLock);
	pWave->pitch = FAudio_clamp(
		pitch,
		FACTPITCH_MIN_TOTAL,
		FACTPITCH_MAX_TOTAL
	);
	FAudioSourceVoice_SetFrequencyRatio(
		pWave->voice,
		(float) FAudio_pow(2.0, pWave->pitch / 1200.0),
		0
	);
	FAudio_PlatformUnlockMutex(pWave->parentBank->parentEngine->apiLock);
	return FAUDIO_OK;
}

uint32_t FACTWave_SetVolume(FACTWave *pWave, float volume)
{
	if (pWave == NULL)
	{
		return 1;
	}
	FAudio_PlatformLockMutex(pWave->parentBank->parentEngine->apiLock);
	pWave->volume = FAudio_clamp(
		volume,
		FACTVOLUME_MIN,
		FACTVOLUME_MAX
	);
	FAudioVoice_SetVolume(
		pWave->voice,
		pWave->volume,
		0
	);
	FAudio_PlatformUnlockMutex(pWave->parentBank->parentEngine->apiLock);
	return FAUDIO_OK;
}

uint32_t FACTWave_SetMatrixCoefficients(
	FACTWave *pWave,
	uint32_t uSrcChannelCount,
	uint32_t uDstChannelCount,
	float *pMatrixCoefficients
) {
	uint32_t i;
	float *mtxDst, *mtxSrc, *mtxTmp = NULL;
	if (pWave == NULL)
	{
		return 1;
	}

	/* There seems to be this weird feature in XACT where the channel count
	 * can be completely wrong and it'll go to the right place.
	 * I guess these XACT functions do some extra work to merge coefficients
	 * but I have no idea where it really happens and XAudio2 definitely
	 * does NOT like it when this is wrong, so here it goes...
	 * -flibit
	 */
	if (uSrcChannelCount == 1 && pWave->srcChannels == 2)
	{
		mtxTmp = (float*) FAudio_alloca(
			sizeof(float) *
			pWave->srcChannels *
			uDstChannelCount
		);
		mtxDst = mtxTmp;
		mtxSrc = pMatrixCoefficients;
		for (i = 0; i < uDstChannelCount; i += 1)
		{
			mtxDst[0] = *mtxSrc;
			mtxDst[1] = *mtxSrc;
			mtxDst += 2;
			mtxSrc += 1;
		}
		uSrcChannelCount = 2;
		pMatrixCoefficients = mtxTmp;
	}
	else if (uSrcChannelCount == 2 && pWave->srcChannels == 1)
	{
		mtxTmp = (float*) FAudio_alloca(
			sizeof(float) *
			pWave->srcChannels *
			uDstChannelCount
		);
		mtxDst = mtxTmp;
		mtxSrc = pMatrixCoefficients;
		for (i = 0; i < uDstChannelCount; i += 1)
		{
			*mtxDst = (mtxSrc[0] + mtxSrc[1]) / 2.0f;
			mtxDst += 1;
			mtxSrc += 2;
		}
		uSrcChannelCount = 1;
		pMatrixCoefficients = mtxTmp;
	}

	FAudio_PlatformLockMutex(pWave->parentBank->parentEngine->apiLock);

	FAudioVoice_SetOutputMatrix(
		pWave->voice,
		pWave->voice->sends.pSends->pOutputVoice,
		uSrcChannelCount,
		uDstChannelCount,
		pMatrixCoefficients,
		0
	);

	FAudio_PlatformUnlockMutex(pWave->parentBank->parentEngine->apiLock);
	if (mtxTmp != NULL)
	{
		FAudio_dealloca(mtxTmp);
	}
	return FAUDIO_OK;
}

uint32_t FACTWave_GetProperties(
	FACTWave *pWave,
	FACTWaveInstanceProperties *pProperties
) {
	if (pWave == NULL)
	{
		return 1;
	}
	FAudio_PlatformLockMutex(pWave->parentBank->parentEngine->apiLock);

	FACTWaveBank_GetWaveProperties(
		pWave->parentBank,
		pWave->index,
		&pProperties->properties
	);

	pProperties->backgroundMusic = pWave->background_music;

	FAudio_PlatformUnlockMutex(pWave->parentBank->parentEngine->apiLock);
	return FAUDIO_OK;
}

/* Cue implementation */

uint32_t FACTCue_Destroy(FACTCue *pCue)
{
	FACTCue *cue, *prev;
	FAudioMutex mutex;
	if (pCue == NULL)
	{
		return 1;
	}

	FAudio_PlatformLockMutex(pCue->parentBank->parentEngine->apiLock);

	/* Stop before we start deleting everything */
	FACTCue_Stop(pCue, FACT_FLAG_STOP_IMMEDIATE);

	/* Remove this Cue from the SoundBank list */
	cue = pCue->parentBank->cueList;
	prev = cue;
	while (cue != NULL)
	{
		if (cue == pCue)
		{
			if (cue == prev) /* First in list */
			{
				pCue->parentBank->cueList = cue->next;
			}
			else
			{
				prev->next = cue->next;
			}
			break;
		}
		prev = cue;
		cue = cue->next;
	}
	FAudio_assert(cue != NULL && "Could not find Cue reference!");

	pCue->parentBank->parentEngine->pFree(pCue->variableValues);
	FACT_INTERNAL_SendCueNotification(pCue, FACTNOTIFICATIONTYPE_CUEDESTROYED);

	mutex = pCue->parentBank->parentEngine->apiLock;
	pCue->parentBank->parentEngine->pFree(pCue);
	FAudio_PlatformUnlockMutex(mutex);
	return FAUDIO_OK;
}

uint32_t FACTCue_Play(FACTCue *pCue)
{
	if (pCue == NULL)
	{
		return 1;
	}

	FAudio_PlatformLockMutex(pCue->parentBank->parentEngine->apiLock);

	if (pCue->state & (FACT_STATE_PLAYING | FACT_STATE_STOPPING | FACT_STATE_STOPPED))
	{
		FAudio_PlatformUnlockMutex(pCue->parentBank->parentEngine->apiLock);
		return FACTENGINE_E_INVALIDUSAGE;
	}

	if (!play_sound(pCue))
	{
		FAudio_PlatformUnlockMutex(
			pCue->parentBank->parentEngine->apiLock
		);
		return FACTENGINE_E_INSTANCELIMITFAILTOPLAY;
	}

	pCue->state |= FACT_STATE_PLAYING;
	pCue->state &= ~FACT_STATE_PREPARED;

	FACT_INTERNAL_SendCueNotification(pCue, FACTNOTIFICATIONTYPE_CUEPLAY);

	pCue->start = FAudio_timems();

	/* If it's a simple wave, just play it! */
	if (pCue->simpleWave != NULL)
	{
		if (pCue->active3D)
		{
			FACTWave_SetMatrixCoefficients(
				pCue->simpleWave,
				pCue->srcChannels,
				pCue->dstChannels,
				pCue->matrixCoefficients
			);
		}
		FACTWave_Play(pCue->simpleWave);
	}

	FAudio_PlatformUnlockMutex(pCue->parentBank->parentEngine->apiLock);
	return FAUDIO_OK;
}

uint32_t FACTCue_Stop(FACTCue *pCue, uint32_t dwFlags)
{
	if (pCue == NULL)
	{
		return 1;
	}
	FAudio_PlatformLockMutex(pCue->parentBank->parentEngine->apiLock);

	/* If we're already stopped, there's nothing to do... */
	if (pCue->state & FACT_STATE_STOPPED)
	{
		FAudio_PlatformUnlockMutex(
			pCue->parentBank->parentEngine->apiLock
		);
		return FAUDIO_OK;
	}

	/* If we're stopping and we haven't asked for IMMEDIATE, we're already
	 * doing what the application is asking us to do...
	 */
	if (	(pCue->state & FACT_STATE_STOPPING) &&
		!(dwFlags & FACT_FLAG_STOP_IMMEDIATE)	)
	{
		FAudio_PlatformUnlockMutex(
			pCue->parentBank->parentEngine->apiLock
		);
		return FAUDIO_OK;
	}

	/* There are three ways that a Cue might be stopped immediately:
	 * 1. The program explicitly asks for it
	 * 2. The Cue is paused and therefore we can't do fade/release effects
	 * 3. The Cue is stopped "as authored" and has no fade effects
	 */
	if (	dwFlags & FACT_FLAG_STOP_IMMEDIATE ||
		pCue->state & FACT_STATE_PAUSED	||
		pCue->playingSound == NULL ||
		(	pCue->parentBank->cues[pCue->index].fadeOutMS == 0 &&
			pCue->maxRpcReleaseTime == 0	)	)
	{
		pCue->start = 0;
		pCue->elapsed = 0;
		pCue->state |= FACT_STATE_STOPPED;
		pCue->state &= ~(
			FACT_STATE_PLAYING |
			FACT_STATE_STOPPING |
			FACT_STATE_PAUSED
		);

		if (pCue->simpleWave != NULL)
		{
			FACTWave_Destroy(pCue->simpleWave);
			pCue->simpleWave = NULL;

			pCue->data->instanceCount -= 1;
		}
		else if (pCue->playingSound != NULL)
		{
			FACT_INTERNAL_DestroySound(pCue->playingSound);
		}
	}
	else
	{
		if (pCue->parentBank->cues[pCue->index].fadeOutMS > 0)
		{
			FACT_INTERNAL_BeginFadeOut(
				pCue->playingSound,
				pCue->parentBank->cues[pCue->index].fadeOutMS
			);
		}
		else if (pCue->maxRpcReleaseTime > 0)
		{
			FACT_INTERNAL_BeginReleaseRPC(
				pCue->playingSound,
				pCue->maxRpcReleaseTime
			);
		}
		else
		{
			/* Pretty sure this doesn't happen, but just in case? */
			pCue->state |= FACT_STATE_STOPPING;
		}
	}

	FACT_INTERNAL_SendCueNotification(pCue, FACTNOTIFICATIONTYPE_CUESTOP);

	FAudio_PlatformUnlockMutex(pCue->parentBank->parentEngine->apiLock);
	return FAUDIO_OK;
}

uint32_t FACTCue_GetState(FACTCue *pCue, uint32_t *pdwState)
{
	if (pCue == NULL)
	{
		*pdwState = 0;
		return 1;
	}
	FAudio_PlatformLockMutex(pCue->parentBank->parentEngine->apiLock);
	*pdwState = pCue->state;
	FAudio_PlatformUnlockMutex(pCue->parentBank->parentEngine->apiLock);
	return FAUDIO_OK;
}

uint32_t FACTCue_SetMatrixCoefficients(
	FACTCue *pCue,
	uint32_t uSrcChannelCount,
	uint32_t uDstChannelCount,
	float *pMatrixCoefficients
) {
	uint8_t i;

	if (uSrcChannelCount < 1 || uSrcChannelCount > 8)
		return FAUDIO_E_FAIL;
	if (uDstChannelCount != pCue->parentBank->parentEngine->output_format.Format.nChannels)
		return FAUDIO_E_INVALID_ARG;

	FAudio_PlatformLockMutex(pCue->parentBank->parentEngine->apiLock);

	/* See FACTCue.matrixCoefficients declaration */
	FAudio_assert(uSrcChannelCount <= 2);

	/* Local storage */
	pCue->srcChannels = uSrcChannelCount;
	pCue->dstChannels = uDstChannelCount;
	FAudio_memcpy(
		pCue->matrixCoefficients,
		pMatrixCoefficients,
		sizeof(float) * uSrcChannelCount * uDstChannelCount
	);
	pCue->active3D = true;

	/* Apply to Waves if they exist */
	if (pCue->simpleWave != NULL)
	{
		FACTWave_SetMatrixCoefficients(
			pCue->simpleWave,
			uSrcChannelCount,
			uDstChannelCount,
			pMatrixCoefficients
		);
	}
	else if (pCue->playingSound != NULL)
	{
		for (i = 0; i < pCue->playingSound->sound->trackCount; i += 1)
		{
			if (pCue->playingSound->tracks[i].activeWave.wave != NULL)
			{
				FACTWave_SetMatrixCoefficients(
					pCue->playingSound->tracks[i].activeWave.wave,
					uSrcChannelCount,
					uDstChannelCount,
					pMatrixCoefficients
				);
			}
		}
	}

	FACT_INTERNAL_SendCueNotification(pCue, FACTNOTIFICATIONTYPE_CUESTOP);

	FAudio_PlatformUnlockMutex(pCue->parentBank->parentEngine->apiLock);
	return FAUDIO_OK;
}

uint16_t FACTCue_GetVariableIndex(
	FACTCue *pCue,
	const char *szFriendlyName
) {
	FACTAudioEngine *engine = pCue->parentBank->parentEngine;

	if (pCue == NULL)
	{
		return FACTVARIABLEINDEX_INVALID;
	}
	for (uint16_t i = 0; i < engine->variableCount; ++i)
	{
		if (!FAudio_strcmp(szFriendlyName, engine->variableNames[i]) &&
			(engine->variables[i].accessibility & ACCESSIBILITY_CUE) &&
			(engine->variables[i].accessibility & ACCESSIBILITY_PUBLIC))
			return i;
	}
	return FACTVARIABLEINDEX_INVALID;
}

uint32_t FACTCue_SetVariable(
	FACTCue *pCue,
	uint16_t nIndex,
	float nValue
) {
	FACTVariable *var;
	if (pCue == NULL)
	{
		return 1;
	}

	if (nIndex >= pCue->parentBank->parentEngine->variableCount)
		return FACTENGINE_E_INVALIDVARIABLEINDEX;
	var = &pCue->parentBank->parentEngine->variables[nIndex];
	if (!(var->accessibility & ACCESSIBILITY_PUBLIC) || !(var->accessibility & ACCESSIBILITY_CUE)
			|| (var->accessibility & ACCESSIBILITY_READONLY))
		return FACTENGINE_E_INVALIDVARIABLEINDEX;

	FAudio_PlatformLockMutex(pCue->parentBank->parentEngine->apiLock);

	pCue->variableValues[nIndex] = FAudio_clamp(
		nValue,
		var->minValue,
		var->maxValue
	);

	FAudio_PlatformUnlockMutex(pCue->parentBank->parentEngine->apiLock);
	return FAUDIO_OK;
}

uint32_t FACTCue_GetVariable(
	FACTCue *pCue,
	uint16_t nIndex,
	float *nValue
) {
	FACTVariable *var;
	if (pCue == NULL)
	{
		*nValue = 0.0f;
		return 1;
	}

	if (nIndex >= pCue->parentBank->parentEngine->variableCount)
		return FACTENGINE_E_INVALIDVARIABLEINDEX;
	var = &pCue->parentBank->parentEngine->variables[nIndex];
	if (!(var->accessibility & ACCESSIBILITY_PUBLIC) || !(var->accessibility & ACCESSIBILITY_CUE))
		return FACTENGINE_E_INVALIDVARIABLEINDEX;

	FAudio_PlatformLockMutex(pCue->parentBank->parentEngine->apiLock);

	if (nIndex == 0) /* NumCueInstances */
	{
		*nValue = pCue->parentBank->cues[pCue->index].instanceCount;
	}
	else
	{
		*nValue = pCue->variableValues[nIndex];
	}

	FAudio_PlatformUnlockMutex(pCue->parentBank->parentEngine->apiLock);
	return FAUDIO_OK;
}

uint32_t FACTCue_Pause(FACTCue *pCue, int32_t fPause)
{
	uint8_t i;
	if (pCue == NULL)
	{
		return 1;
	}

	FAudio_PlatformLockMutex(pCue->parentBank->parentEngine->apiLock);

	/* "A stopping or stopped cue cannot be paused." */
	if (pCue->state & (FACT_STATE_STOPPING | FACT_STATE_STOPPED))
	{
		FAudio_PlatformUnlockMutex(
			pCue->parentBank->parentEngine->apiLock
		);
		return FAUDIO_OK;
	}

	/* Store elapsed time */
	pCue->elapsed += FAudio_timems() - pCue->start;

	/* All we do is set the flag, not much to see here */
	if (fPause)
	{
		pCue->state |= FACT_STATE_PAUSED;
	}
	else
	{
		pCue->state &= ~FACT_STATE_PAUSED;
	}

	/* Pause the Waves */
	if (pCue->simpleWave != NULL)
	{
		FACTWave_Pause(pCue->simpleWave, fPause);
	}
	else if (pCue->playingSound != NULL)
	{
		for (i = 0; i < pCue->playingSound->sound->trackCount; i += 1)
		{
			if (pCue->playingSound->tracks[i].activeWave.wave != NULL)
			{
				FACTWave_Pause(
					pCue->playingSound->tracks[i].activeWave.wave,
					fPause
				);
			}
		}
	}

	FAudio_PlatformUnlockMutex(pCue->parentBank->parentEngine->apiLock);
	return FAUDIO_OK;
}

uint32_t FACTCue_GetProperties(
	FACTCue *pCue,
	FACTCueInstanceProperties **ppProperties
) {
	uint32_t i;
	size_t allocSize;
	FACTCueInstanceProperties *cueProps;
	FACTVariationProperties *varProps;
	FACTSoundProperties *sndProps;
	FACTWaveInstanceProperties waveProps;
	if (pCue == NULL)
	{
		return 1;
	}

	FAudio_PlatformLockMutex(pCue->parentBank->parentEngine->apiLock);

	/* Alloc container (including variable length array space) */
	allocSize = sizeof(FACTCueInstanceProperties);
	if (pCue->playingSound != NULL)
	{
		allocSize += (
			sizeof(FACTTrackProperties) *
			pCue->playingSound->sound->trackCount
		);
	}
	cueProps = (FACTCueInstanceProperties*) pCue->parentBank->parentEngine->pMalloc(
		allocSize
	);
	FAudio_zero(cueProps, allocSize);

	/* Cue Properties */
	FACTSoundBank_GetCueProperties(
		pCue->parentBank,
		pCue->index,
		&cueProps->cueProperties
	);

	/* Variation Properties */
	varProps = &cueProps->activeVariationProperties.variationProperties;
	if (pCue->playingSound)
	{
		if (pCue->data->flags & CUE_FLAG_SINGLE_SOUND)
		{
			varProps->weight = 0xff;
		}
		else
		{
			const FACTVariation *variation = &pCue->variation->entries[pCue->playingSound->variation_index];

			varProps->index = pCue->playingSound->variation_index;

			if (pCue->variation->type == VARIATION_TABLE_TYPE_INTERACTIVE)
			{
				varProps->iaVariableMin = variation->interactive.var_min;
				varProps->iaVariableMax = variation->interactive.var_max;
				varProps->weight = 0xff;
			}
			else
			{
				varProps->weight = variation->noninteractive.weight_max;
			}

			varProps->linger = variation->linger;
		}
	}

	/* Sound Properties */
	sndProps = &cueProps->activeVariationProperties.soundProperties;
	if (pCue->playingSound != NULL)
	{
		sndProps->category = pCue->playingSound->sound->category;
		sndProps->priority = pCue->playingSound->sound->priority;
		sndProps->pitch = pCue->playingSound->sound->pitch;
		sndProps->volume = pCue->playingSound->sound->volume;
		sndProps->numTracks = pCue->playingSound->sound->trackCount;

		for (i = 0; i < sndProps->numTracks; i += 1)
		{
			FACTTrackProperties *track_props = &sndProps->arrTrackProperties[i];
			FACTTrackInstance *track = &pCue->playingSound->tracks[i];

			FAudio_assert(track->activeWave.wave);
			FACTWave_GetProperties(track->activeWave.wave, &waveProps);

			track_props->duration = (waveProps.properties.durationInSamples * 1000)
				/ waveProps.properties.format.nSamplesPerSec;
			track_props->numChannels = waveProps.properties.format.nChannels;
			if (track->waveEvt->wave.isComplex)
			{
				track_props->numVariations = track->waveEvt->wave.complex.wave_count;
				track_props->waveVariation = track->waveEvtInst->valuei;
			}
			else
			{
				track_props->numVariations = 1;
				track_props->waveVariation = 0;
			}
			track_props->loopCount = pCue->playingSound->tracks[i].waveEvt->wave.loopCount;
			/* Native doesn't take variation into account here. */
			track_props->duration *= (track_props->loopCount + 1);
		}
	}

	FAudio_PlatformUnlockMutex(pCue->parentBank->parentEngine->apiLock);

	*ppProperties = cueProps;
	return FAUDIO_OK;
}

uint32_t FACTCue_SetOutputVoices(
	FACTCue *pCue,
	const FAudioVoiceSends *pSendList /* Optional XAUDIO2_VOICE_SENDS */
) {
	/* TODO */
	return FAUDIO_OK;
}

uint32_t FACTCue_SetOutputVoiceMatrix(
	FACTCue *pCue,
	const FAudioVoice *pDestinationVoice, /* Optional! */
	uint32_t SourceChannels,
	uint32_t DestinationChannels,
	const float *pLevelMatrix /* SourceChannels * DestinationChannels */
) {
	/* TODO */
	return FAUDIO_OK;
}

/* vim: set noexpandtab shiftwidth=8 tabstop=8: */
