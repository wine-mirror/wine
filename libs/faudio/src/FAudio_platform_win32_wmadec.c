/* FAudio WMADEC implementation over Win32 MF */

#ifdef HAVE_WMADEC

#include "FAudio_internal.h"

#include <stddef.h>

#define COBJMACROS
#include <windows.h>
#include <mfidl.h>
#include <mfapi.h>
#include <mferror.h>

#include <initguid.h>

DEFINE_GUID(CLSID_CWMADecMediaObject, 0x2eeb4adf, 0x4578, 0x4d10, 0xbc, 0xa7, 0xbb, 0x95, 0x5f, 0x56, 0x32, 0x0a);

DEFINE_MEDIATYPE_GUID(MFAudioFormat_XMAudio2, FAUDIO_FORMAT_XMAUDIO2);

struct FAudioWMADEC
{
	IMFTransform *decoder;
	IMFSample *output_sample;

	char *output_buf;
	size_t output_pos;
	size_t output_size;
	size_t input_pos;
	size_t input_size;
};

static HRESULT FAudio_WMAMF_ProcessInput(
	FAudioVoice *voice,
	FAudioBuffer *buffer
) {
	struct FAudioWMADEC *impl = voice->src.wmadec;
	IMFMediaBuffer *media_buffer;
	IMFSample *sample;
	DWORD copy_size;
	BYTE *copy_buf;
	HRESULT hr;

	copy_size = (DWORD)min(buffer->AudioBytes - impl->input_pos, impl->input_size);
	if (!copy_size) return S_FALSE;
	LOG_INFO(voice->audio, "pushing %lx bytes at %Ix", copy_size, impl->input_pos);

	hr = MFCreateSample(&sample);
	FAudio_assert(!FAILED(hr) && "Failed to create sample!");
	hr = MFCreateMemoryBuffer(copy_size, &media_buffer);
	FAudio_assert(!FAILED(hr) && "Failed to create buffer!");
	hr = IMFMediaBuffer_SetCurrentLength(media_buffer, copy_size);
	FAudio_assert(!FAILED(hr) && "Failed to set buffer length!");
	hr = IMFMediaBuffer_Lock(
		media_buffer,
		&copy_buf,
		NULL,
		&copy_size
	);
	FAudio_assert(!FAILED(hr) && "Failed to lock buffer bytes!");
	FAudio_memcpy(copy_buf, buffer->pAudioData + impl->input_pos, copy_size);
	hr = IMFMediaBuffer_Unlock(media_buffer);
	FAudio_assert(!FAILED(hr) && "Failed to unlock buffer bytes!");

	hr = IMFSample_AddBuffer(sample, media_buffer);
	FAudio_assert(!FAILED(hr) && "Failed to buffer to sample!");
	IMFMediaBuffer_Release(media_buffer);

	hr = IMFTransform_ProcessInput(impl->decoder, 0, sample, 0);
	IMFSample_Release(sample);
	if (hr == MF_E_NOTACCEPTING) return S_OK;
	if (FAILED(hr))
	{
		LOG_ERROR(voice->audio, "IMFTransform_ProcessInput returned %#lx", hr);
		return hr;
	}

	impl->input_pos += copy_size;
	return S_OK;
};

static HRESULT FAudio_WMAMF_ProcessOutput(
	FAudioVoice *voice,
	FAudioBuffer *buffer
) {
	struct FAudioWMADEC *impl = voice->src.wmadec;
	MFT_OUTPUT_DATA_BUFFER output;
	IMFMediaBuffer *media_buffer;
	DWORD status, copy_size;
	BYTE *copy_buf;
	HRESULT hr;

	while (1)
	{
		FAudio_memset(&output, 0, sizeof(output));
		output.pSample = impl->output_sample;
		hr = IMFTransform_ProcessOutput(impl->decoder, 0, 1, &output, &status);
		if (hr == MF_E_TRANSFORM_NEED_MORE_INPUT) return S_FALSE;
		if (FAILED(hr))
		{
			LOG_ERROR(voice->audio, "IMFTransform_ProcessInput returned %#lx", hr);
			return hr;
		}

		if (output.dwStatus & MFT_OUTPUT_DATA_BUFFER_NO_SAMPLE) continue;

		hr = IMFSample_ConvertToContiguousBuffer(
			output.pSample,
			&media_buffer
		);
		FAudio_assert(!FAILED(hr) && "Failed to get sample buffer!");
		hr = IMFMediaBuffer_Lock(
			media_buffer,
			&copy_buf,
			NULL,
			&copy_size
		);
		FAudio_assert(!FAILED(hr) && "Failed to lock buffer bytes!");
		if (impl->output_pos + copy_size > impl->output_size)
		{
			impl->output_size = max(
				impl->output_pos + copy_size,
				impl->output_size * 3 / 2
			);
			impl->output_buf = voice->audio->pRealloc(
				impl->output_buf,
				impl->output_size
			);
			FAudio_assert(impl->output_buf && "Failed to resize output buffer!");
		}
		FAudio_memcpy(impl->output_buf + impl->output_pos, copy_buf, copy_size);
		impl->output_pos += copy_size;
		LOG_INFO(voice->audio, "pulled %lx bytes at %Ix", copy_size, impl->output_pos);
		hr = IMFMediaBuffer_Unlock(media_buffer);
		FAudio_assert(!FAILED(hr) && "Failed to unlock buffer bytes!");

		IMFMediaBuffer_Release(media_buffer);
		if (!impl->output_sample) IMFSample_Release(output.pSample);
	}

	return S_OK;
};

static void FAudio_INTERNAL_DecodeWMAMF(
	FAudioVoice *voice,
	FAudioBuffer *buffer,
	float *decodeCache,
	uint32_t samples
) {
	const FAudioWaveFormatExtensible *wfx = (FAudioWaveFormatExtensible *)voice->src.format;
	size_t samples_pos, samples_size, copy_size = 0;
	struct FAudioWMADEC *impl = voice->src.wmadec;
	HRESULT hr;

	LOG_FUNC_ENTER(voice->audio)

	if (!impl->output_pos)
	{
		if (wfx->Format.wFormatTag == FAUDIO_FORMAT_EXTENSIBLE)
		{
			const FAudioBufferWMA *wma = &voice->src.bufferList->bufferWMA;
			const UINT32 *output_sizes = wma->pDecodedPacketCumulativeBytes;

			impl->input_size = wfx->Format.nBlockAlign;
			impl->output_size = max(
				impl->output_size,
				output_sizes[wma->PacketCount - 1]
			);
		}
		else
		{
			const FAudioXMA2WaveFormat *xwf = (const FAudioXMA2WaveFormat *)wfx;

			impl->input_size = xwf->dwBytesPerBlock;
			impl->output_size = max(
				impl->output_size,
				(size_t) xwf->dwSamplesEncoded *
				voice->src.format->nChannels *
				(voice->src.format->wBitsPerSample / 8)
			);
		}

		impl->output_buf = voice->audio->pRealloc(
			impl->output_buf,
			impl->output_size
		);
		FAudio_assert(impl->output_buf && "Failed to allocate output buffer!");

		LOG_INFO(voice->audio, "sending BOS to %p", impl->decoder);
		hr = IMFTransform_ProcessMessage(
			impl->decoder,
			MFT_MESSAGE_NOTIFY_START_OF_STREAM,
			0
		);
		FAudio_assert(!FAILED(hr) && "Failed to notify decoder stream start!");
		FAudio_WMAMF_ProcessInput(voice, buffer);
	}

	samples_pos = voice->src.curBufferOffset * voice->src.format->nChannels * sizeof(float);
	samples_size = samples * voice->src.format->nChannels * sizeof(float);

	while (impl->output_pos < samples_pos + samples_size)
	{
		hr = FAudio_WMAMF_ProcessOutput(voice, buffer);
		if (FAILED(hr)) goto error;
		if (hr == S_OK) continue;

		hr  = FAudio_WMAMF_ProcessInput(voice, buffer);
		if (FAILED(hr)) goto error;
		if (hr == S_OK) continue;

		if (!impl->input_size) break;

		LOG_INFO(voice->audio, "sending EOS to %p", impl->decoder);
		hr = IMFTransform_ProcessMessage(
			impl->decoder,
			MFT_MESSAGE_NOTIFY_END_OF_STREAM,
			0
		);
		FAudio_assert(!FAILED(hr) && "Failed to send EOS!");
		impl->input_size = 0;
	}

	if (impl->output_pos > samples_pos)
	{
		copy_size = FAudio_min(impl->output_pos - samples_pos, samples_size);
		FAudio_memcpy(decodeCache, impl->output_buf + samples_pos, copy_size);
	}
	FAudio_zero((char *)decodeCache + copy_size, samples_size - copy_size);
	LOG_INFO(
		voice->audio,
		"decoded %Ix / %Ix bytes, copied %Ix / %Ix bytes",
		impl->output_pos,
		impl->output_size,
		copy_size,
		samples_size
	);

	LOG_FUNC_EXIT(voice->audio)
	return;

error:
	FAudio_zero(decodeCache, samples * voice->src.format->nChannels * sizeof(float));
	LOG_FUNC_EXIT(voice->audio)
}

uint32_t FAudio_WMADEC_init(FAudioSourceVoice *voice, uint32_t type)
{
	static const uint8_t fake_codec_data[16] = {0, 0, 0, 0, 31, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	uint8_t fake_codec_data_wma3[18] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 224, 0, 0, 0};
	const FAudioWaveFormatExtensible *wfx = (FAudioWaveFormatExtensible *)voice->src.format;
	struct FAudioWMADEC *impl;
	MFT_OUTPUT_STREAM_INFO info = {0};
	IMFMediaBuffer *media_buffer;
	IMFMediaType *media_type;
	IMFTransform *decoder;
	HRESULT hr;
	UINT32 i, value;
	GUID guid;

	LOG_FUNC_ENTER(voice->audio)

	if (!(impl = voice->audio->pMalloc(sizeof(*impl)))) return -1;
	FAudio_memset(impl, 0, sizeof(*impl));

	hr = CoCreateInstance(
		&CLSID_CWMADecMediaObject,
		0,
		CLSCTX_INPROC_SERVER,
		&IID_IMFTransform,
		(void **)&decoder
	);
	if (FAILED(hr))
	{
		voice->audio->pFree(impl->output_buf);
		return -2;
	}

	hr = MFCreateMediaType(&media_type);
	FAudio_assert(!FAILED(hr) && "Failed create media type!");
	hr = IMFMediaType_SetGUID(
		media_type,
		&MF_MT_MAJOR_TYPE,
		&MFMediaType_Audio
	);
	FAudio_assert(!FAILED(hr) && "Failed set media major type!");

	switch (type)
	{
	case FAUDIO_FORMAT_WMAUDIO2:
		hr = IMFMediaType_SetBlob(
			media_type,
			&MF_MT_USER_DATA,
			(void *)fake_codec_data,
			sizeof(fake_codec_data)
		);
		FAudio_assert(!FAILED(hr) && "Failed set codec private data!");
		hr = IMFMediaType_SetGUID(
			media_type,
			&MF_MT_SUBTYPE,
			&MFAudioFormat_WMAudioV8
		);
		FAudio_assert(!FAILED(hr) && "Failed set media sub type!");
		hr = IMFMediaType_SetUINT32(
			media_type,
			&MF_MT_AUDIO_BLOCK_ALIGNMENT,
			wfx->Format.nBlockAlign
		);
		FAudio_assert(!FAILED(hr) && "Failed set input block align!");
		break;
	case FAUDIO_FORMAT_WMAUDIO3:
                *(uint16_t *)fake_codec_data_wma3  = voice->src.format->wBitsPerSample;
                for (i = 0; i < voice->src.format->nChannels; i++)
                {
                    fake_codec_data_wma3[2] <<= 1;
                    fake_codec_data_wma3[2] |= 1;
                }
		hr = IMFMediaType_SetBlob(
			media_type,
			&MF_MT_USER_DATA,
			(void *)fake_codec_data_wma3,
			sizeof(fake_codec_data_wma3)
		);
		FAudio_assert(!FAILED(hr) && "Failed set codec private data!");
		hr = IMFMediaType_SetGUID(
			media_type,
			&MF_MT_SUBTYPE,
			&MFAudioFormat_WMAudioV9
		);
		FAudio_assert(!FAILED(hr) && "Failed set media sub type!");
		hr = IMFMediaType_SetUINT32(
			media_type,
			&MF_MT_AUDIO_BLOCK_ALIGNMENT,
			wfx->Format.nBlockAlign
		);
		FAudio_assert(!FAILED(hr) && "Failed set input block align!");
		break;
	case FAUDIO_FORMAT_WMAUDIO_LOSSLESS:
		hr = IMFMediaType_SetBlob(
			media_type,
			&MF_MT_USER_DATA,
			(void *)&wfx->Samples,
			wfx->Format.cbSize
		);
		FAudio_assert(!FAILED(hr) && "Failed set codec private data!");
		hr = IMFMediaType_SetGUID(
			media_type,
			&MF_MT_SUBTYPE,
			&MFAudioFormat_WMAudio_Lossless
		);
		FAudio_assert(!FAILED(hr) && "Failed set media sub type!");
		hr = IMFMediaType_SetUINT32(
			media_type,
			&MF_MT_AUDIO_BLOCK_ALIGNMENT,
			wfx->Format.nBlockAlign
		);
		FAudio_assert(!FAILED(hr) && "Failed set input block align!");
		break;
	case FAUDIO_FORMAT_XMAUDIO2:
	{
		const FAudioXMA2WaveFormat *xwf = (const FAudioXMA2WaveFormat *)wfx;
		hr = IMFMediaType_SetBlob(
			media_type,
			&MF_MT_USER_DATA,
			(void *)&wfx->Samples,
			wfx->Format.cbSize
		);
		FAudio_assert(!FAILED(hr) && "Failed set codec private data!");
		hr = IMFMediaType_SetGUID(
			media_type,
			&MF_MT_SUBTYPE,
			&MFAudioFormat_XMAudio2
		);
		FAudio_assert(!FAILED(hr) && "Failed set media sub type!");
		hr = IMFMediaType_SetUINT32(
			media_type,
			&MF_MT_AUDIO_BLOCK_ALIGNMENT,
			xwf->dwBytesPerBlock
		);
		FAudio_assert(!FAILED(hr) && "Failed set input block align!");
		break;
	}
	default:
		FAudio_assert(0 && "Unsupported type!");
		break;
	}

	hr = IMFMediaType_SetUINT32(
		media_type,
		&MF_MT_AUDIO_BITS_PER_SAMPLE,
		wfx->Format.wBitsPerSample
	);
	FAudio_assert(!FAILED(hr) && "Failed set input bits per sample!");
	hr = IMFMediaType_SetUINT32(
		media_type,
		&MF_MT_AUDIO_AVG_BYTES_PER_SECOND,
		wfx->Format.nAvgBytesPerSec
	);
	FAudio_assert(!FAILED(hr) && "Failed set input bytes per sample!");
	hr = IMFMediaType_SetUINT32(
		media_type,
		&MF_MT_AUDIO_NUM_CHANNELS,
		wfx->Format.nChannels
	);
	FAudio_assert(!FAILED(hr) && "Failed set input channel count!");
	hr = IMFMediaType_SetUINT32(
		media_type,
		&MF_MT_AUDIO_SAMPLES_PER_SECOND,
		wfx->Format.nSamplesPerSec
	);
	FAudio_assert(!FAILED(hr) && "Failed set input sample rate!");

	hr = IMFTransform_SetInputType(
		decoder,
		0,
		media_type,
		0
	);
	FAudio_assert(!FAILED(hr) && "Failed set decoder input type!");
	IMFMediaType_Release(media_type);

	i = 0;
	while (SUCCEEDED(hr))
	{
		hr = IMFTransform_GetOutputAvailableType(
			decoder,
			0,
			i++,
			&media_type
		);
		FAudio_assert(!FAILED(hr) && "Failed get output media type!");

		hr = IMFMediaType_GetGUID(
			media_type,
			&MF_MT_MAJOR_TYPE,
			&guid
		);
		FAudio_assert(!FAILED(hr) && "Failed get media major type!");
		if (!IsEqualGUID(&MFMediaType_Audio, &guid)) goto next;

		hr = IMFMediaType_GetGUID(
			media_type,
			&MF_MT_SUBTYPE,
			&guid
		);
		FAudio_assert(!FAILED(hr) && "Failed get media major type!");
		if (!IsEqualGUID(&MFAudioFormat_Float, &guid)) goto next;

		hr = IMFMediaType_GetUINT32(
			media_type,
			&MF_MT_AUDIO_BITS_PER_SAMPLE,
			&value
		);
		if (FAILED(hr))
		{
			value = 32;
			hr = IMFMediaType_SetUINT32(
				media_type,
				&MF_MT_AUDIO_BITS_PER_SAMPLE,
				value
			);
		}
		FAudio_assert(!FAILED(hr) && "Failed get bits per sample!");
		if (value != 32) goto next;

		hr = IMFMediaType_GetUINT32(
			media_type,
			&MF_MT_AUDIO_NUM_CHANNELS,
			&value
		);
		if (FAILED(hr))
		{
			value = wfx->Format.nChannels;
			hr = IMFMediaType_SetUINT32(
				media_type,
				&MF_MT_AUDIO_NUM_CHANNELS,
				value
			);
		}
		FAudio_assert(!FAILED(hr) && "Failed get channel count!");
		if (value != wfx->Format.nChannels) goto next;

		hr = IMFMediaType_GetUINT32(
			media_type,
			&MF_MT_AUDIO_SAMPLES_PER_SECOND,
			&value
		);
		if (FAILED(hr))
		{
			value = wfx->Format.nSamplesPerSec;
			hr = IMFMediaType_SetUINT32(
				media_type,
				&MF_MT_AUDIO_SAMPLES_PER_SECOND,
				value
			);
		}
		FAudio_assert(!FAILED(hr) && "Failed get sample rate!");
		if (value != wfx->Format.nSamplesPerSec) goto next;

		hr = IMFMediaType_GetUINT32(
			media_type,
			&MF_MT_AUDIO_BLOCK_ALIGNMENT,
			&value
		);
		if (FAILED(hr))
		{
			value = wfx->Format.nChannels * sizeof(float);
			hr = IMFMediaType_SetUINT32(
				media_type,
				&MF_MT_AUDIO_BLOCK_ALIGNMENT,
				value
			);
		}
		FAudio_assert(!FAILED(hr) && "Failed get block align!");
		if (value == wfx->Format.nChannels * sizeof(float)) break;

next:
		IMFMediaType_Release(media_type);
	}
	FAudio_assert(!FAILED(hr) && "Failed to find output media type!");
	hr = IMFTransform_SetOutputType(decoder, 0, media_type, 0);
	FAudio_assert(!FAILED(hr) && "Failed set decoder output type!");
	IMFMediaType_Release(media_type);

	hr = IMFTransform_GetOutputStreamInfo(decoder, 0, &info);
	FAudio_assert(!FAILED(hr) && "Failed to get output stream info!");

	impl->decoder = decoder;
	if (!(info.dwFlags & MFT_OUTPUT_STREAM_CAN_PROVIDE_SAMPLES))
	{
		hr = MFCreateSample(&impl->output_sample);
		FAudio_assert(!FAILED(hr) && "Failed to create sample!");
		hr = MFCreateMemoryBuffer(info.cbSize, &media_buffer);
		FAudio_assert(!FAILED(hr) && "Failed to create buffer!");
		hr = IMFSample_AddBuffer(impl->output_sample, media_buffer);
		FAudio_assert(!FAILED(hr) && "Failed to buffer to sample!");
		IMFMediaBuffer_Release(media_buffer);
	}

	hr = IMFTransform_ProcessMessage(
		decoder,
		MFT_MESSAGE_NOTIFY_BEGIN_STREAMING,
		0
	);
	FAudio_assert(!FAILED(hr) && "Failed to start decoder stream!");

	voice->src.wmadec = impl;
	voice->src.decode = FAudio_INTERNAL_DecodeWMAMF;

	LOG_FUNC_EXIT(voice->audio);
	return 0;
}

void FAudio_WMADEC_free(FAudioSourceVoice *voice)
{
	struct FAudioWMADEC *impl = voice->src.wmadec;
	HRESULT hr;

	LOG_FUNC_ENTER(voice->audio)
	FAudio_PlatformLockMutex(voice->audio->sourceLock);
	LOG_MUTEX_LOCK(voice->audio, voice->audio->sourceLock)

	if (impl->input_size)
	{
		LOG_INFO(voice->audio, "sending EOS to %p", impl->decoder);
		hr = IMFTransform_ProcessMessage(
			impl->decoder,
			MFT_MESSAGE_NOTIFY_END_OF_STREAM,
			0
		);
		FAudio_assert(!FAILED(hr) && "Failed to send EOS!");
		impl->input_size = 0;
	}
	if (impl->output_pos)
	{
		LOG_INFO(voice->audio, "sending DRAIN to %p", impl->decoder);
		hr = IMFTransform_ProcessMessage(
			impl->decoder,
			MFT_MESSAGE_COMMAND_DRAIN,
			0
		);
		FAudio_assert(!FAILED(hr) && "Failed to send DRAIN!");
		impl->output_pos = 0;
	}

	if (impl->output_sample) IMFSample_Release(impl->output_sample);
	IMFTransform_Release(impl->decoder);
	voice->audio->pFree(impl->output_buf);
	voice->audio->pFree(voice->src.wmadec);
	voice->src.wmadec = NULL;
	voice->src.decode = NULL;

	FAudio_PlatformUnlockMutex(voice->audio->sourceLock);
	LOG_MUTEX_UNLOCK(voice->audio, voice->audio->sourceLock)
	LOG_FUNC_EXIT(voice->audio)
}

void FAudio_WMADEC_end_buffer(FAudioSourceVoice *voice)
{
	struct FAudioWMADEC *impl = voice->src.wmadec;
	HRESULT hr;

	LOG_FUNC_ENTER(voice->audio)

	if (impl->input_size)
	{
		LOG_INFO(voice->audio, "sending EOS to %p", impl->decoder);
		hr = IMFTransform_ProcessMessage(
			impl->decoder,
			MFT_MESSAGE_NOTIFY_END_OF_STREAM,
			0
		);
		FAudio_assert(!FAILED(hr) && "Failed to send EOS!");
		impl->input_size = 0;
	}
	impl->output_pos = 0;
	impl->input_pos = 0;

	LOG_FUNC_EXIT(voice->audio)
}

#endif /* HAVE_WMADEC */
