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

#include "FAudio_internal.h"

#ifndef FAUDIO_DISABLE_DEBUGCONFIGURATION
void FAudio_INTERNAL_debug(
	FAudio *audio,
	const char *file,
	uint32_t line,
	const char *func,
	const char *fmt,
	...
) {
	char output[1024];
	char *out = output;
	va_list va;
	out[0] = '\0';

	/* Logging extras */
	if (audio->debug.LogThreadID)
	{
		out += FAudio_snprintf(
			out,
			sizeof(output) - (out - output),
			"0x%" FAudio_PRIx64 " ",
			FAudio_PlatformGetThreadID()
		);
	}
	if (audio->debug.LogFileline)
	{
		out += FAudio_snprintf(
			out,
			sizeof(output) - (out - output),
			"%s:%u ",
			file,
			line
		);
	}
	if (audio->debug.LogFunctionName)
	{
		out += FAudio_snprintf(
			out,
			sizeof(output) - (out - output),
			"%s ",
			func
		);
	}
	if (audio->debug.LogTiming)
	{
		out += FAudio_snprintf(
			out,
			sizeof(output) - (out - output),
			"%dms ",
			FAudio_timems()
		);
	}

	/* The actual message... */
	va_start(va, fmt);
	FAudio_vsnprintf(
		out,
		sizeof(output) - (out - output),
		fmt,
		va
	);
	va_end(va);

	/* Print, finally. */
	FAudio_Log(output);
}

static const char *get_wformattag_string(const FAudioWaveFormatEx *fmt)
{
#define FMT_STRING(suffix) \
	if (fmt->wFormatTag == FAUDIO_FORMAT_##suffix) \
	{ \
		return #suffix; \
	}
	FMT_STRING(PCM)
	FMT_STRING(MSADPCM)
	FMT_STRING(IEEE_FLOAT)
	FMT_STRING(XMAUDIO2)
	FMT_STRING(WMAUDIO2)
	FMT_STRING(WMAUDIO3)
	FMT_STRING(EXTENSIBLE)
#undef FMT_STRING
	return "UNKNOWN!";
}

static const char *get_subformat_string(const FAudioWaveFormatEx *fmt)
{
	const FAudioWaveFormatExtensible *fmtex = (const FAudioWaveFormatExtensible*) fmt;

	if (fmt->wFormatTag != FAUDIO_FORMAT_EXTENSIBLE)
	{
		return "N/A";
	}
	if (!FAudio_memcmp(&fmtex->SubFormat, &DATAFORMAT_SUBTYPE_IEEE_FLOAT, sizeof(FAudioGUID)))
	{
		return "IEEE_FLOAT";
	}
	if (!FAudio_memcmp(&fmtex->SubFormat, &DATAFORMAT_SUBTYPE_PCM, sizeof(FAudioGUID)))
	{
		return "PCM";
	}
	return "UNKNOWN!";
}

void FAudio_INTERNAL_debug_fmt(
	FAudio *audio,
	const char *file,
	uint32_t line,
	const char *func,
	const FAudioWaveFormatEx *fmt
) {
	FAudio_INTERNAL_debug(
		audio,
		file,
		line,
		func,
		(
			"{"
			"wFormatTag: 0x%x %s, "
			"nChannels: %u, "
			"nSamplesPerSec: %u, "
			"wBitsPerSample: %u, "
			"nBlockAlign: %u, "
			"SubFormat: %s"
			"}"
		),
		fmt->wFormatTag,
		get_wformattag_string(fmt),
		fmt->nChannels,
		fmt->nSamplesPerSec,
		fmt->wBitsPerSample,
		fmt->nBlockAlign,
		get_subformat_string(fmt)
	);
}
#endif /* FAUDIO_DISABLE_DEBUGCONFIGURATION */

bool array_reserve(FAudio *audio, void **elements, size_t *capacity, size_t count, size_t size)
{
    unsigned int new_capacity, max_capacity;
    void *new_elements;

    if (count <= *capacity)
        return true;

    max_capacity = ~(size_t)0 / size;
    if (count > max_capacity)
        return false;

    new_capacity = FAudio_max(4, *capacity);
    while (new_capacity < count && new_capacity <= max_capacity / 2)
        new_capacity *= 2;
    if (new_capacity < count)
        new_capacity = max_capacity;

    if (!(new_elements = audio->pRealloc(*elements, new_capacity * size)))
        return false;

    *elements = new_elements;
    *capacity = new_capacity;

    return true;
}

void LinkedList_AddEntry(
	LinkedList **start,
	void* toAdd,
	FAudioMutex lock,
	FAudioMallocFunc pMalloc
) {
	LinkedList *newEntry, *latest;
	newEntry = (LinkedList*) pMalloc(sizeof(LinkedList));
	newEntry->entry = toAdd;
	newEntry->next = NULL;
	FAudio_PlatformLockMutex(lock);
	if (*start == NULL)
	{
		*start = newEntry;
	}
	else
	{
		latest = *start;
		while (latest->next != NULL)
		{
			latest = latest->next;
		}
		latest->next = newEntry;
	}
	FAudio_PlatformUnlockMutex(lock);
}

void LinkedList_PrependEntry(
	LinkedList **start,
	void* toAdd,
	FAudioMutex lock,
	FAudioMallocFunc pMalloc
) {
	LinkedList *newEntry;
	newEntry = (LinkedList*) pMalloc(sizeof(LinkedList));
	newEntry->entry = toAdd;
	FAudio_PlatformLockMutex(lock);
	newEntry->next = *start;
	*start = newEntry;
	FAudio_PlatformUnlockMutex(lock);
}

void LinkedList_RemoveEntry(
	LinkedList **start,
	void* toRemove,
	FAudioMutex lock,
	FAudioFreeFunc pFree
) {
	LinkedList *latest, *prev;
	FAudio_PlatformLockMutex(lock);
	latest = *start;
	prev = latest;
	while (latest != NULL)
	{
		if (latest->entry == toRemove)
		{
			if (latest == prev) /* First in list */
			{
				*start = latest->next;
			}
			else
			{
				prev->next = latest->next;
			}
			pFree(latest);
			FAudio_PlatformUnlockMutex(lock);
			return;
		}
		prev = latest;
		latest = latest->next;
	}
	FAudio_PlatformUnlockMutex(lock);
	FAudio_assert(0 && "LinkedList element not found!");
}

void FAudio_INTERNAL_InsertSubmixSorted(
	LinkedList **start,
	FAudioSubmixVoice *toAdd,
	FAudioMutex lock,
	FAudioMallocFunc pMalloc
) {
	LinkedList *newEntry, *latest;
	newEntry = (LinkedList*) pMalloc(sizeof(LinkedList));
	newEntry->entry = toAdd;
	newEntry->next = NULL;
	FAudio_PlatformLockMutex(lock);
	if (*start == NULL)
	{
		*start = newEntry;
	}
	else
	{
		latest = *start;

		/* Special case if the new stage is lower than everyone else */
		if (toAdd->mix.processingStage < ((FAudioSubmixVoice*) latest->entry)->mix.processingStage)
		{
			newEntry->next = latest;
			*start = newEntry;
		}
		else
		{
			/* If we got here, we know that the new stage is
			 * _at least_ as high as the first submix in the list.
			 *
			 * Each loop iteration checks to see if the new stage
			 * is smaller than `latest->next`, meaning it fits
			 * between `latest` and `latest->next`.
			 */
			while (latest->next != NULL)
			{
				if (toAdd->mix.processingStage < ((FAudioSubmixVoice *) latest->next->entry)->mix.processingStage)
				{
					newEntry->next = latest->next;
					latest->next = newEntry;
					break;
				}
				latest = latest->next;
			}
			/* If newEntry didn't get a `next` value, that means
			 * it didn't fall in between any stages and `latest`
			 * is the last entry in the list. Add it to the end!
			 */
			if (newEntry->next == NULL)
			{
				latest->next = newEntry;
			}
		}
	}
	FAudio_PlatformUnlockMutex(lock);
}

static uint32_t FAudio_INTERNAL_GetBytesRequested(
	FAudioSourceVoice *voice,
	uint32_t decoding
) {
	const uint32_t block_size = voice->src.format->nBlockAlign;
	const uint32_t samples_per_block = voice->src.samples_per_block;
	uint32_t result = (decoding * block_size / samples_per_block);
	FAudioWaveFormatExtensible *fmt;

	LOG_FUNC_ENTER(voice->audio)

#ifdef HAVE_WMADEC
	if (voice->src.wmadec != NULL)
	{
		/* Always 0, per the spec */
		LOG_FUNC_EXIT(voice->audio)
		return 0;
	}
#endif /* HAVE_WMADEC */

	for (size_t i = 0; i < voice->src.queued_buffer_count; ++i)
	{
		const struct queued_buffer *buffer = &voice->src.queued_buffers[i];
		uint32_t size = 0;

		if (buffer->buffer.LoopCount > 0)
			size += buffer->loop_bytes * buffer->buffer.LoopCount;
		size += buffer->play_bytes;

		size -= voice->src.curBufferOffset * block_size / samples_per_block;

		if (size > result)
		{
			LOG_FUNC_EXIT(voice->audio)
			return 0;
		}
		result -= size;
	}

	LOG_FUNC_EXIT(voice->audio)
	return result;
}

static uint32_t buffer_get_end(FAudioSourceVoice *voice, const struct queued_buffer *buffer)
{
	const uint32_t block_size = voice->src.format->nBlockAlign;
	const uint32_t samples_per_block = voice->src.samples_per_block;

#ifdef HAVE_WMADEC
	if (voice->src.wmadec)
	{
		uint32_t buffer_size;

		if (voice->src.format->wFormatTag == FAUDIO_FORMAT_XMAUDIO2)
		{
			FAudioXMA2WaveFormat *fmtex = (FAudioXMA2WaveFormat *)voice->src.format;
			buffer_size = fmtex->dwSamplesEncoded;
		}
		else
		{
			buffer_size = buffer->bufferWMA.pDecodedPacketCumulativeBytes[buffer->bufferWMA.PacketCount - 1] /
				(voice->src.format->nChannels * voice->src.format->wBitsPerSample / 8);
		}

		if (buffer->buffer.LoopCount)
		{
			if (buffer->buffer.LoopLength)
				return buffer->buffer.LoopBegin + buffer->buffer.LoopLength;
			return buffer_size;
		}

		if (buffer->buffer.PlayLength)
			return buffer->buffer.PlayBegin + buffer->buffer.PlayLength;
		return buffer_size;
	}
#endif

	if (buffer->buffer.LoopCount)
		return buffer->buffer.LoopBegin + ((buffer->loop_bytes - buffer->first_block_offset) / block_size * samples_per_block);

	return buffer->buffer.PlayBegin + ((buffer->play_bytes - buffer->first_block_offset) / block_size * samples_per_block);
}

/* If there is any leftover unaligned data at the end of this buffer, stash it
 * stash it in the voice.
 * We will later combine it with the beginning of the next buffer. */
static void save_unaligned_end_data(FAudioSourceVoice *voice, const struct queued_buffer *buffer)
{
	const uint32_t samples_per_block = voice->src.samples_per_block;
	const uint32_t block_size = voice->src.format->nBlockAlign;
	uint32_t byte_pos, end_pos;

#ifdef HAVE_WMADEC
	if (voice->src.wmadec)
		return;
#endif

	byte_pos = buffer->first_block_offset;
	byte_pos += voice->src.curBufferOffset / samples_per_block * block_size;

	if (buffer->buffer.LoopCount)
	{
		end_pos = buffer->buffer.LoopBegin / samples_per_block * block_size;
		end_pos += buffer->loop_bytes;
	}
	else
	{
		end_pos = buffer->buffer.PlayBegin / samples_per_block * block_size;
		end_pos += buffer->play_bytes;
	}

	if (byte_pos == end_pos)
		return;

	/* If the last buffer was also unaligned, and this one
	 * wasn't big enough to complete the block, then we'll
	 * already have some unaligned bytes. If this one was
	 * big enough to complete the block, we shouldn't have
	 * any bytes left over from the last buffer. */
	FAudio_assert(voice->src.unaligned_size + (end_pos - byte_pos) < block_size);

	if (!voice->src.unaligned_data)
		voice->src.unaligned_data = voice->audio->pMalloc(block_size);
	FAudio_memcpy(voice->src.unaligned_data + voice->src.unaligned_size,
		buffer->buffer.pAudioData + byte_pos, end_pos - byte_pos);
	voice->src.unaligned_size += (end_pos - byte_pos);
}

static void start_buffer(FAudioSourceVoice *voice, struct queued_buffer *buffer)
{
	if (!buffer->sent_OnStartBuffer)
	{
		buffer->sent_OnStartBuffer = true;

		if (	!buffer->internal &&
			voice->src.callback != NULL &&
			voice->src.callback->OnBufferStart != NULL	)
		{
			FAudio_PlatformUnlockMutex(voice->src.bufferLock);
			LOG_MUTEX_UNLOCK(voice->audio, voice->src.bufferLock)

			FAudio_PlatformUnlockMutex(voice->sendLock);
			LOG_MUTEX_UNLOCK(voice->audio, voice->sendLock)

			FAudio_PlatformUnlockMutex(voice->audio->sourceLock);
			LOG_MUTEX_UNLOCK(voice->audio, voice->audio->sourceLock)

			voice->src.callback->OnBufferStart(
				voice->src.callback,
				buffer->buffer.pContext
			);

			FAudio_PlatformLockMutex(voice->audio->sourceLock);
			LOG_MUTEX_LOCK(voice->audio, voice->audio->sourceLock)

			FAudio_PlatformLockMutex(voice->sendLock);
			LOG_MUTEX_LOCK(voice->audio, voice->sendLock)

			FAudio_PlatformLockMutex(voice->src.bufferLock);
			LOG_MUTEX_LOCK(voice->audio, voice->src.bufferLock)
		}
	}
}

static void end_buffer(FAudioSourceVoice *voice)
{
	struct queued_buffer *buffer = &voice->src.queued_buffers[0];
	bool eos = buffer->buffer.Flags & FAUDIO_END_OF_STREAM;
	FAudioVoiceCallback *callback = voice->src.callback;
	void *context = buffer->buffer.pContext;
	bool internal = buffer->internal;

	save_unaligned_end_data(voice, buffer);

	if (buffer->buffer.LoopCount > 0)
	{
		voice->src.curBufferOffset = buffer->buffer.LoopBegin;
		if (buffer->buffer.LoopCount < FAUDIO_LOOP_INFINITE)
			--buffer->buffer.LoopCount;

		if (callback && callback->OnLoopEnd)
		{
			FAudio_PlatformUnlockMutex(voice->src.bufferLock);
			LOG_MUTEX_UNLOCK(voice->audio, voice->src.bufferLock)

			FAudio_PlatformUnlockMutex(voice->sendLock);
			LOG_MUTEX_UNLOCK(voice->audio, voice->sendLock)

			FAudio_PlatformUnlockMutex(voice->audio->sourceLock);
			LOG_MUTEX_UNLOCK(voice->audio, voice->audio->sourceLock)

			callback->OnLoopEnd(callback, context);

			FAudio_PlatformLockMutex(voice->audio->sourceLock);
			LOG_MUTEX_LOCK(voice->audio, voice->audio->sourceLock)

			FAudio_PlatformLockMutex(voice->sendLock);
			LOG_MUTEX_LOCK(voice->audio, voice->sendLock)

			FAudio_PlatformLockMutex(voice->src.bufferLock);
			LOG_MUTEX_LOCK(voice->audio, voice->src.bufferLock)
		}

		buffer->first_block_offset = 0;
		return;
	}

#ifdef HAVE_WMADEC
	if (voice->src.wmadec)
		FAudio_WMADEC_end_buffer(voice);
#endif /* HAVE_WMADEC */

	if (eos)
	{
		voice->src.curBufferOffsetDec = 0;
		voice->src.totalSamples = 0;
	}

	LOG_INFO(voice->audio, "Voice %p, finished with buffer %p", voice, buffer)

	if (buffer->internal)
		voice->audio->pFree((void *)buffer->buffer.pAudioData);

	FAudio_memmove(&voice->src.queued_buffers[0], &voice->src.queued_buffers[1],
		(voice->src.queued_buffer_count - 1) * sizeof(*voice->src.queued_buffers));
	--voice->src.queued_buffer_count;

	if (voice->src.queued_buffer_count)
		voice->src.curBufferOffset = voice->src.queued_buffers[0].buffer.PlayBegin;

	if (callback && !internal)
	{
		FAudio_PlatformUnlockMutex(voice->src.bufferLock);
		LOG_MUTEX_UNLOCK(voice->audio, voice->src.bufferLock)

		FAudio_PlatformUnlockMutex(voice->sendLock);
		LOG_MUTEX_UNLOCK(voice->audio, voice->sendLock)

		FAudio_PlatformUnlockMutex(voice->audio->sourceLock);
		LOG_MUTEX_UNLOCK(voice->audio, voice->audio->sourceLock)

		if (callback->OnBufferEnd)
			callback->OnBufferEnd(callback, context);

		if (eos && callback->OnStreamEnd)
			callback->OnStreamEnd(callback);

		FAudio_PlatformLockMutex(voice->audio->sourceLock);
		LOG_MUTEX_LOCK(voice->audio, voice->audio->sourceLock)

		FAudio_PlatformLockMutex(voice->sendLock);
		LOG_MUTEX_LOCK(voice->audio, voice->sendLock)

		FAudio_PlatformLockMutex(voice->src.bufferLock);
		LOG_MUTEX_LOCK(voice->audio, voice->src.bufferLock)
	}
}

/* If we have saved unaligned data, and the next buffer has enough data to
 * complete the block, then turn it into an internal queued buffer entry
 * and offset the next buffer's byte position accordingly. */
static void try_collect_unaligned_data(FAudioSourceVoice *voice)
{
	const uint32_t samples_per_block = voice->src.samples_per_block;
	const uint32_t block_size = voice->src.format->nBlockAlign;
	struct queued_buffer *buffer;
	uint32_t begin_bytes;

	if (!voice->src.queued_buffer_count)
		return;
	buffer = &voice->src.queued_buffers[0];
	if (!voice->src.unaligned_size)
		return;

	if (buffer->buffer.LoopCount)
	{
		/* If there's not enough data to complete the block, the buffer
		 * will be immediately ended in end_buffer(), and its data
		 * appended to the unaligned data we already have. */
		if (voice->src.unaligned_size + buffer->loop_bytes < block_size)
			return;
		begin_bytes = buffer->buffer.LoopBegin / samples_per_block * block_size;
	}
	else
	{
		if (voice->src.unaligned_size + buffer->play_bytes < block_size)
			return;
		begin_bytes = buffer->buffer.PlayBegin / samples_per_block * block_size;
	}

	buffer->first_block_offset = block_size - voice->src.unaligned_size;
	FAudio_memcpy(voice->src.unaligned_data + voice->src.unaligned_size,
		buffer->buffer.pAudioData + begin_bytes, buffer->first_block_offset);

	/* Put this data into a new internal buffer. */
	array_reserve(voice->audio, (void **)&voice->src.queued_buffers, &voice->src.queued_buffers_capacity,
		voice->src.queued_buffer_count + 1, sizeof(*voice->src.queued_buffers));
	FAudio_memmove(&voice->src.queued_buffers[1], &voice->src.queued_buffers[0],
		voice->src.queued_buffer_count * sizeof(*voice->src.queued_buffers));
	++voice->src.queued_buffer_count;

	buffer = &voice->src.queued_buffers[0];
	FAudio_memset(buffer, 0, sizeof(*buffer));
	buffer->buffer.pAudioData = voice->src.unaligned_data;
	buffer->internal = true;
	buffer->play_bytes = block_size;

	voice->src.curBufferOffset = 0;

	/* Reset the unaligned size for next time. */
	voice->src.unaligned_data = NULL;
	voice->src.unaligned_size = 0;
}

static void FAudio_INTERNAL_DecodeBuffers(
	FAudioSourceVoice *voice,
	uint64_t *toDecode
) {
	const uint32_t samples_per_block = voice->src.samples_per_block;
	const uint32_t block_size = voice->src.format->nBlockAlign;
	uint32_t decoded = 0;

	LOG_FUNC_ENTER(voice->audio)

	/* This should never go past the max ratio size */
	FAudio_assert(*toDecode <= voice->src.decodeSamples);

	while (decoded < *toDecode && voice->src.queued_buffer_count)
	{
		float *dst = voice->audio->decodeCache + (decoded * voice->src.format->nChannels);
		struct queued_buffer *buffer = &voice->src.queued_buffers[0];
		uint32_t decode_count;

		try_collect_unaligned_data(voice);

		/* Start-of-buffer behavior */
		start_buffer(voice, buffer);

		/* Number of samples we are decoding in one call. */
		decode_count = FAudio_min(*toDecode - decoded,
			buffer_get_end(voice, buffer) - voice->src.curBufferOffset);

#ifdef HAVE_WMADEC
		if (voice->src.wmadec)
		{
			decode_wma(voice, buffer, dst, decode_count);
		}
		else
#endif
		{
			uint32_t block_offset = voice->src.curBufferOffset % samples_per_block;
			const uint8_t *src = buffer->buffer.pAudioData + buffer->first_block_offset;

			src += (voice->src.curBufferOffset / samples_per_block) * block_size;

			voice->src.decode(voice, src, dst, block_offset, decode_count);
		}

		LOG_INFO(
			voice->audio,
			"Voice %p, buffer %p, decoded %u samples from [%u,%u)",
			(void*) voice,
			(void*) buffer,
			decode_count,
			voice->src.curBufferOffset,
			voice->src.curBufferOffset + decode_count
		)

		decoded += decode_count;
		voice->src.curBufferOffset += decode_count;
		voice->src.totalSamples += decode_count;

		/* End-of-buffer behavior */
		if (decoded < *toDecode)
			end_buffer(voice);
	}

	/* ... FIXME: I keep going past the buffer so fuck it */

	if (decoded < *toDecode)
	{
		FAudio_zero(
			voice->audio->decodeCache + (
				decoded *
				voice->src.format->nChannels
			),
			sizeof(float) * (
				(*toDecode - decoded) *
				voice->src.format->nChannels
			)
		);
	}

	if (voice->src.queued_buffer_count)
	{
		float *dst = voice->audio->decodeCache + (decoded * voice->src.format->nChannels);
		struct queued_buffer *buffer = &voice->src.queued_buffers[0];
		uint32_t decode_count;

		/* Number of samples we are decoding in one call. */
		decode_count = FAudio_min(EXTRA_DECODE_PADDING,
			buffer_get_end(voice, buffer) - voice->src.curBufferOffset);

#ifdef HAVE_WMADEC
		if (voice->src.wmadec)
		{
			decode_wma(voice, buffer, dst, decode_count);
		}
		else
#endif
		{
			uint32_t block_offset = voice->src.curBufferOffset % samples_per_block;
			const uint8_t *src = buffer->buffer.pAudioData;

			src += (voice->src.curBufferOffset / samples_per_block) * block_size;

			voice->src.decode(voice, src, dst, block_offset, decode_count);
		}

		/* Do NOT increment curBufferOffset! */

		if (decode_count < EXTRA_DECODE_PADDING)
		{
			FAudio_zero(
				voice->audio->decodeCache + (
					decoded * voice->src.format->nChannels
				),
				sizeof(float) * (
					(EXTRA_DECODE_PADDING - decode_count) *
					voice->src.format->nChannels
				)
			);
		}
	}
	else
	{
		FAudio_zero(
			voice->audio->decodeCache + (
				decoded * voice->src.format->nChannels
			),
			sizeof(float) * (
				EXTRA_DECODE_PADDING *
				voice->src.format->nChannels
			)
		);
	}

	*toDecode = decoded;
	LOG_FUNC_EXIT(voice->audio)
}

static inline void FAudio_INTERNAL_FilterVoice(
	FAudio *audio,
	const FAudioFilterParametersEXT *filter,
	FAudioFilterState *filterState,
	float *samples,
	uint32_t numSamples,
	uint16_t numChannels
) {
	uint32_t j, ci;

	LOG_FUNC_ENTER(audio)

	/* Apply a digital state-variable filter to the voice.
	 * The difference equations of the filter are:
	 *
	 * Yl(n) = F Yb(n - 1) + Yl(n - 1)
	 * Yh(n) = x(n) - Yl(n) - OneOverQ Yb(n - 1)
	 * Yb(n) = F Yh(n) + Yb(n - 1)
	 * Yn(n) = Yl(n) + Yh(n)
	 *
	 * Please note that FAudioFilterParameters.Frequency is defined as:
	 *
	 * (2 * sin(pi * (desired filter cutoff frequency) / sampleRate))
	 *
	 * - @JohanSmet
	 */

	for (j = 0; j < numSamples; j += 1)
	for (ci = 0; ci < numChannels; ci += 1)
	{
		filterState[ci][FAudioLowPassFilter] = filterState[ci][FAudioLowPassFilter] + (filter->Frequency * filterState[ci][FAudioBandPassFilter]);
		filterState[ci][FAudioHighPassFilter] = samples[j * numChannels + ci] - filterState[ci][FAudioLowPassFilter] - (filter->OneOverQ * filterState[ci][FAudioBandPassFilter]);
		filterState[ci][FAudioBandPassFilter] = (filter->Frequency * filterState[ci][FAudioHighPassFilter]) + filterState[ci][FAudioBandPassFilter];
		filterState[ci][FAudioNotchFilter] = filterState[ci][FAudioHighPassFilter] + filterState[ci][FAudioLowPassFilter];
		samples[j * numChannels + ci] = filterState[ci][filter->Type] * filter->WetDryMix + samples[j * numChannels + ci] * (1.0f - filter->WetDryMix);
	}

	LOG_FUNC_EXIT(audio)
}

static void FAudio_INTERNAL_ResizeEffectChainCache(FAudio *audio, uint32_t samples)
{
	LOG_FUNC_ENTER(audio)
	if (samples > audio->effectChainSamples)
	{
		audio->effectChainSamples = samples;
		audio->effectChainCache = (float*) audio->pRealloc(
			audio->effectChainCache,
			sizeof(float) * audio->effectChainSamples
		);
	}
	LOG_FUNC_EXIT(audio)
}

static inline float *FAudio_INTERNAL_ProcessEffectChain(
	FAudioVoice *voice,
	float *buffer,
	uint32_t *samples
) {
	uint32_t i;
	FAPO *fapo;
	FAPOProcessBufferParameters srcParams, dstParams;

	LOG_FUNC_ENTER(voice->audio)

	/* Set up the buffer to be written into */
	srcParams.pBuffer = buffer;
	srcParams.BufferFlags = FAPO_BUFFER_SILENT;
	srcParams.ValidFrameCount = *samples;
	for (i = 0; i < srcParams.ValidFrameCount; i += 1)
	{
		if (buffer[i] != 0.0f) /* Arbitrary! */
		{
			srcParams.BufferFlags = FAPO_BUFFER_VALID;
			break;
		}
	}

	/* Initialize output parameters to something sane */
	dstParams.pBuffer = srcParams.pBuffer;
	dstParams.BufferFlags = FAPO_BUFFER_VALID;
	dstParams.ValidFrameCount = srcParams.ValidFrameCount;

	/* Update parameters, process! */
	for (i = 0; i < voice->effects.count; i += 1)
	{
		fapo = voice->effects.desc[i].pEffect;

		if (!voice->effects.inPlaceProcessing[i])
		{
			if (dstParams.pBuffer == buffer)
			{
				FAudio_INTERNAL_ResizeEffectChainCache(
					voice->audio,
					voice->effects.desc[i].OutputChannels * srcParams.ValidFrameCount
				);
				dstParams.pBuffer = voice->audio->effectChainCache;
			}
			else
			{
				/* FIXME: What if this is smaller because
				 * inputChannels < desc[i].OutputChannels?
				 */
				dstParams.pBuffer = buffer;
			}

			FAudio_zero(
				dstParams.pBuffer,
				voice->effects.desc[i].OutputChannels * srcParams.ValidFrameCount * sizeof(float)
			);
		}

		if (voice->effects.parameterUpdates[i])
		{
			fapo->SetParameters(
				fapo,
				voice->effects.parameters[i],
				voice->effects.parameterSizes[i]
			);
			voice->effects.parameterUpdates[i] = 0;
		}

		fapo->Process(
			fapo,
			1,
			&srcParams,
			1,
			&dstParams,
			voice->effects.desc[i].InitialState
		);

		FAudio_memcpy(&srcParams, &dstParams, sizeof(dstParams));
	}

	*samples = dstParams.ValidFrameCount;

	/* Save the output buffer-flags so the mixer-function can determine when it's save to stop processing the effect chain */
	voice->effects.state = dstParams.BufferFlags;

	LOG_FUNC_EXIT(voice->audio)
	return (float*) dstParams.pBuffer;
}

static void FAudio_INTERNAL_ResizeResampleCache(FAudio *audio, uint32_t samples)
{
       LOG_FUNC_ENTER(audio)
       if (samples > audio->resampleSamples)
       {
               audio->resampleSamples = samples;
               audio->resampleCache = (float*) audio->pRealloc(
                       audio->resampleCache,
                       sizeof(float) * audio->resampleSamples
               );
       }
       LOG_FUNC_EXIT(audio)
}

static void FAudio_INTERNAL_MixSource(FAudioSourceVoice *voice)
{
	/* Iterators */
	uint32_t i;
	/* Decode/Resample variables */
	uint64_t toDecode;
	uint64_t toResample;
	/* Output mix variables */
	float *stream;
	uint32_t mixed;
	uint32_t oChan;
	FAudioVoice *out;
	uint32_t outputRate;
	double stepd;
	float *finalSamples;

	LOG_FUNC_ENTER(voice->audio)

	FAudio_PlatformLockMutex(voice->sendLock);
	LOG_MUTEX_LOCK(voice->audio, voice->sendLock)

	/* Calculate the resample stepping value */
	if (voice->src.resampleFreq != voice->src.freqRatio * voice->src.format->nSamplesPerSec)
	{
		out = (voice->sends.SendCount == 0) ?
			voice->audio->master : /* Barf */
			voice->sends.pSends->pOutputVoice;
		outputRate = (out->type == FAUDIO_VOICE_MASTER) ?
			out->master.inputSampleRate :
			out->mix.inputSampleRate;
		stepd = (
			voice->src.freqRatio *
			(double) voice->src.format->nSamplesPerSec /
			(double) outputRate
		);
		voice->src.resampleStep = DOUBLE_TO_FIXED(stepd);
		voice->src.resampleFreq = voice->src.freqRatio * voice->src.format->nSamplesPerSec;
	}

	if (voice->src.active == 2)
	{
		/* We're just playing tails, skip all buffer stuff */
		FAudio_INTERNAL_ResizeResampleCache(
				voice->audio,
				voice->src.resampleSamples * voice->src.format->nChannels
		);
		mixed = voice->src.resampleSamples;
		FAudio_zero(
			voice->audio->resampleCache,
			mixed * voice->src.format->nChannels * sizeof(float)
		);
		finalSamples = voice->audio->resampleCache;
		goto sendwork;
	}

	/* Base decode size, int to fixed... */
	toDecode = voice->src.resampleSamples * voice->src.resampleStep;
	/* ... rounded up based on current offset... */
	toDecode += voice->src.curBufferOffsetDec + FIXED_FRACTION_MASK;
	/* ... fixed to int, truncating extra fraction from rounding. */
	toDecode >>= FIXED_PRECISION;

	/* First voice callback */
	if (	voice->src.callback != NULL &&
		voice->src.callback->OnVoiceProcessingPassStart != NULL	)
	{
		FAudio_PlatformUnlockMutex(voice->sendLock);
		LOG_MUTEX_UNLOCK(voice->audio, voice->sendLock)

		FAudio_PlatformUnlockMutex(voice->audio->sourceLock);
		LOG_MUTEX_UNLOCK(voice->audio, voice->audio->sourceLock)

		voice->src.callback->OnVoiceProcessingPassStart(
			voice->src.callback,
			FAudio_INTERNAL_GetBytesRequested(voice, (uint32_t) toDecode)
		);

		FAudio_PlatformLockMutex(voice->audio->sourceLock);
		LOG_MUTEX_LOCK(voice->audio, voice->audio->sourceLock)

		FAudio_PlatformLockMutex(voice->sendLock);
		LOG_MUTEX_LOCK(voice->audio, voice->sendLock)
	}

	FAudio_PlatformLockMutex(voice->src.bufferLock);
	LOG_MUTEX_LOCK(voice->audio, voice->src.bufferLock)

	/* Nothing to do? */
	if (!voice->src.queued_buffer_count)
	{
		FAudio_PlatformUnlockMutex(voice->src.bufferLock);
		LOG_MUTEX_UNLOCK(voice->audio, voice->src.bufferLock)

		if (voice->effects.count > 0 && voice->effects.state != FAPO_BUFFER_SILENT)
		{
			/* do not stop while the effect chain generates a non-silent buffer */
			FAudio_INTERNAL_ResizeResampleCache(
					voice->audio,
					voice->src.resampleSamples * voice->src.format->nChannels
			);
			mixed = voice->src.resampleSamples;
			FAudio_zero(
				voice->audio->resampleCache,
				mixed * voice->src.format->nChannels * sizeof(float)
			);
			finalSamples = voice->audio->resampleCache;
			goto sendwork;
		}

		FAudio_PlatformUnlockMutex(voice->sendLock);
		LOG_MUTEX_UNLOCK(voice->audio, voice->sendLock)

		FAudio_PlatformUnlockMutex(voice->audio->sourceLock);
		LOG_MUTEX_UNLOCK(voice->audio, voice->audio->sourceLock)

		if (	voice->src.callback != NULL &&
			voice->src.callback->OnVoiceProcessingPassEnd != NULL)
		{
			voice->src.callback->OnVoiceProcessingPassEnd(
				voice->src.callback
			);
		}

		FAudio_PlatformLockMutex(voice->audio->sourceLock);
		LOG_MUTEX_LOCK(voice->audio, voice->audio->sourceLock)

		LOG_FUNC_EXIT(voice->audio)
		return;
	}

	/* Decode... */
	FAudio_INTERNAL_DecodeBuffers(voice, &toDecode);

	/* Subtract any padding samples from the total, if applicable */
	if (	voice->src.curBufferOffsetDec > 0 &&
		voice->src.totalSamples > 0	)
	{
		voice->src.totalSamples -= 1;
	}

	/* Okay, we're done messing with client data */
	if (	voice->src.callback != NULL &&
		voice->src.callback->OnVoiceProcessingPassEnd != NULL)
	{
		FAudio_PlatformUnlockMutex(voice->src.bufferLock);
		LOG_MUTEX_UNLOCK(voice->audio, voice->src.bufferLock)

		FAudio_PlatformUnlockMutex(voice->sendLock);
		LOG_MUTEX_UNLOCK(voice->audio, voice->sendLock)

		FAudio_PlatformUnlockMutex(voice->audio->sourceLock);
		LOG_MUTEX_UNLOCK(voice->audio, voice->audio->sourceLock)

		voice->src.callback->OnVoiceProcessingPassEnd(
			voice->src.callback
		);

		FAudio_PlatformLockMutex(voice->audio->sourceLock);
		LOG_MUTEX_LOCK(voice->audio, voice->audio->sourceLock)

		FAudio_PlatformLockMutex(voice->sendLock);
		LOG_MUTEX_LOCK(voice->audio, voice->sendLock)

		FAudio_PlatformLockMutex(voice->src.bufferLock);
		LOG_MUTEX_LOCK(voice->audio, voice->src.bufferLock)
	}

	/* Nothing to resample? */
	if (toDecode == 0)
	{
		FAudio_PlatformUnlockMutex(voice->src.bufferLock);
		LOG_MUTEX_UNLOCK(voice->audio, voice->src.bufferLock)

		FAudio_PlatformUnlockMutex(voice->sendLock);
		LOG_MUTEX_UNLOCK(voice->audio, voice->sendLock)

		LOG_FUNC_EXIT(voice->audio)
		return;
	}

	/* int to fixed... */
	toResample = toDecode << FIXED_PRECISION;
	/* ... round back down based on current offset... */
	toResample -= voice->src.curBufferOffsetDec;
	/* ... but also ceil for any fraction value... */
	toResample += FIXED_FRACTION_MASK;
	/* ... undo step size, fixed to int. */
	toResample /= voice->src.resampleStep;
	/* Add the padding, for some reason this helps? */
	toResample += EXTRA_DECODE_PADDING;
	/* FIXME: I feel like this should be an assert but I suck */
	toResample = FAudio_min(toResample, voice->src.resampleSamples);

	/* Resample... */
	if (voice->src.resampleStep == FIXED_ONE)
	{
		/* Actually, just use the existing buffer... */
		finalSamples = voice->audio->decodeCache;
	}
	else
	{
		FAudio_INTERNAL_ResizeResampleCache(
				voice->audio,
				voice->src.resampleSamples * voice->src.format->nChannels
		);
		voice->src.resample(
			voice->audio->decodeCache,
			voice->audio->resampleCache,
			&voice->src.resampleOffset,
			voice->src.resampleStep,
			toResample,
			(uint8_t) voice->src.format->nChannels
		);
		finalSamples = voice->audio->resampleCache;
	}

	/* Update buffer offsets */
	if (voice->src.queued_buffer_count)
	{
		/* Increment fixed offset by resample size, int to fixed... */
		voice->src.curBufferOffsetDec += toResample * voice->src.resampleStep;
		/* ... chop off any ints we got from the above increment */
		voice->src.curBufferOffsetDec &= FIXED_FRACTION_MASK;

		/* Dec >0? We need one frame from the past...
		 * FIXME: We can't go back to a prev buffer though?
		 */
		if (	voice->src.curBufferOffsetDec > 0 &&
			voice->src.curBufferOffset > 0	)
		{
			voice->src.curBufferOffset -= 1;
		}
	}
	else
	{
		voice->src.curBufferOffsetDec = 0;
		voice->src.curBufferOffset = 0;
	}

	/* Done with buffers, finally. */
	FAudio_PlatformUnlockMutex(voice->src.bufferLock);
	LOG_MUTEX_UNLOCK(voice->audio, voice->src.bufferLock)
	mixed = (uint32_t) toResample;

sendwork:

	/* Filters */
	if (voice->flags & FAUDIO_VOICE_USEFILTER)
	{
		FAudio_PlatformLockMutex(voice->filterLock);
		LOG_MUTEX_LOCK(voice->audio, voice->filterLock)
		FAudio_INTERNAL_FilterVoice(
			voice->audio,
			&voice->filter,
			voice->filterState,
			finalSamples,
			mixed,
			voice->src.format->nChannels
		);
		FAudio_PlatformUnlockMutex(voice->filterLock);
		LOG_MUTEX_UNLOCK(voice->audio, voice->filterLock)
	}

	/* Process effect chain */
	FAudio_PlatformLockMutex(voice->effectLock);
	LOG_MUTEX_LOCK(voice->audio, voice->effectLock)
	if (voice->effects.count > 0)
	{
		/* If we didn't get the full size of the update, we have to fill
		 * it with silence so the effect can process a whole update
		 */
		if (mixed < voice->src.resampleSamples)
		{
			FAudio_zero(
				finalSamples + (mixed * voice->src.format->nChannels),
				(voice->src.resampleSamples - mixed) * voice->src.format->nChannels * sizeof(float)
			);
			mixed = voice->src.resampleSamples;
		}
		finalSamples = FAudio_INTERNAL_ProcessEffectChain(
			voice,
			finalSamples,
			&mixed
		);
	}
	FAudio_PlatformUnlockMutex(voice->effectLock);
	LOG_MUTEX_UNLOCK(voice->audio, voice->effectLock)

	/* Nowhere to send it? Just skip the rest...*/
	if (voice->sends.SendCount == 0)
	{
		FAudio_PlatformUnlockMutex(voice->sendLock);
		LOG_MUTEX_UNLOCK(voice->audio, voice->sendLock)
		LOG_FUNC_EXIT(voice->audio)
		return;
	}

	/* Send float cache to sends */
	FAudio_PlatformLockMutex(voice->volumeLock);
	LOG_MUTEX_LOCK(voice->audio, voice->volumeLock)
	for (i = 0; i < voice->sends.SendCount; i += 1)
	{
		out = voice->sends.pSends[i].pOutputVoice;
		if (out->type == FAUDIO_VOICE_MASTER)
		{
			stream = out->master.output;
			oChan = out->master.inputChannels;
		}
		else
		{
			stream = out->mix.inputCache;
			oChan = out->mix.inputChannels;
		}

		voice->sendMix[i](
			mixed,
			voice->outputChannels,
			oChan,
			finalSamples,
			stream,
			voice->mixCoefficients[i]
		);

		if (voice->sends.pSends[i].Flags & FAUDIO_SEND_USEFILTER)
		{
			FAudio_INTERNAL_FilterVoice(
				voice->audio,
				&voice->sendFilter[i],
				voice->sendFilterState[i],
				stream,
				mixed,
				oChan
			);
		}
	}
	FAudio_PlatformUnlockMutex(voice->volumeLock);
	LOG_MUTEX_UNLOCK(voice->audio, voice->volumeLock)

	FAudio_PlatformUnlockMutex(voice->sendLock);
	LOG_MUTEX_UNLOCK(voice->audio, voice->sendLock)
	LOG_FUNC_EXIT(voice->audio)
}

static void FAudio_INTERNAL_MixSubmix(FAudioSubmixVoice *voice)
{
	uint32_t i;
	float *stream;
	uint32_t oChan;
	FAudioVoice *out;
	uint32_t resampled;
	uint64_t resampleOffset = 0;
	float *finalSamples;

	LOG_FUNC_ENTER(voice->audio)
	FAudio_PlatformLockMutex(voice->sendLock);
	LOG_MUTEX_LOCK(voice->audio, voice->sendLock)

	/* Resample */
	if (voice->mix.resampleStep == FIXED_ONE)
	{
		/* Actually, just use the existing buffer... */
		finalSamples = voice->mix.inputCache;
	}
	else
	{
		FAudio_INTERNAL_ResizeResampleCache(
				voice->audio,
				voice->mix.outputSamples * voice->mix.inputChannels
		);
		voice->mix.resample(
			voice->mix.inputCache,
			voice->audio->resampleCache,
			&resampleOffset,
			voice->mix.resampleStep,
			voice->mix.outputSamples,
			(uint8_t) voice->mix.inputChannels
		);
		finalSamples = voice->audio->resampleCache;
	}
	resampled = voice->mix.outputSamples * voice->mix.inputChannels;

	/* Submix overall volume is applied _before_ effects/filters, blech! */
	if (voice->volume != 1.0f)
	{
		FAudio_INTERNAL_Amplify(
			finalSamples,
			resampled,
			voice->volume
		);
	}
	resampled /= voice->mix.inputChannels;

	/* Filters */
	if (voice->flags & FAUDIO_VOICE_USEFILTER)
	{
		FAudio_PlatformLockMutex(voice->filterLock);
		LOG_MUTEX_LOCK(voice->audio, voice->filterLock)
		FAudio_INTERNAL_FilterVoice(
			voice->audio,
			&voice->filter,
			voice->filterState,
			finalSamples,
			resampled,
			voice->mix.inputChannels
		);
		FAudio_PlatformUnlockMutex(voice->filterLock);
		LOG_MUTEX_UNLOCK(voice->audio, voice->filterLock)
	}

	/* Process effect chain */
	FAudio_PlatformLockMutex(voice->effectLock);
	LOG_MUTEX_LOCK(voice->audio, voice->effectLock)
	if (voice->effects.count > 0)
	{
		finalSamples = FAudio_INTERNAL_ProcessEffectChain(
			voice,
			finalSamples,
			&resampled
		);
	}
	FAudio_PlatformUnlockMutex(voice->effectLock);
	LOG_MUTEX_UNLOCK(voice->audio, voice->effectLock)

	/* Nothing more to do? */
	if (voice->sends.SendCount == 0)
	{
		goto end;
	}

	/* Send float cache to sends */
	FAudio_PlatformLockMutex(voice->volumeLock);
	LOG_MUTEX_LOCK(voice->audio, voice->volumeLock)
	for (i = 0; i < voice->sends.SendCount; i += 1)
	{
		out = voice->sends.pSends[i].pOutputVoice;
		if (out->type == FAUDIO_VOICE_MASTER)
		{
			stream = out->master.output;
			oChan = out->master.inputChannels;
		}
		else
		{
			stream = out->mix.inputCache;
			oChan = out->mix.inputChannels;
		}

		voice->sendMix[i](
			resampled,
			voice->outputChannels,
			oChan,
			finalSamples,
			stream,
			voice->mixCoefficients[i]
		);

		if (voice->sends.pSends[i].Flags & FAUDIO_SEND_USEFILTER)
		{
			FAudio_INTERNAL_FilterVoice(
				voice->audio,
				&voice->sendFilter[i],
				voice->sendFilterState[i],
				stream,
				resampled,
				oChan
			);
		}
	}
	FAudio_PlatformUnlockMutex(voice->volumeLock);
	LOG_MUTEX_UNLOCK(voice->audio, voice->volumeLock)

	/* Zero this at the end, for the next update */
end:
	FAudio_PlatformUnlockMutex(voice->sendLock);
	LOG_MUTEX_UNLOCK(voice->audio, voice->sendLock)
	FAudio_zero(
		voice->mix.inputCache,
		sizeof(float) * voice->mix.inputSamples
	);
	LOG_FUNC_EXIT(voice->audio)
}

static void FAudio_INTERNAL_FlushPendingBuffers(FAudioSourceVoice *voice)
{
	FAudio_PlatformLockMutex(voice->src.bufferLock);
	LOG_MUTEX_LOCK(voice->audio, voice->src.bufferLock)

	if (voice->src.callback == NULL || voice->src.callback->OnBufferEnd == NULL)
	{
		/* We can skip the memory churn below if nobody's looking */
		voice->src.flush_buffer_count = 0;
	}

	/* Remove pending flushed buffers and send an event for each one */
	else while (voice->src.flush_buffer_count > 0)
	{
		void* pContext = voice->src.flush_buffers[0].buffer.pContext;

		/* Subtract each one instead of setting 0 at the end; this is
		 * needed to make GetState accurate inside this callback
		 */
		voice->src.flush_buffer_count -= 1;
		FAudio_memmove(&voice->src.flush_buffers[0], &voice->src.flush_buffers[1],
			voice->src.flush_buffer_count * sizeof(*voice->src.flush_buffers));

		FAudio_PlatformUnlockMutex(voice->src.bufferLock);
		LOG_MUTEX_UNLOCK(voice->audio, voice->src.bufferLock)

		FAudio_PlatformUnlockMutex(voice->audio->sourceLock);
		LOG_MUTEX_UNLOCK(voice->audio, voice->audio->sourceLock)

		voice->src.callback->OnBufferEnd(voice->src.callback, pContext);

		FAudio_PlatformLockMutex(voice->audio->sourceLock);
		LOG_MUTEX_LOCK(voice->audio, voice->audio->sourceLock)

		FAudio_PlatformLockMutex(voice->src.bufferLock);
		LOG_MUTEX_LOCK(voice->audio, voice->src.bufferLock)
	}


	FAudio_PlatformUnlockMutex(voice->src.bufferLock);
	LOG_MUTEX_UNLOCK(voice->audio, voice->src.bufferLock)
}

static void FAUDIOCALL FAudio_INTERNAL_GenerateOutput(FAudio *audio, float *output)
{
	uint32_t totalSamples;
	LinkedList *list;
	float *effectOut;
	FAudioEngineCallback *callback;

	LOG_FUNC_ENTER(audio)
	if (!audio->active)
	{
		LOG_FUNC_EXIT(audio)
		return;
	}

	/* Apply any committed changes */
	FAudio_OPERATIONSET_Execute(audio);

	/* ProcessingPassStart callbacks */
	FAudio_PlatformLockMutex(audio->callbackLock);
	LOG_MUTEX_LOCK(audio, audio->callbackLock)
	list = audio->callbacks;
	while (list != NULL)
	{
		callback = (FAudioEngineCallback*) list->entry;
		if (callback->OnProcessingPassStart != NULL)
		{
			callback->OnProcessingPassStart(
				callback
			);
		}
		list = list->next;
	}
	FAudio_PlatformUnlockMutex(audio->callbackLock);
	LOG_MUTEX_UNLOCK(audio, audio->callbackLock)

	/* Writes to master will directly write to output, but ONLY if there
	 * isn't any channel-changing effect processing to do first.
	 */
	if (audio->master->master.effectCache != NULL)
	{
		audio->master->master.output = audio->master->master.effectCache;
		FAudio_zero(
			audio->master->master.effectCache,
			(
				sizeof(float) *
				audio->updateSize *
				audio->master->master.inputChannels
			)
		);
	}
	else
	{
		audio->master->master.output = output;
	}

	/* Mix sources */
	FAudio_PlatformLockMutex(audio->sourceLock);
	LOG_MUTEX_LOCK(audio, audio->sourceLock)
	list = audio->sources;
	while (list != NULL)
	{
		audio->processingSource = (FAudioSourceVoice*) list->entry;

		FAudio_INTERNAL_FlushPendingBuffers(audio->processingSource);
		if (audio->processingSource->src.active)
		{
			FAudio_INTERNAL_MixSource(audio->processingSource);
			FAudio_INTERNAL_FlushPendingBuffers(audio->processingSource);
		}

		list = list->next;
	}
	audio->processingSource = NULL;
	FAudio_PlatformUnlockMutex(audio->sourceLock);
	LOG_MUTEX_UNLOCK(audio, audio->sourceLock)

	/* Mix submixes, ordered by processing stage */
	FAudio_PlatformLockMutex(audio->submixLock);
	LOG_MUTEX_LOCK(audio, audio->submixLock)
	list = audio->submixes;
	while (list != NULL)
	{
		FAudio_INTERNAL_MixSubmix((FAudioSubmixVoice*) list->entry);
		list = list->next;
	}
	FAudio_PlatformUnlockMutex(audio->submixLock);
	LOG_MUTEX_UNLOCK(audio, audio->submixLock)

	/* Apply master volume */
	if (audio->master->volume != 1.0f)
	{
		FAudio_INTERNAL_Amplify(
			audio->master->master.output,
			audio->updateSize * audio->master->master.inputChannels,
			audio->master->volume
		);
	}

	/* Process master effect chain */
	FAudio_PlatformLockMutex(audio->master->effectLock);
	LOG_MUTEX_LOCK(audio, audio->master->effectLock)
	if (audio->master->effects.count > 0)
	{
		totalSamples = audio->updateSize;
		effectOut = FAudio_INTERNAL_ProcessEffectChain(
			audio->master,
			audio->master->master.output,
			&totalSamples
		);

		if (effectOut != output)
		{
			FAudio_memcpy(
				output,
				effectOut,
				totalSamples * audio->master->outputChannels * sizeof(float)
			);
		}
		if (totalSamples < audio->updateSize)
		{
			FAudio_zero(
				output + (totalSamples * audio->master->outputChannels),
				(audio->updateSize - totalSamples) * sizeof(float)
			);
		}
	}
	FAudio_PlatformUnlockMutex(audio->master->effectLock);
	LOG_MUTEX_UNLOCK(audio, audio->master->effectLock)

	/* OnProcessingPassEnd callbacks */
	FAudio_PlatformLockMutex(audio->callbackLock);
	LOG_MUTEX_LOCK(audio, audio->callbackLock)
	list = audio->callbacks;
	while (list != NULL)
	{
		callback = (FAudioEngineCallback*) list->entry;
		if (callback->OnProcessingPassEnd != NULL)
		{
			callback->OnProcessingPassEnd(
				callback
			);
		}
		list = list->next;
	}
	FAudio_PlatformUnlockMutex(audio->callbackLock);
	LOG_MUTEX_UNLOCK(audio, audio->callbackLock)

	LOG_FUNC_EXIT(audio)
}

void FAudio_INTERNAL_UpdateEngine(FAudio *audio, float *output)
{
	LOG_FUNC_ENTER(audio)
	if (audio->pClientEngineProc)
	{
		audio->pClientEngineProc(
			&FAudio_INTERNAL_GenerateOutput,
			audio,
			output,
			audio->clientEngineUser
		);
	}
	else
	{
		FAudio_INTERNAL_GenerateOutput(audio, output);
	}
	LOG_FUNC_EXIT(audio)
}

void FAudio_INTERNAL_ResizeDecodeCache(FAudio *audio, uint32_t samples)
{
	LOG_FUNC_ENTER(audio)
	FAudio_PlatformLockMutex(audio->sourceLock);
	LOG_MUTEX_LOCK(audio, audio->sourceLock)
	if (samples > audio->decodeSamples)
	{
		audio->decodeSamples = samples;
		audio->decodeCache = (float*) audio->pRealloc(
			audio->decodeCache,
			sizeof(float) * audio->decodeSamples
		);
	}
	FAudio_PlatformUnlockMutex(audio->sourceLock);
	LOG_MUTEX_UNLOCK(audio, audio->sourceLock)
	LOG_FUNC_EXIT(audio)
}

void FAudio_INTERNAL_AllocEffectChain(
	FAudioVoice *voice,
	const FAudioEffectChain *pEffectChain
) {
	uint32_t i;

	LOG_FUNC_ENTER(voice->audio)
	voice->effects.state = FAPO_BUFFER_VALID;
	voice->effects.count = pEffectChain->EffectCount;
	if (voice->effects.count == 0)
	{
		LOG_FUNC_EXIT(voice->audio)
		return;
	}

	for (i = 0; i < pEffectChain->EffectCount; i += 1)
	{
		pEffectChain->pEffectDescriptors[i].pEffect->AddRef(pEffectChain->pEffectDescriptors[i].pEffect);
	}

	voice->effects.desc = (FAudioEffectDescriptor*) voice->audio->pMalloc(
		voice->effects.count * sizeof(FAudioEffectDescriptor)
	);
	FAudio_memcpy(
		voice->effects.desc,
		pEffectChain->pEffectDescriptors,
		voice->effects.count * sizeof(FAudioEffectDescriptor)
	);
	#define ALLOC_EFFECT_PROPERTY(prop, type) \
		voice->effects.prop = (type*) voice->audio->pMalloc( \
			voice->effects.count * sizeof(type) \
		); \
		FAudio_zero( \
			voice->effects.prop, \
			voice->effects.count * sizeof(type) \
		);
	ALLOC_EFFECT_PROPERTY(parameters, void*)
	ALLOC_EFFECT_PROPERTY(parameterSizes, uint32_t)
	ALLOC_EFFECT_PROPERTY(parameterUpdates, uint8_t)
	ALLOC_EFFECT_PROPERTY(inPlaceProcessing, uint8_t)
	#undef ALLOC_EFFECT_PROPERTY
	LOG_FUNC_EXIT(voice->audio)
}

void FAudio_INTERNAL_FreeEffectChain(FAudioVoice *voice)
{
	uint32_t i;

	LOG_FUNC_ENTER(voice->audio)
	if (voice->effects.count == 0)
	{
		LOG_FUNC_EXIT(voice->audio)
		return;
	}

	for (i = 0; i < voice->effects.count; i += 1)
	{
		voice->effects.desc[i].pEffect->UnlockForProcess(voice->effects.desc[i].pEffect);
		voice->effects.desc[i].pEffect->Release(voice->effects.desc[i].pEffect);
		voice->audio->pFree(voice->effects.parameters[i]);
	}

	voice->audio->pFree(voice->effects.desc);
	voice->audio->pFree(voice->effects.parameters);
	voice->audio->pFree(voice->effects.parameterSizes);
	voice->audio->pFree(voice->effects.parameterUpdates);
	voice->audio->pFree(voice->effects.inPlaceProcessing);
	LOG_FUNC_EXIT(voice->audio)
}

uint32_t FAudio_INTERNAL_VoiceOutputFrequency(
	FAudioVoice *voice,
	const FAudioVoiceSends *pSendList
) {
	uint32_t outSampleRate;
	uint32_t newResampleSamples;
	uint64_t resampleSanityCheck;

	LOG_FUNC_ENTER(voice->audio)

	if ((pSendList == NULL) || (pSendList->SendCount == 0))
	{
		/* When we're deliberately given no sends, use master rate! */
		FAudio_assert(voice->audio->master != NULL);
		outSampleRate = voice->audio->master->master.inputSampleRate;
	}
	else
	{
		outSampleRate = pSendList->pSends[0].pOutputVoice->type == FAUDIO_VOICE_MASTER ?
			pSendList->pSends[0].pOutputVoice->master.inputSampleRate :
			pSendList->pSends[0].pOutputVoice->mix.inputSampleRate;
	}
	newResampleSamples = (uint32_t) FAudio_ceil(
		voice->audio->updateSize *
		(double) outSampleRate /
		(double) voice->audio->master->master.inputSampleRate
	);
	if (voice->type == FAUDIO_VOICE_SOURCE)
	{
		if (	(voice->src.resampleSamples != 0) &&
			(newResampleSamples != voice->src.resampleSamples) &&
			(voice->effects.count > 0)	)
		{
			LOG_FUNC_EXIT(voice->audio)
			return FAUDIO_E_INVALID_CALL;
		}
		voice->src.resampleSamples = newResampleSamples;
	}
	else /* (voice->type == FAUDIO_VOICE_SUBMIX) */
	{
		if (	(voice->mix.outputSamples != 0) &&
			(newResampleSamples != voice->mix.outputSamples) &&
			(voice->effects.count > 0)	)
		{
			LOG_FUNC_EXIT(voice->audio)
			return FAUDIO_E_INVALID_CALL;
		}
		voice->mix.outputSamples = newResampleSamples;

		voice->mix.resampleStep = DOUBLE_TO_FIXED((
			(double) voice->mix.inputSampleRate /
			(double) outSampleRate
		));

		/* Because we used ceil earlier, there's a chance that
		 * downsampling submixes will go past the number of samples
		 * available. Sources can do this thanks to padding, but we
		 * don't have that luxury for submixes, so unfortunately we
		 * just have to undo the ceil and turn it into a floor.
		 * -flibit
		 */
		resampleSanityCheck = (
			voice->mix.resampleStep * voice->mix.outputSamples
		) >> FIXED_PRECISION;
		if (resampleSanityCheck > (voice->mix.inputSamples / voice->mix.inputChannels))
		{
			voice->mix.outputSamples -= 1;
		}
	}

	LOG_FUNC_EXIT(voice->audio)
	return 0;
}

const float FAUDIO_INTERNAL_MATRIX_DEFAULTS[8][8][64] =
{
	#include "matrix_defaults.inl"
};

/* PCM Decoding */

void FAudio_INTERNAL_DecodePCM8(FAudioVoice *voice, const void *src,
	float *decodeCache, uint32_t block_offset, uint32_t samples)
{
	LOG_FUNC_ENTER(voice->audio)
	FAudio_INTERNAL_Convert_U8_To_F32(src, decodeCache, samples * voice->src.format->nChannels);
	LOG_FUNC_EXIT(voice->audio)
}

void FAudio_INTERNAL_DecodePCM16(FAudioVoice *voice, const void *src,
	float *decodeCache, uint32_t block_offset, uint32_t samples)
{
	LOG_FUNC_ENTER(voice->audio)
	FAudio_INTERNAL_Convert_S16_To_F32(src, decodeCache, samples * voice->src.format->nChannels);
	LOG_FUNC_EXIT(voice->audio)
}

void FAudio_INTERNAL_DecodePCM24(FAudioVoice *voice, const void *src,
	float *decodeCache, uint32_t block_offset, uint32_t samples)
{
	uint32_t i, j;
	const uint8_t *buf = src;
	LOG_FUNC_ENTER(voice->audio)

	/* FIXME: Uh... is this something that can be SIMD-ified? */
	for (i = 0; i < samples; i += 1, buf += voice->src.format->nBlockAlign)
	for (j = 0; j < voice->src.format->nChannels; j += 1)
	{
		*decodeCache++ = ((int32_t) (
			((uint32_t) buf[(j * 3) + 2] << 24) |
			((uint32_t) buf[(j * 3) + 1] << 16) |
			((uint32_t) buf[(j * 3) + 0] << 8)
		) >> 8) / 8388607.0f;
	}

	LOG_FUNC_EXIT(voice->audio)
}

void FAudio_INTERNAL_DecodePCM32(FAudioVoice *voice, const void *src,
	float *decodeCache, uint32_t block_offset, uint32_t samples)
{
	LOG_FUNC_ENTER(voice->audio)
	FAudio_INTERNAL_Convert_S32_To_F32(src, decodeCache, samples * voice->src.format->nChannels);
	LOG_FUNC_EXIT(voice->audio)
}

void FAudio_INTERNAL_DecodePCM32F(FAudioVoice *voice, const void *src,
	float *decodeCache, uint32_t block_offset, uint32_t samples)
{
	LOG_FUNC_ENTER(voice->audio)
	FAudio_memcpy(decodeCache, src, sizeof(float) * samples * voice->src.format->nChannels);
	LOG_FUNC_EXIT(voice->audio)
}

/* MSADPCM Decoding */

static float FAudio_INTERNAL_ParseNibble(
	uint8_t nibble,
	uint8_t predictor,
	int16_t *delta,
	int16_t *sample1,
	int16_t *sample2
) {
	static const int32_t AdaptionTable[16] =
	{
		230, 230, 230, 230, 307, 409, 512, 614,
		768, 614, 512, 409, 307, 230, 230, 230
	};
	static const int32_t AdaptCoeff_1[7] =
	{
		256, 512, 0, 192, 240, 460, 392
	};
	static const int32_t AdaptCoeff_2[7] =
	{
		0, -256, 0, 64, 0, -208, -232
	};

	int8_t signedNibble;
	int32_t sampleInt;
	int16_t sample;

	signedNibble = (int8_t) nibble;
	if (signedNibble & 0x08)
	{
		signedNibble -= 0x10;
	}

	sampleInt = (
		(*sample1 * AdaptCoeff_1[predictor]) +
		(*sample2 * AdaptCoeff_2[predictor])
	) / 256;
	sampleInt += signedNibble * (*delta);
	sample = FAudio_clamp(sampleInt, -32768, 32767);

	*sample2 = *sample1;
	*sample1 = sample;
	*delta = (int16_t) (AdaptionTable[nibble] * (int32_t) (*delta) / 256);
	if (*delta < 16)
	{
		*delta = 16;
	}
	return sample / 32768.0;
}

static void decode_mono_adpcm_block(const uint8_t *src, float *dst, uint32_t offset, uint32_t count)
{
	uint32_t i;

	/* Temp storage for ADPCM blocks */
	uint8_t predictor;
	int16_t delta;
	int16_t sample1;
	int16_t sample2;

	predictor = src[0];
	delta = *(int16_t *)&src[1];
	sample1 = *(int16_t *)&src[3];
	sample2 = *(int16_t *)&src[5];
	src += 7;

	/* Samples */
	for (i = 0; i < FAudio_min(2, offset + count); ++i)
	{
		if (i < offset) continue;
		if (i == 0) *dst++ = sample2 / 32768.0;
		if (i == 1) *dst++ = sample1 / 32768.0;
	}
	for (; i < offset + count; i += 2)
	{
		float high, low;

		high = FAudio_INTERNAL_ParseNibble(
			*src >> 4,
			predictor,
			&delta,
			&sample1,
			&sample2
		);
		low = FAudio_INTERNAL_ParseNibble(
			*src & 0xf,
			predictor,
			&delta,
			&sample1,
			&sample2
		);

		if (i >= offset)
		{
			*dst++ = high;
			if (i + 1 < offset + count)
				*dst++ = low;
		}

		++src;
	}
}

static void decode_stereo_adpcm_block(const uint8_t *src, float *dst, uint32_t offset, uint32_t count)
{
	uint32_t i;

	/* Temp storage for ADPCM blocks */
	uint8_t l_predictor;
	uint8_t r_predictor;
	int16_t l_delta;
	int16_t r_delta;
	int16_t l_sample1;
	int16_t r_sample1;
	int16_t l_sample2;
	int16_t r_sample2;

	/* Preamble */
	l_predictor = src[0];
	r_predictor = src[1];
	l_delta = *(int16_t *)&src[2];
	r_delta = *(int16_t *)&src[4];
	l_sample1 = *(int16_t *)&src[6];
	r_sample1 = *(int16_t *)&src[8];
	l_sample2 = *(int16_t *)&src[10];
	r_sample2 = *(int16_t *)&src[12];
	src += 14;

	/* Samples */
	for (i = 0; i < FAudio_min(2, offset + count); ++i)
	{
		if (i < offset) continue;
		if (i == 0)
		{
			*dst++ = l_sample2 / 32768.0;
			*dst++ = r_sample2 / 32768.0;
		}
		if (i == 1)
		{
			*dst++ = l_sample1 / 32768.0;
			*dst++ = r_sample1 / 32768.0;
		}
	}
	for (; i < offset + count; ++i)
	{
		float left, right;

		left = FAudio_INTERNAL_ParseNibble(
			*src >> 4,
			l_predictor,
			&l_delta,
			&l_sample1,
			&l_sample2
		);
		right = FAudio_INTERNAL_ParseNibble(
			*src & 0xf,
			r_predictor,
			&r_delta,
			&r_sample1,
			&r_sample2
		);

		if (i >= offset)
		{
			*dst++ = left;
			*dst++ = right;
		}

		++src;
	}
}

void FAudio_INTERNAL_DecodeMonoMSADPCM(FAudioVoice *voice, const void *src,
	float *decodeCache, uint32_t block_offset, uint32_t samples)
{
	const uint32_t block_size = voice->src.format->nBlockAlign;

	/* Loop variables */
	uint32_t copy, done = 0;

	/* Block size */
	uint32_t samples_per_block = ((FAudioADPCMWaveFormat *)voice->src.format)->wSamplesPerBlock;

	LOG_FUNC_ENTER(voice->audio)

	/* Read in each block directly to the decode cache */
	while (done < samples)
	{
		copy = FAudio_min(samples - done, samples_per_block - block_offset);
		decode_mono_adpcm_block(src, decodeCache, block_offset, copy);
		src = (char *)src + block_size;
		decodeCache += copy;
		done += copy;
		block_offset = 0;
	}
	LOG_FUNC_EXIT(voice->audio)
}

void FAudio_INTERNAL_DecodeStereoMSADPCM(FAudioVoice *voice, const void *src,
	float *decodeCache, uint32_t block_offset, uint32_t samples)
{
	const uint32_t block_size = voice->src.format->nBlockAlign;

	/* Loop variables */
	uint32_t copy, done = 0;

	/* Align, block size */
	uint32_t samples_per_block = ((FAudioADPCMWaveFormat *)voice->src.format)->wSamplesPerBlock;

	LOG_FUNC_ENTER(voice->audio)

	/* Read in each block directly to the decode cache */
	while (done < samples)
	{
		copy = FAudio_min(samples - done, samples_per_block - block_offset);
		decode_stereo_adpcm_block(src, decodeCache, block_offset, copy);
		src = (char *)src + block_size;
		decodeCache += copy * 2;
		done += copy;
		block_offset = 0;
	}
	LOG_FUNC_EXIT(voice->audio)
}

/* vim: set noexpandtab shiftwidth=8 tabstop=8: */
