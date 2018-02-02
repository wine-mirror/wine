/*
 * Soundblaster Emulation
 *
 * Copyright 2002 Christian Costa
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

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "dosexe.h"
#include "wine/debug.h"
#include "wingdi.h"
#include "mmsystem.h"
#include "dsound.h"

WINE_DEFAULT_DEBUG_CHANNEL(sblaster);

/* Board Configuration */
/* FIXME: Should be in a config file */
#define SB_IRQ 5
#define SB_IRQ_PRI 11
#define SB_DMA 1

/* Soundblaster state */
static int SampleMode;         /* Mono / Stereo */
static int SampleRate;
static int SamplesCount;
static BYTE DSP_Command[256];  /* Store param numbers in bytes for each command */
static BYTE DSP_InBuffer[10];  /* Store DSP command bytes parameters from host */
static int InSize;             /* Nb of bytes in InBuffer */
static BYTE DSP_OutBuffer[10]; /* Store DSP information bytes to host */
static int OutSize;            /* Nb of bytes in InBuffer */
static int command;            /* Current command */
static BOOL end_sound_loop = FALSE;
static BOOL dma_enable = FALSE;

/* The maximum size of a dma transfer can be 65536 */
#define DMATRFSIZE 1024

/* DMA can perform 8 or 16-bit transfer */
static BYTE dma_buffer[DMATRFSIZE*2];

/* Direct Sound buffer config */
#define DSBUFLEN 4096 /* FIXME: Only this value seems to work */

/* Direct Sound playback stuff */
static LPDIRECTSOUND lpdsound;
static LPDIRECTSOUNDBUFFER lpdsbuf;
static DSBUFFERDESC buf_desc;
static WAVEFORMATEX wav_fmt;
static HANDLE SB_Thread;
static UINT buf_off;
extern HWND vga_hwnd;

/* SB_Poll performs DMA transfers and fills the Direct Sound Buffer */
static DWORD CALLBACK SB_Poll( void *dummy )
{
    HRESULT result;
    LPBYTE lpbuf1 = NULL;
    LPBYTE lpbuf2 = NULL;
    DWORD dwsize1 = 0;
    DWORD dwsize2 = 0;
    DWORD dwbyteswritten1 = 0;
    DWORD dwbyteswritten2 = 0;
    int size;

    /* FIXME: this loop must be improved */
    while(!end_sound_loop)
    {
        Sleep(10);

        if (dma_enable) {
            size = DMA_Transfer(SB_DMA,min(DMATRFSIZE,SamplesCount),dma_buffer);
        } else
            continue;

        result = IDirectSoundBuffer_Lock(lpdsbuf,buf_off,size,(LPVOID *)&lpbuf1,&dwsize1,(LPVOID *)&lpbuf2,&dwsize2,0);
        if (result != DS_OK) {
	  ERR("Unable to lock sound buffer !\n");
          continue;
        }

        dwbyteswritten1 = min(size,dwsize1);
        memcpy(lpbuf1,dma_buffer,dwbyteswritten1);
        if (size>dwsize1) {
            dwbyteswritten2 = min(size - dwbyteswritten1,dwsize2);
            memcpy(lpbuf2,dma_buffer+dwbyteswritten1,dwbyteswritten2);
        }
        buf_off = (buf_off + dwbyteswritten1 + dwbyteswritten2) % DSBUFLEN;

        result = IDirectSoundBuffer_Unlock(lpdsbuf,lpbuf1,dwbyteswritten1,lpbuf2,dwbyteswritten2);
        if (result!=DS_OK)
	    ERR("Unable to unlock sound buffer !\n");

        SamplesCount -= size;
        if (!SamplesCount) dma_enable = FALSE;
    }
    return 0;
}

static BOOL SB_Init(void)
{
    HRESULT result;

    if (!lpdsound) {
        result = DirectSoundCreate(NULL,&lpdsound,NULL);
        if (result != DS_OK) {
            ERR("Unable to initialize Sound Subsystem err = %x !\n",result);
            return FALSE;
        }

        /* FIXME: To uncomment when :
           - SetCooperative level is correctly implemented
           - an always valid and non changing handle to a windows  (vga_hwnd) is available
             (this surely needs some work in vga.c)
        result = IDirectSound_SetCooperativeLevel(lpdsound,vga_hwnd,DSSCL_EXCLUSIVE|DSSCL_PRIORITY);
        if (result != DS_OK) {
            ERR("Can't set cooperative level !\n");
            return FALSE;
        }
        */

        /* Default format */
        wav_fmt.wFormatTag = WAVE_FORMAT_PCM;
        wav_fmt.nChannels = 1;
        wav_fmt.nSamplesPerSec = 22050;
        wav_fmt.nAvgBytesPerSec = 22050;
        wav_fmt.nBlockAlign = 1;
        wav_fmt.wBitsPerSample = 8;
        wav_fmt.cbSize = 0;

        memset(&buf_desc,0,sizeof(DSBUFFERDESC));
        buf_desc.dwSize = sizeof(DSBUFFERDESC);
        buf_desc.dwBufferBytes = DSBUFLEN;
        buf_desc.lpwfxFormat = &wav_fmt;
        result = IDirectSound_CreateSoundBuffer(lpdsound,&buf_desc,&lpdsbuf,NULL);
        if (result != DS_OK) {
            ERR("Can't create sound buffer !\n");
            return FALSE;
        }

        result = IDirectSoundBuffer_Play(lpdsbuf,0, 0, DSBPLAY_LOOPING);
        if (result != DS_OK) {
            ERR("Can't start playing !\n");
            return FALSE;
        }

        buf_off = 0;
        end_sound_loop = FALSE;
        SB_Thread = CreateThread(NULL, 0, SB_Poll, NULL, 0, NULL);
        TRACE("thread\n");
        if (!SB_Thread) {
            ERR("Can't create thread !\n");
            return FALSE;
        }
    }
    return TRUE;
}

static void SB_Reset(void)
{
    int i;

    for(i=0;i<256;i++)
        DSP_Command[i]=0;

    /* Set Time Constant */
    DSP_Command[0x40]=1;
    /* Generate IRQ */
    DSP_Command[0xF2]=0;
    /* DMA DAC 8-bits */
    DSP_Command[0x14]=2;
    /* Generic DAC/ADC DMA (16-bit, 8-bit) */
    for(i=0xB0;i<=0xCF;i++)
        DSP_Command[i]=3;
    /* DSP Identification */
    DSP_Command[0xE0]=1;

    /* Clear command and input buffer */
    command = -1;
    InSize = 0;

    /* Put a garbage value in the output buffer */
    OutSize = 1;
    if (SB_Init())
        /* All right, let's put the magic value for autodetection */
        DSP_OutBuffer[0] = 0xaa;
    else
        /* Something is wrong, put 0 to failed autodetection */
        DSP_OutBuffer[0] = 0x00;
}

/* Find a standard sampling rate for DirectSound */
static int SB_StdSampleRate(int SampleRate)
{
  if (SampleRate>((44100+48000)/2)) return 48000;
  if (SampleRate>((32000+44100)/2)) return 44100;
  if (SampleRate>((24000+32000)/2)) return 32000;
  if (SampleRate>((22050+24000)/2)) return 24000;
  if (SampleRate>((16000+22050)/2)) return 22050;
  if (SampleRate>((12000+16000)/2)) return 16000;
  if (SampleRate>((11025+12000)/2)) return 12000;
  if (SampleRate>((8000+11025)/2))  return 11025;
  return 8000;
}

void SB_ioport_out( WORD port, BYTE val )
{
    switch(port)
    {
    /* DSP - Reset */
    case 0x226:
        TRACE("Resetting DSP.\n");
        SB_Reset();
        break;
    /* DSP - Write Data or Command */
    case 0x22c:
        TRACE("val=%x\n",val);
        if (command == -1) {
          /* Clear input buffer and set the current command */
          command = val;
          InSize = 0;
        }
        if (InSize!=DSP_Command[command])
	   /* Fill the input buffer the command parameters if any */
           DSP_InBuffer[InSize++]=val;
        else {
	    /* Process command */
            switch(command)
            {
            case 0x10: /* SB */
                FIXME("Direct DAC (8-bit) - Not Implemented\n");
                break;
            case 0x14: /* SB */
                SamplesCount = DSP_InBuffer[1]+(val<<8)+1;
                TRACE("DMA DAC (8-bit) for %x samples\n",SamplesCount);
                dma_enable = TRUE;
                break;
            case 0x20:
                FIXME("Direct ADC (8-bit) - Not Implemented\n");
                break;
            case 0x24: /* SB */
                FIXME("DMA ADC (8-bit) - Not Implemented\n");
                break;
            case 0x40: /* SB */
                SampleRate = 1000000/(256-val);
                TRACE("Set Time Constant (%d <-> %d Hz => %d Hz)\n",DSP_InBuffer[0],
                    SampleRate,SB_StdSampleRate(SampleRate));
                SampleRate = SB_StdSampleRate(SampleRate);
                wav_fmt.nSamplesPerSec = SampleRate;
                wav_fmt.nAvgBytesPerSec = SampleRate;
                IDirectSoundBuffer_SetFormat(lpdsbuf,&wav_fmt);
                break;
	    /* case 0xBX/0xCX -> See below */
            case 0xD0: /* SB */
                TRACE("Halt DMA operation (8-bit)\n");
                dma_enable = FALSE;
                break;
            case 0xD1: /* SB */
                FIXME("Enable Speaker - Not Implemented\n");
                break;
            case 0xD3: /* SB */
                FIXME("Disable Speaker - Not Implemented\n");
                break;
            case 0xD4: /* SB */
                FIXME("Continue DMA operation (8-bit) - Not Implemented\n");
                break;
            case 0xD8: /* SB */
                FIXME("Speaker Status - Not Implemented\n");
                break;
	    case 0xE0: /* SB 2.0 */
                TRACE("DSP Identification\n");
                DSP_OutBuffer[OutSize++] = ~val;
                break;
            case 0xE1: /* SB */
               TRACE("DSP Version\n");
               OutSize=2;
               DSP_OutBuffer[0]=0; /* returns version 1.0 */
               DSP_OutBuffer[1]=1;
                break;
            case 0xF2: /* SB */
                TRACE("IRQ Request (8-bit)\n");
                break;
            default:
	      if (((command&0xF0)==0xB0)||((DSP_InBuffer[0]&0xF0)==0xC0)) {
		    /* SB16 */
                    FIXME("Generic DAC/ADC DMA (16-bit, 8-bit) - %d % d\n",command,DSP_InBuffer[1]);
                    if (command&0x02)
		        FIXME("Generic DAC/ADC fifo mode not supported\n");
                    if (command&0x04)
		        FIXME("Generic DAC/ADC autoinit dma mode not supported\n");
                    if (command&0x08)
		        FIXME("Generic DAC/ADC adc mode not supported\n");
                    switch(command>>4) {
                    case 0xB:
		        FIXME("Generic DAC/ADC 8-bit not supported\n");
                        SampleMode = 0;
                        break;
                    case 0xC:
		        FIXME("Generic DAC/ADC 16-bit not supported\n");
                        SampleMode = 1;
                        break;
                    default:
		        ERR("Generic DAC/ADC resolution unknown\n");
                        break;
                    }
                    if (DSP_InBuffer[1]&0x010)
		        FIXME("Generic DAC/ADC signed sample mode not supported\n");
                    if (DSP_InBuffer[1]&0x020)
		        FIXME("Generic DAC/ADC stereo mode not supported\n");
                    SamplesCount = DSP_InBuffer[2]+(val<<8)+1;
                    TRACE("Generic DMA for %x samples\n",SamplesCount);
                    dma_enable = TRUE;
	        }
                else
                    FIXME("DSP command %x not supported\n",val);
            }
            /* Empty the input buffer and end the command */
            InSize = 0;
            command = -1;
        }
    }
}

BYTE SB_ioport_in( WORD port )
{
    BYTE res = 0;

    switch(port)
    {
    /* DSP Read Data */
    case 0x22a:
        /* Value in the read buffer */
      if (OutSize)
          res = DSP_OutBuffer[--OutSize];
      else
	  /* return the last byte */
	  res = DSP_OutBuffer[0];
      break;
    /* DSP - Write Buffer Status */
    case 0x22c:
        /* DSP always ready for writing */
        res = 0x00;
        break;
    /* DSP - Data Available Status */
    /* DSP - IRQ Acknowledge, 8-bit */
    case 0x22e:
        /* DSP data availability check */
        if (OutSize)
            res = 0x80;
        else
	    res = 0x00;
        break;
    }
    return res;
}
