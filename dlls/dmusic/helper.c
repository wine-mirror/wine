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
					UNFO.name = (WCHAR*) HeapAlloc (GetProcessHeap (), 0, sizeof(WCHAR) * chunk.size); /* allocate space */
					ReadFile (fd, UNFO.name, chunk.size, &BytesRead, NULL);
					TRACE_(dmfiledat)("=> name = %s\n", debugstr_w(UNFO.name));
					break;
				} case DMUS_FOURCC_UART_CHUNK: {
					TRACE("'UART': artist\n");
					UNFO.artist = (WCHAR*) HeapAlloc (GetProcessHeap (), 0, sizeof(WCHAR) * chunk.size); /* allocate space */
					ReadFile (fd, UNFO.artist, chunk.size, &BytesRead, NULL);
					TRACE_(dmfiledat)("artist = %s\n", debugstr_w(UNFO.artist));
					break;
				} case DMUS_FOURCC_UCOP_CHUNK: {
					TRACE("'UCOP': copyright\n");
					UNFO.copyright = (WCHAR*) HeapAlloc (GetProcessHeap (), 0, sizeof(WCHAR) * chunk.size); /* allocate space */
					ReadFile (fd, UNFO.copyright, chunk.size, &BytesRead, NULL);
					TRACE_(dmfiledat)("=> copyright = %s\n", debugstr_w(UNFO.copyright));
					break;
				} case DMUS_FOURCC_USBJ_CHUNK:{
					TRACE("'USBJ': subject\n");
					UNFO.subject = (WCHAR*) HeapAlloc (GetProcessHeap (), 0, sizeof(WCHAR) * chunk.size); /* allocate space */
					ReadFile (fd, UNFO.subject, chunk.size, &BytesRead, NULL);
					TRACE_(dmfiledat)("=> subject = %s\n", debugstr_w(UNFO.subject));
					break;
				} case DMUS_FOURCC_UCMT_CHUNK: {
					TRACE("'UCMT': comment\n");
					UNFO.comment = (WCHAR*) HeapAlloc (GetProcessHeap (), 0, sizeof(WCHAR) * chunk.size); /* allocate space */
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
					reference.name = (WCHAR*) HeapAlloc (GetProcessHeap (), 0, sizeof(WCHAR) * chunk.size);
					ReadFile (fd, reference.name, chunk.size, &BytesRead, NULL);
					TRACE_(dmfiledat)("=> name = %s\n", debugstr_w (reference.name));
					break;
				} case DMUS_FOURCC_FILE_CHUNK: {
					TRACE("'file': file name\n");
					reference.file = (WCHAR*) HeapAlloc (GetProcessHeap (), 0, sizeof(WCHAR) * chunk.size);
					ReadFile (fd, reference.file, chunk.size, &BytesRead, NULL);
					TRACE_(dmfiledat)("=> file name = %s\n", debugstr_w (reference.file));
					break;
				} case DMUS_FOURCC_CATEGORY_CHUNK: {
					TRACE("'catg': category\n");
					reference.category = (WCHAR*) HeapAlloc (GetProcessHeap (), 0, sizeof(WCHAR) * chunk.size);
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
 *	- fills a IDirectMusicTrackImpl struct with data from file handle. 
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
	DWORD BytesRead, ListCount = 0, ListCount2 = 0, ListSize, ListSize2, FileCount = 0, FileSize, FileCount2 = 0, FileSize2 /* *2s are for various subchunks  */;
	
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
											int i;
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
																	} else WARN("invalid chunk (only 'RIFF' chunk allowed)\n");
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
					} case DMUS_FOURCC_PERS_TRACK_LIST: {
						FIXME("'pftr': chordmap track list: not supported yet\n");
						break;
					} case DMUS_FOURCC_COMMANDTRACK_CHUNK: {
						TRACE("'cmnd': command track\n");
						ReadFile (fd, &chunk.size, sizeof(DWORD), &BytesRead, NULL); /* read sizeof(DMUS_IO_COMMAND) */
						ReadFile (fd, &command, chunk.size, &BytesRead, NULL); /* read DMUS_IO_COMMAND */
						TRACE_(dmfiledat)("wMeasure = %d; bBeat = %i; bCommand = %i; bGrooveLevel = %i; bGrooveRange = %i; bRepeatMode = %i\n", \
							command.wMeasure, command.bBeat, command.bCommand, command.bGrooveLevel, command.bGrooveRange, command.bRepeatMode);
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
					}  case DMUS_FOURCC_MUTE_CHUNK: {
						FIXME("'mute': mute track chunk: not supported yet\n");
						break;
					}  case DMUS_FOURCC_PARAMCONTROLTRACK_TRACK_LIST: {
						FIXME("'prmt': parameter control track list: not supported yet\n");
						break;
					}  case DMUS_FOURCC_PATTERN_FORM: {
						FIXME("'DMPT': pattern track form: not supported yet\n");
						break;
					}  case DMUS_FOURCC_SCRIPTTRACK_LIST: {
						FIXME("'scrt': script track list: not supported yet\n");
						break;
					}  case DMUS_FOURCC_SEGTRACK_LIST: {
						FIXME("'segt': segment trigger track list: not supported yet\n");
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
							WARN("there seem so te ba problem: file claims that size of DMUSIC_IO_TEMPO_ITEM is %ld, while real sizeof returns %i", \
								chunk.size, sizeof(DMUS_IO_TEMPO_ITEM));
						ReadFile (fd, &tempo, chunk.size, &BytesRead, NULL);
						TRACE_(dmfiledat)("lTime = %ld; dblTempo = %f\n", tempo.lTime, tempo.dblTempo);
						break;
					}  case DMUS_FOURCC_TIMESIGNATURE_TRACK: {
						FIXME("'tims': time signature track list: not supported yet\n");
						break;
					}  case DMUS_FOURCC_WAVETRACK_LIST: {
						FIXME("'wavt': wave track list not supported yet\n");
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
						TRACE_(dmfiledat)("dwRepeats = %ld; mtLength = %li; mtPlayStart = %li; mtLoopStart = %li; mtLoopEnd = %li; dwResolution = %ld; rtLenght = FIXME; dwFlags = %ld; dwReserved = %ld\n", \
							header.dwRepeats, header.mtLength, header.mtPlayStart, header.mtLoopStart, header.mtLoopEnd, header.dwResolution/*, header.rtLenght*/, header.dwFlags, header.dwReserved);
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
					} case DMUS_FOURCC_CONTAINER_FORM: {
						FIXME("'DMCN': container form chunk: not supported yet\n");
						break;
					} case DMUS_FOURCC_TOOLGRAPH_FORM: {
						FIXME("'DMTG': toolgraph chunk: not supported yet\n");
						break;
					} case DMUS_FOURCC_AUDIOPATH_FORM: {
						FIXME("'DMAP': audiopath chunk: not supported yet\n");
						break;
					} default: {
						FIXME("invalid chunk (only 'segh', 'guid', 'vers', 'LIST', 'DMCN', 'DMTG' and 'DMAP' chunks allowed)\n");
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
