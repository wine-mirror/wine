/*
 * Sample MIXER Wine Driver for Mac OS X (based on OSS mixer)
 *
 * Copyright 	1997 Marcus Meissner
 * 		1999,2001 Eric Pouech
 *              2006,2007 Emmanuel Maillard
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
#include "wine/port.h"

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "mmddk.h"
#include "coreaudio.h"
#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mixer);

#if defined(HAVE_COREAUDIO_COREAUDIO_H)
#include <CoreAudio/CoreAudio.h>
#include <CoreFoundation/CoreFoundation.h>

#define	WINE_MIXER_NAME "CoreAudio Mixer"

#define InputDevice (1 << 0)
#define OutputDevice (1 << 1)

#define IsInput(dir) ((dir) & InputDevice)
#define IsOutput(dir) ((dir) & OutputDevice)

#define ControlsPerLine 2 /* number of control per line : volume & (mute | onoff) */

#define IDControlVolume 0
#define IDControlMute 1

typedef struct tagMixerLine
{
    char *name;
    int direction;
    int numChannels;
    int componentType;
    AudioDeviceID deviceID;
} MixerLine;

typedef struct tagMixerCtrl
{
    DWORD dwLineID;
    MIXERCONTROLW ctrl;
} MixerCtrl;

typedef struct tagCoreAudio_Mixer
{
    MIXERCAPSW caps;

    MixerCtrl *mixerCtrls;
    MixerLine *lines;
    DWORD numCtrl;
} CoreAudio_Mixer;

static CoreAudio_Mixer mixer;
static int numMixers = 1;

/**************************************************************************
*/

static const char * getMessage(UINT uMsg)
{
    static char str[64];
#define MSG_TO_STR(x) case x: return #x;
    switch (uMsg) {
        MSG_TO_STR(DRVM_INIT);
        MSG_TO_STR(DRVM_EXIT);
        MSG_TO_STR(DRVM_ENABLE);
        MSG_TO_STR(DRVM_DISABLE);
        MSG_TO_STR(MXDM_GETDEVCAPS);
        MSG_TO_STR(MXDM_GETLINEINFO);
        MSG_TO_STR(MXDM_GETNUMDEVS);
        MSG_TO_STR(MXDM_OPEN);
        MSG_TO_STR(MXDM_CLOSE);
        MSG_TO_STR(MXDM_GETLINECONTROLS);
        MSG_TO_STR(MXDM_GETCONTROLDETAILS);
        MSG_TO_STR(MXDM_SETCONTROLDETAILS);
    }
#undef MSG_TO_STR
        sprintf(str, "UNKNOWN(%08x)", uMsg);
    return str;
}

static const char * getControlType(DWORD dwControlType)
{
    static char str[64];
#define TYPE_TO_STR(x) case x: return #x;
    switch (dwControlType) {
        TYPE_TO_STR(MIXERCONTROL_CONTROLTYPE_CUSTOM);
        TYPE_TO_STR(MIXERCONTROL_CONTROLTYPE_BOOLEANMETER);
        TYPE_TO_STR(MIXERCONTROL_CONTROLTYPE_SIGNEDMETER);
        TYPE_TO_STR(MIXERCONTROL_CONTROLTYPE_PEAKMETER);
        TYPE_TO_STR(MIXERCONTROL_CONTROLTYPE_UNSIGNEDMETER);
        TYPE_TO_STR(MIXERCONTROL_CONTROLTYPE_BOOLEAN);
        TYPE_TO_STR(MIXERCONTROL_CONTROLTYPE_ONOFF);
        TYPE_TO_STR(MIXERCONTROL_CONTROLTYPE_MUTE);
        TYPE_TO_STR(MIXERCONTROL_CONTROLTYPE_MONO);
        TYPE_TO_STR(MIXERCONTROL_CONTROLTYPE_LOUDNESS);
        TYPE_TO_STR(MIXERCONTROL_CONTROLTYPE_STEREOENH);
        TYPE_TO_STR(MIXERCONTROL_CONTROLTYPE_BASS_BOOST);
        TYPE_TO_STR(MIXERCONTROL_CONTROLTYPE_BUTTON);
        TYPE_TO_STR(MIXERCONTROL_CONTROLTYPE_DECIBELS);
        TYPE_TO_STR(MIXERCONTROL_CONTROLTYPE_SIGNED);
        TYPE_TO_STR(MIXERCONTROL_CONTROLTYPE_UNSIGNED);
        TYPE_TO_STR(MIXERCONTROL_CONTROLTYPE_PERCENT);
        TYPE_TO_STR(MIXERCONTROL_CONTROLTYPE_SLIDER);
        TYPE_TO_STR(MIXERCONTROL_CONTROLTYPE_PAN);
        TYPE_TO_STR(MIXERCONTROL_CONTROLTYPE_QSOUNDPAN);
        TYPE_TO_STR(MIXERCONTROL_CONTROLTYPE_FADER);
        TYPE_TO_STR(MIXERCONTROL_CONTROLTYPE_VOLUME);
        TYPE_TO_STR(MIXERCONTROL_CONTROLTYPE_BASS);
        TYPE_TO_STR(MIXERCONTROL_CONTROLTYPE_TREBLE);
        TYPE_TO_STR(MIXERCONTROL_CONTROLTYPE_EQUALIZER);
        TYPE_TO_STR(MIXERCONTROL_CONTROLTYPE_SINGLESELECT);
        TYPE_TO_STR(MIXERCONTROL_CONTROLTYPE_MUX);
        TYPE_TO_STR(MIXERCONTROL_CONTROLTYPE_MULTIPLESELECT);
        TYPE_TO_STR(MIXERCONTROL_CONTROLTYPE_MIXER);
        TYPE_TO_STR(MIXERCONTROL_CONTROLTYPE_MICROTIME);
        TYPE_TO_STR(MIXERCONTROL_CONTROLTYPE_MILLITIME);
    }
#undef TYPE_TO_STR
        sprintf(str, "UNKNOWN(%08x)", dwControlType);
    return str;
}

static const char * getComponentType(DWORD dwComponentType)
{
    static char str[64];
#define TYPE_TO_STR(x) case x: return #x;
    switch (dwComponentType) {
        TYPE_TO_STR(MIXERLINE_COMPONENTTYPE_DST_UNDEFINED);
        TYPE_TO_STR(MIXERLINE_COMPONENTTYPE_DST_DIGITAL);
        TYPE_TO_STR(MIXERLINE_COMPONENTTYPE_DST_LINE);
        TYPE_TO_STR(MIXERLINE_COMPONENTTYPE_DST_MONITOR);
        TYPE_TO_STR(MIXERLINE_COMPONENTTYPE_DST_SPEAKERS);
        TYPE_TO_STR(MIXERLINE_COMPONENTTYPE_DST_HEADPHONES);
        TYPE_TO_STR(MIXERLINE_COMPONENTTYPE_DST_TELEPHONE);
        TYPE_TO_STR(MIXERLINE_COMPONENTTYPE_DST_WAVEIN);
        TYPE_TO_STR(MIXERLINE_COMPONENTTYPE_DST_VOICEIN);
        TYPE_TO_STR(MIXERLINE_COMPONENTTYPE_SRC_UNDEFINED);
        TYPE_TO_STR(MIXERLINE_COMPONENTTYPE_SRC_DIGITAL);
        TYPE_TO_STR(MIXERLINE_COMPONENTTYPE_SRC_LINE);
        TYPE_TO_STR(MIXERLINE_COMPONENTTYPE_SRC_MICROPHONE);
        TYPE_TO_STR(MIXERLINE_COMPONENTTYPE_SRC_SYNTHESIZER);
        TYPE_TO_STR(MIXERLINE_COMPONENTTYPE_SRC_COMPACTDISC);
        TYPE_TO_STR(MIXERLINE_COMPONENTTYPE_SRC_TELEPHONE);
        TYPE_TO_STR(MIXERLINE_COMPONENTTYPE_SRC_PCSPEAKER);
        TYPE_TO_STR(MIXERLINE_COMPONENTTYPE_SRC_WAVEOUT);
        TYPE_TO_STR(MIXERLINE_COMPONENTTYPE_SRC_AUXILIARY);
        TYPE_TO_STR(MIXERLINE_COMPONENTTYPE_SRC_ANALOG);
    }
#undef TYPE_TO_STR
        sprintf(str, "UNKNOWN(%08x)", dwComponentType);
    return str;
}

static const char * getTargetType(DWORD dwType)
{
    static char str[64];
#define TYPE_TO_STR(x) case x: return #x;
    switch (dwType) {
        TYPE_TO_STR(MIXERLINE_TARGETTYPE_UNDEFINED);
        TYPE_TO_STR(MIXERLINE_TARGETTYPE_WAVEOUT);
        TYPE_TO_STR(MIXERLINE_TARGETTYPE_WAVEIN);
        TYPE_TO_STR(MIXERLINE_TARGETTYPE_MIDIOUT);
        TYPE_TO_STR(MIXERLINE_TARGETTYPE_MIDIIN);
        TYPE_TO_STR(MIXERLINE_TARGETTYPE_AUX);
    }
#undef TYPE_TO_STR
        sprintf(str, "UNKNOWN(%08x)", dwType);
    return str;
}

/* FIXME is there a better way ? */
static DWORD DeviceComponentType(char *name)
{
    if (strcmp(name, "Built-in Microphone") == 0)
        return MIXERLINE_COMPONENTTYPE_SRC_MICROPHONE;

    if (strcmp(name, "Built-in Line Input") == 0)
        return MIXERLINE_COMPONENTTYPE_SRC_LINE;

    if (strcmp(name, "Built-in Output") == 0)
        return MIXERLINE_COMPONENTTYPE_DST_SPEAKERS;

    return MIXERLINE_COMPONENTTYPE_SRC_UNDEFINED;
}

static BOOL DeviceHasMute(AudioDeviceID deviceID, Boolean isInput)
{
    Boolean writable = false;
    OSStatus err = noErr;
    err = AudioDeviceGetPropertyInfo(deviceID, 0, isInput, kAudioDevicePropertyMute, NULL, NULL);
    if (err == noErr)
    {
        /* check if we can set it */
        err = AudioDeviceGetPropertyInfo(deviceID, 0, isInput, kAudioDevicePropertyMute, NULL, &writable);
        if (err == noErr)
            return writable;
    }
    return FALSE;
}

static void MIX_FillControls(void)
{
    int i;
    int ctrl = 0;
    MixerLine *line;
    for (i = 0; i < mixer.caps.cDestinations; i++)
    {
        line = &mixer.lines[i];
        mixer.mixerCtrls[ctrl].dwLineID = i;
        mixer.mixerCtrls[ctrl].ctrl.cbStruct = sizeof(MIXERCONTROLW);
        mixer.mixerCtrls[ctrl].ctrl.dwControlType = MIXERCONTROL_CONTROLTYPE_VOLUME;
        mixer.mixerCtrls[ctrl].ctrl.dwControlID = ctrl;
        mixer.mixerCtrls[ctrl].ctrl.Bounds.s1.dwMinimum = 0;
        mixer.mixerCtrls[ctrl].ctrl.Bounds.s1.dwMaximum = 65535;
        mixer.mixerCtrls[ctrl].ctrl.Metrics.cSteps = 656;
        ctrl++;

        mixer.mixerCtrls[ctrl].dwLineID = i;
        if ( !DeviceHasMute(line->deviceID, IsInput(line->direction)) )
            mixer.mixerCtrls[ctrl].ctrl.fdwControl |= MIXERCONTROL_CONTROLF_DISABLED;

        mixer.mixerCtrls[ctrl].ctrl.cbStruct = sizeof(MIXERCONTROLW);
        mixer.mixerCtrls[ctrl].ctrl.dwControlType = MIXERCONTROL_CONTROLTYPE_MUTE;
        mixer.mixerCtrls[ctrl].ctrl.dwControlID = ctrl;
        mixer.mixerCtrls[ctrl].ctrl.Bounds.s1.dwMinimum = 0;
        mixer.mixerCtrls[ctrl].ctrl.Bounds.s1.dwMaximum = 1;
        ctrl++;
    }
    assert(ctrl == mixer.numCtrl);
}

/**************************************************************************
* 				CoreAudio_MixerInit
*/
LONG CoreAudio_MixerInit(void)
{
    OSStatus status;
    UInt32 propertySize;
    AudioDeviceID *deviceArray = NULL;
    char name[MAXPNAMELEN];
    int i;
    int numLines;

    AudioStreamBasicDescription streamDescription;

    /* Find number of lines */
    status = AudioHardwareGetPropertyInfo(kAudioHardwarePropertyDevices, &propertySize, NULL);
    if (status)
    {
        ERR("AudioHardwareGetPropertyInfo for kAudioHardwarePropertyDevices return %c%c%c%c\n", (char) (status >> 24),
            (char) (status >> 16),
            (char) (status >> 8),
            (char) status);
        return 0;
    }

    numLines = propertySize / sizeof(AudioDeviceID);

    mixer.mixerCtrls = NULL;
    mixer.lines = NULL;
    mixer.numCtrl = 0;

    mixer.caps.cDestinations = numLines;
    mixer.caps.wMid = 0xAA;
    mixer.caps.wPid = 0x55;
    mixer.caps.vDriverVersion = 0x0100;

    MultiByteToWideChar(CP_ACP, 0, WINE_MIXER_NAME, -1, mixer.caps.szPname, sizeof(mixer.caps.szPname) / sizeof(WCHAR));

    mixer.caps.fdwSupport = 0; /* No bits defined yet */

    mixer.lines = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(MixerLine) * numLines);
    if (!mixer.lines)
        goto error;

    deviceArray = HeapAlloc(GetProcessHeap(), 0, sizeof(AudioDeviceID) * numLines);

    propertySize = sizeof(AudioDeviceID) * numLines;
    status = AudioHardwareGetProperty(kAudioHardwarePropertyDevices, &propertySize, deviceArray);
    if (status)
    {
        ERR("AudioHardwareGetProperty for kAudioHardwarePropertyDevices return %c%c%c%c\n", (char) (status >> 24),
            (char) (status >> 16),
            (char) (status >> 8),
            (char) status);
        goto error;
    }

    for (i = 0; i < numLines; i++)
    {
        Boolean write;
        MixerLine *line = &mixer.lines[i];

        line->deviceID = deviceArray[i];

        propertySize = MAXPNAMELEN;
        status = AudioDeviceGetProperty(line->deviceID, 0 , FALSE, kAudioDevicePropertyDeviceName, &propertySize, name);
        if (status) {
            ERR("AudioHardwareGetProperty for kAudioDevicePropertyDeviceName return %c%c%c%c\n", (char) (status >> 24),
                (char) (status >> 16),
                (char) (status >> 8),
                (char) status);
            goto error;
        }

        line->name = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, strlen(name) + 1);
        if (!line->name)
            goto error;

        memcpy(line->name, name, strlen(name));

        line->componentType = DeviceComponentType(line->name);

        /* check for directions */
        /* Output ? */
        propertySize = sizeof(UInt32);
	status = AudioDeviceGetPropertyInfo(line->deviceID, 0, FALSE, kAudioDevicePropertyStreams, &propertySize, &write );
        if (status) {
            ERR("AudioDeviceGetPropertyInfo for kAudioDevicePropertyDataSource return %c%c%c%c\n", (char) (status >> 24),
                (char) (status >> 16),
                (char) (status >> 8),
                (char) status);
            goto error;
        }

        if ( (propertySize / sizeof(AudioStreamID)) != 0)
        {
            line->direction |= OutputDevice;

            /* Check the number of channel for the stream */
            propertySize = sizeof(streamDescription);
            status = AudioDeviceGetProperty(line->deviceID, 0, FALSE , kAudioDevicePropertyStreamFormat, &propertySize, &streamDescription);
            if (status != noErr) {
                ERR("AudioHardwareGetProperty for kAudioDevicePropertyStreamFormat return %c%c%c%c\n", (char) (status >> 24),
                    (char) (status >> 16),
                    (char) (status >> 8),
                    (char) status);
                goto error;
            }
            line->numChannels = streamDescription.mChannelsPerFrame;
        }
        else
        {
            /* Input ? */
            propertySize = sizeof(UInt32);
            status = AudioDeviceGetPropertyInfo(line->deviceID, 0, TRUE, kAudioDevicePropertyStreams, &propertySize, &write );
            if (status) {
                ERR("AudioDeviceGetPropertyInfo for kAudioDevicePropertyStreams return %c%c%c%c\n", (char) (status >> 24),
                    (char) (status >> 16),
                    (char) (status >> 8),
                    (char) status);
                goto error;
            }
            if ( (propertySize / sizeof(AudioStreamID)) != 0)
            {
                line->direction |= InputDevice;

                /* Check the number of channel for the stream */
                propertySize = sizeof(streamDescription);
                status = AudioDeviceGetProperty(line->deviceID, 0, TRUE, kAudioDevicePropertyStreamFormat, &propertySize, &streamDescription);
                if (status != noErr) {
                    ERR("AudioHardwareGetProperty for kAudioDevicePropertyStreamFormat return %c%c%c%c\n", (char) (status >> 24),
                        (char) (status >> 16),
                        (char) (status >> 8),
                        (char) status);
                    goto error;
                }
                line->numChannels = streamDescription.mChannelsPerFrame;
            }
        }

        mixer.numCtrl += ControlsPerLine; /* volume & (mute | onoff) */
    }
    mixer.mixerCtrls = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(MixerCtrl) * mixer.numCtrl);
    if (!mixer.mixerCtrls)
        goto error;

    MIX_FillControls();

    HeapFree(GetProcessHeap(), 0, deviceArray);
    return 1;

error:
    if (mixer.lines)
    {
        int i;
        for (i = 0; i < mixer.caps.cDestinations; i++)
        {
            HeapFree(GetProcessHeap(), 0, mixer.lines[i].name);
        }
        HeapFree(GetProcessHeap(), 0, mixer.lines);
    }
    HeapFree(GetProcessHeap(), 0, deviceArray);
    if (mixer.mixerCtrls)
        HeapFree(GetProcessHeap(), 0, mixer.mixerCtrls);
    return 0;
}

/**************************************************************************
* 				CoreAudio_MixerRelease
*/
void CoreAudio_MixerRelease(void)
{
    TRACE("()\n");

    if (mixer.lines)
    {
        int i;
        for (i = 0; i < mixer.caps.cDestinations; i++)
        {
            HeapFree(GetProcessHeap(), 0, mixer.lines[i].name);
        }
        HeapFree(GetProcessHeap(), 0, mixer.lines);
    }
    if (mixer.mixerCtrls)
        HeapFree(GetProcessHeap(), 0, mixer.mixerCtrls);
}

/**************************************************************************
* 				mxdMessage
*/
DWORD WINAPI CoreAudio_mxdMessage(UINT wDevID, UINT wMsg, DWORD_PTR dwUser,
                                  DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
    TRACE("(%04X, %s, %08lX, %08lX, %08lX);\n", wDevID, getMessage(wMsg),
          dwUser, dwParam1, dwParam2);

    switch (wMsg)
    {
        case DRVM_INIT:
        case DRVM_EXIT:
        case DRVM_ENABLE:
        case DRVM_DISABLE:
            /* FIXME: Pretend this is supported */
            return 0;
        case MXDM_OPEN:
        case MXDM_CLOSE:
        case MXDM_GETNUMDEVS:
        case MXDM_GETDEVCAPS:
        case MXDM_GETLINEINFO:
        case MXDM_GETLINECONTROLS:
        case MXDM_GETCONTROLDETAILS:
        case MXDM_SETCONTROLDETAILS:
        default:
            WARN("unknown message %d!\n", wMsg);
            return MMSYSERR_NOTSUPPORTED;
    }
}

#else

DWORD WINAPI CoreAudio_mxdMessage(UINT wDevID, UINT wMsg, DWORD_PTR dwUser,
                                  DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
    TRACE("(%04X, %04x, %08lX, %08lX, %08lX);\n", wDevID, wMsg, dwUser, dwParam1, dwParam2);
    return MMSYSERR_NOTENABLED;
}
#endif /* HAVE_COREAUDIO_COREAUDIO_H */
