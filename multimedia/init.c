/*
 * Initialization procedures for multimedia
 *
 * Copyright 1998 Luiz Otavio L. Zorzella
 */

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "windows.h"
#include "multimedia.h"
#include "mmsystem.h"
#include "xmalloc.h"
#include "debug.h"

#ifdef HAVE_OSS

extern int MODM_NUMDEVS;
extern LPMIDIOUTCAPS16 midiDevices[MAX_MIDIOUTDRV];

#endif

/**************************************************************************
 * 			unixToWindowsDeviceType  		[internal]
 *
 * return the Windows equivalent to a Unix Device Type
 *
 */
#ifdef HAVE_OSS
int unixToWindowsDeviceType(int type)
{
  /* MOD_MIDIPORT     output port 
   * MOD_SYNTH        generic internal synth 
   * MOD_SQSYNTH      square wave internal synth 
   * MOD_FMSYNTH      FM internal synth 
   * MOD_MAPPER       MIDI mapper
   */

  /* FIXME Is this really the correct equivalence from UNIX to Windows Sound type */

  switch (type) {
  case SYNTH_TYPE_FM:     return MOD_FMSYNTH;
  case SYNTH_TYPE_SAMPLE: return MOD_SYNTH;
  case SYNTH_TYPE_MIDI:   return MOD_MIDIPORT;
  default:
    ERR(midi, "Cannot determine the type of this midi device. Assuming FM Synth\n");
    return MOD_FMSYNTH;
  }
}
#endif

/**************************************************************************
 * 			MultimediaInit				[internal]
 *
 * Initializes the MIDI devices information variables
 *
 */
BOOL32 MULTIMEDIA_Init (void)
{
#ifdef HAVE_OSS
  int i, status, numsynthdevs, nummididevs;
  struct synth_info sinfo;
  struct midi_info minfo;
  int fd;        /* file descriptor for MIDI_DEV */

  TRACE (midi, "Initializing the MIDI variables.\n");
  /* try to open device */
  fd = open(MIDI_DEV, O_WRONLY);
  if (fd == -1) {
    TRACE (midi, "No soundcards founds: unable to open `%s'.\n", MIDI_DEV);
    return TRUE;
  }

  /* find how many Synth devices are there in the system */
  status = ioctl(fd, SNDCTL_SEQ_NRSYNTHS, &numsynthdevs);

  if (numsynthdevs > MAX_MIDIOUTDRV) {
    ERR (midi, "MAX_MIDIOUTDRV was enough for the number of devices. Some FM devices will not be available.\n");
    numsynthdevs = MAX_MIDIOUTDRV;
  }
  
  if (status == -1) {
    ERR (midi, "ioctl failed.\n");
    return TRUE;
  }

  for (i = 0 ; i < numsynthdevs ; i++) {
    LPMIDIOUTCAPS16 tmplpCaps;

    sinfo.device = i;
    status = ioctl(fd, SNDCTL_SYNTH_INFO, &sinfo);
    if (status == -1) {
      ERR(midi, "ioctl failed.\n");
      return TRUE;
    }

    tmplpCaps = xmalloc (sizeof (MIDIOUTCAPS16));
    /* We also have the information sinfo.synth_subtype, not used here
     */
    
    /* Manufac ID. We do not have access to this with soundcard.h
     * Does not seem to be a problem, because in mmsystem.h only
     * Microsoft's ID is listed.
     */
    tmplpCaps->wMid = 0x00FF; 
    tmplpCaps->wPid = 0x0001; 	/* FIXME Product ID  */
    tmplpCaps->vDriverVersion = 0x001; /* Product Version. We simply say "1" */
    strcpy(tmplpCaps->szPname, sinfo.name);

    tmplpCaps->wTechnology = unixToWindowsDeviceType (sinfo.synth_type);
    tmplpCaps->wVoices     = sinfo.nr_voices;

    /* FIXME Is it possible to know the maximum
     * number of simultaneous notes of a soundcard ?
     * I beleive we don't have this information, but
     * it's probably equal or more than wVoices
     */
    tmplpCaps->wNotes      = sinfo.nr_voices;  

    /* FIXME Do we have this information?
     * Assuming the soundcards can handle
     * MIDICAPS_VOLUME and MIDICAPS_LRVOLUME but
     * not MIDICAPS_CACHE.
     */
    tmplpCaps->dwSupport   = MIDICAPS_VOLUME|MIDICAPS_LRVOLUME;
    
    midiDevices[i] = tmplpCaps;
    
    TRACE(midi,"techn = %d voices=%d notes = %d support = %ld\n",tmplpCaps->wTechnology,tmplpCaps->wVoices,tmplpCaps->wNotes,tmplpCaps->dwSupport);
  }
  
  /* find how many MIDI devices are there in the system */
  status = ioctl(fd, SNDCTL_SEQ_NRMIDIS, &nummididevs);
  if (status == -1) {
    ERR(midi, "ioctl failed.\n");
    return TRUE;
  }

  if (numsynthdevs + nummididevs > MAX_MIDIOUTDRV) {
    ERR(midi, "MAX_MIDIOUTDRV was enough for the number of devices. Some MIDI devices will not be available.\n");
    nummididevs = MAX_MIDIOUTDRV - numsynthdevs;
  }

  /* windows does not seem to diferentiate Synth from MIDI devices */
  MODM_NUMDEVS = numsynthdevs + nummididevs;
  
  for (i = 0 ; i < nummididevs ; i++) {
    LPMIDIOUTCAPS16 tmplpCaps;

    minfo.device = i;
    status = ioctl(fd, SNDCTL_MIDI_INFO, &minfo);
    if (status == -1) {
      ERR(midi, "ioctl failed.\n");
      return TRUE;
    }

    tmplpCaps = xmalloc (sizeof (MIDIOUTCAPS16));
    /* This whole part is somewhat obscure to me. I'll keep trying to dig
       info about it. If you happen to know, please tell us. The very descritive
       minfo.dev_type was not used here.
     */
    tmplpCaps->wMid = 0x00FF; 	/* Manufac ID. We do not have access to this with soundcard.h
				   Does not seem to be a problem, because in mmsystem.h only
				   Microsoft's ID is listed */
    tmplpCaps->wPid = 0x0001; 	/* FIXME Product ID */
    tmplpCaps->vDriverVersion = 0x001; /* Product Version. We simply say "1" */
    strcpy(tmplpCaps->szPname, minfo.name);

    tmplpCaps->wTechnology =  MOD_MIDIPORT; /* FIXME Is this right? */
    tmplpCaps->wVoices     = 16;            /* Does it make any difference? */
    tmplpCaps->wNotes      = 16;            /* Does it make any difference? */
    tmplpCaps->dwSupport   = MIDICAPS_VOLUME|MIDICAPS_LRVOLUME; /* FIXME Does it make any difference? */

    midiDevices[numsynthdevs + i] = tmplpCaps;

    TRACE(midi,"techn = %d voices=%d notes = %d support = %ld\n",tmplpCaps->wTechnology,tmplpCaps->wVoices,tmplpCaps->wNotes,tmplpCaps->dwSupport);
  }
  
  /* close file and exit */
  close(fd);

#endif /* HAVE_OSS */
  
  return TRUE;
  
}
