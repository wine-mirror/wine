/* Helper functions for dmusic file handling
 *
 * Copyright (C) 2003 Rok Mandeljc
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
#if 0
#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "wingdi.h"
#include "wine/debug.h"
#include "wine/unicode.h"

#include "dmusic_private.h"

/* used while still in testing */
WINE_DEFAULT_DEBUG_CHANNEL(dmfile);
WINE_DECLARE_DEBUG_CHANNEL(dmfiledat);

/******************************************************************************
 * DMUSIC_FillUNFOFromFileHandle: 
 *	- fills a UNFO_List struct (dmusic_private.h) with data from file handle. 
 *	- IMPORTANT: it expects a LIST chunk at beginning, so if you are calling it 
 *	             from another DMUSIC_Fill* function, make sure pointer is at
 *               correct place!
 */
HRESULT WINAPI DMUSIC_FillUNFOFromFileHandle (UNFO_List UNFO, HANDLE fd)
{
	rawChunk chunk;
	DWORD BytesRead, ListCount = 0, ListSize;

	TRACE("reading 'LIST' chunk...\n");
	ReadFile (fd, &chunk.id, sizeof(FOURCC), &BytesRead, NULL); /* read 'LIST' */
	ReadFile (fd, &chunk.size, sizeof(DWORD), &BytesRead, NULL); /* read size of 'LIST' chunk */
	if (chunk.id == FOURCC_LIST && 	ReadFile (fd, &chunk.id, sizeof(FOURCC), &BytesRead, NULL) && chunk.id == DMUS_FOURCC_UNFO_LIST) {
		TRACE("'UNFO': UNFO list\n");
		ListSize = chunk.size - sizeof(FOURCC); /* list contents size is same as size of LIST chunk - size of following field ID*/
		do {
			ReadFile (fd, &chunk.id, sizeof(FOURCC), &BytesRead, NULL); /* read ID of following field */
			ReadFile (fd, &chunk.size, sizeof(DWORD), &BytesRead, NULL); /* read size of following field */
			ListCount += chunk.size + sizeof(DWORD) + sizeof(FOURCC);
			switch (chunk.id)
			{
				case DMUS_FOURCC_UNAM_CHUNK: {
					TRACE("'UNAM': name\n");
					UNFO.name = (WCHAR*) HeapAlloc (GetProcessHeap (), 0, chunk.size); /* allocate space */
					ReadFile (fd, UNFO.name, chunk.size, &BytesRead, NULL);
					TRACE_(dmfiledat)("=> name = %s\n", debugstr_w(UNFO.name));
					break;
				} case DMUS_FOURCC_UART_CHUNK: {
					TRACE("'UART': artist\n");
					UNFO.artist = (WCHAR*) HeapAlloc (GetProcessHeap (), 0, chunk.size); /* allocate space */
					ReadFile (fd, UNFO.artist, chunk.size, &BytesRead, NULL);
					TRACE_(dmfiledat)("artist = %s\n", debugstr_w(UNFO.artist));
					break;
				} case DMUS_FOURCC_UCOP_CHUNK: {
					TRACE("'UCOP': copyright\n");
					UNFO.copyright = (WCHAR*) HeapAlloc (GetProcessHeap (), 0, chunk.size); /* allocate space */
					ReadFile (fd, UNFO.copyright, chunk.size, &BytesRead, NULL);
					TRACE_(dmfiledat)("=> copyright = %s\n", debugstr_w(UNFO.copyright));
					break;
				} case DMUS_FOURCC_USBJ_CHUNK:{
					TRACE("'USBJ': subject\n");
					UNFO.subject = (WCHAR*) HeapAlloc (GetProcessHeap (), 0, chunk.size); /* allocate space */
					ReadFile (fd, UNFO.subject, chunk.size, &BytesRead, NULL);
					TRACE_(dmfiledat)("=> subject = %s\n", debugstr_w(UNFO.subject));
					break;
				} case DMUS_FOURCC_UCMT_CHUNK: {
					TRACE("'UCMT': comment\n");
					UNFO.comment = (WCHAR*) HeapAlloc (GetProcessHeap (), 0, chunk.size); /* allocate space */
					ReadFile (fd, UNFO.comment, chunk.size, &BytesRead, NULL);
					TRACE_(dmfiledat)("=> comment = %s\n", debugstr_w(UNFO.comment));
					break;
				} default: {
					WARN("invalid chunk (only 'UNAM', 'UART', 'UCOP', 'USBJ', 'UCMT' allowed)\n");
					break;
				}
			}
			TRACE("ListCount (%ld) < ListSize(%ld)\n", ListCount, ListSize);
		} while (ListCount < ListSize);
	} else {
		WARN("'UNFO' not found: not an UNFO list\n");
	}		
	return S_OK;	
}

/******************************************************************************
 * DMUSIC_FillReferenceFromFileHandle: 
 *	- fills a Reference struct (dmusic_private.h) with data from file handle. 
 *	- IMPORTANT: it expects a LIST chunk at beginning, so if you are calling it 
 *	             from another DMUSIC_Fill* function, make sure pointer is at
 *               correct place!
 */
HRESULT WINAPI DMUSIC_FillReferenceFromFileHandle (Reference reference, HANDLE fd)
{
	rawChunk chunk;
	DWORD BytesRead, ListCount = 0, ListSize;

	TRACE("reading 'LIST' chunk...\n");
	ReadFile (fd, &chunk.id, sizeof(FOURCC), &BytesRead, NULL); /* read 'LIST' */
	ReadFile (fd, &chunk.size, sizeof(DWORD), &BytesRead, NULL); /* read size of 'LIST' chunk */
	if (chunk.id == FOURCC_LIST && 	ReadFile (fd, &chunk.id, sizeof(FOURCC), &BytesRead, NULL) && chunk.id == DMUS_FOURCC_REF_LIST) {
		TRACE("'DMRF': reference list\n");
		ListSize = chunk.size - sizeof(FOURCC); /* list contents size is same as size of LIST chunk - size of following field ID*/
		do {
			ReadFile (fd, &chunk.id, sizeof(FOURCC), &BytesRead, NULL); /* read ID of following field */
			ReadFile (fd, &chunk.size, sizeof(DWORD), &BytesRead, NULL); /* read size of following field */				
			ListCount += chunk.size + sizeof(DWORD) + sizeof(FOURCC);
			switch (chunk.id)
			{
				case DMUS_FOURCC_REF_CHUNK: {
					TRACE("'refh': reference header\n");
					ReadFile (fd, &reference.header, chunk.size, &BytesRead, NULL);
					TRACE_(dmfiledat)("=> guidClassID = %s; dwValidData = %ld\n", debugstr_guid (&reference.header.guidClassID), reference.header.dwValidData);
					break;
				} case DMUS_FOURCC_GUID_CHUNK: {
					TRACE("'guid': GUID\n");
					ReadFile (fd, &reference.guid, chunk.size, &BytesRead, NULL);
					TRACE_(dmfiledat)("=> GUID = %s\n", debugstr_guid (&reference.guid));
					break;
				} case DMUS_FOURCC_DATE_CHUNK: {
					TRACE("'date': file date\n");
					ReadFile (fd, &reference.date, chunk.size, &BytesRead, NULL);
					TRACE_(dmfiledat)("=> file date = %ld%ld\n", reference.date.dwHighDateTime, reference.date.dwLowDateTime);
					break;
				} case DMUS_FOURCC_NAME_CHUNK: {
					TRACE("'name': name\n");
					reference.name = (WCHAR*) HeapAlloc (GetProcessHeap (), 0, chunk.size);
					ReadFile (fd, reference.name, chunk.size, &BytesRead, NULL);
					TRACE_(dmfiledat)("=> name = %s\n", debugstr_w (reference.name));
					break;
				} case DMUS_FOURCC_FILE_CHUNK: {
					TRACE("'file': file name\n");
					reference.file = (WCHAR*) HeapAlloc (GetProcessHeap (), 0, chunk.size);
					ReadFile (fd, reference.file, chunk.size, &BytesRead, NULL);
					TRACE_(dmfiledat)("=> file name = %s\n", debugstr_w (reference.file));
					break;
				} case DMUS_FOURCC_CATEGORY_CHUNK: {
					TRACE("'catg': category\n");
					reference.category = (WCHAR*) HeapAlloc (GetProcessHeap (), 0, chunk.size);
					ReadFile (fd, reference.category, chunk.size, &BytesRead, NULL);
					TRACE_(dmfiledat)("=> category = %s\n", debugstr_w (reference.category));
					break;
				} case DMUS_FOURCC_VERSION_CHUNK: {
					TRACE("'vers': version\n");
					ReadFile (fd, &reference.version, sizeof(DMUS_IO_VERSION), &BytesRead, NULL);
					TRACE_(dmfiledat)("=> version = %ld%ld\n", reference.version.dwVersionMS, reference.version.dwVersionLS);				
					break;
				} default: {
					WARN("invalid chunk (only 'refh, 'guid', 'date', 'name', 'file', 'catg', 'vers' allowed\n");
					break;
				}	
			}
			TRACE("ListCount (%ld) < ListSize (%ld)\n", ListCount, ListSize);
		} while (ListCount < ListSize);	
	}
	
	return S_OK;
}

/******************************************************************************
 * DMUSIC_FillBandFromFileHandle: 
 *	- fills a IDirectMusicBandImpl struct with data from file handle. 
 *	- IMPORTANT: it expects a RIFF chunk at beginning, so if you are calling it 
 *	             from another DMUSIC_Fill* function, make sure pointer is at
 *               correct place!
 *	- TODO: replace data in function with data in IDirectMusicBandImpl
 */
HRESULT WINAPI DMUSIC_FillBandFromFileHandle (IDirectMusicBandImpl *band, HANDLE fd)
{
	rawChunk chunk;
	DWORD BytesRead, ListCount = 0, ListSize, ListCount2 = 0, ListSize2, FileCount = 0, FileSize;
	/* FIXME: Replace stuff located below with the stuff in band */
	UNFO_List UNFO;
	DMUS_IO_VERSION version;
	GUID guid;
	/* only in singular form for time being */
	DMUS_IO_INSTRUMENT header;
	Reference reference;
	
	TRACE("reading 'RIFF' chunk...\n");
	ReadFile (fd, &chunk.id, sizeof(FOURCC), &BytesRead, NULL); /* read 'RIFF' */
	if (chunk.id == FOURCC_RIFF) {
		TRACE("'RIFF': RIFF file\n");
		ReadFile (fd, &chunk.size, sizeof(DWORD), &BytesRead, NULL); /* read size of 'RIFF' chunk */
		FileSize = chunk.size - sizeof(FOURCC); /* file content size = size of 'RIFF' chunk - FOURCC ID of following form */
		TRACE("reading chunks ...\n");
		ReadFile (fd, &chunk.id, sizeof(FOURCC), &BytesRead, NULL); /* read ID of following form */
		if (chunk.id == DMUS_FOURCC_BAND_FORM) {
			TRACE("'DMBD': band form\n");
			do {
				ReadFile (fd, &chunk.id, sizeof(FOURCC), &BytesRead, NULL);
				ReadFile (fd, &chunk.size, sizeof(DWORD), &BytesRead, NULL);
				FileCount += chunk.size + sizeof(DWORD) + sizeof(FOURCC);
				switch (chunk.id)
				{
					case DMUS_FOURCC_GUID_CHUNK: {
						TRACE("'guid': GUID\n");
						ReadFile (fd, &guid, chunk.size, &BytesRead, NULL);
						TRACE_(dmfiledat)("=> GUID = %s\n", debugstr_guid(&guid));
						break;
					} case DMUS_FOURCC_VERSION_CHUNK: {
						TRACE("'vers': version\n");
						ReadFile (fd, &version, chunk.size, &BytesRead, NULL);
						TRACE_(dmfiledat)("=> version = %ld%ld\n", version.dwVersionMS, version.dwVersionLS);
						break;			
					} case FOURCC_LIST:{
						TRACE("'LIST': list chunk (size = %ld)\n", chunk.size);
						ListSize = chunk.size - sizeof(FOURCC); /* list content size = size of 'LIST' chunk - FOURCC ID of the list */
						ListCount = 0; /* reset */
						ReadFile (fd, &chunk.id, sizeof(FOURCC), &BytesRead, NULL); /* read list ID  */
						switch (chunk.id)
						{
							case DMUS_FOURCC_UNFO_LIST: {
								TRACE("'UNFO': UNFO list (content size = %ld)\n", ListSize);
								SetFilePointer (fd, -(sizeof(DWORD) + 2*sizeof(FOURCC)), NULL, FILE_CURRENT); /* set pointer at beginning of list */
								DMUSIC_FillUNFOFromFileHandle (UNFO, fd); /* forward to DMUSIC_FillUNFOFromFileHandle */
								break;								
							} case DMUS_FOURCC_INSTRUMENTS_LIST: {
								TRACE("'lbil': instrumets list (content size = %ld)\n", ListSize);
								do {
									ReadFile (fd, &chunk.id, sizeof(FOURCC), &BytesRead, NULL);
									ReadFile (fd, &chunk.size, sizeof(DWORD), &BytesRead, NULL);
									ListCount += sizeof(FOURCC) + sizeof(DWORD) + chunk.size;	
									if (chunk.id == FOURCC_LIST && ReadFile (fd, &chunk.id, sizeof(FOURCC), &BytesRead, NULL) && chunk.id == DMUS_FOURCC_INSTRUMENT_LIST) {
										ListSize2 = chunk.size - sizeof(FOURCC); /* list content size = size of 'LIST' chunk - FOURCC ID of the list */
										ListCount2 = 0; /* reset */
										TRACE("'lbin': instrument (size = %ld)\n", ListSize2);
										do {
											ReadFile (fd, &chunk.id, sizeof(FOURCC), &BytesRead, NULL);
											ReadFile (fd, &chunk.size, sizeof(DWORD), &BytesRead, NULL);
											ListCount2 += sizeof(FOURCC) + sizeof(DWORD) + chunk.size;
											switch (chunk.id)
											{
												case DMUS_FOURCC_INSTRUMENT_CHUNK: {
													TRACE("'bins': instrument header\n");
													ReadFile (fd, &header, chunk.size, &BytesRead, NULL);
													TRACE_(dmfiledat)("=> dwPatch = %ld; dwAssignPatch = %ld; dwPChannel = %ld; dwFlags = %ld; bPan = %i; bVolume = %i; nTranspose = %i; dwChannelPriority = %ld; nPitchBendRange = %i", \
														header.dwPatch, header.dwAssignPatch, header.dwPChannel, header.dwFlags, header.bPan, header.bVolume, header.nTranspose, header.dwChannelPriority, header.nPitchBendRange);
													break;
												} case FOURCC_LIST: {
													TRACE("'LIST': list chunk (size = %ld)\n", chunk.size);
													ReadFile (fd, &chunk.id, sizeof(FOURCC), &BytesRead, NULL);
													if (chunk.id == DMUS_FOURCC_REF_LIST) {
														TRACE("'DMRF': reference list (size = %ld)\n", chunk.size - 4); /* set pointer at beginning of list */
														SetFilePointer (fd, -(sizeof(DWORD) + 2*sizeof(FOURCC)), NULL, FILE_CURRENT);
														DMUSIC_FillReferenceFromFileHandle (reference, fd); /* forward to DMUSIC_FillReferenceFromFileHandle */
													} else WARN("invalid chunk (only 'DMRF' chunk allowed\n");						
													break;												
												} default: {
													WARN("invalid chunk (only 'bins' and 'LIST' chunks allowed\n");
													break;
												}
											}
											TRACE("ListCount2 (%ld) < ListSize2 (%ld)\n", ListCount2, ListSize2);							
										} while (ListCount2 < ListSize2);
									} else WARN("invalid chunk (only 'lbin' chunk allowed)\n");
									TRACE("ListCount (%ld) < ListSize (%ld)\n", ListCount, ListSize);							
								} while (ListCount < ListSize);
								break;
							} default: {
								WARN("invalid chunk (only 'UNFO' and 'lbil' chunks allowed\n");
								break;
							}
						}
						break;
					} default: {
						WARN("invalid chunk (only 'guid', 'vers' and 'LIST' chunks allowed)\n");
						break;
					}
				}
				TRACE("FileCount (%ld) < FileSize (%ld)\n", FileCount, FileSize);				
			} while (FileCount < FileSize);
		}
	} else {
		WARN("'RIFF' not found: not a RIFF file\n");
	}
	
	return S_OK;
}

/******************************************************************************
 * DMUSIC_FillTrackFromFileHandle: 
 *	- fills a IDirectMusicTrack8Impl struct with data from file handle. 
 *	- IMPORTANT: it expects a RIFF chunk at beginning, so if you are calling it 
 *	             from another DMUSIC_Fill* function, make sure pointer is at
 *               correct place!
 *	- TODO: replace data in function with data in IDirectMusicTrackImpl
 *			implement loading for missing (empty) clauses
 *			fix a problem with tempo track loading (look at code)
 */
HRESULT WINAPI DMUSIC_FillTrackFromFileHandle (IDirectMusicTrack8Impl *segment, HANDLE fd)
{
	rawChunk chunk;
	DWORD BytesRead, ListCount = 0, ListCount2 = 0, ListCount3 = 0, ListCount4 = 0, \
		ListSize, ListSize2, ListSize3, ListSize4, FileCount = 0, FileSize, FileCount2 = 0, FileSize2 /* *2s, *3s and *4s are for various subchunks  */;
	int i;	
	
	/* general track info */
	DMUS_IO_TRACK_HEADER header;
	DMUS_IO_TRACK_EXTRAS_HEADER extheader;
	GUID guid;
	DMUS_IO_VERSION version;
	UNFO_List UNFO;
	/* tempo track stuff */
	DMUS_IO_TEMPO_ITEM tempo;
	/* chord track stuff */
	DWORD chordHeader;
	ChordData chordData;
	/* command track stuff */
	DMUS_IO_COMMAND command;
	/* sytle list stuff (support only 1 while still in parse development mode)*/
	DWORD timestamp;
	Reference reference;
	/* band track stuff */
	BandTrack bandTrack;
	/* wave track stuff (only singular) */
	WaveTrack waveTrack;
	/* segment trigger track stuff */
	SegTriggerTrack segTriggerTrack;
	/* time signature track stuff */
	TimeSigTrack timeSigTrack;
	/* script track list stuff */
	ScriptEvent event;
	
	TRACE("reading 'RIFF' chunk...\n");
	ReadFile (fd, &chunk.id, sizeof(FOURCC), &BytesRead, NULL);
	if (chunk.id == FOURCC_RIFF) {
		TRACE ("'RIFF': RIFF file\n");
		ReadFile (fd, &chunk.size, sizeof(DWORD), &BytesRead, NULL); /* read size of 'RIFF' chunk */
		FileSize = chunk.size - sizeof(FOURCC); /* file content size = size of 'RIFF' chunk - FOURCC ID of following form */
		TRACE("reading chunks ...\n");
		ReadFile (fd, &chunk.id, sizeof(FOURCC), &BytesRead, NULL);  /* read ID of following form */
		if (chunk.id == DMUS_FOURCC_TRACK_FORM) {
			TRACE("'DMTK': track form\n");
			do {
				ReadFile (fd, &chunk.id, sizeof(FOURCC), &BytesRead, NULL);
				ReadFile (fd, &chunk.size, sizeof(DWORD), &BytesRead, NULL);
				FileCount += chunk.size + sizeof(DWORD) + sizeof(FOURCC);
				switch (chunk.id)
				{
					case DMUS_FOURCC_TRACK_CHUNK: {
						TRACE("'trkh': track header\n");
						ReadFile (fd, &header, chunk.size, &BytesRead, NULL);
						TRACE_(dmfiledat)("=> track guidClassID = %s; dwPosition = %ld; dwGroup = %ld; ckid = %ld; fccType = %ld\n", \
							debugstr_guid (&header.guidClassID), header.dwPosition, header.dwGroup, header.ckid, header.fccType);
						break;
					} case DMUS_FOURCC_TRACK_EXTRAS_CHUNK: {
						TRACE("'trkx': extra track flags\n");
						ReadFile (fd, &extheader, chunk.size, &BytesRead, NULL);
						TRACE_(dmfiledat)("=> dwFlags = %ld; dwPriority = %ld\n", extheader.dwFlags, 
							extheader.dwPriority);
						break;
					} case DMUS_FOURCC_GUID_CHUNK: {
						TRACE("'guid': GUID\n");
						ReadFile (fd, &guid, chunk.size, &BytesRead, NULL);
						TRACE_(dmfiledat)("=> GUID = %s\n", debugstr_guid(&guid));
						break;
					} case DMUS_FOURCC_VERSION_CHUNK: {
						TRACE("'vers': version\n");
						ReadFile (fd, &version, chunk.size, &BytesRead, NULL);
						TRACE_(dmfiledat)("=> version = %ld%ld\n", version.dwVersionMS, version.dwVersionLS);
						break;
					} case FOURCC_LIST:{
						TRACE("'LIST': list (size = %ld)\n", chunk.size);
						ListSize = chunk.size - sizeof(FOURCC); /* list content size = size of 'LIST' chunk - FOURCC ID of the list */
						ListCount = 0; /* reset */
						ReadFile (fd, &chunk.id, sizeof(FOURCC), &BytesRead, NULL);
						switch (chunk.id)
						{
							case DMUS_FOURCC_UNFO_LIST: {
								TRACE("'UNFO': UNFO list (forward to DMUSIC_FillUNFOFromFileHandle(...))\n");
								SetFilePointer (fd, -(sizeof(DWORD) + 2*sizeof(FOURCC)), NULL, FILE_CURRENT); /* place pointer before the 'LIST' chunk */
								DMUSIC_FillUNFOFromFileHandle (UNFO, fd); /* forward to DMUSIC_FillUNFOFromFileHandle */
								break;								
							} case DMUS_FOURCC_CHORDTRACK_LIST: {
								TRACE("'cord': chord track list\n");
								do {
									ReadFile (fd, &chunk.id, sizeof(FOURCC), &BytesRead, NULL);
									ReadFile (fd, &chunk.size, sizeof(DWORD), &BytesRead, NULL);
									ListCount += sizeof(FOURCC) + sizeof(DWORD) + chunk.size;
									switch (chunk.id)
									{
										case DMUS_FOURCC_CHORDTRACKHEADER_CHUNK: {
											TRACE("'crdh': chord header\n");
											ReadFile (fd, &chordHeader, chunk.size, &BytesRead, NULL);
											TRACE_(dmfiledat)("=> chord root = %i; scale = %i\n", (chordHeader && 0xFF000000) >> 24, chordHeader && 0x00FFFFFF);
											break;
										} case DMUS_FOURCC_CHORDTRACKBODY_CHUNK: {
											TRACE("'crdb': chord body\n");
											ReadFile (fd, &chunk.size, sizeof(DWORD), &BytesRead, NULL); /* read sizeof (DMUS_IO_CHORD) */											
											ReadFile (fd, &chordData.chord, chunk.size, &BytesRead, NULL); /* read DMUS_IO_CHORD */
											TRACE_(dmfiledat)("=> wszName[16] = %s; mtTime = %li; chord.wMeasure = %d; chord.bBeat = %i; bFlags = %i\n", \
												debugstr_w (chordData.chord.wszName), chordData.chord.mtTime, chordData.chord.wMeasure, chordData.chord.bBeat, chordData.chord.bFlags);
											ReadFile (fd, &chordData.nrofsubchords, sizeof(DWORD), &BytesRead, NULL); /* read number of subchords */
											TRACE_(dmfiledat)("=> number of subchords = %ld\n", chordData.nrofsubchords);
											ReadFile (fd, &chunk.size, sizeof(DWORD), &BytesRead, NULL); /* read sizeof (DMUS_IO_SUBCHORD) */
											chordData.subchord = (DMUS_IO_SUBCHORD*) HeapAlloc (GetProcessHeap (), 0, sizeof(DMUS_IO_SUBCHORD) * chordData.nrofsubchords); /* allocate space */
											for (i = 0; i < chordData.nrofsubchords; i++)
											{
												TRACE_(dmfiledat)("=> subchord[%i]:  dwChordPattern = %ld; dwScalePattern = %ld; dwInversionPoints = %ld; dwLevels = %ld; bChordRoot = %i; bScaleRoot = %i\n", \
													i, chordData.subchord[i].dwChordPattern, chordData.subchord[i].dwScalePattern, chordData.subchord[i].dwInversionPoints, chordData.subchord[i].dwLevels, \
													chordData.subchord[i].bChordRoot, chordData.subchord[i].bScaleRoot);
											}
											ReadFile (fd, chordData.subchord, chunk.size*chordData.nrofsubchords, &BytesRead, NULL);
											break;
										} default: {
											WARN("Invalid chunk (only 'crdh' and 'crdb' chunks allowed)\n");
											break;
										}
									}
									TRACE("ListCount (%ld) < ListSize (%ld)\n", ListCount, ListSize);
								} while (ListCount < ListSize);
								break;
							}  case DMUS_FOURCC_STYLE_TRACK_LIST: {
								TRACE("'sttr': style track list\n");
								do {
									ReadFile (fd, &chunk.id, sizeof(FOURCC), &BytesRead, NULL);
									ReadFile (fd, &chunk.size, sizeof(DWORD), &BytesRead, NULL);
									ListCount += sizeof(FOURCC) + sizeof(DWORD) + chunk.size;				
									if (chunk.id == FOURCC_LIST && ReadFile (fd, &chunk.id, sizeof(FOURCC), &BytesRead, NULL) && chunk.id == DMUS_FOURCC_STYLE_REF_LIST) {
										ListSize2 = chunk.size - sizeof(FOURCC);
										ListCount2 = 0;
										TRACE("'strf': style reference list (size = %ld)\n", ListSize2);
										do {										
										
											ReadFile (fd, &chunk.id, sizeof(FOURCC), &BytesRead, NULL);
											ReadFile (fd, &chunk.size, sizeof(DWORD), &BytesRead, NULL);
											ListCount2 += sizeof(FOURCC) + sizeof(DWORD) + chunk.size;
											switch (chunk.id)
											{
												case DMUS_FOURCC_TIME_STAMP_CHUNK: {
													TRACE("'stmp': time stamp\n");
													ReadFile (fd, &timestamp, chunk.size, &BytesRead, NULL);
													TRACE_(dmfiledat)("=> time stamp = %ld\n", timestamp);
													break;
												} case FOURCC_LIST: {
													TRACE("'LIST': list\n");
													ReadFile (fd, &chunk.id, sizeof(FOURCC), &BytesRead, NULL);
													if (chunk.id == DMUS_FOURCC_REF_LIST){
														TRACE("'DMRF': reference list (forward to DMUSIC_FillReferenceFromFileHandle(...)\n");
														SetFilePointer (fd, -(sizeof(DWORD) + 2*sizeof(FOURCC)), NULL, FILE_CURRENT); /* place pointer before 'LIST' chunk */
														DMUSIC_FillReferenceFromFileHandle (reference, fd);
													} else {
														WARN("invalid chunk (only 'DMRF' chunk allwed)\n");
													}											
													break;
												} default: {
													WARN("invalid chunk (only 'stmp' and 'LIST' chunk allowed)\n");
													break;
												}
											}
											TRACE("ListCount2 (%ld) < ListSize2 (%ld)\n", ListCount2, ListSize2);
										} while (ListCount2 < ListSize2);
									} else {
										WARN("invalid chunk (only 'strf' allowed)\n");
									}																			
									TRACE("ListCount (%ld) < ListSize (%ld)\n", ListCount, ListSize);
								} while (ListCount < ListSize);
								break;	
							} case DMUS_FOURCC_PERS_TRACK_LIST: {
								FIXME("'pftr': chordmap track list: not supported yet\n");
								break;
							} case DMUS_FOURCC_LYRICSTRACK_LIST: {
								FIXME("'lyrt': lyrics track list: not supported yet\n");
								break;
							}  case DMUS_FOURCC_MARKERTRACK_LIST: {
								FIXME("'MARK': marker track list: not supported yet\n");
								break;
							}  case DMUS_FOURCC_MELODYFORM_TRACK_LIST: {
								FIXME("'mfrm': melody formulation track list: not supported yet\n");
								break;
							}  case DMUS_FOURCC_PARAMCONTROLTRACK_TRACK_LIST: {
								FIXME("'prmt': parameter control track list: not supported yet\n");
								break;						
							}  case DMUS_FOURCC_SCRIPTTRACK_LIST: {
								TRACE("'scrt': script track list\n");
								do {
									ReadFile (fd, &chunk.id, sizeof(FOURCC), &BytesRead, NULL);
									ReadFile (fd, &chunk.size, sizeof(DWORD), &BytesRead, NULL);
									ListCount += sizeof(FOURCC) + sizeof(DWORD) + chunk.size;				
									if (chunk.id == FOURCC_LIST && ReadFile (fd, &chunk.id, sizeof(FOURCC), &BytesRead, NULL) && chunk.id == DMUS_FOURCC_SCRIPTTRACKEVENTS_LIST) {
										TRACE("'scrl': script events list\n");
										ListSize2 = chunk.size - sizeof(FOURCC); /* list content size = size of 'LIST' chunk - FOURCC ID of the list */
										ListCount2 = 0;
										do {
											ReadFile (fd, &chunk.id, sizeof(FOURCC), &BytesRead, NULL);
											ReadFile (fd, &chunk.size, sizeof(DWORD), &BytesRead, NULL);
											ListCount2 += sizeof(FOURCC) + sizeof(DWORD) + chunk.size;
											if (chunk.id == FOURCC_LIST && ReadFile (fd, &chunk.id, sizeof(FOURCC), &BytesRead, NULL) && chunk.id == DMUS_FOURCC_SCRIPTTRACKEVENT_LIST) {
												TRACE("'scre': script event list\n");
												ListSize3 = chunk.size - sizeof(FOURCC); /* list content size = size of 'LIST' chunk - FOURCC ID of the list */
												ListCount3 = 0;
												do {
													ReadFile (fd, &chunk.id, sizeof(FOURCC), &BytesRead, NULL);
													ReadFile (fd, &chunk.size, sizeof(DWORD), &BytesRead, NULL);
													ListCount3 += sizeof(FOURCC) + sizeof(DWORD) + chunk.size;
													switch (chunk.id)
													{
														case DMUS_FOURCC_SCRIPTTRACKEVENTHEADER_CHUNK: {
															TRACE("'scrh': event header\n");
															ReadFile (fd, &event.header, chunk.size, &BytesRead, NULL);
															TRACE_(dmfiledat)("=> dwFlags = %ld; lTimeLogical = %li; lTimePhysical = %li\n", \
																event.header.dwFlags, event.header.lTimeLogical, event.header.lTimePhysical);
															break;
														} case FOURCC_LIST: {
															TRACE("'LIST': list\n");
															ReadFile (fd, &chunk.id, sizeof(FOURCC), &BytesRead, NULL);
															if (chunk.id == DMUS_FOURCC_REF_LIST){
																TRACE("'DMRF': reference list (forward to DMUSIC_FillReferenceFromFileHandle(...)\n");
																SetFilePointer (fd, -(sizeof(DWORD) + 2*sizeof(FOURCC)), NULL, FILE_CURRENT); /* place pointer before 'LIST' chunk */
																DMUSIC_FillReferenceFromFileHandle (event.reference, fd);
															} else {
																WARN("invalid chunk (only 'DMRF' chunk allwed)\n");
															}																
															break;
														} case DMUS_FOURCC_SCRIPTTRACKEVENTNAME_CHUNK: {
															TRACE("'scrn': routine name\n");
															event.name = (WCHAR*) HeapAlloc (GetProcessHeap (), 0, chunk.size);
															ReadFile (fd, event.name, chunk.size, &BytesRead, NULL);
															TRACE_(dmfiledat)("=> routine name = %s\n", debugstr_w (event.name));
															break;
														} default: {
															WARN("invalid chunk (only 'scrh', 'scrn' and 'LIST' chunk allowed)\n");
															break;
														}
													}
													TRACE("ListCount3 (%ld) < ListSize3 (%ld)\n", ListCount3, ListSize3);
												} while (ListCount3 < ListSize3);	
											} else {
												WARN("invalid chunk (only 'scre' chunk allowed)\n");
											}
											TRACE("ListCount2 (%ld) < ListSize2 (%ld)\n", ListCount2, ListSize2);
										} while (ListCount2 < ListSize2);	
									} else {
										WARN("invalid chunk (only 'scrl' chunk allowed)\n");
									}
									TRACE("ListCount (%ld) < ListSize (%ld)\n", ListCount, ListSize);
								} while (ListCount < ListSize);										
								break;
							}  case DMUS_FOURCC_SEGTRACK_LIST: {
								TRACE("'segt': segment trigger track list\n");
								do {
									ReadFile (fd, &chunk.id, sizeof(FOURCC), &BytesRead, NULL);
									ReadFile (fd, &chunk.size, sizeof(DWORD), &BytesRead, NULL);
									ListCount += sizeof(FOURCC) + sizeof(DWORD) + chunk.size;				
									switch (chunk.id)
									{
										case DMUS_FOURCC_SEGTRACK_CHUNK: {
											TRACE("'sgth': segment track header\n");
											ReadFile (fd, &segTriggerTrack.header, chunk.size, &BytesRead, NULL);
											TRACE_(dmfiledat)("=> dwFlags = %ld\n", segTriggerTrack.header.dwFlags);											
											break;
										} case FOURCC_LIST: {
											TRACE("'LIST': list\n");
											ReadFile (fd, &chunk.id, sizeof(FOURCC), &BytesRead, NULL);
											if (chunk.id == DMUS_FOURCC_SEGMENTS_LIST) {
												TRACE("'lsgl': segment lists list\n");
												ListSize2 = chunk.size - sizeof(FOURCC); /* list content size = size of 'LIST' chunk - FOURCC ID of the list */
												ListCount2 = 0;
												do {				
													ReadFile (fd, &chunk.id, sizeof(FOURCC), &BytesRead, NULL);
													ReadFile (fd, &chunk.size, sizeof(DWORD), &BytesRead, NULL);
													ListCount2 += sizeof(FOURCC) + sizeof(DWORD) + chunk.size;
													if (chunk.id == FOURCC_LIST && ReadFile (fd, &chunk.id, sizeof(FOURCC), &BytesRead, NULL) && chunk.id == DMUS_FOURCC_SEGMENT_LIST) {
														ListSize3 = chunk.size - sizeof(FOURCC);
														ListCount3 = 0;
														do {
															ReadFile (fd, &chunk.id, sizeof(FOURCC), &BytesRead, NULL);
															ReadFile (fd, &chunk.size, sizeof(DWORD), &BytesRead, NULL);
															ListCount3 += sizeof(FOURCC) + sizeof(DWORD) + chunk.size;
															switch (chunk.id)
															{
																case DMUS_FOURCC_SEGMENTITEM_CHUNK: {
																	TRACE("'sgih': segment item header\n");
																	ReadFile (fd, &segTriggerTrack.itemHeader, chunk.size, &BytesRead, NULL);
																	TRACE_(dmfiledat)("=> lTimeLogical = %li; lTimePhysical = %li; dwPlayFlags = %ld; dwFlags = %ld\n", \
																		segTriggerTrack.itemHeader.lTimeLogical, segTriggerTrack.itemHeader.lTimePhysical, \
																		segTriggerTrack.itemHeader.dwPlayFlags, segTriggerTrack.itemHeader.dwFlags);
																	break;
																} case DMUS_FOURCC_SEGMENTITEMNAME_CHUNK: {
																	TRACE("'snam': motif name\n");
																	segTriggerTrack.motifName = (WCHAR*) HeapAlloc (GetProcessHeap (), 0, chunk.size);
																	ReadFile (fd, segTriggerTrack.motifName, chunk.size, &BytesRead, NULL);
																	TRACE_(dmfiledat)("=> motif name = %s\n", debugstr_w (segTriggerTrack.motifName));
																	break;
																} case FOURCC_LIST: {
																	TRACE("'LIST': list\n");
																	ReadFile (fd, &chunk.id, sizeof(FOURCC), &BytesRead, NULL);
																	if (chunk.id == DMUS_FOURCC_REF_LIST) {
																		TRACE("'DMRF': reference list (forward to DMUSIC_FillReferenceFromFileHandle(...)\n");
																		SetFilePointer (fd, -(sizeof(DWORD) + 2*sizeof(FOURCC)), NULL, FILE_CURRENT); /* place pointer before 'LIST' chunk */
																		DMUSIC_FillReferenceFromFileHandle (segTriggerTrack.reference, fd);																			
																	} else {
																		WARN("invalid chunk (only 'DMRF' chunk allowed)\n");
																	}
																	break;
																} default: {
																	WARN("invalid chunk (only 'sgih', 'snam' and 'LIST' chunks allowed)\n");
																	break;
																}
															}
															TRACE("ListCount3 (%ld) < ListSize3 (%ld)\n", ListCount3, ListSize3);
														} while (ListCount3 < ListSize3);
													} else {
														WARN("invalid chunk (only 'lseg' chunk allowed)\n");
													}
													TRACE("ListCount2 (%ld) < ListSize2 (%ld)\n", ListCount2, ListSize2);
												} while (ListCount2 < ListSize2);					
											} else {
												WARN("invalid chunk (only 'lsgl' chunk allowed\n");
											}												
											break;
										} default: {
											WARN("invalid chunk (only 'sgth' and 'LIST' chunks allowed)\n");
											break;
										}
									}
									TRACE("ListCount (%ld) < ListSize (%ld)\n", ListCount, ListSize);
								} while (ListCount < ListSize);															
								break;
							}  case DMUS_FOURCC_TIMESIGTRACK_LIST: {
								TRACE("'TIMS': time signature track list\n");
								ReadFile (fd, &chunk.id, sizeof(FOURCC), &BytesRead, NULL);
								if (chunk.id == DMUS_FOURCC_TIMESIGNATURE_TRACK) {
									TRACE("'tims': time signatures\n");
									timeSigTrack.nrofitems = chunk.size - sizeof(DWORD);
									ReadFile (fd, &chunk.size, sizeof(DWORD), &BytesRead, NULL);
									if (chunk.size != sizeof(DMUS_IO_TIMESIGNATURE_ITEM))
										WARN("there seem to be a problem: file claims that size of DMUSIC_IO_TEMPO_ITEM is %ld, while real sizeof returns %i\n", chunk.size, sizeof(DMUS_IO_TIMESIGNATURE_ITEM));
									timeSigTrack.nrofitems /= chunk.size;
									TRACE_(dmfiledat)("=> number of items = %ld\n", timeSigTrack.nrofitems);
									timeSigTrack.items = (DMUS_IO_TIMESIGNATURE_ITEM*) HeapAlloc (GetProcessHeap (), 0, chunk.size * timeSigTrack.nrofitems);
									ReadFile(fd, timeSigTrack.items, chunk.size * timeSigTrack.nrofitems, &BytesRead, NULL);
									for (i = 0; i < timeSigTrack.nrofitems; i++)
									{
										TRACE_(dmfiledat)("=> time signature[%i]: lTime = %li; bBeatsPerMeasure = %i; bBeat = %i; wGridsPerBeat = %d\n", \
											i, timeSigTrack.items[i].lTime, timeSigTrack.items[i].bBeatsPerMeasure, timeSigTrack.items[i].bBeat, timeSigTrack.items[i].wGridsPerBeat);
									}
								} else {
									WARN("invalid chunk (only 'tims' chunk allowed)\n");
								}									
								break;								
							}  case DMUS_FOURCC_WAVETRACK_LIST: {
								TRACE("'wavt': wave track list\n");
								do {
									ReadFile (fd, &chunk.id, sizeof(FOURCC), &BytesRead, NULL);
									ReadFile (fd, &chunk.size, sizeof(DWORD), &BytesRead, NULL);
									ListCount += sizeof(FOURCC) + sizeof(DWORD) + chunk.size;				
									switch (chunk.id)
									{
										case DMUS_FOURCC_WAVETRACK_CHUNK: {
											TRACE("'wath': wave track header\n");
											ReadFile (fd, &waveTrack.header, chunk.size, &BytesRead, NULL);
											TRACE_(dmfiledat)("=> lVolume = %li; dwFlags = %ld\n", waveTrack.header.lVolume, waveTrack.header.dwFlags);
											break;
										} case FOURCC_LIST: {
											TRACE("'LIST': list\n");
											ReadFile (fd, &chunk.id, sizeof(FOURCC), &BytesRead, NULL);
											if (chunk.id == DMUS_FOURCC_WAVEPART_LIST) {
												TRACE("'wavp': wave parts list\n");
												ListSize2 = chunk.size - sizeof(FOURCC);
												ListCount2 = 0;
												do {				
													ReadFile (fd, &chunk.id, sizeof(FOURCC), &BytesRead, NULL);
													ReadFile (fd, &chunk.size, sizeof(DWORD), &BytesRead, NULL);
													ListCount2 += sizeof(FOURCC) + sizeof(DWORD) + chunk.size;
													switch (chunk.id)
													{
														case DMUS_FOURCC_WAVEPART_CHUNK: {
															TRACE("'waph': wave part header\n");
															ReadFile (fd, &waveTrack.partHeader, chunk.size, &BytesRead, NULL);
															TRACE_(dmfiledat)("=> lVolume = %li; dwVariations = %ld; dwPChannel = %ld; dwLockToPart = %ld; dwFlags = %ld; dwIndex = %ld\n", \
																waveTrack.partHeader.lVolume, waveTrack.partHeader.dwVariations, waveTrack.partHeader.dwPChannel, \
																waveTrack.partHeader.dwLockToPart, waveTrack.partHeader.dwFlags, waveTrack.partHeader.dwIndex);
															break;
														} case FOURCC_LIST: {
															TRACE("'LIST': list\n");
															ReadFile (fd, &chunk.id, sizeof(FOURCC), &BytesRead, NULL);
															if (chunk.id == DMUS_FOURCC_WAVEITEM_LIST) {
																TRACE("'wavi': wave items list\n");
																ListSize3 = chunk.size - sizeof(FOURCC);
																ListCount3 = 0;
																do {				
																	ReadFile (fd, &chunk.id, sizeof(FOURCC), &BytesRead, NULL);
																	ReadFile (fd, &chunk.size, sizeof(DWORD), &BytesRead, NULL);
																	ListCount3 += sizeof(FOURCC) + sizeof(DWORD) + chunk.size;
																	if (chunk.id == FOURCC_LIST && ReadFile (fd, &chunk.id, sizeof(FOURCC), &BytesRead, NULL) && chunk.id == DMUS_FOURCC_WAVE_LIST) {
																		TRACE("'wave': wave item list\n");
																		ListSize4 = chunk.size - sizeof(FOURCC); /* list content size = size of 'LIST' chunk - FOURCC ID of the list */
																		ListCount4 = 0; /* reset */
																		do {
																			ReadFile (fd, &chunk.id, sizeof(FOURCC), &BytesRead, NULL);
																			ReadFile (fd, &chunk.size, sizeof(DWORD), &BytesRead, NULL);
																			ListCount4 += sizeof(FOURCC) + sizeof(DWORD) + chunk.size;
																			switch (chunk.id)
																			{
																				case DMUS_FOURCC_WAVEITEM_CHUNK: {
																					TRACE("'waih': wave item header\n");
																					ReadFile (fd, &waveTrack.itemHeader, chunk.size, &BytesRead, NULL);
																					TRACE_(dmfiledat)("=> lVolume = %li; lPitch = %li; dwVariations = %ld; rtTime = FIXME; rtStartOffset = FIXME; rtReserved = FIXME; rtDuration = FIXME; mtLogicalTime = %li; dwLoopStart = %ld; dwLoopEnd = %ld; dwFlags = %ld\n", \
																						waveTrack.itemHeader.lVolume, waveTrack.itemHeader.lPitch, waveTrack.itemHeader.dwVariations, /*waveTrack.itemHeader.rtTime, \
																						waveTrack.itemHeader.rtStartOffset, waveTrack.itemHeader.rtReserved, waveTrack.itemHeader.rtDuration, */waveTrack.itemHeader.mtLogicalTime, \
																						waveTrack.itemHeader.dwLoopStart, waveTrack.itemHeader.dwLoopEnd, waveTrack.itemHeader.dwFlags);
																					break;
																				} case mmioFOURCC('w','v','c','u'): {
																					FIXME("'wvcu': undocumented and unknown chunk type (skipping)\n");
																					SetFilePointer (fd, chunk.size, NULL, FILE_CURRENT); /* skip */
																					break;
																				} case FOURCC_LIST: {
																					TRACE("'LIST': list\n");
																					ReadFile (fd, &chunk.id, sizeof(FOURCC), &BytesRead, NULL);
																					if (chunk.id == DMUS_FOURCC_REF_LIST) {
																						TRACE("'DMRF': reference list (forward to DMUSIC_FillReferenceFromFileHandle(...)\n");
																						SetFilePointer (fd, -(sizeof(DWORD) + 2*sizeof(FOURCC)), NULL, FILE_CURRENT); /* place pointer before 'LIST' chunk */
																						DMUSIC_FillReferenceFromFileHandle (waveTrack.reference, fd);																						
																					} else {
																						WARN ("invalid chunk (only 'DMRF' chunk allowed\n");
																					}
																					break;
																				} default: {
																					WARN("invalid chunk (only 'waih' and 'LIST' (and undocumented 'wvcu') chunks allowed)\n");
																				}
																			}
																			TRACE("ListCount4 (%ld) < ListSize4 (%ld)\n", ListCount4, ListSize4);
																		} while (ListCount4 < ListSize4);
																	} else {
																		WARN("invalid chunk (only 'wave' chunk allowed)\n");
																	}
																	TRACE("ListCount3 (%ld) < ListSize3 (%ld)\n", ListCount3, ListSize3);
																} while (ListCount3 < ListSize3);	
															} else {
																WARN("invalid chunk (only 'wavi' chunk allowed)\n");
															}
															break;
														} default: {
															WARN("invalid chunk (only 'waph' and 'LIST' chunks allowed)\n");
															break;
														}															
													}
													TRACE("ListCount2 (%ld) < ListSize2 (%ld)\n", ListCount2, ListSize2);
												} while (ListCount2 < ListSize2);												
											} else {
												WARN("invalid chunk (only 'wavp' chunk allwed)\n");
											}
											break;
										} default: {
											WARN("invalid chunk (only 'wath' and 'LIST' chunks allowed)\n");
											break;
										}										
									}
									TRACE("ListCount (%ld) < ListSize (%ld)\n", ListCount, ListSize);
								} while (ListCount < ListSize);								
								break;								
							} default: {
								WARN ("invalid chunk (only 'UNFO', 'cord', 'sttr', 'pftr', 'lyrt', 'MARK' and 'mfrm' chunks allowed)\n");
								break;
							}						
						}
						break;
					} case FOURCC_RIFF: {
						TRACE("'RIFF': embedded RIFF chunk (probably band track form)\n");
						ReadFile (fd, &chunk.id, sizeof(FOURCC), &BytesRead, NULL);
						if (chunk.id == DMUS_FOURCC_BANDTRACK_FORM) {
							TRACE("'DMBT': band track form\n");
							FileSize2 = chunk.size - sizeof(FOURCC);
							do {
								ReadFile (fd, &chunk.id, sizeof(FOURCC), &BytesRead, NULL);
								ReadFile (fd, &chunk.size, sizeof(DWORD), &BytesRead, NULL);
								FileCount2 += sizeof(FOURCC) + sizeof(DWORD) + chunk.size;
								switch (chunk.id)
								{
									case DMUS_FOURCC_BANDTRACK_CHUNK: {
										TRACE("'dbth': band track header\n");
										ReadFile (fd, &bandTrack.header, chunk.size, &BytesRead, NULL);
										TRACE_(dmfiledat)("=> bAutoDownload = %d\n", bandTrack.header.bAutoDownload);
										break;
									} case DMUS_FOURCC_GUID_CHUNK: {
										TRACE("'guid': GUID\n");
										ReadFile (fd, &bandTrack.guid, chunk.size, &BytesRead, NULL);
										TRACE_(dmfiledat)("=> GUID = %s\n", debugstr_guid (&bandTrack.guid));
										break;
									} case DMUS_FOURCC_VERSION_CHUNK: {
										TRACE("'vers': version\n");
										ReadFile (fd, &bandTrack.version, chunk.size, &BytesRead, NULL);
										TRACE_(dmfiledat)("=> version = %ld%ld\n", bandTrack.version.dwVersionMS, bandTrack.version.dwVersionLS);				
										break;
									} case FOURCC_LIST: {
										TRACE("'LIST': list (content size = %ld)\n", chunk.size);
										ListSize = chunk.size - sizeof(FOURCC);
										ListCount = 0;
										ReadFile (fd, &chunk.id, sizeof(FOURCC), &BytesRead, NULL);
										switch (chunk.id)
										{
											case DMUS_FOURCC_UNFO_LIST:{
												TRACE("'UNFO': UNFO list (forward to DMUSIC_FillUNFOFromFileHandle)\n");
												SetFilePointer (fd, -(sizeof(DWORD) + 2*sizeof(FOURCC)), NULL, FILE_CURRENT); /* place pointer before 'LIST' chunk */
												DMUSIC_FillUNFOFromFileHandle (UNFO, fd);								
												break;								
											} case DMUS_FOURCC_BANDS_LIST: {
												TRACE("'lbdl': bands list (content size = %ld)\n", ListSize);
												do {
													ReadFile (fd, &chunk.id, sizeof(FOURCC), &BytesRead, NULL);
													ReadFile (fd, &chunk.size, sizeof(DWORD), &BytesRead, NULL);
													ListCount += sizeof(FOURCC) + sizeof(DWORD) + chunk.size;
													if (chunk.id == FOURCC_LIST && ReadFile (fd, &chunk.id, sizeof(FOURCC), &BytesRead, NULL) && chunk.id == DMUS_FOURCC_BAND_LIST) {
														ListSize2 = chunk.size - sizeof(FOURCC);
														ListCount2 = 0;
														TRACE("'lbnd': band list (content size = %ld)\n", ListSize2);
														do {
															ReadFile (fd, &chunk.id, sizeof(FOURCC), &BytesRead, NULL);
															ReadFile (fd, &chunk.size, sizeof(DWORD), &BytesRead, NULL);
															ListCount2 += sizeof(FOURCC) + sizeof(DWORD) + chunk.size;
															switch (chunk.id)
															{
																case DMUS_FOURCC_BANDITEM_CHUNK: {
																	TRACE("'bdih': old band header\n");
																	ReadFile (fd, &bandTrack.header1, chunk.size, &BytesRead, NULL);
																	TRACE_(dmfiledat)("=> lBandTime = %li\n", bandTrack.header1.lBandTime);
																	break;
																} case DMUS_FOURCC_BANDITEM_CHUNK2: {
																	TRACE("'bd2h': new band header\n");
																	ReadFile (fd, &bandTrack.header2, chunk.size, &BytesRead, NULL);
																	TRACE_(dmfiledat)("=> lBandTimeLogical = %li; lBandTimePhysical = %li\n", \
																		bandTrack.header2.lBandTimeLogical, bandTrack.header2.lBandTimePhysical);
																	break;
																} case FOURCC_RIFF: {
																	TRACE("'RIFF': embedded RIFF (size = %ld; could be embedded band form)\n", chunk.size);
																	ReadFile (fd, &chunk.id, sizeof(FOURCC), &BytesRead, NULL);
																	if (chunk.id == DMUS_FOURCC_BAND_FORM) {
																		TRACE("'DMBD': embedded band form (forward to DMUSIC_FillBandFromFileHandle)\n");
																		SetFilePointer (fd, -(sizeof(DWORD) + 2*sizeof(FOURCC)), NULL, FILE_CURRENT); /* place pointer before 'RIFF' chunk */
																		DMUSIC_FillBandFromFileHandle (NULL, fd);
																	} else WARN("invalid chunk (only 'DMBD' chunk allowed)\n");
																	break;
																} default: {
																	WARN("invalid chunk (only 'bdih', 'bd2h' and 'RIFF' chunks allowed)\n");
																	break;
																}
															}
															TRACE("ListCount2 (%ld) < ListSize2 (%ld)\n", ListCount2, ListSize2);									
														} while (ListCount2 < ListSize2);
													} else WARN("unknown chunk - expect error\n");
													
												} while (ListCount < ListSize);
												break;
											} default: {
												WARN("invalid chunk (only 'UNFO' and 'lbdl' chunks allowed)\n");
											}
										}
										break;
									} default: {
										WARN("invalid chunk (only 'dbth', 'guid', 'vers' and 'LIST' chunks allowed)\n");
										break;
									}								
								}
								TRACE("FileCount2 (%ld) < FileSize2 (%ld)\n", FileCount2, FileSize2);									
							} while (FileCount2 < FileSize2);
						} else {
							WARN("invalid chunk (only 'DMBT' chunk allowed\n");
						}
						break;
					} case DMUS_FOURCC_COMMANDTRACK_CHUNK: {
						TRACE("'cmnd': command track\n");
						ReadFile (fd, &chunk.size, sizeof(DWORD), &BytesRead, NULL); /* read sizeof(DMUS_IO_COMMAND) */
						ReadFile (fd, &command, chunk.size, &BytesRead, NULL); /* read DMUS_IO_COMMAND */
						TRACE_(dmfiledat)("wMeasure = %d; bBeat = %i; bCommand = %i; bGrooveLevel = %i; bGrooveRange = %i; bRepeatMode = %i\n", \
							command.wMeasure, command.bBeat, command.bCommand, command.bGrooveLevel, command.bGrooveRange, command.bRepeatMode);
						break;
					}  case DMUS_FOURCC_MUTE_CHUNK: {
						FIXME("'mute': mute track chunk: not supported yet\n");
						break;
					}  case DMUS_FOURCC_PATTERN_FORM: {
						FIXME("'DMPT': pattern track form: not supported yet\n");
						break;
					}  case DMUS_FOURCC_SEQ_TRACK: {
						FIXME("'seqt': sequence track chunk: not supported yet\n");
						break;
					}  case DMUS_FOURCC_SIGNPOST_TRACK_CHUNK: {
						FIXME("'sgnp': signpost track chunk: not supported yet\n");
						break;
					}  case DMUS_FOURCC_SYSEX_TRACK: {
						FIXME("'syex': sysex track chunk: not supported yet\n");
						break;
					}  case DMUS_FOURCC_TEMPO_TRACK: {
						TRACE("'tetr': tempo track chunk\n");
						ReadFile (fd, &chunk.size, sizeof(DWORD), &BytesRead, NULL);
						if (chunk.size != sizeof(DMUS_IO_TEMPO_ITEM))
							WARN("there seem to be a problem: file claims that size of DMUSIC_IO_TEMPO_ITEM is %ld, while real sizeof returns %i\n", \
								chunk.size, sizeof(DMUS_IO_TEMPO_ITEM));
						ReadFile (fd, &tempo, chunk.size, &BytesRead, NULL);
						TRACE_(dmfiledat)("=> lTime = %ld; dblTempo = %f\n", tempo.lTime, tempo.dblTempo);
						break;
					} default: {
						WARN("invalid chunk (too many too list)\n");
						break;
					}
				}
				TRACE("FileCount (%ld) < FileSize (%ld)\n", FileCount, FileSize);
			} while (FileCount < FileSize);
		} else {
			WARN("invalid chunk (only 'DMTK' chunk allowed)\n");
		}
	} else {
		WARN("'RIFF' not found: not a RIFF file\n");
	}
	
	return S_OK;
}

/******************************************************************************
 * DMUSIC_FillSegmentFromFileHandle: 
 *	- fills a IDirectMusicSegment8Impl struct with data from file handle. 
 *	- IMPORTANT: it expects a RIFF chunk at beginning, so if you are calling it 
 *	             from another DMUSIC_Fill* function, make sure pointer is at
 *               correct place!
 *	- TODO: replace data in function with data in IDirectMusicSegmentImpl
 *			implement loading for missing (empty) clauses
 */
HRESULT WINAPI DMUSIC_FillSegmentFromFileHandle (IDirectMusicSegment8Impl *segment, HANDLE fd)
{
	rawChunk chunk;
	DWORD BytesRead, ListCount = 0, ListSize, FileCount = 0, FileSize;
	/* FIXME: Replace stuff located below with the stuff in segment */
	UNFO_List UNFO;
	DMUS_IO_SEGMENT_HEADER header;
	DMUS_IO_VERSION version;
	GUID guid;
	
	TRACE("reading 'RIFF' chunk...\n");
	ReadFile (fd, &chunk.id, sizeof(FOURCC), &BytesRead, NULL);
	if (chunk.id == FOURCC_RIFF) {
		TRACE("'RIFF': RIFF file\n");
		ReadFile (fd, &chunk.size, sizeof(DWORD), &BytesRead, NULL); /* read size of 'RIFF' chunk */
		FileSize = chunk.size - sizeof(FOURCC); /* file content size = size of 'RIFF' chunk - FOURCC ID of following form */
		TRACE("reading chunks ...\n");
		ReadFile (fd, &chunk.id, sizeof(FOURCC), &BytesRead, NULL); /* read ID of following form */		
		if (chunk.id == DMUS_FOURCC_SEGMENT_FORM) {
			TRACE("DMSG: segment form\n");
			do {
				ReadFile (fd, &chunk.id, sizeof(FOURCC), &BytesRead, NULL);
				ReadFile (fd, &chunk.size, sizeof(DWORD), &BytesRead, NULL);
				FileCount += chunk.size + sizeof(DWORD) + sizeof(FOURCC);
				switch (chunk.id)
				{
					case DMUS_FOURCC_SEGMENT_CHUNK: {
						TRACE("segh: segment header\n");
						ReadFile (fd, &header, chunk.size, &BytesRead, NULL);
						TRACE_(dmfiledat)("=> dwRepeats = %ld; mtLength = %li; mtPlayStart = %li; mtLoopStart = %li; mtLoopEnd = %li; dwResolution = %ld; rtLength = FIXME; dwFlags = %ld; dwReserved = %ld\n", \
							header.dwRepeats, header.mtLength, header.mtPlayStart, header.mtLoopStart, header.mtLoopEnd, header.dwResolution/*, header.rtLength*/, header.dwFlags, header.dwReserved);
						break;
					} case DMUS_FOURCC_GUID_CHUNK: {
						TRACE("'guid': GUID\n");
						ReadFile (fd, &guid, chunk.size, &BytesRead, NULL);
						TRACE_(dmfiledat)("=> GUID = %s\n", debugstr_guid(&guid));
						break;
					} case DMUS_FOURCC_VERSION_CHUNK: {
						TRACE("'vers': version\n");
						ReadFile (fd, &version, chunk.size, &BytesRead, NULL);
						TRACE_(dmfiledat)("=> version = %ld%ld\n", version.dwVersionMS, version.dwVersionLS);
						break;
					} case FOURCC_LIST:{
						TRACE("'LIST': list (size) = %ld\n", chunk.size);
						ListSize = chunk.size - sizeof(FOURCC); /* list content size = size of 'LIST' chunk - FOURCC ID of the list */
						ReadFile (fd, &chunk.id, sizeof(FOURCC), &BytesRead, NULL);
						switch (chunk.id)
						{
							case DMUS_FOURCC_UNFO_LIST: {
								TRACE("'UNFO': UNFO list (forward to DMUSIC_FillUNFOFromFileHandle(...))\n");
								SetFilePointer (fd, -(sizeof(DWORD) + 2*sizeof(FOURCC)), NULL, FILE_CURRENT); /* place pointer before the 'LIST' chunk */
								DMUSIC_FillUNFOFromFileHandle (UNFO, fd);								
								break;								
							} case DMUS_FOURCC_TRACK_LIST: {
								TRACE("'trkl': track list chunk (forward)\n");
								do {
									ReadFile (fd, &chunk.id, sizeof(FOURCC), &BytesRead, NULL); /* read RIFF */
									ReadFile (fd, &chunk.size, sizeof(DWORD), &BytesRead, NULL); /* read track size */
									TRACE("track size = %ld\n", chunk.size);
									ListCount += chunk.size + sizeof(DWORD) + sizeof(FOURCC);
									SetFilePointer (fd, -(sizeof(DWORD) + sizeof(FOURCC)), NULL, FILE_CURRENT); /* place pointer before the 'RIFF' chunk */
									DMUSIC_FillTrackFromFileHandle (NULL, fd); /* read encapsulated track as if it was in a track file */
									TRACE("(Track) List Count = %ld < (Track) List Size = %ld\n", ListCount, ListSize);
								} while (ListCount < ListSize);
								break;
							}
						}
						break;
					} case FOURCC_RIFF: {
						TRACE("'RIFF': embedded RIFF (size = %ld; could be embedded container form)\n", chunk.size);
						ReadFile (fd, &chunk.id, sizeof(FOURCC), &BytesRead, NULL);
						if (chunk.id == DMUS_FOURCC_CONTAINER_FORM) {
							TRACE("'DMCN': embedded container form (forward to DMUSIC_FillContainerFromFileHandle(...))\n");
							SetFilePointer (fd, -(sizeof(DWORD) + 2*sizeof(FOURCC)), NULL, FILE_CURRENT); /* place pointer before 'RIFF' chunk */
							DMUSIC_FillContainerFromFileHandle (NULL, fd);
						} else WARN("invalid chunk (only 'DMCN' chunk allowed)\n");
						break;	
					} case DMUS_FOURCC_TOOLGRAPH_FORM: {
						FIXME("'DMTG': toolgraph chunk: not supported yet\n");
						break;
					} case DMUS_FOURCC_AUDIOPATH_FORM: {
						FIXME("'DMAP': audiopath chunk: not supported yet\n");
						break;
					} default: {
						WARN("invalid chunk (only 'segh', 'guid', 'vers', 'LIST', 'RIFF', 'DMTG' and 'DMAP' chunks allowed)\n");
						break;
					}
				}
				TRACE("FileCount (%ld) < FileSize (%ld)\n", FileCount, FileSize);
			} while (FileCount < FileSize);
		} else {
			WARN("invalid chunk (only 'DMSG' chunk allowed)\n");
		}
	} else {
		WARN("'RIFF' not found: not a RIFF file\n");
	}
	
	return S_OK;
}

 /******************************************************************************
 * DMUSIC_FillScriptFromFileHandle: 
 *	- fills a IDirectMusicScriptImpl struct with data from file handle. 
 *	- IMPORTANT: it expects a RIFF chunk at beginning, so if you are calling it 
 *	             from another DMUSIC_Fill* function, make sure pointer is at
 *               correct place!
 *	- TODO: replace data in function with data in IDirectMusicScriptImpl
 */

HRESULT WINAPI DMUSIC_FillScriptFromFileHandle (IDirectMusicScriptImpl *script, HANDLE fd)
{
	rawChunk chunk;
	DWORD BytesRead/*, ListCount = 0*/, ListSize, FileCount = 0, FileSize;
	/* FIXME: Replace stuff located below with the stuff in script */
	UNFO_List UNFO;
	DMUS_IO_SCRIPT_HEADER header;
	DMUS_IO_VERSION version, scriptversion;
	GUID guid;
	WCHAR* scriptlang;
	WCHAR* scriptsrc;
	Reference scriptsrcref;
	
	TRACE("reading 'RIFF' chunk...\n");
	ReadFile (fd, &chunk.id, sizeof(FOURCC), &BytesRead, NULL);
	if (chunk.id == FOURCC_RIFF) {
		TRACE("'RIFF': RIFF file\n");
		ReadFile (fd, &chunk.size, sizeof(DWORD), &BytesRead, NULL); /* read size of 'RIFF' chunk */
		FileSize = chunk.size - sizeof(FOURCC); /* file content size = size of 'RIFF' chunk - FOURCC ID of following form */
		TRACE("reading chunks ...\n");
		ReadFile (fd, &chunk.id, sizeof(FOURCC), &BytesRead, NULL); /* read ID of following form */		
		if (chunk.id == DMUS_FOURCC_SCRIPT_FORM) {
			TRACE("'DMSC': script form\n");
			do {
				ReadFile (fd, &chunk.id, sizeof(FOURCC), &BytesRead, NULL);
				ReadFile (fd, &chunk.size, sizeof(DWORD), &BytesRead, NULL);
				FileCount += chunk.size + sizeof(DWORD) + sizeof(FOURCC);
				switch (chunk.id)
				{
					case DMUS_FOURCC_SCRIPT_CHUNK: {
						TRACE("'schd': script header\n");
						ReadFile (fd, &header, chunk.size, &BytesRead, NULL);
						TRACE_(dmfiledat)("=> dwFlags = %ld\n", header.dwFlags);
						break;
					} case DMUS_FOURCC_GUID_CHUNK: {
						TRACE("'guid': GUID\n");
						ReadFile (fd, &guid, chunk.size, &BytesRead, NULL);
						TRACE_(dmfiledat)("=> GUID = %s\n", debugstr_guid(&guid));
						break;
					} case DMUS_FOURCC_VERSION_CHUNK: {
						TRACE("'vers': version\n");
						ReadFile (fd, &version, chunk.size, &BytesRead, NULL);
						TRACE_(dmfiledat)("=> version = %ld%ld\n", version.dwVersionMS, version.dwVersionLS);
						break;
					} case FOURCC_LIST:{
						TRACE("'LIST': list (size) = %ld\n", chunk.size);
						ListSize = chunk.size - sizeof(FOURCC); /* list content size = size of 'LIST' chunk - FOURCC ID of the list */
						ReadFile (fd, &chunk.id, sizeof(FOURCC), &BytesRead, NULL);
						switch (chunk.id)
						{
							case DMUS_FOURCC_UNFO_LIST: {
								TRACE("'UNFO': UNFO list (forward to DMUSIC_FillUNFOFromFileHandle(...))\n");
								SetFilePointer (fd, -(sizeof(DWORD) + 2*sizeof(FOURCC)), NULL, FILE_CURRENT); /* place pointer before the 'LIST' chunk */
								DMUSIC_FillUNFOFromFileHandle (UNFO, fd);								
								break;
							} case DMUS_FOURCC_REF_LIST: {
								TRACE("'DMRF': reference list (forward to DMUSIC_FillReferenceFromFileHandle(...)\n");
								SetFilePointer (fd, -(sizeof(DWORD) + 2*sizeof(FOURCC)), NULL, FILE_CURRENT); /* place pointer before 'LIST' chunk */
								DMUSIC_FillReferenceFromFileHandle (scriptsrcref, fd);
							} default: {
								WARN("invalid chunk (only 'UNFO' and 'DMRF' chunks allowed)\n");
							}								
						}
						break;
					} case DMUS_FOURCC_SCRIPTVERSION_CHUNK: {
						TRACE("'scve': DirectMusic version\n");
						ReadFile (fd, &scriptversion, chunk.size, &BytesRead, NULL);
						TRACE_(dmfiledat)("=> script version = %ld%ld\n", scriptversion.dwVersionMS, scriptversion.dwVersionLS);
						break;
					} case FOURCC_RIFF: {
						TRACE("'RIFF': embedded RIFF (size = %ld; could be embedded container form)\n", chunk.size);
						ReadFile (fd, &chunk.id, sizeof(FOURCC), &BytesRead, NULL);
						if (chunk.id == DMUS_FOURCC_CONTAINER_FORM) {
							TRACE("'DMCN': embedded container form (forward to DMUSIC_FillContainerFromFileHandle(...))\n");
							SetFilePointer (fd, -(sizeof(DWORD) + 2*sizeof(FOURCC)), NULL, FILE_CURRENT); /* place pointer before 'RIFF' chunk */
							DMUSIC_FillContainerFromFileHandle (NULL, fd);
						} else WARN("invalid chunk (only 'DMCN' chunk allowed)\n");
						break;
					} case DMUS_FOURCC_SCRIPTLANGUAGE_CHUNK: {
						TRACE("'scla': scripting language\n");
						scriptlang = (WCHAR*) HeapAlloc (GetProcessHeap (), 0, chunk.size); /* allocate space */
						ReadFile (fd, scriptlang, chunk.size, &BytesRead, NULL);
						TRACE_(dmfiledat)("script language = %s\n", debugstr_w(scriptlang));						
						break;
					} case DMUS_FOURCC_SCRIPTSOURCE_CHUNK: {
						TRACE("'scsr': script source\n");
						scriptsrc = (WCHAR*) HeapAlloc (GetProcessHeap (), 0, chunk.size); /* allocate space */
						ReadFile (fd, scriptsrc, chunk.size, &BytesRead, NULL);
						TRACE_(dmfiledat)("script source = %s\n", debugstr_w(scriptsrc));							
						break;
					} default: {
						WARN("invalid chunk (only 'schd', 'guid', 'vers', 'LIST', 'scve', 'RIFF' and 'scla' chunks allowed)\n");
						break;
					}
				}
				TRACE("FileCount (%ld) < FileSize (%ld)\n", FileCount, FileSize);
			} while (FileCount < FileSize);
		} else {
			WARN("invalid chunk (only 'DMSC' chunk allowed)\n");
		}
	} else {
		WARN("'RIFF' not found: not a RIFF file\n");
	}
	
	return S_OK;
}

/******************************************************************************
 * DMUSIC_FillContainerFromFileHandle: 
 *	- fills a IDirectMusicContainerImpl struct with data from file handle. 
 *	- IMPORTANT: it expects a RIFF chunk at beginning, so if you are calling it 
 *	             from another DMUSIC_Fill* function, make sure pointer is at
 *               correct place!
 *	- TODO: replace data in function with data in IDirectMusicContainerImpl
 */
HRESULT WINAPI DMUSIC_FillContainerFromFileHandle (IDirectMusicContainerImpl *container, HANDLE fd)
{
	rawChunk chunk;
	DWORD BytesRead, ListCount = 0, ListSize, ListCount2 = 0, ListSize2, FileCount = 0, FileSize;
	/* FIXME: Replace stuff located below with the stuff in container */
	UNFO_List UNFO;
	DMUS_IO_CONTAINER_HEADER header;
	DMUS_IO_VERSION version;
	GUID guid;
	WCHAR* alias;
	DMUS_IO_CONTAINED_OBJECT_HEADER objheader;
	Reference dataref;
	
	TRACE("reading 'RIFF' chunk...\n");
	ReadFile (fd, &chunk.id, sizeof(FOURCC), &BytesRead, NULL);
	if (chunk.id == FOURCC_RIFF) {
		TRACE("'RIFF': RIFF file\n");
		ReadFile (fd, &chunk.size, sizeof(DWORD), &BytesRead, NULL); /* read size of 'RIFF' chunk */
		FileSize = chunk.size - sizeof(FOURCC); /* file content size = size of 'RIFF' chunk - FOURCC ID of following form */
		TRACE("reading chunks ...\n");
		ReadFile (fd, &chunk.id, sizeof(FOURCC), &BytesRead, NULL); /* read ID of following form */		
		if (chunk.id == DMUS_FOURCC_CONTAINER_FORM) {
			TRACE("'DMCN': container form\n");
			do {
				ReadFile (fd, &chunk.id, sizeof(FOURCC), &BytesRead, NULL);
				ReadFile (fd, &chunk.size, sizeof(DWORD), &BytesRead, NULL);
				FileCount += chunk.size + sizeof(DWORD) + sizeof(FOURCC);
				switch (chunk.id)
				{
					case DMUS_FOURCC_CONTAINER_CHUNK: {
						TRACE("'conh': container header\n");
						ReadFile (fd, &header, chunk.size, &BytesRead, NULL);
						TRACE_(dmfiledat)("=> dwFlags = %ld\n", header.dwFlags);
						break;
					} case DMUS_FOURCC_GUID_CHUNK: {
						TRACE("'guid': GUID\n");
						ReadFile (fd, &guid, chunk.size, &BytesRead, NULL);
						TRACE_(dmfiledat)("=> GUID = %s\n", debugstr_guid(&guid));
						break;
					} case DMUS_FOURCC_VERSION_CHUNK: {
						TRACE("'vers': version\n");
						ReadFile (fd, &version, chunk.size, &BytesRead, NULL);
						TRACE_(dmfiledat)("=> version = %ld%ld\n", version.dwVersionMS, version.dwVersionLS);
						break;
					} case FOURCC_LIST:{
						TRACE("'LIST': list (size) = %ld\n", chunk.size);
						ListSize = chunk.size - sizeof(FOURCC); /* list content size = size of 'LIST' chunk - FOURCC ID of the list */
						ReadFile (fd, &chunk.id, sizeof(FOURCC), &BytesRead, NULL);
						switch (chunk.id)
						{
							case DMUS_FOURCC_UNFO_LIST: {
								TRACE("'UNFO': UNFO list (forward to DMUSIC_FillUNFOFromFileHandle(...))\n");
								SetFilePointer (fd, -(sizeof(DWORD) + 2*sizeof(FOURCC)), NULL, FILE_CURRENT); /* place pointer before the 'LIST' chunk */
								DMUSIC_FillUNFOFromFileHandle (UNFO, fd);								
								break;
							} case DMUS_FOURCC_CONTAINED_OBJECTS_LIST: {
								TRACE("'cosl': objects list (content size = %ld)\n", ListSize);
								do {
									ReadFile (fd, &chunk.id, sizeof(FOURCC), &BytesRead, NULL);
									ReadFile (fd, &chunk.size, sizeof(DWORD), &BytesRead, NULL);
									ListCount += sizeof(FOURCC) + sizeof(DWORD) + chunk.size;	
									if (chunk.id == FOURCC_LIST && ReadFile (fd, &chunk.id, sizeof(FOURCC), &BytesRead, NULL) && chunk.id == DMUS_FOURCC_CONTAINED_OBJECT_LIST) {
										ListSize2 = chunk.size - sizeof(FOURCC); /* list content size = size of 'LIST' chunk - FOURCC ID of the list */
										ListCount2 = 0; /* reset */
										TRACE("'cobl': object (content size = %ld)\n", ListSize2);
										do {
											ReadFile (fd, &chunk.id, sizeof(FOURCC), &BytesRead, NULL);
											ReadFile (fd, &chunk.size, sizeof(DWORD), &BytesRead, NULL);
											ListCount2 += sizeof(FOURCC) + sizeof(DWORD) + chunk.size;
											switch (chunk.id)
											{
												case DMUS_FOURCC_CONTAINED_ALIAS_CHUNK: {
													TRACE("'coba': alias (size = %ld)\n", chunk.size);
													alias = (WCHAR*) HeapAlloc (GetProcessHeap (), 0, chunk.size); /* allocate space */
													ReadFile (fd, alias, chunk.size, &BytesRead, NULL);
													TRACE_(dmfiledat)("=> alias = %s\n", debugstr_w(alias));
													break;
												} case DMUS_FOURCC_CONTAINED_OBJECT_CHUNK: {
													TRACE("'cobh': object header (size = %ld)\n", chunk.size);
													ReadFile (fd, &objheader, chunk.size, &BytesRead, NULL);
													TRACE_(dmfiledat)("=> guidClassID = %s; dwFlags = %ld; ckid = %ld; fccType = %ld\n", \
														debugstr_guid(&objheader.guidClassID), objheader.dwFlags, objheader.ckid, objheader.fccType);
													break;
												} case FOURCC_LIST: {
													TRACE("'LIST': list chunk (size = %ld)\n", chunk.size);
													ReadFile (fd, &chunk.id, sizeof(FOURCC), &BytesRead, NULL);
													if (chunk.id == DMUS_FOURCC_REF_LIST) {
														TRACE("'DMRF': reference list (instead of 'data' chunk: size = %ld)\n", chunk.size - 4); /* set pointer at beginning of list */
														SetFilePointer (fd, -(sizeof(DWORD) + 2*sizeof(FOURCC)), NULL, FILE_CURRENT);
														DMUSIC_FillReferenceFromFileHandle (dataref, fd); /* forward to DMUSIC_FillReferenceFromFileHandle */
													} else WARN("invalid chunk (only 'DMRF' chunk allowed\n");						
													break;												
												} case FOURCC_RIFF: {
													TRACE("'RIFF': encapsulated data (can be 'DMSG' or 'DMSG')\n");
													ReadFile (fd, &chunk.id, sizeof(FOURCC), &BytesRead, NULL);
													SetFilePointer (fd, -(sizeof(DWORD) + 2*sizeof(FOURCC)), NULL, FILE_CURRENT); /* place pointer before 'RIFF' chunk */
													switch (chunk.id)
													{
														case DMUS_FOURCC_SEGMENT_FORM: {
															TRACE("'DMSG': embedded segment form (forward to DMUSIC_FillSegmentFromFileHandle(...))\n");
															DMUSIC_FillSegmentFromFileHandle (NULL, fd);
															break;
														} case DMUS_FOURCC_STYLE_FORM: {
															TRACE("'DMST': embedded style form (forward to DMUSIC_FillStyleFromFileHandle(...))\n");															
															DMUSIC_FillStyleFromFileHandle (NULL, fd);
															break;
														} case mmioFOURCC('W','A','V','E'): {
															FIXME("'WAVE': not yet supported (skipping)\n");															
															SetFilePointer (fd, sizeof(FOURCC) + sizeof(DWORD) + chunk.size, NULL, FILE_CURRENT); /* skip */
															break;
														} default: {
															WARN("invalid chunk (only 'DMSG' and 'DMST' chunks allowed)\n");
															break;
														}
													}
													break;
												} default: {
													WARN("invalid chunk (only 'coba', 'cobh', 'data' and 'LIST' chunks allowed\n");
													break;
												}
											}
											TRACE("ListCount2 (%ld) < ListSize2 (%ld)\n", ListCount2, ListSize2);							
										} while (ListCount2 < ListSize2);
									} else WARN("invalid chunk (only 'cobl' chunk allowed)\n");
									TRACE("ListCount (%ld) < ListSize (%ld)\n", ListCount, ListSize);							
								} while (ListCount < ListSize);
								break;								
							} default: {
								WARN("invalid chunk (only 'UNFO' and 'cosl' chunks allowed)\n");
							}								
						}
						break;
					} default: {
						WARN("invalid chunk (only 'schd', 'guid', 'vers', 'LIST', 'scve', 'RIFF' and 'scla' chunks allowed)\n");
						break;
					}
				}
				TRACE("FileCount (%ld) < FileSize (%ld)\n", FileCount, FileSize);
			} while (FileCount < FileSize);
		} else {
			WARN("invalid chunk (only 'DMSC' chunk allowed)\n");
		}
	} else {
		WARN("'RIFF' not found: not a RIFF file\n");
	}
	
	return S_OK;
}

/******************************************************************************
 * DMUSIC_FillStyleFromFileHandle: 
 *	- fills a IDirectMusicStyle8Impl struct with data from file handle. 
 *	- IMPORTANT: it expects a RIFF chunk at beginning, so if you are calling it 
 *	             from another DMUSIC_Fill* function, make sure pointer is at
 *               correct place!
 *	- TODO: replace data in function with data in IDirectMusicStyleImpl
 */
HRESULT WINAPI DMUSIC_FillStyleFromFileHandle (IDirectMusicStyle8Impl *style, HANDLE fd)
{
	rawChunk chunk;
	DWORD BytesRead, ListCount = 0, ListSize, ListCount2 = 0, ListSize2, FileCount = 0, FileSize;
	int i;
	/* FIXME: Replace stuff located below with the stuff in container */
	UNFO_List UNFO;
	DMUS_IO_STYLE header;
	DMUS_IO_VERSION version;
	GUID guid;
	Part part;
	Pattern pattern;
	
	TRACE("reading 'RIFF' chunk...\n");
	ReadFile (fd, &chunk.id, sizeof(FOURCC), &BytesRead, NULL);
	if (chunk.id == FOURCC_RIFF) {
		TRACE("'RIFF': RIFF file\n");
		ReadFile (fd, &chunk.size, sizeof(DWORD), &BytesRead, NULL); /* read size of 'RIFF' chunk */
		FileSize = chunk.size - sizeof(FOURCC); /* file content size = size of 'RIFF' chunk - FOURCC ID of following form */
		TRACE("reading chunks ...\n");
		ReadFile (fd, &chunk.id, sizeof(FOURCC), &BytesRead, NULL); /* read ID of following form */		
		if (chunk.id == DMUS_FOURCC_STYLE_FORM) {
			TRACE("'DMST': style form\n");
			do {
				ReadFile (fd, &chunk.id, sizeof(FOURCC), &BytesRead, NULL);
				ReadFile (fd, &chunk.size, sizeof(DWORD), &BytesRead, NULL);
				FileCount += chunk.size + sizeof(DWORD) + sizeof(FOURCC);

				switch (chunk.id)
				{
					case DMUS_FOURCC_STYLE_CHUNK: {
						TRACE("'styh': style header\n");
						ReadFile (fd, &header, chunk.size, &BytesRead, NULL);
						TRACE_(dmfiledat)("=> timeSig.bBeatsPerMeasure = %i; timeSig.bBeat = %i; timeSig.wGridsPerBeat = %d; dblTempo = %f\n", \
							header.timeSig.bBeatsPerMeasure, header.timeSig.bBeat, header.timeSig.wGridsPerBeat, header.dblTempo);
						break;
					} case DMUS_FOURCC_GUID_CHUNK: {
						TRACE("'guid': GUID\n");
						ReadFile (fd, &guid, chunk.size, &BytesRead, NULL);
						TRACE_(dmfiledat)("=> GUID = %s\n", debugstr_guid(&guid));
						break;
					} case DMUS_FOURCC_VERSION_CHUNK: {
						TRACE("'vers': version\n");
						ReadFile (fd, &version, chunk.size, &BytesRead, NULL);
						TRACE_(dmfiledat)("=> version = %ld%ld\n", version.dwVersionMS, version.dwVersionLS);
						break;
					} case FOURCC_RIFF: {
						TRACE("'RIFF': embedded RIFF (size = %ld; could be embedded band form)\n", chunk.size);
						ReadFile (fd, &chunk.id, sizeof(FOURCC), &BytesRead, NULL);
						if (chunk.id == DMUS_FOURCC_BAND_FORM) {
							TRACE("'DMBD': embedded band form (forward to DMUSIC_FillBandFromFileHandle)\n");
							SetFilePointer (fd, -(sizeof(DWORD) + 2*sizeof(FOURCC)), NULL, FILE_CURRENT); /* place pointer before 'RIFF' chunk */
							DMUSIC_FillBandFromFileHandle (NULL, fd);
						} else WARN("invalid chunk (only 'DMBD' chunk allowed)\n");
						break;						
					} case FOURCC_LIST:{
						TRACE("'LIST': list (size) = %ld\n", chunk.size);
						ListSize = chunk.size - sizeof(FOURCC); /* list content size = size of 'LIST' chunk - FOURCC ID of the list */
						ListCount = 0;
						ReadFile (fd, &chunk.id, sizeof(FOURCC), &BytesRead, NULL);
						switch (chunk.id)
						{
							case DMUS_FOURCC_UNFO_LIST: {
								TRACE("'UNFO': UNFO list (forward to DMUSIC_FillUNFOFromFileHandle(...))\n");
								SetFilePointer (fd, -(sizeof(DWORD) + 2*sizeof(FOURCC)), NULL, FILE_CURRENT); /* place pointer before the 'LIST' chunk */
								DMUSIC_FillUNFOFromFileHandle (UNFO, fd);								
								break;
							} case DMUS_FOURCC_PART_LIST: {
								TRACE("'part': parts list (content size = %ld)\n", ListSize);
								do {
									ReadFile (fd, &chunk.id, sizeof(FOURCC), &BytesRead, NULL);
									ReadFile (fd, &chunk.size, sizeof(DWORD), &BytesRead, NULL);
									ListCount += sizeof(FOURCC) + sizeof(DWORD) + chunk.size;
									switch (chunk.id)
									{
										case DMUS_FOURCC_PART_CHUNK: {
											TRACE("'prth': part header\n");
											ReadFile (fd, &part.header, chunk.size, &BytesRead, NULL);
											TRACE_(dmfiledat)("=> timeSig.bBeatsPerMeasure = %i; timeSig.bBeat = %i; timeSig.wGridsPerBeat = %d; dwVariationChoices = %p; guidPartID = %s; wNbrMeasures = %d; bPlayModeFlags = %i; bInvertUpper = %i; bInvertLower = %i; bPad = %p; dwFlags = %ld\n", \
												part.header.timeSig.bBeatsPerMeasure, part.header.timeSig.bBeat, part.header.timeSig.wGridsPerBeat, part.header.dwVariationChoices, \
												debugstr_guid (&part.header.guidPartID), part.header.wNbrMeasures, part.header.bPlayModeFlags, part.header.bInvertUpper, part.header.bInvertLower, \
												part.header.bPad, part.header.dwFlags);
											break;
										} case DMUS_FOURCC_NOTE_CHUNK: {
											TRACE("'note': notes (size = %ld)\n", chunk.size);
											part.nrofnotes = chunk.size - sizeof(DWORD); /* pure contents of 'note' (without first DWORD) */
											ReadFile (fd, &chunk.size, sizeof(DWORD), &BytesRead, NULL); /* read sizeof (DMUS_IO_STYLENOTE) */
											part.nrofnotes /= chunk.size; /*  nrofnotes = pure contents / sizeof (DMUS_IO_STYLENOTE) */
											part.notes = (DMUS_IO_STYLENOTE*) HeapAlloc (GetProcessHeap (), 0, chunk.size * part.nrofnotes);
											ReadFile (fd, part.notes, chunk.size * part.nrofnotes, &BytesRead, NULL);
											TRACE_(dmfiledat)("=> number of notes = %ld\n", part.nrofnotes);
											for (i = 0; i < part.nrofnotes; i++)
											{
												TRACE_(dmfiledat)("=> note[%i]: mtGridStart = %li; dwVariation = %ld; mtDuration = %li; nTimeOffset = %i; wMusicValue = %d; bVelocity = %i; bTimeRange = %i; bDurRange = %i; bVelRange = %i; bInversionID = %i; bPlayModeFlags = %i; bNoteFlags= %i;\n", \
													i, part.notes[i].mtGridStart, part.notes[i].dwVariation, part.notes[i].mtDuration, part.notes[i].nTimeOffset, part.notes[i].wMusicValue, part.notes[i].bVelocity, part.notes[i].bTimeRange, \
													part.notes[i].bDurRange, part.notes[i].bVelRange, part.notes[i].bInversionID, part.notes[i].bPlayModeFlags, part.notes[i].bNoteFlags);												
											}
											break;
										} case DMUS_FOURCC_CURVE_CHUNK: {
											TRACE("'crve': curves (size = %ld)\n", chunk.size);
											part.nrofcurves = chunk.size - sizeof(DWORD); /* pure contents of 'crve' (without first DWORD) */
											ReadFile (fd, &chunk.size, sizeof(DWORD), &BytesRead, NULL); /* read sizeof (DMUS_IO_STYLECURVE) */
											part.nrofcurves /= chunk.size; /*  nrofnotes = pure contents / sizeof (DMUS_IO_STYLECURVE) */
											part.curves = (DMUS_IO_STYLECURVE*) HeapAlloc (GetProcessHeap (), 0, chunk.size * part.nrofcurves);
											ReadFile (fd, part.curves, chunk.size * part.nrofcurves, &BytesRead, NULL);
											TRACE_(dmfiledat)("=> number of curves = %ld\n", part.nrofcurves);
											for (i = 0; i < part.nrofcurves; i++)
											{
												TRACE_(dmfiledat)("=> curve[%i]: mtGridStart = %li; dwVariation = %ld; mtDuration = %li; mtResetDuration = %li; nTimeOffset = %i; nStartValue = %i; nEndValue = %i; nResetValue = %i; bEventType = %i; bCurveShape = %i; bCCData = %i; bFlags = %i; wParamType = %d;wMergeIndex = %d\n", \
													i, part.curves[i].mtGridStart, part.curves[i].dwVariation, part.curves[i].mtDuration, part.curves[i].mtResetDuration, part.curves[i].nTimeOffset, part.curves[i].nStartValue, part.curves[i].nEndValue,  \
													part.curves[i].nResetValue, part.curves[i].bEventType, part.curves[i].bCurveShape, part.curves[i].bCCData, part.curves[i].bFlags, part.curves[i].wParamType, part.curves[i].wMergeIndex);
											}
											break;
										} case DMUS_FOURCC_MARKER_CHUNK: {
											TRACE("'mrkr': markers (size = %ld)\n", chunk.size);
											part.nrofmarkers = chunk.size - sizeof(DWORD); /* pure contents of 'mrkr' (without first DWORD) */
											ReadFile (fd, &chunk.size, sizeof(DWORD), &BytesRead, NULL); /* read sizeof (DMUS_IO_STYLEMARKER) */
											part.nrofmarkers /= chunk.size; /*  nrofnotes = pure contents / sizeof (DMUS_IO_STYLEMARKER) */
											part.markers = (DMUS_IO_STYLEMARKER*) HeapAlloc (GetProcessHeap (), 0, chunk.size * part.nrofmarkers);
											ReadFile (fd, part.markers, chunk.size * part.nrofmarkers, &BytesRead, NULL);
											TRACE_(dmfiledat)("=> number of markers = %ld\n", part.nrofmarkers);
										for (i = 0; i < part.nrofmarkers; i++)
											{
												TRACE_(dmfiledat)("=> marker[%i]: mtGridStart = %li; dwVariation = %ld; wMarkerFlags = %d\n", \
													i, part.markers[i].mtGridStart, part.markers[i].dwVariation, part.markers[i].wMarkerFlags);
											}
											break;
										} case DMUS_FOURCC_RESOLUTION_CHUNK: {
											TRACE("'rsln': resolutions (size = %ld)\n", chunk.size);
											part.nrofresolutions = chunk.size - sizeof(DWORD); /* pure contents of 'rsln' (without first DWORD) */
											ReadFile (fd, &chunk.size, sizeof(DWORD), &BytesRead, NULL); /* read sizeof (DMUS_IO_STYLERESOLUTION) */
											part.nrofresolutions /= chunk.size; /*  nrofnotes = pure contents / sizeof (DMUS_IO_STYLERESOLUTION) */
											part.resolutions = (DMUS_IO_STYLERESOLUTION*) HeapAlloc (GetProcessHeap (), 0, chunk.size * part.nrofresolutions);
											ReadFile (fd, part.resolutions, chunk.size * part.nrofresolutions, &BytesRead, NULL);
											TRACE_(dmfiledat)("=> number of resolutions = %ld\n", part.nrofresolutions);
										for (i = 0; i < part.nrofresolutions; i++)
											{
												TRACE_(dmfiledat)("=> resolution[%i]: dwVariation = %ld; wMusicValue = %d; bInversionID = %i; bPlayModeFlags = %i", \
													i, part.resolutions[i].dwVariation, part.resolutions[i].wMusicValue, part.resolutions[i].bInversionID, part.resolutions[i].bPlayModeFlags);
											}
											break;
										} case DMUS_FOURCC_ANTICIPATION_CHUNK: {
											TRACE("'anpn': anticipations (size = %ld)\n", chunk.size);
											part.nrofanticipations = chunk.size - sizeof(DWORD); /* pure contents of 'anpn' (without first DWORD) */
											ReadFile (fd, &chunk.size, sizeof(DWORD), &BytesRead, NULL); /* read sizeof (DMUS_IO_STYLE_ANTICIPATION) */
											part.nrofanticipations /= chunk.size; /*  nrofnotes = pure contents / sizeof (DMUS_IO_STYLE_ANTICIPATION) */
											part.anticipations = (DMUS_IO_STYLE_ANTICIPATION*) HeapAlloc (GetProcessHeap (), 0, chunk.size * part.nrofanticipations);
											ReadFile (fd, part.anticipations, chunk.size * part.nrofanticipations, &BytesRead, NULL);
											TRACE_(dmfiledat)("=> number of anticipations = %ld\n", part.nrofanticipations);
											for (i = 0; i < part.nrofanticipations; i++)
											{
												TRACE_(dmfiledat)("=> anticipation[%i]: mtGridStart = %li; dwVariation = %ld; nTimeOffset = %i; bTimeRange = %i\n", \
 													i, part.anticipations[i].mtGridStart, part.anticipations[i].dwVariation, part.anticipations[i].nTimeOffset, part.anticipations[i].bTimeRange);
											}
											break;
										} case FOURCC_LIST: {
											TRACE("'LIST': list chunk (size = %ld)\n", chunk.size);
											ReadFile (fd, &chunk.id, sizeof(FOURCC), &BytesRead, NULL);
											if (chunk.id == DMUS_FOURCC_UNFO_LIST) {
												TRACE("'UNFO': UNFO list (forward to DMUSIC_FillUNFOFromFileHandle(...))\n");
												SetFilePointer (fd, -(sizeof(DWORD) + 2*sizeof(FOURCC)), NULL, FILE_CURRENT);  /* set pointer at beginning of list */
												DMUSIC_FillUNFOFromFileHandle (part.UNFO, fd);								
											} else WARN("invalid chunk (only 'UNFO' chunk allowed\n");						
											break;																								
										} default: {
											WARN("invalid chunk (only 'prth','note', 'crve', 'mrkr', 'rsln', 'anpn' and 'LIST' chunks allowed\n");
											break;
										}
									}
									TRACE("ListCount (%ld) < ListSize (%ld)\n", ListCount, ListSize);							
								} while (ListCount < ListSize);
								break;	
							} case DMUS_FOURCC_PATTERN_LIST: {
								TRACE("'pttn': patterns list (content size = %ld)\n", ListSize);
								do {
									ReadFile (fd, &chunk.id, sizeof(FOURCC), &BytesRead, NULL);
									ReadFile (fd, &chunk.size, sizeof(DWORD), &BytesRead, NULL);
									ListCount += sizeof(FOURCC) + sizeof(DWORD) + chunk.size;
									switch (chunk.id)
									{
										case DMUS_FOURCC_PATTERN_CHUNK: {
											TRACE("'ptnh': pattern header\n");
											ReadFile (fd, &pattern.header, chunk.size, &BytesRead, NULL);
											TRACE_(dmfiledat)("=> timeSig.bBeatsPerMeasure = %i; timeSig.bBeat = %i; timeSig.wGridsPerBeat = %d; bGrooveBottom = %i; bGrooveTop = %i; wEmbellishment = %d; wNbrMeasures = %d; bDestGrooveBottom = %i; bDestGrooveTop = %i; dwFlags = %ld\n", \
												pattern.header.timeSig.bBeatsPerMeasure, pattern.header.timeSig.bBeat, pattern.header.timeSig.wGridsPerBeat, pattern.header.bGrooveBottom, pattern.header.bGrooveTop, pattern.header.wEmbellishment, \
												pattern.header.wNbrMeasures, pattern.header.bDestGrooveBottom, pattern.header.bDestGrooveTop, pattern.header.dwFlags); 
											break;
										} case DMUS_FOURCC_RHYTHM_CHUNK: {
											TRACE("'rhtm': rhytms\n");											
											pattern.nrofrhytms = chunk.size / sizeof(DWORD);
											TRACE_(dmfiledat)("=> number of rhytms = %ld\n", pattern.nrofrhytms);
											pattern.rhytms = (DWORD*) HeapAlloc (GetProcessHeap (), 0, sizeof(DWORD) * pattern.nrofrhytms);
											ReadFile (fd, pattern.rhytms, sizeof(DWORD) * pattern.nrofrhytms, &BytesRead, NULL);
											for (i = 0; i < pattern.nrofrhytms; i++)
											{
												TRACE_(dmfiledat)("=> rhytm[%i] = %ld\n", i, pattern.rhytms[i]);
											}
											break;
										} case DMUS_FOURCC_MOTIFSETTINGS_CHUNK: {
											TRACE("'mtfs': motif settings\n");											
											ReadFile (fd, &pattern.motsettings, chunk.size, &BytesRead, NULL);
											TRACE_(dmfiledat)("=> dwRepeats = %ld; mtPlayStart = %li; mtLoopStart = %li; mtLoopEnd = %li; dwResolution = %ld\n", \
												pattern.motsettings.dwRepeats, pattern.motsettings.mtPlayStart, pattern.motsettings.mtLoopStart, pattern.motsettings.mtLoopEnd, pattern.motsettings.dwResolution);
											break;											
										} case FOURCC_LIST: {
											TRACE("'LIST': list chunk (size = %ld)\n", chunk.size);
											ListSize2 = chunk.size - sizeof(FOURCC); /* list content size = size of 'LIST' chunk - FOURCC ID of the list */
											ListCount2 = 0;
											ReadFile (fd, &chunk.id, sizeof(FOURCC), &BytesRead, NULL);
											switch (chunk.id)
											{
												case DMUS_FOURCC_UNFO_LIST: {
													TRACE("'UNFO': UNFO list (forward to DMUSIC_FillUNFOFromFileHandle(...))\n");
													SetFilePointer (fd, -(sizeof(DWORD) + 2*sizeof(FOURCC)), NULL, FILE_CURRENT); /* place pointer before the 'LIST' chunk */
													DMUSIC_FillUNFOFromFileHandle (UNFO, fd);
													break;
												} case DMUS_FOURCC_PARTREF_LIST: {
													TRACE("'pref': part references list (content size = %ld)\n", ListSize2);
													do {
														ReadFile (fd, &chunk.id, sizeof(FOURCC), &BytesRead, NULL);
														ReadFile (fd, &chunk.size, sizeof(DWORD), &BytesRead, NULL);
														ListCount2 += sizeof(FOURCC) + sizeof(DWORD) + chunk.size;
														switch (chunk.id)
														{ 
															case DMUS_FOURCC_PARTREF_CHUNK:	{
																TRACE("'prfc': part reference\n");
																ReadFile (fd, &pattern.partref, chunk.size, &BytesRead, NULL);
																TRACE_(dmfiledat)("=> guidPartID = %s; wLogicalPartID = %d; bVariationLockID = %i; bSubChordLevel = %i; bPriority = %i; bRandomVariation = %i; wPad = %d; dwPChannel = %ld\n", \
																	debugstr_guid (&pattern.partref.guidPartID), pattern.partref.wLogicalPartID, pattern.partref.bVariationLockID, pattern.partref.bSubChordLevel, \
																	pattern.partref.bPriority, pattern.partref.bRandomVariation, pattern.partref.wPad, pattern.partref.dwPChannel);
															break;
															} case FOURCC_LIST: {
																TRACE("'LIST': list chunk (MSDN doesn't mention it)\n");
																ReadFile (fd, &chunk.id, sizeof(FOURCC), &BytesRead, NULL);
																if (chunk.id == DMUS_FOURCC_UNFO_LIST) {
																	TRACE("'UNFO': UNFO list (forward to DMUSIC_FillUNFOFromFileHandle(...))\n");
																	SetFilePointer (fd, -(sizeof(DWORD) + 2*sizeof(FOURCC)), NULL, FILE_CURRENT); /* place pointer before the 'LIST' chunk */
																	DMUSIC_FillUNFOFromFileHandle (UNFO, fd);
																} else {
																	WARN("invalid chunk (only 'UNFO' chunk allowed)\n");
															}
																break;
															} default: {
																WARN("invalid chunk (only 'prfc' and 'UNFO'chunk allowed)\n");
															}
														}
														TRACE("ListCount2 (%ld) < ListSize2 (%ld)\n", ListCount2, ListSize2);							
													} while (ListCount2 < ListSize2);
													break;
												} default: {
													WARN("invalid chunk (only 'UNFO' and 'pref' chunks allowed\n");
												break;
												}
											}
											break;													
										} case FOURCC_RIFF: {
											TRACE("'RIFF': embedded RIFF (size = %ld; could be embedded band form)\n", chunk.size);
											ReadFile (fd, &chunk.id, sizeof(FOURCC), &BytesRead, NULL);
											if (chunk.id == DMUS_FOURCC_BAND_FORM) {
												TRACE("'DMBD': embedded band form (forward to DMUSIC_FillBandFromFileHandle(...))\n");
												SetFilePointer (fd, -(sizeof(DWORD) + 2*sizeof(FOURCC)), NULL, FILE_CURRENT); /* place pointer before 'RIFF' chunk */
												DMUSIC_FillBandFromFileHandle (NULL, fd);
											} else WARN("invalid chunk (only 'DMBD' chunk allowed)\n");
											break;											
										} default: {
											WARN("invalid chunk (only 'prnh','rhtm', 'mtfs', 'LIST' and 'RIFF' chunks allowed\n");											
											break;
										}
									}
									TRACE("ListCount (%ld) < ListSize (%ld)\n", ListCount, ListSize);							
								} while (ListCount < ListSize);
								break;
							} default: {
							WARN("invalid chunk (only 'UNFO', 'part', 'pttn' and 'RIFF' chunks allowed)\n");
							}
					}
					break;
					} default: {
						WARN("invalid chunk (only 'styh', 'guid', 'vers', 'LIST', and 'RIFF' chunks allowed)\n");
						break;
					}
			}
				TRACE("FileCount (%ld) < FileSize (%ld)\n", FileCount, FileSize);
			} while (FileCount < FileSize);
		} else {
			WARN("invalid chunk (only 'DMST' chunk allowed)\n");
		}
 	} else {
 		WARN("'RIFF' not found: not a RIFF file\n");
	}

	return S_OK;
}

#endif
