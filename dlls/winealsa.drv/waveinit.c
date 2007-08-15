/*
 * Sample Wine Driver for Advanced Linux Sound System (ALSA)
 *      Based on version <final> of the ALSA API
 *
 * This file performs the initialisation and scanning of the sound subsystem.
 *
 * Copyright    2002 Eric Pouech
 *              2002 Marco Pietrobono
 *              2003 Christian Costa : WaveIn support
 *              2006-2007 Maarten Lankhorst
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
#include <errno.h>
#include <limits.h>
#include <fcntl.h>
#ifdef HAVE_SYS_IOCTL_H
# include <sys/ioctl.h>
#endif
#ifdef HAVE_SYS_MMAN_H
# include <sys/mman.h>
#endif
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winerror.h"
#include "winuser.h"
#include "winnls.h"
#include "winreg.h"
#include "mmddk.h"

/* ksmedia.h defines KSDATAFORMAT_SUBTYPE_PCM and KSDATAFORMAT_SUBTYPE_IEEE_FLOAT
 * However either all files that use it will define it, or no files will
 * The only way to solve this is by adding initguid.h here, and include the guid that way
 */
#include "initguid.h"
#include "alsa.h"

#include "wine/library.h"
#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wave);

#ifdef HAVE_ALSA

/*----------------------------------------------------------------------------
**  ALSA_TestDeviceForWine
**
**      Test to see if a given device is sufficient for Wine.
*/
static int ALSA_TestDeviceForWine(int card, int device,  snd_pcm_stream_t streamtype)
{
    snd_pcm_t *pcm = NULL;
    char pcmname[256];
    int retcode;
    snd_pcm_hw_params_t *hwparams;
    const char *reason = NULL;
    unsigned int rrate;

    /* Note that the plug: device masks out a lot of info, we want to avoid that */
    sprintf(pcmname, "hw:%d,%d", card, device);
    retcode = snd_pcm_open(&pcm, pcmname, streamtype, SND_PCM_NONBLOCK);
    if (retcode < 0)
    {
        /* Note that a busy device isn't automatically disqualified */
        if (retcode == (-1 * EBUSY))
            retcode = 0;
        goto exit;
    }

    snd_pcm_hw_params_alloca(&hwparams);

    retcode = snd_pcm_hw_params_any(pcm, hwparams);
    if (retcode < 0)
    {
        reason = "Could not retrieve hw_params";
        goto exit;
    }

    /* set the count of channels */
    retcode = snd_pcm_hw_params_set_channels(pcm, hwparams, 2);
    if (retcode < 0)
    {
        reason = "Could not set channels";
        goto exit;
    }

    rrate = 44100;
    retcode = snd_pcm_hw_params_set_rate_near(pcm, hwparams, &rrate, 0);
    if (retcode < 0)
    {
        reason = "Could not set rate";
        goto exit;
    }

    if (rrate == 0)
    {
        reason = "Rate came back as 0";
        goto exit;
    }

    /* write the parameters to device */
    retcode = snd_pcm_hw_params(pcm, hwparams);
    if (retcode < 0)
    {
        reason = "Could not set hwparams";
        goto exit;
    }

    retcode = 0;

exit:
    if (pcm)
        snd_pcm_close(pcm);

    if (retcode != 0 && retcode != (-1 * ENOENT))
        TRACE("Discarding card %d/device %d:  %s [%d(%s)]\n", card, device, reason, retcode, snd_strerror(retcode));

    return retcode;
}

/*----------------------------------------------------------------------------
** ALSA_RegGetString
**  Retrieve a string from a registry key
*/
static int ALSA_RegGetString(HKEY key, const char *value, char **bufp)
{
    DWORD rc;
    DWORD type;
    DWORD bufsize;

    *bufp = NULL;
    rc = RegQueryValueExA(key, value, NULL, &type, NULL, &bufsize);
    if (rc != ERROR_SUCCESS)
        return(rc);

    if (type != REG_SZ)
        return 1;

    *bufp = HeapAlloc(GetProcessHeap(), 0, bufsize);
    if (! *bufp)
        return 1;

    rc = RegQueryValueExA(key, value, NULL, NULL, (LPBYTE)*bufp, &bufsize);
    return rc;
}

/*----------------------------------------------------------------------------
** ALSA_RegGetBoolean
**  Get a string and interpret it as a boolean
*/

/* Possible truths:
   Y(es), T(rue), 1, E(nabled) */

#define IS_OPTION_TRUE(ch) ((ch) == 'y' || (ch) == 'Y' || (ch) == 't' || (ch) == 'T' || (ch) == '1' || (ch) == 'e' || (ch) == 'E')
static int ALSA_RegGetBoolean(HKEY key, const char *value, BOOL *answer)
{
    DWORD rc;
    char *buf = NULL;

    rc = ALSA_RegGetString(key, value, &buf);
    if (buf)
    {
        *answer = FALSE;
        if (IS_OPTION_TRUE(*buf))
            *answer = TRUE;

        HeapFree(GetProcessHeap(), 0, buf);
    }

    return rc;
}

/*----------------------------------------------------------------------------
** ALSA_RegGetBoolean
**  Get a string and interpret it as a DWORD
*/
static int ALSA_RegGetInt(HKEY key, const char *value, DWORD *answer)
{
    DWORD rc;
    char *buf = NULL;

    rc = ALSA_RegGetString(key, value, &buf);
    if (buf)
    {
        *answer = atoi(buf);
        HeapFree(GetProcessHeap(), 0, buf);
    }

    return rc;
}

/* return a string duplicated on the win32 process heap, free with HeapFree */
static char* ALSA_strdup(const char *s) {
    char *result = HeapAlloc(GetProcessHeap(), 0, strlen(s)+1);
    if (!result)
        return NULL;
    strcpy(result, s);
    return result;
}

#define ALSA_RETURN_ONFAIL(mycall)                                      \
{                                                                       \
    int rc;                                                             \
    {rc = mycall;}                                                      \
    if ((rc) < 0)                                                       \
    {                                                                   \
        ERR("%s failed:  %s(%d)\n", #mycall, snd_strerror(rc), rc);     \
        return(rc);                                                     \
    }                                                                   \
}

/*----------------------------------------------------------------------------
**  ALSA_ComputeCaps
**
**      Given an ALSA PCM, figure out our HW CAPS structure info.
**  ctl can be null, pcm is required, as is all output parms.
**
*/
static int ALSA_ComputeCaps(snd_ctl_t *ctl, snd_pcm_t *pcm,
        WORD *channels, DWORD *flags, DWORD *formats, DWORD *supports)
{
    snd_pcm_hw_params_t *hw_params;
    snd_pcm_format_mask_t *fmask;
    snd_pcm_access_mask_t *acmask;
    unsigned int ratemin = 0;
    unsigned int ratemax = 0;
    unsigned int chmin = 0;
    unsigned int chmax = 0;
    int dir = 0;

    snd_pcm_hw_params_alloca(&hw_params);
    ALSA_RETURN_ONFAIL(snd_pcm_hw_params_any(pcm, hw_params));

    snd_pcm_format_mask_alloca(&fmask);
    snd_pcm_hw_params_get_format_mask(hw_params, fmask);

    snd_pcm_access_mask_alloca(&acmask);
    ALSA_RETURN_ONFAIL(snd_pcm_hw_params_get_access_mask(hw_params, acmask));

    ALSA_RETURN_ONFAIL(snd_pcm_hw_params_get_rate_min(hw_params, &ratemin, &dir));
    ALSA_RETURN_ONFAIL(snd_pcm_hw_params_get_rate_max(hw_params, &ratemax, &dir));
    ALSA_RETURN_ONFAIL(snd_pcm_hw_params_get_channels_min(hw_params, &chmin));
    ALSA_RETURN_ONFAIL(snd_pcm_hw_params_get_channels_max(hw_params, &chmax));

#define X(r,v) \
    if ( (r) >= ratemin && ( (r) <= ratemax || ratemax == -1) ) \
    { \
       if (snd_pcm_format_mask_test( fmask, SND_PCM_FORMAT_U8)) \
       { \
          if (chmin <= 1 && 1 <= chmax) \
              *formats |= WAVE_FORMAT_##v##M08; \
          if (chmin <= 2 && 2 <= chmax) \
              *formats |= WAVE_FORMAT_##v##S08; \
       } \
       if (snd_pcm_format_mask_test( fmask, SND_PCM_FORMAT_S16_LE)) \
       { \
          if (chmin <= 1 && 1 <= chmax) \
              *formats |= WAVE_FORMAT_##v##M16; \
          if (chmin <= 2 && 2 <= chmax) \
              *formats |= WAVE_FORMAT_##v##S16; \
       } \
    }
    X(11025,1);
    X(22050,2);
    X(44100,4);
    X(48000,48);
    X(96000,96);
#undef X

    if (chmin > 1)
        FIXME("Device has a minimum of %d channels\n", chmin);
    *channels = chmax;

    /* FIXME: is sample accurate always true ?
    ** Can we do WAVECAPS_PITCH, WAVECAPS_SYNC, or WAVECAPS_PLAYBACKRATE? */
    *supports |= WAVECAPS_SAMPLEACCURATE;

    /* FIXME: NONITERLEAVED and COMPLEX are not supported right now */
    if ( snd_pcm_access_mask_test( acmask, SND_PCM_ACCESS_MMAP_INTERLEAVED ) )
        *supports |= WAVECAPS_DIRECTSOUND;

    /* check for volume control support */
    if (ctl) {
        if (snd_ctl_name(ctl))
        {
            snd_hctl_t *hctl;
            if (snd_hctl_open(&hctl, snd_ctl_name(ctl), 0) >= 0)
            {
                snd_hctl_load(hctl);
                if (!ALSA_CheckSetVolume( hctl, NULL, NULL, NULL, NULL, NULL, NULL, NULL ))
                {
                    *supports |= WAVECAPS_VOLUME;
                    if (chmin <= 2 && 2 <= chmax)
                        *supports |= WAVECAPS_LRVOLUME;
                }
                snd_hctl_free(hctl);
                snd_hctl_close(hctl);
            }
        }
    }

    if (*formats & (WAVE_FORMAT_1M08  | WAVE_FORMAT_2M08  |
                               WAVE_FORMAT_4M08  | WAVE_FORMAT_48M08 |
                               WAVE_FORMAT_96M08 | WAVE_FORMAT_1M16  |
                               WAVE_FORMAT_2M16  | WAVE_FORMAT_4M16  |
                               WAVE_FORMAT_48M16 | WAVE_FORMAT_96M16) )
        *flags |= DSCAPS_PRIMARYMONO;

    if (*formats & (WAVE_FORMAT_1S08  | WAVE_FORMAT_2S08  |
                               WAVE_FORMAT_4S08  | WAVE_FORMAT_48S08 |
                               WAVE_FORMAT_96S08 | WAVE_FORMAT_1S16  |
                               WAVE_FORMAT_2S16  | WAVE_FORMAT_4S16  |
                               WAVE_FORMAT_48S16 | WAVE_FORMAT_96S16) )
        *flags |= DSCAPS_PRIMARYSTEREO;

    if (*formats & (WAVE_FORMAT_1M08  | WAVE_FORMAT_2M08  |
                               WAVE_FORMAT_4M08  | WAVE_FORMAT_48M08 |
                               WAVE_FORMAT_96M08 | WAVE_FORMAT_1S08  |
                               WAVE_FORMAT_2S08  | WAVE_FORMAT_4S08  |
                               WAVE_FORMAT_48S08 | WAVE_FORMAT_96S08) )
        *flags |= DSCAPS_PRIMARY8BIT;

    if (*formats & (WAVE_FORMAT_1M16  | WAVE_FORMAT_2M16  |
                               WAVE_FORMAT_4M16  | WAVE_FORMAT_48M16 |
                               WAVE_FORMAT_96M16 | WAVE_FORMAT_1S16  |
                               WAVE_FORMAT_2S16  | WAVE_FORMAT_4S16  |
                               WAVE_FORMAT_48S16 | WAVE_FORMAT_96S16) )
        *flags |= DSCAPS_PRIMARY16BIT;

    return(0);
}

/*----------------------------------------------------------------------------
**  ALSA_AddCommonDevice
**
**      Perform Alsa initialization common to both capture and playback
**
**  Side Effect:  ww->pcname and ww->ctlname may need to be freed.
**
**  Note:  this was originally coded by using snd_pcm_name(pcm), until
**         I discovered that with at least one version of alsa lib,
**         the use of a pcm named default:0 would cause snd_pcm_name() to fail.
**         So passing the name in is logically extraneous.  Sigh.
*/
static int ALSA_AddCommonDevice(snd_ctl_t *ctl, snd_pcm_t *pcm, const char *pcmname, WINE_WAVEDEV *ww)
{
    snd_pcm_info_t *infop;

    snd_pcm_info_alloca(&infop);
    ALSA_RETURN_ONFAIL(snd_pcm_info(pcm, infop));

    if (pcm && pcmname)
        ww->pcmname = ALSA_strdup(pcmname);
    else
        return -1;

    if (ctl && snd_ctl_name(ctl))
        ww->ctlname = ALSA_strdup(snd_ctl_name(ctl));

    strcpy(ww->interface_name, "winealsa: ");
    memcpy(ww->interface_name + strlen(ww->interface_name),
            ww->pcmname,
            min(strlen(ww->pcmname), sizeof(ww->interface_name) - strlen("winealsa:   ")));

    strcpy(ww->ds_desc.szDrvname, "winealsa.drv");

    memcpy(ww->ds_desc.szDesc, snd_pcm_info_get_name(infop),
            min( (sizeof(ww->ds_desc.szDesc) - 1), strlen(snd_pcm_info_get_name(infop))) );

    ww->ds_caps.dwMinSecondarySampleRate = DSBFREQUENCY_MIN;
    ww->ds_caps.dwMaxSecondarySampleRate = DSBFREQUENCY_MAX;
    ww->ds_caps.dwPrimaryBuffers = 1;

    return 0;
}

/*----------------------------------------------------------------------------
** ALSA_FreeDevice
*/
static void ALSA_FreeDevice(WINE_WAVEDEV *ww)
{
    HeapFree(GetProcessHeap(), 0, ww->pcmname);
    ww->pcmname = NULL;

    HeapFree(GetProcessHeap(), 0, ww->ctlname);
    ww->ctlname = NULL;
}

/*----------------------------------------------------------------------------
**  ALSA_AddDeviceToArray
**
**      Dynamically size one of the wavein or waveout arrays of devices,
**  and add a fully configured device node to the array.
**
*/
static int ALSA_AddDeviceToArray(WINE_WAVEDEV *ww, WINE_WAVEDEV **array,
        DWORD *count, DWORD *alloced, int isdefault)
{
    int i = *count;

    if (*count >= *alloced)
    {
        (*alloced) += WAVEDEV_ALLOC_EXTENT_SIZE;
        if (! (*array))
            *array = HeapAlloc(GetProcessHeap(), 0, sizeof(*ww) * (*alloced));
        else
            *array = HeapReAlloc(GetProcessHeap(), 0, *array, sizeof(*ww) * (*alloced));

        if (!*array)
        {
            return -1;
        }
    }

    /* If this is the default, arrange for it to be the first element */
    if (isdefault && i > 0)
    {
        (*array)[*count] = (*array)[0];
        i = 0;
    }

    (*array)[i] = *ww;

    (*count)++;
    return 0;
}

/*----------------------------------------------------------------------------
**  ALSA_AddPlaybackDevice
**
**      Add a given Alsa device to Wine's internal list of Playback
**  devices.
*/
static int ALSA_AddPlaybackDevice(snd_ctl_t *ctl, snd_pcm_t *pcm, const char *pcmname, int isdefault)
{
    WINE_WAVEDEV    wwo;
    int rc;

    memset(&wwo, '\0', sizeof(wwo));

    rc = ALSA_AddCommonDevice(ctl, pcm, pcmname, &wwo);
    if (rc)
        return(rc);

    MultiByteToWideChar(CP_ACP, 0, wwo.ds_desc.szDesc, -1,
                        wwo.outcaps.szPname, sizeof(wwo.outcaps.szPname)/sizeof(WCHAR));
    wwo.outcaps.szPname[sizeof(wwo.outcaps.szPname)/sizeof(WCHAR) - 1] = '\0';

    wwo.outcaps.wMid = MM_CREATIVE;
    wwo.outcaps.wPid = MM_CREATIVE_SBP16_WAVEOUT;
    wwo.outcaps.vDriverVersion = 0x0100;

    rc = ALSA_ComputeCaps(ctl, pcm, &wwo.outcaps.wChannels, &wwo.ds_caps.dwFlags,
            &wwo.outcaps.dwFormats, &wwo.outcaps.dwSupport);
    if (rc)
    {
        WARN("Error calculating device caps for pcm [%s]\n", wwo.pcmname);
        ALSA_FreeDevice(&wwo);
        return(rc);
    }

    rc = ALSA_AddDeviceToArray(&wwo, &WOutDev, &ALSA_WodNumDevs, &ALSA_WodNumMallocedDevs, isdefault);
    if (rc)
        ALSA_FreeDevice(&wwo);
    return (rc);
}

/*----------------------------------------------------------------------------
**  ALSA_AddCaptureDevice
**
**      Add a given Alsa device to Wine's internal list of Capture
**  devices.
*/
static int ALSA_AddCaptureDevice(snd_ctl_t *ctl, snd_pcm_t *pcm, const char *pcmname, int isdefault)
{
    WINE_WAVEDEV    wwi;
    int rc;

    memset(&wwi, '\0', sizeof(wwi));

    rc = ALSA_AddCommonDevice(ctl, pcm, pcmname, &wwi);
    if (rc)
        return(rc);

    MultiByteToWideChar(CP_ACP, 0, wwi.ds_desc.szDesc, -1,
                        wwi.incaps.szPname, sizeof(wwi.incaps.szPname) / sizeof(WCHAR));
    wwi.incaps.szPname[sizeof(wwi.incaps.szPname)/sizeof(WCHAR) - 1] = '\0';

    wwi.incaps.wMid = MM_CREATIVE;
    wwi.incaps.wPid = MM_CREATIVE_SBP16_WAVEOUT;
    wwi.incaps.vDriverVersion = 0x0100;

    rc = ALSA_ComputeCaps(ctl, pcm, &wwi.incaps.wChannels, &wwi.ds_caps.dwFlags,
            &wwi.incaps.dwFormats, &wwi.dwSupport);
    if (rc)
    {
        WARN("Error calculating device caps for pcm [%s]\n", wwi.pcmname);
        ALSA_FreeDevice(&wwi);
        return(rc);
    }

    rc = ALSA_AddDeviceToArray(&wwi, &WInDev, &ALSA_WidNumDevs, &ALSA_WidNumMallocedDevs, isdefault);
    if (rc)
        ALSA_FreeDevice(&wwi);
    return(rc);
}

/*----------------------------------------------------------------------------
**  ALSA_CheckEnvironment
**
**      Given an Alsa style configuration node, scan its subitems
**  for environment variable names, and use them to find an override,
**  if appropriate.
**      This is essentially a long and convolunted way of doing:
**          getenv("ALSA_CARD")
**          getenv("ALSA_CTL_CARD")
**          getenv("ALSA_PCM_CARD")
**          getenv("ALSA_PCM_DEVICE")
**
**  The output value is set with the atoi() of the first environment
**  variable found to be set, if any; otherwise, it is left alone
*/
static void ALSA_CheckEnvironment(snd_config_t *node, int *outvalue)
{
    snd_config_iterator_t iter;

    for (iter = snd_config_iterator_first(node);
         iter != snd_config_iterator_end(node);
         iter = snd_config_iterator_next(iter))
    {
        snd_config_t *leaf = snd_config_iterator_entry(iter);
        if (snd_config_get_type(leaf) == SND_CONFIG_TYPE_STRING)
        {
            const char *value;
            if (snd_config_get_string(leaf, &value) >= 0)
            {
                char *p = getenv(value);
                if (p)
                {
                    *outvalue = atoi(p);
                    return;
                }
            }
        }
    }
}

/*----------------------------------------------------------------------------
**  ALSA_DefaultDevices
**
**      Jump through Alsa style hoops to (hopefully) properly determine
**  Alsa defaults for CTL Card #, as well as for PCM Card + Device #.
**  We'll also find out if the user has set any of the environment
**  variables that specify we're to use a specific card or device.
**
**  Parameters:
**      directhw        Whether to use a direct hardware device or not;
**                      essentially switches the pcm device name from
**                      one of 'default:X' or 'plughw:X' to "hw:X"
**      defctlcard      If !NULL, will hold the ctl card number given
**                      by the ALSA config as the default
**      defpcmcard      If !NULL, default pcm card #
**      defpcmdev       If !NULL, default pcm device #
**      fixedctlcard    If !NULL, and the user set the appropriate
**                          environment variable, we'll set to the
**                          card the user specified.
**      fixedpcmcard    If !NULL, and the user set the appropriate
**                          environment variable, we'll set to the
**                          card the user specified.
**      fixedpcmdev     If !NULL, and the user set the appropriate
**                          environment variable, we'll set to the
**                          device the user specified.
**
**  Returns:  0 on success, < 0 on failiure
*/
static int ALSA_DefaultDevices(int directhw,
            long *defctlcard,
            long *defpcmcard, long *defpcmdev,
            int *fixedctlcard,
            int *fixedpcmcard, int *fixedpcmdev)
{
    snd_config_t   *configp;
    char pcmsearch[256];

    ALSA_RETURN_ONFAIL(snd_config_update());

    if (defctlcard)
        if (snd_config_search(snd_config, "defaults.ctl.card", &configp) >= 0)
            snd_config_get_integer(configp, defctlcard);

    if (defpcmcard)
        if (snd_config_search(snd_config, "defaults.pcm.card", &configp) >= 0)
            snd_config_get_integer(configp, defpcmcard);

    if (defpcmdev)
        if (snd_config_search(snd_config, "defaults.pcm.device", &configp) >= 0)
            snd_config_get_integer(configp, defpcmdev);


    if (fixedctlcard)
    {
        if (snd_config_search(snd_config, "ctl.hw.@args.CARD.default.vars", &configp) >= 0)
            ALSA_CheckEnvironment(configp, fixedctlcard);
    }

    if (fixedpcmcard)
    {
        sprintf(pcmsearch, "pcm.%s.@args.CARD.default.vars", directhw ? "hw" : "plughw");
        if (snd_config_search(snd_config, pcmsearch, &configp) >= 0)
            ALSA_CheckEnvironment(configp, fixedpcmcard);
    }

    if (fixedpcmdev)
    {
        sprintf(pcmsearch, "pcm.%s.@args.DEV.default.vars", directhw ? "hw" : "plughw");
        if (snd_config_search(snd_config, pcmsearch, &configp) >= 0)
            ALSA_CheckEnvironment(configp, fixedpcmdev);
    }

    return 0;
}


/*----------------------------------------------------------------------------
**  ALSA_ScanDevices
**
**      Iterate through all discoverable ALSA cards, searching
**  for usable PCM devices.
**
**  Parameters:
**      directhw        Whether to use a direct hardware device or not;
**                      essentially switches the pcm device name from
**                      one of 'default:X' or 'plughw:X' to "hw:X"
**      defctlcard      Alsa's notion of the default ctl card.
**      defpcmcard         . pcm card
**      defpcmdev          . pcm device
**      fixedctlcard    If not -1, then gives the value of ALSA_CTL_CARD
**                          or equivalent environment variable
**      fixedpcmcard    If not -1, then gives the value of ALSA_PCM_CARD
**                          or equivalent environment variable
**      fixedpcmdev     If not -1, then gives the value of ALSA_PCM_DEVICE
**                          or equivalent environment variable
**
**  Returns:  0 on success, < 0 on failiure
*/
static int ALSA_ScanDevices(int directhw,
        long defctlcard, long defpcmcard, long defpcmdev,
        int fixedctlcard, int fixedpcmcard, int fixedpcmdev)
{
    int card = fixedpcmcard;
    int scan_devices = (fixedpcmdev == -1);

    /*------------------------------------------------------------------------
    ** Loop through all available cards
    **----------------------------------------------------------------------*/
    if (card == -1)
        snd_card_next(&card);

    for (; card != -1; snd_card_next(&card))
    {
        char ctlname[256];
        snd_ctl_t *ctl;
        int rc;
        int device;

        /*--------------------------------------------------------------------
        ** Try to open a ctl handle; Wine doesn't absolutely require one,
        **  but it does allow for volume control and for device scanning
        **------------------------------------------------------------------*/
        sprintf(ctlname, "default:%d", fixedctlcard == -1 ? card : fixedctlcard);
        rc = snd_ctl_open(&ctl, ctlname, SND_CTL_NONBLOCK);
        if (rc < 0)
        {
            sprintf(ctlname, "hw:%d", fixedctlcard == -1 ? card : fixedctlcard);
            rc = snd_ctl_open(&ctl, ctlname, SND_CTL_NONBLOCK);
        }
        if (rc < 0)
        {
            ctl = NULL;
            WARN("Unable to open an alsa ctl for [%s] (pcm card %d): %s; not scanning devices\n",
                    ctlname, card, snd_strerror(rc));
            if (fixedpcmdev == -1)
                fixedpcmdev = 0;
        }

        /*--------------------------------------------------------------------
        ** Loop through all available devices on this card
        **------------------------------------------------------------------*/
        device = fixedpcmdev;
        if (device == -1)
            snd_ctl_pcm_next_device(ctl, &device);

        for (; device != -1; snd_ctl_pcm_next_device(ctl, &device))
        {
            char defaultpcmname[256];
            char plugpcmname[256];
            char hwpcmname[256];
            char *pcmname = NULL;
            snd_pcm_t *pcm;

            sprintf(defaultpcmname, "default:%d", card);
            sprintf(plugpcmname,    "plughw:%d,%d", card, device);
            sprintf(hwpcmname,      "hw:%d,%d", card, device);

            /*----------------------------------------------------------------
            ** See if it's a valid playback device
            **--------------------------------------------------------------*/
            if (ALSA_TestDeviceForWine(card, device, SND_PCM_STREAM_PLAYBACK) == 0)
            {
                /* If we can, try the default:X device name first */
                if (! scan_devices && ! directhw)
                {
                    pcmname = defaultpcmname;
                    rc = snd_pcm_open(&pcm, pcmname, SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK);
                }
                else
                    rc = -1;

                if (rc < 0)
                {
                    pcmname = directhw ? hwpcmname : plugpcmname;
                    rc = snd_pcm_open(&pcm, pcmname, SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK);
                }

                if (rc >= 0)
                {
                    if (defctlcard == card && defpcmcard == card && defpcmdev == device)
                        ALSA_AddPlaybackDevice(ctl, pcm, pcmname, TRUE);
                    else
                        ALSA_AddPlaybackDevice(ctl, pcm, pcmname, FALSE);
                    snd_pcm_close(pcm);
                }
                else
                {
                    TRACE("Device [%s/%s] failed to open for playback: %s\n",
                        directhw || scan_devices ? "(N/A)" : defaultpcmname,
                        directhw ? hwpcmname : plugpcmname,
                        snd_strerror(rc));
                }
            }

            /*----------------------------------------------------------------
            ** See if it's a valid capture device
            **--------------------------------------------------------------*/
            if (ALSA_TestDeviceForWine(card, device, SND_PCM_STREAM_CAPTURE) == 0)
            {
                /* If we can, try the default:X device name first */
                if (! scan_devices && ! directhw)
                {
                    pcmname = defaultpcmname;
                    rc = snd_pcm_open(&pcm, pcmname, SND_PCM_STREAM_CAPTURE, SND_PCM_NONBLOCK);
                }
                else
                    rc = -1;

                if (rc < 0)
                {
                    pcmname = directhw ? hwpcmname : plugpcmname;
                    rc = snd_pcm_open(&pcm, pcmname, SND_PCM_STREAM_CAPTURE, SND_PCM_NONBLOCK);
                }

                if (rc >= 0)
                {
                    if (defctlcard == card && defpcmcard == card && defpcmdev == device)
                        ALSA_AddCaptureDevice(ctl, pcm, pcmname, TRUE);
                    else
                        ALSA_AddCaptureDevice(ctl, pcm, pcmname, FALSE);

                    snd_pcm_close(pcm);
                }
                else
                {
                    TRACE("Device [%s/%s] failed to open for capture: %s\n",
                        directhw || scan_devices ? "(N/A)" : defaultpcmname,
                        directhw ? hwpcmname : plugpcmname,
                        snd_strerror(rc));
                }
            }

            if (! scan_devices)
                break;
        }

        if (ctl)
            snd_ctl_close(ctl);

        /*--------------------------------------------------------------------
        ** If the user has set env variables such that we're pegged to
        **  a specific card, then break after we've examined it
        **------------------------------------------------------------------*/
        if (fixedpcmcard != -1)
            break;
    }

    return 0;

}

/*----------------------------------------------------------------------------
** ALSA_PerformDefaultScan
**  Perform the basic default scanning for devices within ALSA.
**  The hope is that this routine implements a 'correct'
**  scanning algorithm from the Alsalib point of view.
**
**      Note that Wine, overall, has other mechanisms to
**  override and specify exact CTL and PCM device names,
**  but this routine is imagined as the default that
**  99% of users will use.
**
**      The basic algorithm is simple:
**  Use snd_card_next to iterate cards; within cards, use
**  snd_ctl_pcm_next_device to iterate through devices.
**
**      We add a little complexity by taking into consideration
**  environment variables such as ALSA_CARD (et all), and by
**  detecting when a given device matches the default specified
**  by Alsa.
**
**  Parameters:
**      directhw        If !0, indicates we should use the hw:X
**                      PCM interface, rather than first try
**                      the 'default' device followed by the plughw
**                      device.  (default and plughw do fancy mixing
**                      and audio scaling, if they are available).
**      devscan         If TRUE, we should scan all devices, not
**                      juse use device 0 on each card
**
**  Returns:
**      0   on success
**
**  Effects:
**      Invokes the ALSA_AddXXXDevice functions on valid
**  looking devices
*/
static int ALSA_PerformDefaultScan(int directhw, BOOL devscan)
{
    long defctlcard = -1, defpcmcard = -1, defpcmdev = -1;
    int fixedctlcard = -1, fixedpcmcard = -1, fixedpcmdev = -1;
    int rc;

    /* FIXME:  We should dlsym the new snd_names_list/snd_names_list_free 1.0.9 apis,
    **          and use them instead of this scan mechanism if they are present         */

    rc = ALSA_DefaultDevices(directhw, &defctlcard, &defpcmcard, &defpcmdev,
            &fixedctlcard, &fixedpcmcard, &fixedpcmdev);
    if (rc)
        return(rc);

    if (fixedpcmdev == -1 && ! devscan)
        fixedpcmdev = 0;

    return(ALSA_ScanDevices(directhw, defctlcard, defpcmcard, defpcmdev, fixedctlcard, fixedpcmcard, fixedpcmdev));
}


/*----------------------------------------------------------------------------
** ALSA_AddUserSpecifiedDevice
**  Add a device given from the registry
*/
static int ALSA_AddUserSpecifiedDevice(const char *ctlname, const char *pcmname)
{
    int rc;
    int okay = 0;
    snd_ctl_t *ctl = NULL;
    snd_pcm_t *pcm = NULL;

    if (ctlname)
    {
        rc = snd_ctl_open(&ctl, ctlname, SND_CTL_NONBLOCK);
        if (rc < 0)
            ctl = NULL;
    }

    rc = snd_pcm_open(&pcm, pcmname, SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK);
    if (rc >= 0)
    {
        ALSA_AddPlaybackDevice(ctl, pcm, pcmname, FALSE);
        okay++;
        snd_pcm_close(pcm);
    }

    rc = snd_pcm_open(&pcm, pcmname, SND_PCM_STREAM_CAPTURE, SND_PCM_NONBLOCK);
    if (rc >= 0)
    {
        ALSA_AddCaptureDevice(ctl, pcm, pcmname, FALSE);
        okay++;
        snd_pcm_close(pcm);
    }

    if (ctl)
        snd_ctl_close(ctl);

    return (okay == 0);
}


/*----------------------------------------------------------------------------
** ALSA_WaveInit
**  Initialize the Wine Alsa sub system.
** The main task is to probe for and store a list of all appropriate playback
** and capture devices.
**  Key control points are from the registry key:
**  [Software\Wine\Alsa Driver]
**  AutoScanCards           Whether or not to scan all known sound cards
**                          and add them to Wine's list (default yes)
**  AutoScanDevices         Whether or not to scan all known PCM devices
**                          on each card (default no)
**  UseDirectHW             Whether or not to use the hw:X device,
**                          instead of the fancy default:X or plughw:X device.
**                          The hw:X device goes straight to the hardware
**                          without any fancy mixing or audio scaling in between.
**  DeviceCount             If present, specifies the number of hard coded
**                          Alsa devices to add to Wine's list; default 0
**  DevicePCMn              Specifies the Alsa PCM devices to open for
**                          Device n (where n goes from 1 to DeviceCount)
**  DeviceCTLn              Specifies the Alsa control devices to open for
**                          Device n (where n goes from 1 to DeviceCount)
**
**                          Using AutoScanCards no, and then Devicexxx info
**                          is a way to exactly specify the devices used by Wine.
**
*/
LONG ALSA_WaveInit(void)
{
    DWORD rc;
    BOOL  AutoScanCards = TRUE;
    BOOL  AutoScanDevices = FALSE;
    BOOL  UseDirectHW = FALSE;
    DWORD DeviceCount = 0;
    HKEY  key = 0;
    int   i;

    if (!wine_dlopen("libasound.so.2", RTLD_LAZY|RTLD_GLOBAL, NULL, 0))
    {
        ERR("Error: ALSA lib needs to be loaded with flags RTLD_LAZY and RTLD_GLOBAL.\n");
        return -1;
    }

    /* @@ Wine registry key: HKCU\Software\Wine\Alsa Driver */
    rc = RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\Wine\\Alsa Driver", 0, KEY_QUERY_VALUE, &key);
    if (rc == ERROR_SUCCESS)
    {
        ALSA_RegGetBoolean(key, "AutoScanCards", &AutoScanCards);
        ALSA_RegGetBoolean(key, "AutoScanDevices", &AutoScanDevices);
        ALSA_RegGetBoolean(key, "UseDirectHW", &UseDirectHW);
        ALSA_RegGetInt(key, "DeviceCount", &DeviceCount);
    }

    if (AutoScanCards)
        rc = ALSA_PerformDefaultScan(UseDirectHW, AutoScanDevices);

    for (i = 0; i < DeviceCount; i++)
    {
        char *ctl_name = NULL;
        char *pcm_name = NULL;
        char value[30];

        sprintf(value, "DevicePCM%d", i + 1);
        if (ALSA_RegGetString(key, value, &pcm_name) == ERROR_SUCCESS)
        {
            sprintf(value, "DeviceCTL%d", i + 1);
            ALSA_RegGetString(key, value, &ctl_name);
            ALSA_AddUserSpecifiedDevice(ctl_name, pcm_name);
        }

        HeapFree(GetProcessHeap(), 0, ctl_name);
        HeapFree(GetProcessHeap(), 0, pcm_name);
    }

    if (key)
        RegCloseKey(key);

    return (rc);
}

#endif /* HAVE_ALSA */
