/* -*- tab-width: 8; c-basic-offset: 4 -*- */

/*
 * Initialization procedures for multimedia
 *
 * Copyright 1998 Luiz Otavio L. Zorzella
 */

#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "winbase.h"
#include "multimedia.h"
#include "xmalloc.h"
#include "options.h"
#include "debugtools.h"

DECLARE_DEBUG_CHANNEL(mci)
DECLARE_DEBUG_CHANNEL(midi)
DECLARE_DEBUG_CHANNEL(mmsys)

#ifdef HAVE_OSS

extern int 		MODM_NUMDEVS;
extern int		MODM_NUMFMSYNTHDEVS;
extern int		MODM_NUMMIDIDEVS;
extern LPMIDIOUTCAPS16	midiOutDevices[MAX_MIDIOUTDRV];

extern int		MIDM_NUMDEVS;
extern LPMIDIINCAPS16	midiInDevices [MAX_MIDIINDRV];

#endif

/**************************************************************************
 * 			unixToWindowsDeviceType  		[internal]
 *
 * return the Windows equivalent to a Unix Device Type
 *
 */
#ifdef HAVE_OSS
static	int unixToWindowsDeviceType(int type)
{
#if !defined(__NetBSD__) && !defined(__OpenBSD__)
    /* MOD_MIDIPORT     output port 
     * MOD_SYNTH        generic internal synth 
     * MOD_SQSYNTH      square wave internal synth 
     * MOD_FMSYNTH      FM internal synth 
     * MOD_MAPPER       MIDI mapper
     */
    
    /* FIXME Is this really the correct equivalence from UNIX to 
       Windows Sound type */
    
    switch (type) {
    case SYNTH_TYPE_FM:     return MOD_FMSYNTH;
    case SYNTH_TYPE_SAMPLE: return MOD_SYNTH;
    case SYNTH_TYPE_MIDI:   return MOD_MIDIPORT;
    default:
	ERR_(midi)("Cannot determine the type of this midi device. "
	    "Assuming FM Synth\n");
	return MOD_FMSYNTH;
    }
#else
    return MOD_FMSYNTH;
#endif
}
#endif

/**************************************************************************
 * 			MULTIMEDIA_MidiInit			[internal]
 *
 * Initializes the MIDI devices information variables
 *
 */
static	BOOL MULTIMEDIA_MidiInit(void)
{
#if defined(HAVE_OSS) && !defined(__NetBSD__) && !defined(__OpenBSD__)
    int 		i, status, numsynthdevs = 255, nummididevs = 255;
    struct synth_info 	sinfo;
    struct midi_info 	minfo;
    int 		fd;        /* file descriptor for MIDI_SEQ */
    
    TRACE_(midi)("Initializing the MIDI variables.\n");
    
    /* try to open device */
    /* FIXME: should use function midiOpenSeq() in midi.c */
    fd = open(MIDI_SEQ, O_WRONLY);
    if (fd == -1) {
	TRACE_(midi)("No sequencer found: unable to open `%s'.\n", MIDI_SEQ);
	return TRUE;
    }
    
    /* find how many Synth devices are there in the system */
    status = ioctl(fd, SNDCTL_SEQ_NRSYNTHS, &numsynthdevs);
    
    if (status == -1) {
	ERR_(midi)("ioctl for nr synth failed.\n");
	close(fd);
	return TRUE;
    }

    if (numsynthdevs > MAX_MIDIOUTDRV) {
	ERR_(midi)("MAX_MIDIOUTDRV (%d) was enough for the number of devices (%d). "
	    "Some FM devices will not be available.\n",MAX_MIDIOUTDRV,numsynthdevs);
	numsynthdevs = MAX_MIDIOUTDRV;
    }
    
    for (i = 0; i < numsynthdevs; i++) {
	LPMIDIOUTCAPS16 tmplpCaps;
	
	sinfo.device = i;
	status = ioctl(fd, SNDCTL_SYNTH_INFO, &sinfo);
	if (status == -1) {
	    ERR_(midi)("ioctl for synth info failed.\n");
	    close(fd);
	    return TRUE;
	}
	
	tmplpCaps = xmalloc(sizeof(MIDIOUTCAPS16));
	/* We also have the information sinfo.synth_subtype, not used here
	 */
	
	/* Manufac ID. We do not have access to this with soundcard.h
	 * Does not seem to be a problem, because in mmsystem.h only
	 * Microsoft's ID is listed.
	 */
	tmplpCaps->wMid = 0x00FF; 
	tmplpCaps->wPid = 0x0001; 	/* FIXME Product ID  */
	/* Product Version. We simply say "1" */
	tmplpCaps->vDriverVersion = 0x001; 
	strcpy(tmplpCaps->szPname, sinfo.name);
	
	tmplpCaps->wTechnology = unixToWindowsDeviceType(sinfo.synth_type);
	tmplpCaps->wVoices     = sinfo.nr_voices;
	
	/* FIXME Is it possible to know the maximum
	 * number of simultaneous notes of a soundcard ?
	 * I believe we don't have this information, but
	 * it's probably equal or more than wVoices
	 */
	tmplpCaps->wNotes      = sinfo.nr_voices;  
	
	/* FIXME Do we have this information?
	 * Assuming the soundcards can handle
	 * MIDICAPS_VOLUME and MIDICAPS_LRVOLUME but
	 * not MIDICAPS_CACHE.
	 */
	tmplpCaps->dwSupport   = MIDICAPS_VOLUME|MIDICAPS_LRVOLUME;
	
	midiOutDevices[i] = tmplpCaps;
	
	if (sinfo.capabilities & SYNTH_CAP_INPUT) {
	    FIXME_(midi)("Synthetizer support MIDI in. Not supported yet (please report)\n");
	}
	
	TRACE_(midi)("name='%s', techn=%d voices=%d notes=%d support=%ld\n", 
	      tmplpCaps->szPname, tmplpCaps->wTechnology,
	      tmplpCaps->wVoices, tmplpCaps->wNotes, tmplpCaps->dwSupport);
	TRACE_(midi)("OSS info: synth subtype=%d capa=%Xh\n", 
	      sinfo.synth_subtype, sinfo.capabilities);
    }
    
    /* find how many MIDI devices are there in the system */
    status = ioctl(fd, SNDCTL_SEQ_NRMIDIS, &nummididevs);
    if (status == -1) {
	ERR_(midi)("ioctl on nr midi failed.\n");
	return TRUE;
    }
    
    /* FIXME: the two restrictions below could be loosen in some cases */
    if (numsynthdevs + nummididevs > MAX_MIDIOUTDRV) {
	ERR_(midi)("MAX_MIDIOUTDRV was not enough for the number of devices. "
	    "Some MIDI devices will not be available.\n");
	nummididevs = MAX_MIDIOUTDRV - numsynthdevs;
    }
    
    if (nummididevs > MAX_MIDIINDRV) {
	ERR_(midi)("MAX_MIDIINDRV (%d) was not enough for the number of devices (%d). "
	    "Some MIDI devices will not be available.\n",MAX_MIDIINDRV,nummididevs);
	nummididevs = MAX_MIDIINDRV;
    }
    
    for (i = 0; i < nummididevs; i++) {
	LPMIDIOUTCAPS16 tmplpOutCaps;
	LPMIDIINCAPS16  tmplpInCaps;
	
	minfo.device = i;
	status = ioctl(fd, SNDCTL_MIDI_INFO, &minfo);
	if (status == -1) {
	    ERR_(midi)("ioctl on midi info failed.\n");
	    close(fd);
	    return TRUE;
	}
	
	tmplpOutCaps = xmalloc(sizeof(MIDIOUTCAPS16));
	/* This whole part is somewhat obscure to me. I'll keep trying to dig
	   info about it. If you happen to know, please tell us. The very 
	   descritive minfo.dev_type was not used here.
	*/
	/* Manufac ID. We do not have access to this with soundcard.h
	   Does not seem to be a problem, because in mmsystem.h only
	   Microsoft's ID is listed */
	tmplpOutCaps->wMid = 0x00FF; 	
	tmplpOutCaps->wPid = 0x0001; 	/* FIXME Product ID */
	/* Product Version. We simply say "1" */
	tmplpOutCaps->vDriverVersion = 0x001; 
	strcpy(tmplpOutCaps->szPname, minfo.name);
	
	tmplpOutCaps->wTechnology = MOD_MIDIPORT; /* FIXME Is this right? */
	/* Does it make any difference? */
	tmplpOutCaps->wVoices     = 16;            
	/* Does it make any difference? */
	tmplpOutCaps->wNotes      = 16;
	/* FIXME Does it make any difference? */
	tmplpOutCaps->dwSupport   = MIDICAPS_VOLUME|MIDICAPS_LRVOLUME; 
	
	midiOutDevices[numsynthdevs + i] = tmplpOutCaps;
	
	tmplpInCaps = xmalloc(sizeof(MIDIOUTCAPS16));
	/* This whole part is somewhat obscure to me. I'll keep trying to dig
	   info about it. If you happen to know, please tell us. The very 
	   descritive minfo.dev_type was not used here.
	*/
	/* Manufac ID. We do not have access to this with soundcard.h
	   Does not seem to be a problem, because in mmsystem.h only
	   Microsoft's ID is listed */
	tmplpInCaps->wMid = 0x00FF; 	
	tmplpInCaps->wPid = 0x0001; 	/* FIXME Product ID */
	/* Product Version. We simply say "1" */
	tmplpInCaps->vDriverVersion = 0x001; 
	strcpy(tmplpInCaps->szPname, minfo.name);
	
	/* FIXME : could we get better information than that ? */
	tmplpInCaps->dwSupport   = MIDICAPS_VOLUME|MIDICAPS_LRVOLUME; 
	
	midiInDevices[i] = tmplpInCaps;
	
	TRACE_(midi)("name='%s' techn=%d voices=%d notes=%d support=%ld\n",
	      tmplpOutCaps->szPname, tmplpOutCaps->wTechnology, tmplpOutCaps->wVoices,
	      tmplpOutCaps->wNotes, tmplpOutCaps->dwSupport);
	TRACE_(midi)("OSS info: midi dev-type=%d, capa=%d\n", 
	      minfo.dev_type, minfo.capabilities);
    }
    
    /* windows does not seem to differentiate Synth from MIDI devices */
    MODM_NUMFMSYNTHDEVS = numsynthdevs;
    MODM_NUMMIDIDEVS    = nummididevs;
    MODM_NUMDEVS        = numsynthdevs + nummididevs;
    
    MIDM_NUMDEVS        = nummididevs;
    
    /* close file and exit */
    close(fd);	
#endif /* HAVE_OSS */
    
    return TRUE;
}

/**************************************************************************
 * 			MULTIMEDIA_MciInit			[internal]
 *
 * Initializes the MCI internal variables.
 *
 */static	BOOL MULTIMEDIA_MciInit(void)
{
    LPSTR	ptr1, ptr2;

    mciInstalledCount = 0;
    ptr1 = lpmciInstallNames = xmalloc(2048);

    /* FIXME: should do also some registry diving here */
    if (PROFILE_GetWineIniString("options", "mci", "", lpmciInstallNames, 2048) > 0) {
	TRACE_(mci)("Wine => '%s' \n", ptr1);
	while ((ptr2 = strchr(ptr1, ':')) != 0) {
	    *ptr2++ = 0;
	    TRACE_(mci)("---> '%s' \n", ptr1);
	    mciInstalledCount++;
	    ptr1 = ptr2;
	}
	mciInstalledCount++;
	TRACE_(mci)("---> '%s' \n", ptr1);
	ptr1 += strlen(ptr1) + 1;
    } else {
	GetPrivateProfileStringA("mci", NULL, "", lpmciInstallNames, 2048, "SYSTEM.INI");
	while (strlen(ptr1) > 0) {
	    TRACE_(mci)("---> '%s' \n", ptr1);
	    ptr1 += (strlen(ptr1) + 1);
	    mciInstalledCount++;
	}
    }
    mciInstalledListLen = ptr1 - lpmciInstallNames;

    return TRUE;
}

HINSTANCE	WINMM_hInstance = 0; 
HINSTANCE	MMSYSTEM_hInstance = 0; 
static BOOL     bInitDone = FALSE;

/**************************************************************************
 * 			WINMM_LibMain				[EntryPoint]
 *
 * WINMM DLL entry point
 *
 */
BOOL WINAPI WINMM_LibMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID fImpLoad)
{
    TRACE_(mmsys)("0x%x 0x%lx %p\n", hinstDLL, fdwReason, fImpLoad);

    switch (fdwReason) {
    case DLL_PROCESS_ATTACH:
	if (!bInitDone) {
	    if (MULTIMEDIA_MidiInit() && MULTIMEDIA_MciInit()) {
		bInitDone = TRUE;
	    } else {
		return FALSE;
	    }
	}
	WINMM_hInstance = hinstDLL;
	break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
	break;
    }
    return TRUE;
}

BOOL WINAPI MMSYSTEM_LibMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID fImpLoad)
{
    TRACE_(mmsys)("0x%x 0x%lx %p\n", hinstDLL, fdwReason, fImpLoad);

    switch (fdwReason) {
    case DLL_PROCESS_ATTACH:
	if (!bInitDone) {
	    if (MULTIMEDIA_MidiInit() && MULTIMEDIA_MciInit()) {
		bInitDone = TRUE;
	    } else {
		return FALSE;
	    }
	}
	MMSYSTEM_hInstance = hinstDLL;
	break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
	break;
    }
    return TRUE;
}
