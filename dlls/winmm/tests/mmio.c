/*
 * Unit tests for mmio APIs
 *
 * Copyright 2005 Dmitry Timoshkov
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

#include <assert.h>
#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "mmsystem.h"
#include "vfw.h"
#include "wine/test.h"

static const DWORD RIFF_buf[] =
{
    FOURCC_RIFF, 7*sizeof(DWORD)+sizeof(MainAVIHeader), mmioFOURCC('A','V','I',' '),
    FOURCC_LIST, sizeof(DWORD)+sizeof(MMCKINFO)+sizeof(MainAVIHeader), listtypeAVIHEADER,
    ckidAVIMAINHDR, sizeof(MainAVIHeader),
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static void test_mmioDescend(void)
{
    MMRESULT ret;
    HMMIO hmmio;
    MMIOINFO mmio;
    MMCKINFO ckRiff, ckList, ck;

    memset(&mmio, 0, sizeof(mmio));
    mmio.fccIOProc = FOURCC_MEM;
    mmio.cchBuffer = sizeof(RIFF_buf);
    mmio.pchBuffer = (char *)&RIFF_buf;

    /*hmmio = mmioOpen("msrle.avi", NULL, MMIO_READ);*/
    hmmio = mmioOpen(NULL, &mmio, MMIO_READ);
    ok(hmmio != 0, "mmioOpen error %u\n", mmio.wErrorRet);
    trace("hmmio = %p\n", hmmio);

    /* first normal RIFF AVI parsing */
    ret = mmioDescend(hmmio, &ckRiff, NULL, 0);
    ok(ret == MMSYSERR_NOERROR, "mmioDescend error %u\n", ret);
    ok(ckRiff.ckid == FOURCC_RIFF, "wrong ckid: %04x\n", ckRiff.ckid);
    ok(ckRiff.fccType == formtypeAVI, "wrong fccType: %04x\n", ckRiff.fccType);
    trace("ckid %4.4s cksize %04x fccType %4.4s off %04x flags %04x\n",
          (LPCSTR)&ckRiff.ckid, ckRiff.cksize, (LPCSTR)&ckRiff.fccType,
          ckRiff.dwDataOffset, ckRiff.dwFlags);

    ret = mmioDescend(hmmio, &ckList, &ckRiff, 0);
    ok(ret == MMSYSERR_NOERROR, "mmioDescend error %u\n", ret);
    ok(ckList.ckid == FOURCC_LIST, "wrong ckid: %04x\n", ckList.ckid);
    ok(ckList.fccType == listtypeAVIHEADER, "wrong fccType: %04x\n", ckList.fccType);
    trace("ckid %4.4s cksize %04x fccType %4.4s off %04x flags %04x\n",
          (LPCSTR)&ckList.ckid, ckList.cksize, (LPCSTR)&ckList.fccType,
          ckList.dwDataOffset, ckList.dwFlags);

    ret = mmioDescend(hmmio, &ck, &ckList, 0);
    ok(ret == MMSYSERR_NOERROR, "mmioDescend error %u\n", ret);
    ok(ck.ckid == ckidAVIMAINHDR, "wrong ckid: %04x\n", ck.ckid);
    ok(ck.fccType == 0, "wrong fccType: %04x\n", ck.fccType);
    trace("ckid %4.4s cksize %04x fccType %4.4s off %04x flags %04x\n",
          (LPCSTR)&ck.ckid, ck.cksize, (LPCSTR)&ck.fccType,
          ck.dwDataOffset, ck.dwFlags);

    /* test various mmioDescend flags */

    mmioSeek(hmmio, 0, SEEK_SET);
    memset(&ck, 0x66, sizeof(ck));
    ret = mmioDescend(hmmio, &ck, NULL, MMIO_FINDRIFF);
    ok(ret == MMIOERR_CHUNKNOTFOUND ||
       ret == MMIOERR_INVALIDFILE, "mmioDescend returned %u\n", ret);

    mmioSeek(hmmio, 0, SEEK_SET);
    memset(&ck, 0x66, sizeof(ck));
    ck.ckid = 0;
    ret = mmioDescend(hmmio, &ck, NULL, MMIO_FINDRIFF);
    ok(ret == MMIOERR_CHUNKNOTFOUND ||
       ret == MMIOERR_INVALIDFILE, "mmioDescend returned %u\n", ret);

    mmioSeek(hmmio, 0, SEEK_SET);
    memset(&ck, 0x66, sizeof(ck));
    ck.fccType = 0;
    ret = mmioDescend(hmmio, &ck, NULL, MMIO_FINDRIFF);
    ok(ret == MMSYSERR_NOERROR, "mmioDescend error %u\n", ret);
    ok(ck.ckid == FOURCC_RIFF, "wrong ckid: %04x\n", ck.ckid);
    ok(ck.fccType == formtypeAVI, "wrong fccType: %04x\n", ck.fccType);

    mmioSeek(hmmio, 0, SEEK_SET);
    memset(&ck, 0x66, sizeof(ck));
    ret = mmioDescend(hmmio, &ck, NULL, 0);
    ok(ret == MMSYSERR_NOERROR, "mmioDescend error %u\n", ret);
    ok(ck.ckid == FOURCC_RIFF, "wrong ckid: %04x\n", ck.ckid);
    ok(ck.fccType == formtypeAVI, "wrong fccType: %04x\n", ck.fccType);

    /* do NOT seek, use current file position */
    memset(&ck, 0x66, sizeof(ck));
    ck.fccType = 0;
    ret = mmioDescend(hmmio, &ck, NULL, MMIO_FINDLIST);
    ok(ret == MMSYSERR_NOERROR, "mmioDescend error %u\n", ret);
    ok(ck.ckid == FOURCC_LIST, "wrong ckid: %04x\n", ck.ckid);
    ok(ck.fccType == listtypeAVIHEADER, "wrong fccType: %04x\n", ck.fccType);

    mmioSeek(hmmio, 0, SEEK_SET);
    memset(&ck, 0x66, sizeof(ck));
    ck.ckid = 0;
    ck.fccType = listtypeAVIHEADER;
    ret = mmioDescend(hmmio, &ck, NULL, MMIO_FINDCHUNK);
    ok(ret == MMSYSERR_NOERROR, "mmioDescend error %u\n", ret);
    ok(ck.ckid == FOURCC_RIFF, "wrong ckid: %04x\n", ck.ckid);
    ok(ck.fccType == formtypeAVI, "wrong fccType: %04x\n", ck.fccType);

    /* do NOT seek, use current file position */
    memset(&ck, 0x66, sizeof(ck));
    ck.ckid = FOURCC_LIST;
    ret = mmioDescend(hmmio, &ck, NULL, MMIO_FINDCHUNK);
    ok(ret == MMSYSERR_NOERROR, "mmioDescend error %u\n", ret);
    ok(ck.ckid == FOURCC_LIST, "wrong ckid: %04x\n", ck.ckid);
    ok(ck.fccType == listtypeAVIHEADER, "wrong fccType: %04x\n", ck.fccType);

    mmioSeek(hmmio, 0, SEEK_SET);
    memset(&ck, 0x66, sizeof(ck));
    ck.ckid = FOURCC_RIFF;
    ret = mmioDescend(hmmio, &ck, NULL, MMIO_FINDCHUNK);
    ok(ret == MMSYSERR_NOERROR, "mmioDescend error %u\n", ret);
    ok(ck.ckid == FOURCC_RIFF, "wrong ckid: %04x\n", ck.ckid);
    ok(ck.fccType == formtypeAVI, "wrong fccType: %04x\n", ck.fccType);

    /* do NOT seek, use current file position */
    memset(&ckList, 0x66, sizeof(ckList));
    ckList.ckid = 0;
    ret = mmioDescend(hmmio, &ckList, &ck, MMIO_FINDCHUNK);
    ok(ret == MMSYSERR_NOERROR, "mmioDescend error %u\n", ret);
    ok(ckList.ckid == FOURCC_LIST, "wrong ckid: %04x\n", ckList.ckid);
    ok(ckList.fccType == listtypeAVIHEADER, "wrong fccType: %04x\n", ckList.fccType);

    mmioSeek(hmmio, 0, SEEK_SET);
    memset(&ck, 0x66, sizeof(ck));
    ret = mmioDescend(hmmio, &ck, NULL, MMIO_FINDCHUNK);
    ok(ret == MMIOERR_CHUNKNOTFOUND ||
       ret == MMIOERR_INVALIDFILE, "mmioDescend returned %u\n", ret);
    ok(ck.ckid != 0x66666666, "wrong ckid: %04x\n", ck.ckid);
    ok(ck.fccType != 0x66666666, "wrong fccType: %04x\n", ck.fccType);
    ok(ck.dwDataOffset != 0x66666666, "wrong dwDataOffset: %04x\n", ck.dwDataOffset);

    mmioSeek(hmmio, 0, SEEK_SET);
    memset(&ck, 0x66, sizeof(ck));
    ret = mmioDescend(hmmio, &ck, NULL, MMIO_FINDRIFF);
    ok(ret == MMIOERR_CHUNKNOTFOUND ||
       ret == MMIOERR_INVALIDFILE, "mmioDescend returned %u\n", ret);

    mmioClose(hmmio, 0);
}

START_TEST(mmio)
{
    test_mmioDescend();
}
