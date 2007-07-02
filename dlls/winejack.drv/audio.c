/*
 * Wine Driver for jack Sound Server
 *   http://jackit.sourceforge.net
 *
 * Copyright 1994 Martin Ayotte
 * Copyright 1999 Eric Pouech (async playing in waveOut/waveIn)
 * Copyright 2000 Eric Pouech (loops in waveOut)
 * Copyright 2002 Chris Morgan (jack version of this file)
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

/*
 * TODO:
 *  implement audio stream resampling for any arbitrary frequenty
 *    right now we use the winmm layer to do resampling although it would 
 *    be nice to have a full set of algorithms to choose from based on cpu 
 *    time
 *
 * FIXME:
 *  pause in waveOut during loop is not handled correctly
 */

#include "config.h"

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <fcntl.h>
#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "wingdi.h"
#include "winerror.h"
#include "mmddk.h"
#include "dsound.h"
#include "dsdriver.h"
#include "jack.h"
#include "wine/unicode.h"
#include "wine/library.h"
#include "wine/debug.h"

#ifdef HAVE_JACK_JACK_H
#include <jack/jack.h>
#endif


WINE_DEFAULT_DEBUG_CHANNEL(wave);

#ifdef SONAME_LIBJACK

#define MAKE_FUNCPTR(f) static typeof(f) * fp_##f = NULL;

/* Function pointers for dynamic loading of libjack */
/* these are prefixed with "fp_", ie. "fp_jack_client_new" */
MAKE_FUNCPTR(jack_activate);
MAKE_FUNCPTR(jack_connect);
MAKE_FUNCPTR(jack_client_new);
MAKE_FUNCPTR(jack_client_close);
MAKE_FUNCPTR(jack_deactivate);
MAKE_FUNCPTR(jack_set_process_callback);
MAKE_FUNCPTR(jack_set_buffer_size_callback);
MAKE_FUNCPTR(jack_set_sample_rate_callback);
MAKE_FUNCPTR(jack_on_shutdown);
MAKE_FUNCPTR(jack_get_sample_rate);
MAKE_FUNCPTR(jack_port_register);
MAKE_FUNCPTR(jack_port_get_buffer);
MAKE_FUNCPTR(jack_get_ports);
MAKE_FUNCPTR(jack_port_name);
MAKE_FUNCPTR(jack_get_buffer_size);
#undef MAKE_FUNCPTR

/* define the below to work around a bug in jack where closing a port */
/* takes a very long time, so to get around this we actually don't */
/* close the port when the device is closed but instead mark the */
/* corresponding device as unused */
#define JACK_CLOSE_HACK    1

typedef jack_default_audio_sample_t sample_t;
typedef jack_nframes_t nframes_t;

/* only allow 10 output devices through this driver, this ought to be adequate */
#define MAX_WAVEOUTDRV  (10)
#define MAX_WAVEINDRV   (10)

/* state diagram for waveOut writing:
 *
 * +---------+-------------+---------------+---------------------------------+
 * |  state  |  function   |     event     |            new state	     |
 * +---------+-------------+---------------+---------------------------------+
 * |	     | open()	   |		   | STOPPED		       	     |
 * | PAUSED  | write()	   | 		   | PAUSED		       	     |
 * | STOPPED | write()	   | <thrd create> | PLAYING		  	     |
 * | PLAYING | write()	   | HEADER        | PLAYING		  	     |
 * | (other) | write()	   | <error>       |		       		     |
 * | (any)   | pause()	   | PAUSING	   | PAUSED		       	     |
 * | PAUSED  | restart()   | RESTARTING    | PLAYING (if no thrd => STOPPED) |
 * | (any)   | reset()	   | RESETTING     | STOPPED		      	     |
 * | (any)   | close()	   | CLOSING	   | CLOSED		      	     |
 * +---------+-------------+---------------+---------------------------------+
 */

/* states of the playing device */
#define	WINE_WS_PLAYING   0
#define	WINE_WS_PAUSED    1
#define	WINE_WS_STOPPED   2
#define WINE_WS_CLOSED    3

typedef struct {
    volatile int      state;      /* one of the WINE_WS_ manifest constants */
    WAVEOPENDESC      waveDesc;
    WORD              wFlags;
    PCMWAVEFORMAT     format;
    WAVEOUTCAPSW      caps;
    WORD              wDevID;
    char              interface_name[32];

    jack_port_t*      out_port_l;   /* ports for left and right channels */
    jack_port_t*      out_port_r;
    jack_client_t*    client;
    long              sample_rate;        /* jack server sample rate */

#if JACK_CLOSE_HACK
    BOOL              in_use; /* TRUE if this device is in use */
#endif

    char*             sound_buffer;
    unsigned long     buffer_size;

    DWORD             volume_left;
    DWORD             volume_right;

    LPWAVEHDR         lpQueuePtr;   /* start of queued WAVEHDRs (waiting to be notified) */
    LPWAVEHDR         lpPlayPtr;    /* start of not yet fully played buffers */
    DWORD             dwPartialOffset;  /* Offset of not yet written bytes in lpPlayPtr */

    LPWAVEHDR         lpLoopPtr;    /* pointer of first buffer in loop, if any */
    DWORD             dwLoops;      /* private copy of loop counter */
    
    DWORD             dwPlayedTotal;    /* number of bytes actually played since opening */
    DWORD             dwWrittenTotal;   /* number of bytes written to jack since opening */

    DWORD	      bytesInJack; /* bytes that we wrote during the previous JACK_Callback() */
    DWORD	      tickCountMS; /* time in MS of last JACK_Callback() */

    /* synchronization stuff */
    CRITICAL_SECTION    access_crst;
} WINE_WAVEOUT;

typedef struct {
    volatile int    state;
    WAVEOPENDESC    waveDesc;
    WORD            wFlags;
    PCMWAVEFORMAT   format;
    LPWAVEHDR       lpQueuePtr;
    DWORD           dwTotalRecorded;
    WAVEINCAPSW     caps;
    BOOL            bTriggerSupport;
    WORD              wDevID;
    char              interface_name[32];

    jack_port_t*      in_port_l;   /* ports for left and right channels */
    jack_port_t*      in_port_r;
    jack_client_t*    client;
    long              sample_rate;        /* jack server sample rate */

#if JACK_CLOSE_HACK
    BOOL              in_use; /* TRUE if this device is in use */
#endif

    char*             sound_buffer;
    unsigned long     buffer_size;

    /* synchronization stuff */
    CRITICAL_SECTION    access_crst;
} WINE_WAVEIN;

static WINE_WAVEOUT WOutDev   [MAX_WAVEOUTDRV];
static WINE_WAVEIN  WInDev    [MAX_WAVEINDRV ];

static DWORD wodDsCreate(UINT wDevID, PIDSDRIVER* drv);
static DWORD wodDsDesc(UINT wDevID, PDSDRIVERDESC desc);

static LPWAVEHDR wodHelper_PlayPtrNext(WINE_WAVEOUT* wwo);
static DWORD wodHelper_NotifyCompletions(WINE_WAVEOUT* wwo, BOOL force);

static int JACK_OpenWaveOutDevice(WINE_WAVEOUT* wwo);
static int JACK_OpenWaveInDevice(WINE_WAVEIN* wwi, WORD nChannels);

#if JACK_CLOSE_HACK
static void	JACK_CloseWaveOutDevice(WINE_WAVEOUT* wwo, BOOL close_client);
#else
static void	JACK_CloseWaveOutDevice(WINE_WAVEOUT* wwo);
#endif

#if JACK_CLOSE_HACK
static void	JACK_CloseWaveInDevice(WINE_WAVEIN* wwi, BOOL close_client);
#else
static void	JACK_CloseWaveInDevice(WINE_WAVEIN* wwi);
#endif

static DWORD bytes_to_mmtime(LPMMTIME lpTime, DWORD position,
                             PCMWAVEFORMAT* format)
{
    TRACE("wType=%04X wBitsPerSample=%u nSamplesPerSec=%u nChannels=%u nAvgBytesPerSec=%u\n",
          lpTime->wType, format->wBitsPerSample, format->wf.nSamplesPerSec,
          format->wf.nChannels, format->wf.nAvgBytesPerSec);
    TRACE("Position in bytes=%u\n", position);

    switch (lpTime->wType) {
    case TIME_SAMPLES:
        lpTime->u.sample = position / (format->wBitsPerSample / 8 * format->wf.nChannels);
        TRACE("TIME_SAMPLES=%u\n", lpTime->u.sample);
        break;
    case TIME_MS:
        lpTime->u.ms = 1000.0 * position / (format->wBitsPerSample / 8 * format->wf.nChannels * format->wf.nSamplesPerSec);
        TRACE("TIME_MS=%u\n", lpTime->u.ms);
        break;
    case TIME_SMPTE:
        lpTime->u.smpte.fps = 30;
        position = position / (format->wBitsPerSample / 8 * format->wf.nChannels);
        position += (format->wf.nSamplesPerSec / lpTime->u.smpte.fps) - 1; /* round up */
        lpTime->u.smpte.sec = position / format->wf.nSamplesPerSec;
        position -= lpTime->u.smpte.sec * format->wf.nSamplesPerSec;
        lpTime->u.smpte.min = lpTime->u.smpte.sec / 60;
        lpTime->u.smpte.sec -= 60 * lpTime->u.smpte.min;
        lpTime->u.smpte.hour = lpTime->u.smpte.min / 60;
        lpTime->u.smpte.min -= 60 * lpTime->u.smpte.hour;
        lpTime->u.smpte.fps = 30;
        lpTime->u.smpte.frame = position * lpTime->u.smpte.fps / format->wf.nSamplesPerSec;
        TRACE("TIME_SMPTE=%02u:%02u:%02u:%02u\n",
              lpTime->u.smpte.hour, lpTime->u.smpte.min,
              lpTime->u.smpte.sec, lpTime->u.smpte.frame);
        break;
    default:
        WARN("Format %d not supported, using TIME_BYTES !\n", lpTime->wType);
        lpTime->wType = TIME_BYTES;
        /* fall through */
    case TIME_BYTES:
        lpTime->u.cb = position;
        TRACE("TIME_BYTES=%u\n", lpTime->u.cb);
        break;
    }
    return MMSYSERR_NOERROR;
}


/*======================================================================*
 *                  Low level WAVE implementation			*
 *======================================================================*/

#define SAMPLE_MAX_16BIT  32767.0f

/* Alsaplayer function that applies volume changes to a buffer */
/* (C) Andy Lo A Foe */
/* Length is in terms of 32 bit samples */
void volume_effect32(void *buffer, int length, int left, int right)
{
        short *data = (short *)buffer;
        int i, v;
    
        if (right == -1) right = left;
 
        for(i = 0; i < length; i++) {
                v = (int) ((*(data) * left) / 100);
                *(data++) = (v>32767) ? 32767 : ((v<-32768) ? -32768 : v);
                v = (int) ((*(data) * right) / 100);
                *(data++) = (v>32767) ? 32767 : ((v<-32768) ? -32768 : v);
        }
}

/* move 16 bit mono/stereo to 16 bit stereo */
void sample_move_d16_d16(short *dst, short *src,
                  unsigned long nsamples, int nChannels)
{
  while(nsamples--)
  {
    *dst = *src;
    dst++;

    if(nChannels == 2) src++;

    *dst = *src;
    dst++;

    src++;
  }
}

/* convert from 16 bit to floating point */
/* allow for copying of stereo data with alternating left/right */
/* channels to a buffer that will hold a single channel stream */
/* nsamples is in terms of 16bit samples */
/* src_skip is in terms of 16bit samples */
void sample_move_d16_s16 (sample_t *dst, short *src,
                        unsigned long nsamples, unsigned long src_skip)
{
  /* ALERT: signed sign-extension portability !!! */
  while (nsamples--)
  {
    *dst = (*src) / SAMPLE_MAX_16BIT;
    dst++;
    src += src_skip;
  }
}       

/* convert from floating point to 16 bit */
/* allow for copying of a buffer that will hold a single channel stream */
/* to stereo data with alternating left/right channels */
/* nsamples is in terms of float samples */
/* dst_skip is in terms of 16bit samples */
void sample_move_s16_d16 (short *dst, sample_t *src,
                        unsigned long nsamples, unsigned long dst_skip)
{
  /* ALERT: signed sign-extension portability !!! */
  while (nsamples--)
  {
      *dst = (*src) * SAMPLE_MAX_16BIT;
/*      TRACE("src=(%.8f,%p) dst=(%d,%p)\n",*src,src,*dst,dst); */
      dst += dst_skip;
      src++;
  }
}       


/* fill dst buffer with nsamples worth of silence */
void sample_silence_dS (sample_t *dst, unsigned long nsamples)
{
  /* ALERT: signed sign-extension portability !!! */
  while (nsamples--)
  {
    *dst = 0;
    dst++;
  }
}       

/******************************************************************
 *    JACK_callback_wwo
 */
/* everytime the jack server wants something from us it calls this 
function, so we either deliver it some sound to play or deliver it nothing 
to play */
int JACK_callback_wwo (nframes_t nframes, void *arg)
{
  sample_t* out_l;
  sample_t* out_r;
  WINE_WAVEOUT* wwo = (WINE_WAVEOUT*)arg;

  TRACE("wDevID: %u, nframes %u state=%u\n", wwo->wDevID, nframes,wwo->state);

  if(!wwo->client)
    ERR("client is closed, this is weird...\n");
    
  out_l = (sample_t *) fp_jack_port_get_buffer(wwo->out_port_l, nframes);
  out_r = (sample_t *) fp_jack_port_get_buffer(wwo->out_port_r, nframes);

  if(wwo->state == WINE_WS_PLAYING)
  {
    DWORD jackFramesAvailable = nframes;
    DWORD outputFramesAvailable;
    DWORD numFramesToWrite;

    long written = 0;
    char* buffer;

#if JACK_CLOSE_HACK
    if(wwo->in_use == FALSE)
    {
      /* output silence if nothing is being outputted */
      sample_silence_dS(out_l, nframes);
      sample_silence_dS(out_r, nframes);

      return 0;
    }
#endif

    TRACE("wwo.state == WINE_WS_PLAYING\n");

    /* see if our sound_buffer is large enough to hold the number of frames jack requested */
    /* Note: sound_buffer is always filled with 16-bit stereo data, even for mono mode */
    if(wwo->buffer_size < (nframes * sizeof(short) * 2))
    {
      ERR("for some reason JACK_BufSize() didn't allocate enough memory\n");
      ERR("allocated %ld bytes, need %d bytes\n", wwo->buffer_size, (nframes * sizeof(short) * 2));
      return 0;
    }

    /* while we have jackFramesAvailable and a wave header to be played */
    while(jackFramesAvailable && wwo->lpPlayPtr)
    {
      /* find the amount of audio to be played at this time */
      outputFramesAvailable = (wwo->lpPlayPtr->dwBufferLength - wwo->dwPartialOffset) / wwo->format.wf.nBlockAlign;

      numFramesToWrite = min(jackFramesAvailable, outputFramesAvailable);
      TRACE("dwBufferLength=(%d) dwPartialOffset=(%d)\n",wwo->lpPlayPtr->dwBufferLength,wwo->dwPartialOffset);
      TRACE("outputFramesAvailable == %d, jackFramesAvailable == %d\n", outputFramesAvailable, jackFramesAvailable);

      buffer = wwo->lpPlayPtr->lpData + wwo->dwPartialOffset;

      /* convert from mono to stereo if necessary */
      /* otherwise just memcpy to the output buffer */

      if(wwo->format.wf.nChannels == 1)
      {
	sample_move_d16_d16((short*)wwo->sound_buffer + ((nframes - jackFramesAvailable) * sizeof(short)),
			    (short*)buffer, numFramesToWrite, wwo->format.wf.nChannels);
      } else /* just copy the memory over */
      {
        memcpy(wwo->sound_buffer + ((nframes - jackFramesAvailable) * wwo->format.wf.nBlockAlign),
	       buffer, numFramesToWrite * wwo->format.wf.nBlockAlign);
      }

      /* advance to the next wave header if possible, or advance pointer */
      /* inside of the current header if we haven't completed it */
      if(numFramesToWrite == outputFramesAvailable)
      {
        wodHelper_PlayPtrNext(wwo);            /* we wrote the whole waveheader, skip to the next one*/
      }
      else
      {
        wwo->dwPartialOffset+=(numFramesToWrite * wwo->format.wf.nBlockAlign); /* else advance by the bytes we took in to write */
      }

      written+=(numFramesToWrite * wwo->format.wf.nBlockAlign); /* add on what we wrote */
      jackFramesAvailable-=numFramesToWrite; /* take away what was written in terms of output bytes */
    }

    wwo->tickCountMS = GetTickCount();    /* record the current time */
    wwo->dwWrittenTotal+=written; /* update states on wave device */
    wwo->dwPlayedTotal+=wwo->bytesInJack; /* we must have finished with the last bytes or we wouldn't be back inside of this callback again... */
    wwo->bytesInJack = written; /* record the bytes inside of jack */

    /* Now that we have finished filling the buffer either until it is full or until */
    /* we have run out of application sound data to process, apply volume and output */
    /* the audio to the jack server */

    /* apply volume to the buffer */
    volume_effect32(wwo->sound_buffer, (nframes - jackFramesAvailable), wwo->volume_left, wwo->volume_right);

    /* convert from stereo 16 bit to single channel 32 bit float */
    /* for each jack server channel */
    /* NOTE: we skip over two sample since we want to only get either the left or right channel */
    sample_move_d16_s16(out_l, (short*)wwo->sound_buffer,     (nframes - jackFramesAvailable), 2);
    sample_move_d16_s16(out_r, (short*)wwo->sound_buffer + 1, (nframes - jackFramesAvailable), 2);

    /* see if we still have jackBytesLeft here, if we do that means that we
    ran out of wave data to play and had a buffer underrun, fill in
    the rest of the space with zero bytes */
    if(jackFramesAvailable)
    {
      ERR("buffer underrun of %d frames\n", jackFramesAvailable);
      sample_silence_dS(out_l + (nframes - jackFramesAvailable), jackFramesAvailable);
      sample_silence_dS(out_r + (nframes - jackFramesAvailable), jackFramesAvailable);
    }
  }
  else if(wwo->state == WINE_WS_PAUSED || 
    wwo->state == WINE_WS_STOPPED ||
    wwo->state == WINE_WS_CLOSED)
  {
      /* output silence if nothing is being outputted */
      sample_silence_dS(out_l, nframes);
      sample_silence_dS(out_r, nframes);
  }

  /* notify the client of completed wave headers */
  EnterCriticalSection(&wwo->access_crst);
  wodHelper_NotifyCompletions(wwo, FALSE);
  LeaveCriticalSection(&wwo->access_crst);

  return 0;
}

/******************************************************************
 *		JACK_bufsize_wwo
 *
 *		Called whenever the jack server changes the the max number 
 *		of frames passed to JACK_callback
 */
int JACK_bufsize_wwo (nframes_t nframes, void *arg)
{
  WINE_WAVEOUT* wwo = (WINE_WAVEOUT*)arg;
  DWORD buffer_required;
  TRACE("wDevID=%d\n",wwo->wDevID);
  TRACE("the maximum buffer size is now %u frames\n", nframes);

  /* make sure the callback routine has adequate memory */
    /* see if our buffer is large enough for the data we are writing */
    /* ie. Buffer_size < (bytes we already wrote + bytes we are going to write in this loop) */
  EnterCriticalSection(&wwo->access_crst);

  /* wwo->sound_buffer is always filled with 16-bit stereo data, even for mono streams */
  buffer_required = nframes * sizeof(short) * 2;
  TRACE("wwo->buffer_size (%ld) buffer_required (%d).\n", wwo->buffer_size,buffer_required);
  if(wwo->buffer_size < buffer_required)
  {
    TRACE("expanding buffer from wwo->buffer_size == %ld, to %d\n",
      wwo->buffer_size, buffer_required);
    TRACE("GetProcessHeap() == %p\n", GetProcessHeap());
    wwo->buffer_size = buffer_required;

    if (wwo->sound_buffer)
      wwo->sound_buffer = HeapReAlloc(GetProcessHeap(), 0, wwo->sound_buffer, wwo->buffer_size);
    else
      wwo->sound_buffer = HeapAlloc(GetProcessHeap(), 0, wwo->buffer_size);

    /* if we don't have a buffer then error out */
    if(!wwo->sound_buffer)
    {
        ERR("error allocating sound_buffer memory\n");
        LeaveCriticalSection(&wwo->access_crst);
        return 0;
    }
  }

  LeaveCriticalSection(&wwo->access_crst);

  TRACE("ending\n");

  return 0;
}
/******************************************************************
 *		JACK_bufsize_wwi
 *
 *		Called whenever the jack server changes the the max number 
 *		of frames passed to JACK_callback
 */
int JACK_bufsize_wwi (nframes_t nframes, void *arg)
{
  TRACE("the maximum buffer size is now %u frames\n", nframes);
  return 0;
}

/******************************************************************
 *		JACK_srate
 */
int JACK_srate (nframes_t nframes, void *arg)
{
  TRACE("the sample rate is now %u/sec\n", nframes);
  return 0;
}


/******************************************************************
 *		JACK_shutdown_wwo
 */
/* if this is called then jack shut down... handle this appropriately */
void JACK_shutdown_wwo(void* arg)
{
  WINE_WAVEOUT* wwo = (WINE_WAVEOUT*)arg;

  wwo->client = 0; /* reset client */

  TRACE("trying to reconnect after sleeping for a short while...\n");
  
  /* lets see if we can't reestablish the connection */
  Sleep(750); /* pause for a short period of time */
  if(!JACK_OpenWaveOutDevice(wwo))
  {
    ERR("unable to reconnect with jack...\n");
  }
}

/******************************************************************
 *		JACK_shutdown_wwi
 */
/* if this is called then jack shut down... handle this appropriately */
void JACK_shutdown_wwi(void* arg)
{
  WINE_WAVEIN* wwi = (WINE_WAVEIN*)arg;

  wwi->client = 0; /* reset client */

  TRACE("trying to reconnect after sleeping for a short while...\n");
  
  /* lets see if we can't reestablish the connection */
  Sleep(750); /* pause for a short period of time */
  if(!JACK_OpenWaveInDevice(wwi,wwi->format.wf.nChannels))
  {
    ERR("unable to reconnect with jack...\n");
  }
}


/******************************************************************
 *		JACK_OpenWaveOutDevice
 */
static int JACK_OpenWaveOutDevice(WINE_WAVEOUT* wwo)
{
  const char** ports;
  int i;
  char client_name[64];
  jack_port_t* out_port_l;
  jack_port_t* out_port_r;
  jack_client_t* client;
  int failed = 0;

  TRACE("creating jack client and setting up callbacks\n");

#if JACK_CLOSE_HACK
  /* see if this device is already open */
        if(wwo->client)
        {
          /* if this device is already in use then it is bad for us to be in here */
          if(wwo->in_use)
            return 0;

          TRACE("using existing client\n");
          wwo->in_use = TRUE;
          return 1;
        }
#endif

        /* zero out the buffer pointer and the size of the buffer */
        wwo->sound_buffer = 0;
        wwo->buffer_size = 0;

        /* try to become a client of the JACK server */
        snprintf(client_name, sizeof(client_name), "wine_jack_out_%d", wwo->wDevID);
	TRACE("client name '%s'\n", client_name);
        if ((client = fp_jack_client_new (client_name)) == 0)
        {
                /* jack has problems with shutting down clients, so lets */
                /* wait a short while and try once more before we give up */
                Sleep(250);
                if ((client = fp_jack_client_new (client_name)) == 0)
                {
                  ERR("jack server not running?\n");
                  return 0;
                }
        }
	                
        /* tell the JACK server to call `JACK_callback_wwo()' whenever
           there is work to be done. */
        fp_jack_set_process_callback (client, JACK_callback_wwo, wwo);
        
        /* tell the JACK server to call `JACK_bufsize_wwo()' whenever   
           the maximum number of frames that will be passed
           to `JACK_Callback()' changes */
        fp_jack_set_buffer_size_callback (client, JACK_bufsize_wwo, wwo);

        /* tell the JACK server to call `srate()' whenever
           the sample rate of the system changes. */
        fp_jack_set_sample_rate_callback (client, JACK_srate, wwo);

        /* tell the JACK server to call `jack_shutdown()' if
           it ever shuts down, either entirely, or if it
           just decides to stop calling us. */
        fp_jack_on_shutdown (client, JACK_shutdown_wwo, wwo);
        
        /* display the current sample rate. once the client is activated
           (see below), you should rely on your own sample rate
           callback (see above) for this value. */
        wwo->sample_rate = fp_jack_get_sample_rate(client);
        TRACE("engine sample rate: %lu\n", wwo->sample_rate);
          
        /* create the left and right channel output ports */
        /* jack's ports are all mono so for stereo you need two */
        out_port_l = fp_jack_port_register (client, "out_l",
                         JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);

        out_port_r = fp_jack_port_register (client, "out_r",
                         JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);

	TRACE("Created ports. (%p) (%p)\n",out_port_l, out_port_r);

        /* save away important values to the WINE_WAVEOUT struct */
        wwo->client = client;
        wwo->out_port_l = out_port_l;
        wwo->out_port_r = out_port_r;

#if JACK_CLOSE_HACK
        wwo->in_use = TRUE; /* mark this device as in use since it now is ;-) */
#endif

	/* set initial buffer size */
	JACK_bufsize_wwo (fp_jack_get_buffer_size(client),wwo);

        /* tell the JACK server that we are ready to roll */
        if (fp_jack_activate (client))
        {
          ERR( "cannot activate client\n");
          return 0;
        }

	TRACE("jack activate.\n");
        /* figure out what the ports that we want to output on are */
        /* NOTE: we do this instead of using stuff like "alsa_pcm:playback_X" because */
        /*   this way works if names are changed */
        ports = fp_jack_get_ports(client, NULL, NULL, JackPortIsPhysical|JackPortIsInput);

        /* display a trace of the output ports we found */
        for(i = 0; ports[i]; i++)
        {
          TRACE("ports[%d] = '%s'\n", i, ports[i]);
        }

        if(!ports)
        {
          ERR("jack_get_ports() failed to find 'JackPortIsPhysical|JackPortIsInput'\n");
        }

        /* connect the ports. Note: you can't do this before
           the client is activated (this may change in the future).
        */ 
        /* we want to connect to two ports so we have stereo output ;-) */

        if(fp_jack_connect(client, fp_jack_port_name(out_port_l), ports[0]))
        {
          ERR ("cannot connect to output port %d('%s')\n", 0, ports[0]);
          failed = 1;
        }

        if(fp_jack_connect(client, fp_jack_port_name(out_port_r), ports[1]))
        {
          ERR ("cannot connect to output port %d('%s')\n", 1, ports[1]);
          failed = 1;
        }

        free(ports); /* free the returned array of ports */

        /* if something failed we need to shut the client down and return 0 */
        if(failed)
        {
#if JACK_CLOSE_HACK
          JACK_CloseWaveOutDevice(wwo, TRUE);
#else
          JACK_CloseWaveOutDevice(wwo);
#endif
          return 0;
        }

        return 1; /* return success */
}

/******************************************************************
 *		JACK_CloseWaveOutDevice
 *
 *	Close the connection to the server cleanly.
 *  If close_client is TRUE we close the client for this device instead of
 *    just marking the device as in_use(JACK_CLOSE_HACK only)
 */
#if JACK_CLOSE_HACK
static void	JACK_CloseWaveOutDevice(WINE_WAVEOUT* wwo, BOOL close_client)
#else
static void	JACK_CloseWaveOutDevice(WINE_WAVEOUT* wwo)
#endif
{
#if JACK_CLOSE_HACK
  TRACE("wDevID: %d, close_client (wwo): %d\n", wwo->wDevID, close_client);
#else
  TRACE("wDevID: %d\n", wwo->wDevID);
#endif

#if JACK_CLOSE_HACK
  if(close_client)
  {
#endif
    fp_jack_deactivate(wwo->client); /* supposed to help the jack_client_close() to succeed */
    fp_jack_client_close (wwo->client);

    EnterCriticalSection(&wwo->access_crst);
    wwo->client = 0; /* reset client */
    HeapFree(GetProcessHeap(), 0, wwo->sound_buffer); /* free buffer memory */
    wwo->sound_buffer = 0;
    wwo->buffer_size = 0; /* zero out size of the buffer */
    LeaveCriticalSection(&wwo->access_crst);
#if JACK_CLOSE_HACK
  } else
  {
    EnterCriticalSection(&wwo->access_crst);
    TRACE("setting in_use to FALSE\n");
    wwo->in_use = FALSE;
    LeaveCriticalSection(&wwo->access_crst);
  }
#endif
}

/******************************************************************
 *		JACK_CloseWaveInDevice
 *
 *	Close the connection to the server cleanly.
 *  If close_client is TRUE we close the client for this device instead of
 *    just marking the device as in_use(JACK_CLOSE_HACK only)
 */
#if JACK_CLOSE_HACK
static void	JACK_CloseWaveInDevice(WINE_WAVEIN* wwi, BOOL close_client)
#else
static void	JACK_CloseWaveInDevice(WINE_WAVEIN* wwi)
#endif
{
#if JACK_CLOSE_HACK
  TRACE("wDevID: %d, close_client (wwi): %d\n", wwi->wDevID, close_client);
#else
  TRACE("wDevID: %d\n", wwi->wDevID);
#endif

#if JACK_CLOSE_HACK
  if(close_client)
  {
#endif
    fp_jack_deactivate(wwi->client); /* supposed to help the jack_client_close() to succeed */
    fp_jack_client_close (wwi->client);

    EnterCriticalSection(&wwi->access_crst);
    wwi->client = 0; /* reset client */
    HeapFree(GetProcessHeap(), 0, wwi->sound_buffer); /* free buffer memory */
    wwi->sound_buffer = 0;
    wwi->buffer_size = 0; /* zero out size of the buffer */
    LeaveCriticalSection(&wwi->access_crst);
#if JACK_CLOSE_HACK
  } else
  {
    EnterCriticalSection(&wwi->access_crst);
    TRACE("setting in_use to FALSE\n");
    wwi->in_use = FALSE;
    LeaveCriticalSection(&wwi->access_crst);
  }
#endif
}

/******************************************************************
 *		JACK_WaveRelease
 *
 *
 */
LONG	JACK_WaveRelease(void)
{ 
  int iDevice;

  TRACE("closing all open waveout devices\n");

  /* close all open output devices */
  for(iDevice = 0; iDevice < MAX_WAVEOUTDRV; iDevice++)
  {
    TRACE("iDevice == %d\n", iDevice);
    if(WOutDev[iDevice].client)
    {
#if JACK_CLOSE_HACK
      JACK_CloseWaveOutDevice(&WOutDev[iDevice], TRUE); /* close the device, FORCE the client to close */
#else
      JACK_CloseWaveOutDevice(&WOutDev[iDevice]); /* close the device, FORCE the client to close */
#endif
      DeleteCriticalSection(&(WOutDev[iDevice].access_crst)); /* delete the critical section */
    }
  }

  TRACE("closing all open wavein devices\n");

  /* close all open input devices */
  for(iDevice = 0; iDevice < MAX_WAVEINDRV; iDevice++)
  {
    TRACE("iDevice == %d\n", iDevice);
    if(WInDev[iDevice].client)
    {
#if JACK_CLOSE_HACK
      JACK_CloseWaveInDevice(&WInDev[iDevice], TRUE); /* close the device, FORCE the client to close */
#else
      JACK_CloseWaveInDevice(&WInDev[iDevice]); /* close the device, FORCE the client to close */
#endif
      DeleteCriticalSection(&(WInDev[iDevice].access_crst)); /* delete the critical section */
    }
  }

  TRACE("returning 1\n");

  return 1;
}

/******************************************************************
 *		JACK_WaveInit
 *
 * Initialize internal structures from JACK server info
 */
LONG JACK_WaveInit(void)
{
    int i;
    CHAR szPname[MAXPNAMELEN];

    TRACE("called\n");

    /* setup function pointers */
#define LOAD_FUNCPTR(f) if((fp_##f = wine_dlsym(jackhandle, #f, NULL, 0)) == NULL) goto sym_not_found;    
    LOAD_FUNCPTR(jack_activate);
    LOAD_FUNCPTR(jack_connect);
    LOAD_FUNCPTR(jack_client_new);
    LOAD_FUNCPTR(jack_client_close);
    LOAD_FUNCPTR(jack_deactivate);
    LOAD_FUNCPTR(jack_set_process_callback);
    LOAD_FUNCPTR(jack_set_buffer_size_callback);
    LOAD_FUNCPTR(jack_set_sample_rate_callback);
    LOAD_FUNCPTR(jack_on_shutdown);
    LOAD_FUNCPTR(jack_get_sample_rate);
    LOAD_FUNCPTR(jack_port_register);
    LOAD_FUNCPTR(jack_port_get_buffer);
    LOAD_FUNCPTR(jack_get_ports);
    LOAD_FUNCPTR(jack_port_name);
    LOAD_FUNCPTR(jack_get_buffer_size);
#undef LOAD_FUNCPTR

    /* start with output device */

    for (i = 0; i < MAX_WAVEOUTDRV; ++i)
    {
      WOutDev[i].client = 0; /* initialize the client to 0 */

#if JACK_CLOSE_HACK
      WOutDev[i].in_use = FALSE;
      WInDev[i].in_use  = FALSE;
#endif

      memset(&WOutDev[i].caps, 0, sizeof(WOutDev[i].caps));

      WOutDev[i].caps.wMid = 0x00FF; 	/* Manufac ID */
      WOutDev[i].caps.wPid = 0x0001; 	/* Product ID */
      snprintf(szPname, sizeof(szPname), "JACK WaveOut %d", i);
      MultiByteToWideChar(CP_ACP, 0, szPname, -1, WOutDev[i].caps.szPname, sizeof(WOutDev[i].caps.szPname)/sizeof(WCHAR));
      snprintf(WOutDev[i].interface_name, sizeof(WOutDev[i].interface_name), "winejack: %d", i);

      WOutDev[i].caps.vDriverVersion = 0x0100;
      WOutDev[i].caps.dwFormats = 0x00000000;
      WOutDev[i].caps.dwSupport = WAVECAPS_VOLUME;
    
      WOutDev[i].caps.wChannels = 2;
      WOutDev[i].caps.dwSupport |= WAVECAPS_LRVOLUME;

/* NOTE: we don't support any 8 bit modes so note that */
/*      WOutDev[i].caps.dwFormats |= WAVE_FORMAT_4M08;
      WOutDev[i].caps.dwFormats |= WAVE_FORMAT_4S08; */
      WOutDev[i].caps.dwFormats |= WAVE_FORMAT_4S16;
      WOutDev[i].caps.dwFormats |= WAVE_FORMAT_4M16;
/*      WOutDev[i].caps.dwFormats |= WAVE_FORMAT_2M08;
      WOutDev[i].caps.dwFormats |= WAVE_FORMAT_2S08; */
      WOutDev[i].caps.dwFormats |= WAVE_FORMAT_2M16;
      WOutDev[i].caps.dwFormats |= WAVE_FORMAT_2S16;
/*      WOutDev[i].caps.dwFormats |= WAVE_FORMAT_1M08;
      WOutDev[i].caps.dwFormats |= WAVE_FORMAT_1S08;*/
      WOutDev[i].caps.dwFormats |= WAVE_FORMAT_1M16;
      WOutDev[i].caps.dwFormats |= WAVE_FORMAT_1S16;
    }

    /* then do input device */
    for (i = 0; i < MAX_WAVEINDRV; ++i)
    {
        /* TODO: we should initialize read stuff here */
        memset(&WInDev[i].caps, 0, sizeof(WInDev[i].caps));

	WInDev[i].caps.wMid = 0x00FF;
	WInDev[i].caps.wPid = 0x0001;
        snprintf(szPname, sizeof(szPname), "JACK WaveIn %d", i);
        MultiByteToWideChar(CP_ACP, 0, szPname, -1, WInDev[i].caps.szPname, sizeof(WInDev[i].caps.szPname)/sizeof(WCHAR));
        snprintf(WInDev[i].interface_name, sizeof(WInDev[i].interface_name), "winejack: %d", i);

	WInDev[i].caps.vDriverVersion = 0x0100;

	WInDev[i].caps.wChannels = 0x2;
	/* NOTE: we don't support any 8 bit modes so note that */
/*      WInDev[i].caps.dwFormats |= WAVE_FORMAT_4M08;
	WInDev[i].caps.dwFormats |= WAVE_FORMAT_4S08; */
	WInDev[i].caps.dwFormats |= WAVE_FORMAT_4S16;
	WInDev[i].caps.dwFormats |= WAVE_FORMAT_4M16;
/*      WInDev[i].caps.dwFormats |= WAVE_FORMAT_2M08;
	WInDev[i].caps.dwFormats |= WAVE_FORMAT_2S08; */
	WInDev[i].caps.dwFormats |= WAVE_FORMAT_2M16;
	WInDev[i].caps.dwFormats |= WAVE_FORMAT_2S16;
/*      WInDev[i].caps.dwFormats |= WAVE_FORMAT_1M08;
	WInDev[i].caps.dwFormats |= WAVE_FORMAT_1S08;*/
	WInDev[i].caps.dwFormats |= WAVE_FORMAT_1M16;
	WInDev[i].caps.dwFormats |= WAVE_FORMAT_1S16;
	WInDev[i].caps.wReserved1 = 0;
    }
    
    return 1;		/* return success */

/* error path for function pointer loading errors */
sym_not_found:
    WINE_MESSAGE(
      "Wine cannot find certain functions that it needs inside the jack"
      "library.  To enable Wine to use the jack audio server please "
      "install libjack\n");
    wine_dlclose(jackhandle, NULL, 0);
    jackhandle = NULL;
    return FALSE;
}

/*======================================================================*
 *                  Low level WAVE OUT implementation			*
 *======================================================================*/

/**************************************************************************
 * 			wodNotifyClient			[internal]
 */
static DWORD wodNotifyClient(WINE_WAVEOUT* wwo, WORD wMsg, DWORD dwParam1, DWORD dwParam2)
{
    TRACE("wMsg = 0x%04x dwParm1 = %04X dwParam2 = %04X\n", wMsg, dwParam1, dwParam2);
    
    switch (wMsg) {
    case WOM_OPEN:
    case WOM_CLOSE:
    case WOM_DONE:
      if (wwo->wFlags != DCB_NULL &&
        !DriverCallback(wwo->waveDesc.dwCallback, wwo->wFlags,
          (HDRVR)wwo->waveDesc.hWave, wMsg, wwo->waveDesc.dwInstance,
          dwParam1, dwParam2))
      {
        WARN("can't notify client !\n");
        return MMSYSERR_ERROR;
      }
    break;
    default:
      FIXME("Unknown callback message %u\n", wMsg);
        return MMSYSERR_INVALPARAM;
    }
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				wodHelper_BeginWaveHdr          [internal]
 *
 * Makes the specified lpWaveHdr the currently playing wave header.
 * If the specified wave header is a begin loop and we're not already in
 * a loop, setup the loop.
 */
static void wodHelper_BeginWaveHdr(WINE_WAVEOUT* wwo, LPWAVEHDR lpWaveHdr)
{
    EnterCriticalSection(&wwo->access_crst);

    wwo->lpPlayPtr = lpWaveHdr;

    if (!lpWaveHdr)
    {
       LeaveCriticalSection(&wwo->access_crst);
       return;
    }

    if (lpWaveHdr->dwFlags & WHDR_BEGINLOOP)
    {
      if (wwo->lpLoopPtr)
      {
        WARN("Already in a loop. Discarding loop on this header (%p)\n", lpWaveHdr);
        TRACE("Already in a loop. Discarding loop on this header (%p)\n", lpWaveHdr);
      } else
      {
        TRACE("Starting loop (%dx) with %p\n", lpWaveHdr->dwLoops, lpWaveHdr);
        wwo->lpLoopPtr = lpWaveHdr;
        /* Windows does not touch WAVEHDR.dwLoops,
         * so we need to make an internal copy */
        wwo->dwLoops = lpWaveHdr->dwLoops;
      }
    }
    wwo->dwPartialOffset = 0;

    LeaveCriticalSection(&wwo->access_crst);
}


/**************************************************************************
 * 				wodHelper_PlayPtrNext	        [internal]
 *
 * Advance the play pointer to the next waveheader, looping if required.
 */
static LPWAVEHDR wodHelper_PlayPtrNext(WINE_WAVEOUT* wwo)
{
  LPWAVEHDR lpWaveHdr;

  EnterCriticalSection(&wwo->access_crst);

  lpWaveHdr = wwo->lpPlayPtr;

  wwo->dwPartialOffset = 0;
  if ((lpWaveHdr->dwFlags & WHDR_ENDLOOP) && wwo->lpLoopPtr)
  {
    /* We're at the end of a loop, loop if required */
    if (--wwo->dwLoops > 0)
    {
      wwo->lpPlayPtr = wwo->lpLoopPtr;
    } else
    {
      /* Handle overlapping loops correctly */
      if (wwo->lpLoopPtr != lpWaveHdr && (lpWaveHdr->dwFlags & WHDR_BEGINLOOP)) {
        FIXME("Correctly handled case ? (ending loop buffer also starts a new loop)\n");
        /* shall we consider the END flag for the closing loop or for
         * the opening one or for both ???
         * code assumes for closing loop only
         */
      } else
      {
        lpWaveHdr = lpWaveHdr->lpNext;
      }
      wwo->lpLoopPtr = NULL;
      wodHelper_BeginWaveHdr(wwo, lpWaveHdr);
    }
  } else
  {
     /* We're not in a loop.  Advance to the next wave header */
    TRACE("not inside of a loop, advancing to next wave header\n");
    wodHelper_BeginWaveHdr(wwo, lpWaveHdr = lpWaveHdr->lpNext);
  }

  LeaveCriticalSection(&wwo->access_crst);

  return lpWaveHdr;
}

/* if force is TRUE then notify the client that all the headers were completed */
static DWORD wodHelper_NotifyCompletions(WINE_WAVEOUT* wwo, BOOL force)
{
  LPWAVEHDR		lpWaveHdr;
  DWORD retval;

  TRACE("called\n");

  EnterCriticalSection(&wwo->access_crst);

  /* Start from lpQueuePtr and keep notifying until:
   * - we hit an unwritten wavehdr
   * - we hit the beginning of a running loop
   * - we hit a wavehdr which hasn't finished playing
   */
  while ((lpWaveHdr = wwo->lpQueuePtr) &&
           (force || 
            (lpWaveHdr != wwo->lpPlayPtr &&
             lpWaveHdr != wwo->lpLoopPtr)))
  {
    wwo->lpQueuePtr = lpWaveHdr->lpNext;

    lpWaveHdr->dwFlags &= ~WHDR_INQUEUE;
    lpWaveHdr->dwFlags |= WHDR_DONE;
    TRACE("notifying client: lpWaveHdr=(%p) lpPlayPtr=(%p) dwFlags=(%d)\n",
	  lpWaveHdr, wwo->lpPlayPtr, lpWaveHdr->dwFlags);

    wodNotifyClient(wwo, WOM_DONE, (DWORD)lpWaveHdr, 0);
  }
  TRACE("Not notifying client: lpWaveHdr=(%p) lpPlayPtr=(%p) lpLoopPtr=(%p)\n",
	lpWaveHdr, wwo->lpPlayPtr, wwo->lpLoopPtr);
  retval = (lpWaveHdr && lpWaveHdr != wwo->lpPlayPtr && lpWaveHdr != 
              wwo->lpLoopPtr) ? 0 : INFINITE;

  LeaveCriticalSection(&wwo->access_crst);

  return retval;
}

/**************************************************************************
 * 				wodHelper_Reset			[internal]
 *
 * Resets current output stream.
 */
static  void  wodHelper_Reset(WINE_WAVEOUT* wwo, BOOL reset)
{
    EnterCriticalSection(&wwo->access_crst);
 
    /* updates current notify list */
    wodHelper_NotifyCompletions(wwo, FALSE);

    if (reset)
    {
        /* remove all wave headers and notify client that all headers were completed */
        wodHelper_NotifyCompletions(wwo, TRUE);

        wwo->lpPlayPtr = wwo->lpQueuePtr = wwo->lpLoopPtr = NULL;
        wwo->state = WINE_WS_STOPPED;
        wwo->dwPlayedTotal = wwo->dwWrittenTotal = wwo->bytesInJack = 0;

        wwo->dwPartialOffset = 0;        /* Clear partial wavehdr */
    } else
    {
        if (wwo->lpLoopPtr)
        {
            /* complicated case, not handled yet (could imply modifying the loop counter) */
            FIXME("Pausing while in loop isn't correctly handled yet, except strange results\n");
            wwo->lpPlayPtr = wwo->lpLoopPtr;
            wwo->dwPartialOffset = 0;
            wwo->dwWrittenTotal = wwo->dwPlayedTotal; /* this is wrong !!! */
        } else
        {
            LPWAVEHDR   ptr;
            DWORD       sz = wwo->dwPartialOffset;

            /* reset all the data as if we had written only up to lpPlayedTotal bytes */
            /* compute the max size playable from lpQueuePtr */
            for (ptr = wwo->lpQueuePtr; ptr != wwo->lpPlayPtr; ptr = ptr->lpNext)
            {
                sz += ptr->dwBufferLength;
            }

            /* because the reset lpPlayPtr will be lpQueuePtr */
            if (wwo->dwWrittenTotal > wwo->dwPlayedTotal + sz) ERR("doh\n");
            wwo->dwPartialOffset = sz - (wwo->dwWrittenTotal - wwo->dwPlayedTotal);
            wwo->dwWrittenTotal = wwo->dwPlayedTotal;
            wwo->lpPlayPtr = wwo->lpQueuePtr;
        }

        wwo->state = WINE_WS_PAUSED;
    }
 
    LeaveCriticalSection(&wwo->access_crst);
}

/**************************************************************************
 * 			wodGetDevCaps				[internal]
 */
static DWORD wodGetDevCaps(WORD wDevID, LPWAVEOUTCAPSW lpCaps, DWORD dwSize)
{
    TRACE("(%u, %p, %u);\n", wDevID, lpCaps, dwSize);
    
    if (lpCaps == NULL) return MMSYSERR_NOTENABLED;
    
    if (wDevID >= MAX_WAVEOUTDRV)
    {
      TRACE("MAX_WAVOUTDRV reached !\n");
      return MMSYSERR_BADDEVICEID;
    }

    TRACE("dwSupport=(0x%x), dwFormats=(0x%x)\n", WOutDev[wDevID].caps.dwSupport, WOutDev[wDevID].caps.dwFormats);
    memcpy(lpCaps, &WOutDev[wDevID].caps, min(dwSize, sizeof(*lpCaps)));
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				wodOpen				[internal]
 *
 * NOTE: doesn't it seem like there is a race condition if you try to open 
 * the same device twice?
 */
static DWORD wodOpen(WORD wDevID, LPWAVEOPENDESC lpDesc, DWORD dwFlags)
{
    WINE_WAVEOUT*	wwo;
    DWORD retval;

    TRACE("(%u, %p, %08X);\n", wDevID, lpDesc, dwFlags);
    if (lpDesc == NULL)
    {
      WARN("Invalid Parameter !\n");
      return MMSYSERR_INVALPARAM;
    }
    if (wDevID >= MAX_WAVEOUTDRV) {
      TRACE("MAX_WAVOUTDRV reached !\n");
      return MMSYSERR_BADDEVICEID;
    }

#if JACK_CLOSE_HACK
    if(WOutDev[wDevID].client && WOutDev[wDevID].in_use)
#else
    if(WOutDev[wDevID].client)
#endif
    {
      TRACE("device %d already allocated\n", wDevID);
      return MMSYSERR_ALLOCATED;
    }

    /* Only the PCM format is supported so far...
     * Also we only support 16 bit mode.
     */
    if (lpDesc->lpFormat->wFormatTag != WAVE_FORMAT_PCM ||
      lpDesc->lpFormat->nChannels == 0 ||
      lpDesc->lpFormat->nSamplesPerSec == 0 ||
      lpDesc->lpFormat->wBitsPerSample != 16)
    {
      WARN("Bad format: tag=%04X nChannels=%d nSamplesPerSec=%d wBitsPerSample=%d !\n",
       lpDesc->lpFormat->wFormatTag, lpDesc->lpFormat->nChannels,
       lpDesc->lpFormat->nSamplesPerSec, lpDesc->lpFormat->wBitsPerSample);
      return WAVERR_BADFORMAT;
    }

    if (dwFlags & WAVE_FORMAT_QUERY)
    {
      TRACE("Query format: tag=%04X nChannels=%d nSamplesPerSec=%d !\n",
       lpDesc->lpFormat->wFormatTag, lpDesc->lpFormat->nChannels,
       lpDesc->lpFormat->nSamplesPerSec);
      return MMSYSERR_NOERROR;
    }

    wwo = &WOutDev[wDevID];
    wwo->wDevID = wDevID;

    /* Set things up before we call JACK_OpenWaveOutDevice because */
    /* we will start getting callbacks before JACK_OpenWaveOutDevice */
    /* even returns and we want to be initialized before then */
    wwo->state = WINE_WS_STOPPED; /* start in a stopped state */
    wwo->dwPlayedTotal = 0; /* zero out these totals */
    wwo->dwWrittenTotal = 0;
    wwo->bytesInJack = 0;
    wwo->tickCountMS = 0;

    /* Initialize volume to full level */
    wwo->volume_left = 100;
    wwo->volume_right = 100;

    InitializeCriticalSection(&wwo->access_crst); /* initialize the critical section */
    EnterCriticalSection(&wwo->access_crst);

    dwFlags &= ~WAVE_DIRECTSOUND;  /* direct sound not supported, ignore the flag */

    wwo->wFlags = HIWORD(dwFlags & CALLBACK_TYPEMASK);
    
    memcpy(&wwo->waveDesc, lpDesc, 	     sizeof(WAVEOPENDESC));
    memcpy(&wwo->format,   lpDesc->lpFormat, sizeof(PCMWAVEFORMAT));

    /* open up jack ports for this device */
    if (!JACK_OpenWaveOutDevice(&WOutDev[wDevID]))
    {
      ERR("JACK_OpenWaveOutDevice(%d) failed\n", wDevID);
      LeaveCriticalSection(&wwo->access_crst);
      DeleteCriticalSection(&wwo->access_crst); /* delete the critical section so we can initialize it again from wodOpen() */
      return MMSYSERR_ERROR;		/* return unspecified error */
    }

    LeaveCriticalSection(&wwo->access_crst);

    /* display the current wave format */
    TRACE("wBitsPerSample=%u, nAvgBytesPerSec=%u, nSamplesPerSec=%u, nChannels=%u nBlockAlign=%u!\n",
    wwo->format.wBitsPerSample, wwo->format.wf.nAvgBytesPerSec,
    wwo->format.wf.nSamplesPerSec, wwo->format.wf.nChannels,
    wwo->format.wf.nBlockAlign);

    /* make sure that we have the same sample rate in our audio stream */
    /* as we do in the jack server */
    if(wwo->format.wf.nSamplesPerSec != wwo->sample_rate)
    {
      TRACE("error: jack server sample rate is '%ld', wave sample rate is '%d'\n",
         wwo->sample_rate, wwo->format.wf.nSamplesPerSec);

#if JACK_CLOSE_HACK
      JACK_CloseWaveOutDevice(wwo, FALSE); /* close this device, don't force the client to close */
#else
      JACK_CloseWaveOutDevice(wwo); /* close this device */
#endif
      DeleteCriticalSection(&wwo->access_crst); /* delete the critical section so we can initialize it again from wodOpen() */
      return WAVERR_BADFORMAT;
    }

    /* check for an invalid number of bits per sample */
    if (wwo->format.wBitsPerSample == 0)
    {
      WARN("Resetting zeroed wBitsPerSample\n");
      wwo->format.wBitsPerSample = 8 *
      (wwo->format.wf.nAvgBytesPerSec /
       wwo->format.wf.nSamplesPerSec) /
       wwo->format.wf.nChannels;
    }

    EnterCriticalSection(&wwo->access_crst);
    retval = wodNotifyClient(wwo, WOM_OPEN, 0L, 0L);
    LeaveCriticalSection(&wwo->access_crst);

    return retval;
}

/**************************************************************************
 * 				wodClose			[internal]
 */
static DWORD wodClose(WORD wDevID)
{
    DWORD		ret = MMSYSERR_NOERROR;
    WINE_WAVEOUT*	wwo;

    TRACE("(%u);\n", wDevID);

    if (wDevID >= MAX_WAVEOUTDRV || !WOutDev[wDevID].client)
    {
      WARN("bad device ID !\n");
      return MMSYSERR_BADDEVICEID;
    }
    
    wwo = &WOutDev[wDevID];
    if (wwo->lpQueuePtr)
    {
      WARN("buffers still playing !\n");
      ret = WAVERR_STILLPLAYING;
    } else
    {
      /* sanity check: this should not happen since the device must have been reset before */
      if (wwo->lpQueuePtr || wwo->lpPlayPtr) ERR("out of sync\n");

      wwo->state = WINE_WS_CLOSED; /* mark the device as closed */

#if JACK_CLOSE_HACK
      JACK_CloseWaveOutDevice(wwo, FALSE); /* close the jack device, DO NOT force the client to close */
#else
      JACK_CloseWaveOutDevice(wwo); /* close the jack device */
#endif
      DeleteCriticalSection(&wwo->access_crst); /* delete the critical section so we can initialize it again from wodOpen() */

      ret = wodNotifyClient(wwo, WOM_CLOSE, 0L, 0L);
    }

    return ret;
}


/**************************************************************************
 * 				wodWrite			[internal]
 * 
 */
static DWORD wodWrite(WORD wDevID, LPWAVEHDR lpWaveHdr, DWORD dwSize)
{
    LPWAVEHDR*wh;
    WINE_WAVEOUT *wwo;

    TRACE("(%u, %p, %08X);\n", wDevID, lpWaveHdr, dwSize);
    
    /* first, do the sanity checks... */
    if (wDevID >= MAX_WAVEOUTDRV || !WOutDev[wDevID].client)
    {
      WARN("bad dev ID !\n");
      return MMSYSERR_BADDEVICEID;
    }

    wwo = &WOutDev[wDevID];

    if (lpWaveHdr->lpData == NULL || !(lpWaveHdr->dwFlags & WHDR_PREPARED))
    {
      TRACE("unprepared\n");
      return WAVERR_UNPREPARED;
    }
    
    if (lpWaveHdr->dwFlags & WHDR_INQUEUE) 
    {
      TRACE("still playing\n");
      return WAVERR_STILLPLAYING;
    }

    lpWaveHdr->dwFlags &= ~WHDR_DONE;
    lpWaveHdr->dwFlags |= WHDR_INQUEUE;
    lpWaveHdr->lpNext = 0;

    EnterCriticalSection(&wwo->access_crst);

    /* insert buffer at the end of queue */
    for (wh = &(wwo->lpQueuePtr); *wh; wh = &((*wh)->lpNext));
    *wh = lpWaveHdr;

    if (!wwo->lpPlayPtr)
      wodHelper_BeginWaveHdr(wwo,lpWaveHdr);
    if (wwo->state == WINE_WS_STOPPED)
      wwo->state = WINE_WS_PLAYING;
    LeaveCriticalSection(&wwo->access_crst);

    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 			wodPause				[internal]
 */
static DWORD wodPause(WORD wDevID)
{
  TRACE("(%u);!\n", wDevID);
    
  if (wDevID >= MAX_WAVEOUTDRV || !WOutDev[wDevID].client)
  {
    WARN("bad device ID !\n");
    return MMSYSERR_BADDEVICEID;
  }
    
  TRACE("[3-PAUSING]\n");

  EnterCriticalSection(&(WOutDev[wDevID].access_crst));
  wodHelper_Reset(&WOutDev[wDevID], FALSE);
  LeaveCriticalSection(&(WOutDev[wDevID].access_crst));

  return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 			wodRestart				[internal]
 */
static DWORD wodRestart(WORD wDevID)
{
    TRACE("(%u);\n", wDevID);

    if (wDevID >= MAX_WAVEOUTDRV || !WOutDev[wDevID].client)
    {
      WARN("bad device ID !\n");
      return MMSYSERR_BADDEVICEID;
    }

    if (WOutDev[wDevID].state == WINE_WS_PAUSED)
    {
      EnterCriticalSection(&(WOutDev[wDevID].access_crst));
      WOutDev[wDevID].state = WINE_WS_PLAYING;
      LeaveCriticalSection(&(WOutDev[wDevID].access_crst));
    }
    
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 			wodReset				[internal]
 */
static DWORD wodReset(WORD wDevID)
{
    TRACE("(%u);\n", wDevID);
    
    if (wDevID >= MAX_WAVEOUTDRV || !WOutDev[wDevID].client)
    {
      WARN("bad device ID !\n");
      return MMSYSERR_BADDEVICEID;
    }

    EnterCriticalSection(&(WOutDev[wDevID].access_crst));
    wodHelper_Reset(&WOutDev[wDevID], TRUE);
    LeaveCriticalSection(&(WOutDev[wDevID].access_crst));

    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				wodGetPosition			[internal]
 */
static DWORD wodGetPosition(WORD wDevID, LPMMTIME lpTime, DWORD uSize)
{
    DWORD		val;
    WINE_WAVEOUT*	wwo;
    DWORD elapsedMS;

    TRACE("(%u, %p, %u);\n", wDevID, lpTime, uSize);

    if (wDevID >= MAX_WAVEOUTDRV || !WOutDev[wDevID].client)
    {
      WARN("bad device ID !\n");
      return MMSYSERR_BADDEVICEID;
    }

    /* if null pointer to time structure return error */
    if (lpTime == NULL)	return MMSYSERR_INVALPARAM;

    wwo = &WOutDev[wDevID];

    EnterCriticalSection(&(WOutDev[wDevID].access_crst));
    val = wwo->dwPlayedTotal;
    elapsedMS = GetTickCount() - wwo->tickCountMS;
    LeaveCriticalSection(&(WOutDev[wDevID].access_crst));

    /* account for the bytes played since the last JACK_Callback() */
    val+=((elapsedMS * wwo->format.wf.nAvgBytesPerSec) / 1000);

    return bytes_to_mmtime(lpTime, val, &wwo->format);
}

/**************************************************************************
 * 				wodBreakLoop			[internal]
 */
static DWORD wodBreakLoop(WORD wDevID)
{
  TRACE("(%u);\n", wDevID);

  if (wDevID >= MAX_WAVEOUTDRV || !WOutDev[wDevID].client)
  {
    WARN("bad device ID !\n");
    return MMSYSERR_BADDEVICEID;
  }

  EnterCriticalSection(&(WOutDev[wDevID].access_crst));

  if (WOutDev[wDevID].state == WINE_WS_PLAYING && WOutDev[wDevID].lpLoopPtr != NULL)
  {
    /* ensure exit at end of current loop */
    WOutDev[wDevID].dwLoops = 1;
  }

  LeaveCriticalSection(&(WOutDev[wDevID].access_crst));

  return MMSYSERR_NOERROR;
}
    
/**************************************************************************
 * 				wodGetVolume			[internal]
 */
static DWORD wodGetVolume(WORD wDevID, LPDWORD lpdwVol)
{
  DWORD left, right;
    
  left = WOutDev[wDevID].volume_left;
  right = WOutDev[wDevID].volume_right;
        
  TRACE("(%u, %p);\n", wDevID, lpdwVol);
 
  *lpdwVol = ((left * 0xFFFFl) / 100) + (((right * 0xFFFFl) / 100) <<
              16);
    
  return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				wodSetVolume			[internal]
 */
static DWORD wodSetVolume(WORD wDevID, DWORD dwParam)
{
  DWORD left, right;
 
  left  = (LOWORD(dwParam) * 100) / 0xFFFFl;
  right = (HIWORD(dwParam) * 100) / 0xFFFFl;
 
  TRACE("(%u, %08X);\n", wDevID, dwParam);

  EnterCriticalSection(&(WOutDev[wDevID].access_crst));

  WOutDev[wDevID].volume_left = left;
  WOutDev[wDevID].volume_right = right;

  LeaveCriticalSection(&(WOutDev[wDevID].access_crst));

  return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				wodGetNumDevs			[internal]
 */
static DWORD wodGetNumDevs(void)
{
  return MAX_WAVEOUTDRV;
}

/**************************************************************************
 *                              wodDevInterfaceSize             [internal]
 */
static DWORD wodDevInterfaceSize(UINT wDevID, LPDWORD dwParam1)
{
    TRACE("(%u, %p)\n", wDevID, dwParam1);
                                                                                                       
    *dwParam1 = MultiByteToWideChar(CP_ACP, 0, WOutDev[wDevID].interface_name, -1,
                                    NULL, 0 ) * sizeof(WCHAR);
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 *                              wodDevInterface                 [internal]
 */
static DWORD wodDevInterface(UINT wDevID, PWCHAR dwParam1, DWORD dwParam2)
{
    if (dwParam2 >= MultiByteToWideChar(CP_ACP, 0, WOutDev[wDevID].interface_name, -1,
                                        NULL, 0 ) * sizeof(WCHAR))
    {
        MultiByteToWideChar(CP_ACP, 0, WOutDev[wDevID].interface_name, -1,
                            dwParam1, dwParam2 / sizeof(WCHAR));
        return MMSYSERR_NOERROR;
    }
    return MMSYSERR_INVALPARAM;
}

/**************************************************************************
 * 				wodMessage (WINEJACK.7)
 */
DWORD WINAPI JACK_wodMessage(UINT wDevID, UINT wMsg, DWORD dwUser, 
			    DWORD dwParam1, DWORD dwParam2)
{
  TRACE("(%u, %04X, %08X, %08X, %08X);\n",
  wDevID, wMsg, dwUser, dwParam1, dwParam2);
    
  switch (wMsg) {
  case DRVM_INIT:
  case DRVM_EXIT:
  case DRVM_ENABLE:
  case DRVM_DISABLE:
    /* FIXME: Pretend this is supported */
    return 0;
  case WODM_OPEN:             return wodOpen(wDevID, (LPWAVEOPENDESC)dwParam1,	dwParam2);
  case WODM_CLOSE:            return wodClose(wDevID);
  case WODM_WRITE:            return wodWrite(wDevID, (LPWAVEHDR)dwParam1,		dwParam2);
  case WODM_PAUSE:            return wodPause(wDevID);
  case WODM_GETPOS:           return wodGetPosition(wDevID, (LPMMTIME)dwParam1, 		dwParam2);
  case WODM_BREAKLOOP:        return wodBreakLoop(wDevID);
  case WODM_PREPARE:          return MMSYSERR_NOTSUPPORTED;
  case WODM_UNPREPARE:        return MMSYSERR_NOTSUPPORTED;
  case WODM_GETDEVCAPS:       return wodGetDevCaps(wDevID, (LPWAVEOUTCAPSW)dwParam1,	dwParam2);
  case WODM_GETNUMDEVS:       return wodGetNumDevs();
  case WODM_GETPITCH:         return MMSYSERR_NOTSUPPORTED;
  case WODM_SETPITCH:         return MMSYSERR_NOTSUPPORTED;
  case WODM_GETPLAYBACKRATE:	return MMSYSERR_NOTSUPPORTED;
  case WODM_SETPLAYBACKRATE:	return MMSYSERR_NOTSUPPORTED;
  case WODM_GETVOLUME:        return wodGetVolume(wDevID, (LPDWORD)dwParam1);
  case WODM_SETVOLUME:        return wodSetVolume(wDevID, dwParam1);
  case WODM_RESTART:          return wodRestart(wDevID);
  case WODM_RESET:            return wodReset(wDevID);

  case DRV_QUERYDEVICEINTERFACESIZE: return wodDevInterfaceSize       (wDevID, (LPDWORD)dwParam1);
  case DRV_QUERYDEVICEINTERFACE:     return wodDevInterface           (wDevID, (PWCHAR)dwParam1, dwParam2);
  case DRV_QUERYDSOUNDIFACE:	return wodDsCreate(wDevID, (PIDSDRIVER*)dwParam1);
  case DRV_QUERYDSOUNDDESC:	return wodDsDesc(wDevID, (PDSDRIVERDESC)dwParam1);
  default:
    FIXME("unknown message %d!\n", wMsg);
    }
    return MMSYSERR_NOTSUPPORTED;
}

/*======================================================================*
 *                  Low level DSOUND implementation			*
 *======================================================================*/

typedef struct IDsDriverImpl IDsDriverImpl;
typedef struct IDsDriverBufferImpl IDsDriverBufferImpl;

struct IDsDriverImpl
{
    /* IUnknown fields */
    const IDsDriverVtbl *lpVtbl;
    DWORD		ref;
    /* IDsDriverImpl fields */
    UINT		wDevID;
    IDsDriverBufferImpl*primary;
};

struct IDsDriverBufferImpl
{
    /* IUnknown fields */
    const IDsDriverBufferVtbl *lpVtbl;
    DWORD ref;
    /* IDsDriverBufferImpl fields */
    IDsDriverImpl* drv;
    DWORD buflen;
};

static DWORD wodDsCreate(UINT wDevID, PIDSDRIVER* drv)
{
    /* we can't perform memory mapping as we don't have a file stream 
      interface with jack like we do with oss */
    MESSAGE("This sound card's driver does not support direct access\n");
    MESSAGE("The (slower) DirectSound HEL mode will be used instead.\n");
    return MMSYSERR_NOTSUPPORTED;
}

static DWORD wodDsDesc(UINT wDevID, PDSDRIVERDESC desc)
{
    memset(desc, 0, sizeof(*desc));
    strcpy(desc->szDesc, "Wine jack DirectSound Driver");
    strcpy(desc->szDrvname, "winejack.drv");
    return MMSYSERR_NOERROR;
}

/*======================================================================*
 *                  Low level WAVE IN implementation			*
 *======================================================================*/

/**************************************************************************
 * 			widNotifyClient			[internal]
 */
static DWORD widNotifyClient(WINE_WAVEIN* wwi, WORD wMsg, DWORD dwParam1, DWORD dwParam2)
{
    TRACE("wMsg = 0x%04x dwParm1 = %04X dwParam2 = %04X\n", wMsg, dwParam1, dwParam2);
    
    switch (wMsg) {
    case WIM_OPEN:
    case WIM_CLOSE:
    case WIM_DATA:
      if (wwi->wFlags != DCB_NULL &&
        !DriverCallback(wwi->waveDesc.dwCallback, wwi->wFlags,
          (HDRVR)wwi->waveDesc.hWave, wMsg, wwi->waveDesc.dwInstance,
          dwParam1, dwParam2))
      {
        WARN("can't notify client !\n");
        return MMSYSERR_ERROR;
      }
    break;
    default:
      FIXME("Unknown callback message %u\n", wMsg);
        return MMSYSERR_INVALPARAM;
    }
    return MMSYSERR_NOERROR;
}

/******************************************************************
 *    JACK_callback_wwi
 */
/* everytime the jack server wants something from us it calls this 
   function */
int JACK_callback_wwi (nframes_t nframes, void *arg)
{
    sample_t* in_l;
    sample_t* in_r = 0;
    WINE_WAVEIN* wwi = (WINE_WAVEIN*)arg;

    TRACE("wDevID: %u, nframes %u\n", wwi->wDevID, nframes);
    
    if(!wwi->client)
	ERR("client is closed, this is weird...\n");
    
    in_l = (sample_t *) fp_jack_port_get_buffer(wwi->in_port_l, nframes);

    if (wwi->in_port_r)
      in_r = (sample_t *) fp_jack_port_get_buffer(wwi->in_port_r, nframes);
    
    EnterCriticalSection(&wwi->access_crst);
    
    if((wwi->lpQueuePtr != NULL) && (wwi->state == WINE_WS_PLAYING))
    {
	LPWAVEHDR lpWaveHdr = wwi->lpQueuePtr;
	nframes_t jackFramesLeft = nframes;

#if JACK_CLOSE_HACK
	if(wwi->in_use == FALSE)
	{
            /* do nothing if nothing is being recorded */
            LeaveCriticalSection(&wwi->access_crst);
	    return 0;
	}
#endif
	
	TRACE("wwi.state == WINE_WS_PLAYING\n");
	
	while (lpWaveHdr && jackFramesLeft)
	{
	    DWORD waveHdrFramesLeft = (lpWaveHdr->dwBufferLength - lpWaveHdr->dwBytesRecorded) / (sizeof(short) * wwi->format.wf.nChannels); 
	    DWORD numFrames = min (jackFramesLeft, waveHdrFramesLeft);
	    
            TRACE ("dwBufferLength=(%u) dwBytesRecorded=(%d)\n",   lpWaveHdr->dwBufferLength, lpWaveHdr->dwBytesRecorded);
            TRACE ("jackFramesLeft=(%u) waveHdrFramesLeft=(%u)\n", jackFramesLeft, waveHdrFramesLeft);

	    if (!in_r) {
	      /* mono */
	      sample_move_s16_d16((short *)((char *)lpWaveHdr->lpData + lpWaveHdr->dwBytesRecorded), in_l+(nframes-jackFramesLeft), numFrames, 1);
	    } else {
	      /* stereo */
	      sample_move_s16_d16((short *)((char *)lpWaveHdr->lpData + lpWaveHdr->dwBytesRecorded), 
				  in_l+(nframes-jackFramesLeft), numFrames, 2);
	      sample_move_s16_d16((short *)((char *)lpWaveHdr->lpData + lpWaveHdr->dwBytesRecorded + sizeof(short)), 
				  in_r+(nframes-jackFramesLeft), numFrames, 2);
	    }
	    
	    lpWaveHdr->dwBytesRecorded += (numFrames * sizeof(short) * wwi->format.wf.nChannels );
	    jackFramesLeft -= numFrames;

	    if (lpWaveHdr->dwBytesRecorded >= lpWaveHdr->dwBufferLength)
	    {
		/* must copy the value of next waveHdr, because we have no idea of what
		 * will be done with the content of lpWaveHdr in callback
		 */
		LPWAVEHDR	lpNext = lpWaveHdr->lpNext;
				
		lpWaveHdr->dwFlags &= ~WHDR_INQUEUE;
		lpWaveHdr->dwFlags |=  WHDR_DONE;

		TRACE("WaveHdr full. dwBytesRecorded=(%u) dwFlags=(0x%x)\n",lpWaveHdr->dwBytesRecorded,lpWaveHdr->dwFlags);

		widNotifyClient(wwi, WIM_DATA, (DWORD)lpWaveHdr, 0);

		lpWaveHdr = wwi->lpQueuePtr = lpNext;
	    }
	}
	TRACE ("jackFramesLeft=(%u) lpWaveHdr=(%p)\n", jackFramesLeft, lpWaveHdr);
	if (jackFramesLeft > 0) { WARN("Record buffer ran out of WaveHdrs\n"); }
    }
    
    LeaveCriticalSection(&wwi->access_crst);

    return 0;
}

/******************************************************************
 *		JACK_OpenWaveInDevice
 */
static int JACK_OpenWaveInDevice(WINE_WAVEIN* wwi, WORD nChannels)
{
  const char** ports;
  int i;
  char client_name[64];
  jack_port_t* in_port_l;
  jack_port_t* in_port_r = 0;
  jack_client_t* client;
  int failed = 0;

  TRACE("creating jack client and setting up callbacks\n");

  if ((nChannels == 0) || (nChannels > 2)) {
    ERR ("nChannels = (%d), but we only support mono or stereo.\n", nChannels);
    return 0;
  }

#if JACK_CLOSE_HACK
  /* see if this device is already open */
        if(wwi->client)
        {
          /* if this device is already in use then it is bad for us to be in here */
          if(wwi->in_use)
            return 0;

          TRACE("using existing client\n");
          wwi->in_use = TRUE;
          return 1;
        }
#endif

        /* zero out the buffer pointer and the size of the buffer */
        wwi->sound_buffer = 0;
        wwi->buffer_size = 0;

        /* try to become a client of the JACK server */
        snprintf(client_name, sizeof(client_name), "wine_jack_in_%d", wwi->wDevID);
        TRACE("client name '%s'\n", client_name);
        if ((client = fp_jack_client_new (client_name)) == 0)
        {
                /* jack has problems with shutting down clients, so lets */
                /* wait a short while and try once more before we give up */
                Sleep(250);
                if ((client = fp_jack_client_new (client_name)) == 0)
                {
                  ERR("jack server not running?\n");
                  return 0;
                }
        }
        wwi->client = client;
                
        /* tell the JACK server to call `JACK_wwi_callback()' whenever
           there is work to be done. */
        fp_jack_set_process_callback (client, JACK_callback_wwi, wwi);
        
        /* tell the JACK server to call `JACK_bufsize_wwi()' whenever   
           the maximum number of frames that will be passed
           to `JACK_Callback()' changes */
        fp_jack_set_buffer_size_callback (client, JACK_bufsize_wwi, wwi);
          
        /* tell the JACK server to call `srate()' whenever
           the sample rate of the system changes. */
        fp_jack_set_sample_rate_callback (client, JACK_srate, wwi);

        /* tell the JACK server to call `jack_shutdown()' if
           it ever shuts down, either entirely, or if it
           just decides to stop calling us. */
        fp_jack_on_shutdown (client, JACK_shutdown_wwi, wwi);
        
        /* display the current sample rate. once the client is activated
           (see below), you should rely on your own sample rate
           callback (see above) for this value. */
        wwi->sample_rate = fp_jack_get_sample_rate(client);
        TRACE("engine sample rate: %lu\n", wwi->sample_rate);
          
        /* create the left and right channel output ports */
        /* jack's ports are all mono so for stereo you need two */
        in_port_l = fp_jack_port_register (client, "in_l",
                         JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
        wwi->in_port_l = in_port_l;
	TRACE("Created port. (%p)\n", in_port_l);

	if (nChannels == 2)
	{
	  in_port_r = fp_jack_port_register (client, "in_r",
			   JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0); 
	  TRACE("Created port. (%p)\n", in_port_r);
	} 
	wwi->in_port_r = in_port_r;

#if JACK_CLOSE_HACK
        wwi->in_use = TRUE; /* mark this device as in use since it now is ;-) */
#endif

	TRACE("activating client.\n");
        /* tell the JACK server that we are ready to roll */
        if (fp_jack_activate (client))
        {
          ERR( "cannot activate client\n");
          return 0;
        }
	TRACE("activated client.\n");
        /* figure out what the ports that we want to output on are */
        /* NOTE: we do this instead of using stuff like "alsa_pcm:playback_X" because */
        /*   this way works if names are changed */
        ports = fp_jack_get_ports(client, NULL, NULL, JackPortIsPhysical|JackPortIsOutput);

        /* display a trace of the output ports we found */
        for(i = 0; ports[i]; i++)
        {
          TRACE("ports[%d] = '%s'\n", i, ports[i]);
        }

        if(!ports)
        {
          ERR("jack_get_ports() failed to find 'JackPortIsPhysical|JackPortIsOutput'\n");
        }

        /* connect the ports. Note: you can't do this before
           the client is activated (this may change in the future).
        */ 
        /* we want to connect to two ports so we have stereo input ;-) */

	if(fp_jack_connect(client, ports[0], fp_jack_port_name(in_port_l)))
	{
	  ERR ("cannot connect to input port %d('%s')\n", 0, ports[0]);
	  failed = 1;
	}
	TRACE("Connected (%s)<->(%s)\n",ports[0],fp_jack_port_name(in_port_l));

	if ((nChannels == 2) && in_port_r) {
	  if(fp_jack_connect(client, ports[1], fp_jack_port_name(in_port_r)))
	  {
	    ERR ("cannot connect to input port %d('%s')\n", 1, ports[1]);
	    failed = 1;
	  }
	  TRACE("Connected (%s)<->(%s)\n",ports[1],fp_jack_port_name(in_port_r));
	}
        free(ports); /* free the returned array of ports */

        /* if something failed we need to shut the client down and return 0 */
        if(failed)
        {
#if JACK_CLOSE_HACK
          JACK_CloseWaveInDevice(wwi, TRUE);
#else
          JACK_CloseWaveInDevice(wwi);
#endif
          return 0;
        }

	TRACE("return success.\n");
        return 1; /* return success */
}

/**************************************************************************
 * 			widGetDevCaps				[internal]
 */
static DWORD widGetDevCaps(WORD wDevID, LPWAVEINCAPSW lpCaps, DWORD dwSize)
{
    TRACE("(%u, %p, %u);\n", wDevID, lpCaps, dwSize);

    if (lpCaps == NULL) return MMSYSERR_NOTENABLED;

    if (wDevID >= MAX_WAVEINDRV) {
	TRACE("MAX_WAVEINDRV reached !\n");
	return MMSYSERR_BADDEVICEID;
    }

    memcpy(lpCaps, &WInDev[wDevID].caps, min(dwSize, sizeof(*lpCaps)));
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				widOpen				[internal]
 */
static DWORD widOpen(WORD wDevID, LPWAVEOPENDESC lpDesc, DWORD dwFlags)
{
    WINE_WAVEIN*	wwi;
    DWORD retval;

    TRACE("(%u, %p, %08X);\n", wDevID, lpDesc, dwFlags);
    if (lpDesc == NULL)
    {
	WARN("Invalid Parameter !\n");
	return MMSYSERR_INVALPARAM;
    }
    if (wDevID >= MAX_WAVEINDRV) {
	TRACE ("MAX_WAVEINDRV reached !\n");
	return MMSYSERR_BADDEVICEID;
    }

#if JACK_CLOSE_HACK
    if(WInDev[wDevID].client && WOutDev[wDevID].in_use)
#else
    if(WInDev[wDevID].client)
#endif
    {
      TRACE("device %d already allocated\n", wDevID);
      return MMSYSERR_ALLOCATED;
    }

    /* Only the PCM format is supported so far...
     * Also we only support 16 bit mode.
     */
    if (lpDesc->lpFormat->wFormatTag != WAVE_FORMAT_PCM ||
      lpDesc->lpFormat->nChannels == 0 ||
      lpDesc->lpFormat->nSamplesPerSec == 0 ||
      lpDesc->lpFormat->wBitsPerSample!=16)
    {
      WARN("Bad format: tag=%04X nChannels=%d nSamplesPerSec=%d wBitsPerSample=%d !\n",
       lpDesc->lpFormat->wFormatTag, lpDesc->lpFormat->nChannels,
       lpDesc->lpFormat->nSamplesPerSec, lpDesc->lpFormat->wBitsPerSample);
      return WAVERR_BADFORMAT;
    }

    if (dwFlags & WAVE_FORMAT_QUERY)
    {
      TRACE("Query format: tag=%04X nChannels=%d nSamplesPerSec=%d !\n",
       lpDesc->lpFormat->wFormatTag, lpDesc->lpFormat->nChannels,
       lpDesc->lpFormat->nSamplesPerSec);
      return MMSYSERR_NOERROR;
    }

    wwi = &WInDev[wDevID];
    wwi->wDevID = wDevID;

    /* Set things up before we call JACK_OpenWaveOutDevice because */
    /* we will start getting callbacks before JACK_OpenWaveOutDevice */
    /* even returns and we want to be initialized before then */
    wwi->state = WINE_WS_STOPPED; /* start in a stopped state */

    InitializeCriticalSection(&wwi->access_crst); /* initialize the critical section */
    EnterCriticalSection(&wwi->access_crst);

    /* open up jack ports for this device */
    if (!JACK_OpenWaveInDevice(&WInDev[wDevID], lpDesc->lpFormat->nChannels))
    {
      ERR("JACK_OpenWaveInDevice(%d) failed\n", wDevID);
      LeaveCriticalSection(&wwi->access_crst);
      DeleteCriticalSection(&wwi->access_crst);
      return MMSYSERR_ERROR;		/* return unspecified error */
    }

    dwFlags &= ~WAVE_DIRECTSOUND;  /* direct sound not supported, ignore the flag */

    wwi->wFlags = HIWORD(dwFlags & CALLBACK_TYPEMASK);
    
    memcpy(&wwi->waveDesc, lpDesc, 	     sizeof(WAVEOPENDESC));
    memcpy(&wwi->format,   lpDesc->lpFormat, sizeof(PCMWAVEFORMAT));

    LeaveCriticalSection(&wwi->access_crst);

    /* display the current wave format */
    TRACE("wBitsPerSample=%u, nAvgBytesPerSec=%u, nSamplesPerSec=%u, nChannels=%u nBlockAlign=%u!\n",
    wwi->format.wBitsPerSample, wwi->format.wf.nAvgBytesPerSec,
    wwi->format.wf.nSamplesPerSec, wwi->format.wf.nChannels,
    wwi->format.wf.nBlockAlign);

    /* make sure that we have the same sample rate in our audio stream */
    /* as we do in the jack server */
    if(wwi->format.wf.nSamplesPerSec != wwi->sample_rate)
    {
      TRACE("error: jack server sample rate is '%ld', wave sample rate is '%d'\n",
         wwi->sample_rate, wwi->format.wf.nSamplesPerSec);

#if JACK_CLOSE_HACK
      JACK_CloseWaveInDevice(wwi, FALSE); /* close this device, don't force the client to close */
#else
      JACK_CloseWaveInDevice(wwi); /* close this device */
#endif
      DeleteCriticalSection(&wwi->access_crst);
      return WAVERR_BADFORMAT;
    }

    /* check for an invalid number of bits per sample */
    if (wwi->format.wBitsPerSample == 0)
    {
      WARN("Resetting zeroed wBitsPerSample\n");
      wwi->format.wBitsPerSample = 8 *
      (wwi->format.wf.nAvgBytesPerSec /
       wwi->format.wf.nSamplesPerSec) /
       wwi->format.wf.nChannels;
    }

    TRACE("notify client.\n");
    EnterCriticalSection(&wwi->access_crst);
    retval = widNotifyClient(wwi, WIM_OPEN, 0L, 0L);
    LeaveCriticalSection(&wwi->access_crst);

    return retval;
}
/**************************************************************************
 * 				widClose			[internal]
 */
static DWORD widClose(WORD wDevID)
{
    DWORD		ret = MMSYSERR_NOERROR;
    WINE_WAVEIN*	wwi;

    TRACE("(%u);\n", wDevID);

    if (wDevID >= MAX_WAVEINDRV || !WInDev[wDevID].client)
    {
      WARN("bad device ID !\n");
      return MMSYSERR_BADDEVICEID;
    }
    
    wwi = &WInDev[wDevID];
    if (wwi->lpQueuePtr)
    {
      WARN("buffers still playing !\n");
      ret = WAVERR_STILLPLAYING;
    } else
    {
	/* sanity check: this should not happen since the device must have been reset before */
	if (wwi->lpQueuePtr) ERR("out of sync\n");
	
	wwi->state = WINE_WS_CLOSED; /* mark the device as closed */
	
#if JACK_CLOSE_HACK
	JACK_CloseWaveInDevice(wwi, FALSE); /* close the jack device, DO NOT force the client to close */
#else
	JACK_CloseWaveInDevice(wwi); /* close the jack device */
#endif
	DeleteCriticalSection(&wwi->access_crst); /* delete the critical section so we can initialize it again from wodOpen() */
	
	ret = widNotifyClient(wwi, WIM_CLOSE, 0L, 0L);
    }

    return ret;
}

/**************************************************************************
 * 				widAddBuffer		[internal]
 */
static DWORD widAddBuffer(WORD wDevID, LPWAVEHDR lpWaveHdr, DWORD dwSize)
{
    WINE_WAVEIN* wwi = &WInDev[wDevID];

    TRACE("(%u, %p, %08X);\n", wDevID, lpWaveHdr, dwSize);

    if (wDevID >= MAX_WAVEINDRV || WInDev[wDevID].state == WINE_WS_CLOSED) {
				WARN("can't do it !\n");
				return MMSYSERR_INVALHANDLE;
    }
    if (!(lpWaveHdr->dwFlags & WHDR_PREPARED)) {
				TRACE("never been prepared !\n");
				return WAVERR_UNPREPARED;
    }
    if (lpWaveHdr->dwFlags & WHDR_INQUEUE) {
				TRACE("header already in use !\n");
				return WAVERR_STILLPLAYING;
    }

    lpWaveHdr->dwFlags |= WHDR_INQUEUE;
    lpWaveHdr->dwFlags &= ~WHDR_DONE;
    lpWaveHdr->dwBytesRecorded = 0;
    lpWaveHdr->lpNext = NULL;

    EnterCriticalSection(&wwi->access_crst);
    /* insert buffer at end of queue */
    {
	LPWAVEHDR* wh;
	for (wh = &(wwi->lpQueuePtr); *wh; wh = &((*wh)->lpNext));
	*wh=lpWaveHdr;
    }
    LeaveCriticalSection(&wwi->access_crst);

    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 			widStart				[internal]
 */
static DWORD widStart(WORD wDevID)
{
    TRACE("(%u);\n", wDevID);
    if (wDevID >= MAX_WAVEINDRV || WInDev[wDevID].state == WINE_WS_CLOSED) {
	WARN("can't start recording !\n");
	return MMSYSERR_INVALHANDLE;
    }

    WInDev[wDevID].state = WINE_WS_PLAYING;
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 			widStop					[internal]
 */
static DWORD widStop(WORD wDevID)
{
    WINE_WAVEIN* wwi = &WInDev[wDevID];

    TRACE("(%u);\n", wDevID);
    if (wDevID >= MAX_WAVEINDRV || WInDev[wDevID].state == WINE_WS_CLOSED) {
	WARN("can't stop !\n");
	return MMSYSERR_INVALHANDLE;
    }
    
    if (wwi->state != WINE_WS_STOPPED)
    {
	WAVEHDR*		lpWaveHdr;
	/* do something here to stop recording ??? */
	
	/* return current buffer to app */
	lpWaveHdr = wwi->lpQueuePtr;
	if (lpWaveHdr)
	{
	    LPWAVEHDR	lpNext = lpWaveHdr->lpNext;
	    TRACE("stop %p %p\n", lpWaveHdr, lpWaveHdr->lpNext);
	    lpWaveHdr->dwFlags &= ~WHDR_INQUEUE;
	    lpWaveHdr->dwFlags |= WHDR_DONE;
	    widNotifyClient(wwi, WIM_DATA, (DWORD)lpWaveHdr, 0);
	    wwi->lpQueuePtr = lpNext;
	}
    }
    wwi->state = WINE_WS_STOPPED;

    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 			widReset				[internal]
 */
static DWORD widReset(WORD wDevID)
{
    WINE_WAVEIN* wwi = &WInDev[wDevID];
    WAVEHDR*		lpWaveHdr;

    TRACE("(%u);\n", wDevID);
    if (wDevID >= MAX_WAVEINDRV || WInDev[wDevID].state == WINE_WS_CLOSED) {
	WARN("can't reset !\n");
	return MMSYSERR_INVALHANDLE;
    }

    wwi->state = WINE_WS_STOPPED;

    /* return all buffers to the app */
    for (lpWaveHdr = wwi->lpQueuePtr; lpWaveHdr; lpWaveHdr = lpWaveHdr->lpNext) {
	TRACE("reset %p %p\n", lpWaveHdr, lpWaveHdr->lpNext);
	lpWaveHdr->dwFlags &= ~WHDR_INQUEUE;
	lpWaveHdr->dwFlags |= WHDR_DONE;
	
	widNotifyClient(wwi, WIM_DATA, (DWORD)lpWaveHdr, 0);
    }
    wwi->lpQueuePtr = NULL;

    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				widGetNumDevs			[internal]
 */
static DWORD widGetNumDevs(void)
{
  return MAX_WAVEINDRV;
}

/**************************************************************************
 *                              widDevInterfaceSize             [internal]
 */
static DWORD widDevInterfaceSize(UINT wDevID, LPDWORD dwParam1)
{
    TRACE("(%u, %p)\n", wDevID, dwParam1);
                                                                                                       
                                                                                                       
    *dwParam1 = MultiByteToWideChar(CP_ACP, 0, WInDev[wDevID].interface_name, -1,
                                    NULL, 0 ) * sizeof(WCHAR);
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 *                              widDevInterface                 [internal]
 */
static DWORD widDevInterface(UINT wDevID, PWCHAR dwParam1, DWORD dwParam2)
{
    if (dwParam2 >= MultiByteToWideChar(CP_ACP, 0, WInDev[wDevID].interface_name, -1,
                                        NULL, 0 ) * sizeof(WCHAR))
    {
        MultiByteToWideChar(CP_ACP, 0, WInDev[wDevID].interface_name, -1,
                            dwParam1, dwParam2 / sizeof(WCHAR));
        return MMSYSERR_NOERROR;
    }
    return MMSYSERR_INVALPARAM;
}

/**************************************************************************
 * 				widMessage (WINEJACK.6)
 */
DWORD WINAPI JACK_widMessage(WORD wDevID, WORD wMsg, DWORD dwUser, 
			    DWORD dwParam1, DWORD dwParam2)
{
  TRACE("(%u, %04X, %08X, %08X, %08X);\n",
  wDevID, wMsg, dwUser, dwParam1, dwParam2);

    switch (wMsg) {
    case DRVM_INIT:
    case DRVM_EXIT:
    case DRVM_ENABLE:
    case DRVM_DISABLE:
	/* FIXME: Pretend this is supported */
	return 0;
    case WIDM_OPEN:	 	return widOpen		(wDevID, (LPWAVEOPENDESC)dwParam1,	dwParam2);
    case WIDM_CLOSE:		return widClose		(wDevID);
    case WIDM_ADDBUFFER:	return widAddBuffer	(wDevID, (LPWAVEHDR)dwParam1, dwParam2);
    case WIDM_PREPARE:		return MMSYSERR_NOTSUPPORTED;
    case WIDM_UNPREPARE:	return MMSYSERR_NOTSUPPORTED;
    case WIDM_GETDEVCAPS:	return widGetDevCaps	(wDevID, (LPWAVEINCAPSW)dwParam1,	dwParam2);
    case WIDM_GETNUMDEVS:	return widGetNumDevs();
    case WIDM_RESET:		return widReset		(wDevID);
    case WIDM_START:		return widStart		(wDevID);
    case WIDM_STOP:		return widStop		(wDevID);
    case DRV_QUERYDEVICEINTERFACESIZE: return widDevInterfaceSize       (wDevID, (LPDWORD)dwParam1);
    case DRV_QUERYDEVICEINTERFACE:     return widDevInterface           (wDevID, (PWCHAR)dwParam1, dwParam2);
    default:
	FIXME("unknown message %d!\n", wMsg);
    }

  return MMSYSERR_NOTSUPPORTED;
}

#else /* !SONAME_LIBJACK */

/**************************************************************************
 * 				widMessage (WINEJACK.6)
 */
DWORD WINAPI JACK_widMessage(WORD wDevID, WORD wMsg, DWORD dwUser, 
			    DWORD dwParam1, DWORD dwParam2)
{
  FIXME("(%u, %04X, %08X, %08X, %08X):jack support not compiled into wine\n", wDevID, wMsg, dwUser, dwParam1, dwParam2);
  return MMSYSERR_NOTENABLED;
}

/**************************************************************************
 * 				wodMessage (WINEJACK.7)
 */
DWORD WINAPI JACK_wodMessage(WORD wDevID, WORD wMsg, DWORD dwUser, 
			    DWORD dwParam1, DWORD dwParam2)
{
  FIXME("(%u, %04X, %08X, %08X, %08X):jack support not compiled into wine\n", wDevID, wMsg, dwUser, dwParam1, dwParam2);
  return MMSYSERR_NOTENABLED;
}

#endif /* SONAME_LIBJACK */
