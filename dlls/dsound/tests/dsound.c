/*
 * Unit tests for dsound functions
 *
 * Copyright (c) 2002 Francois Gouget
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "wine/test.h"
#include "dsound.h"



BOOL WINAPI dsenum_callback(LPGUID lpGuid, LPCSTR lpcstrDescription,
                            LPCSTR lpcstrModule, LPVOID lpContext)
{
    HRESULT rc;
    LPDIRECTSOUND dso;

    winetest_trace("Testing %s - %s\n",lpcstrDescription,lpcstrModule);
    rc=DirectSoundCreate(lpGuid,&dso,NULL);
    ok(rc==DS_OK,"DirectSoundCreate failed: %lx\n",rc);
    if (rc==DS_OK) {
        DSCAPS caps;

        caps.dwSize=0;
        rc=IDirectSound_GetCaps(dso,&caps);
        ok(rc==DSERR_INVALIDPARAM,"GetCaps should have failed: %lx\n",rc);

        caps.dwSize=sizeof(caps);
        rc=IDirectSound_GetCaps(dso,&caps);
        ok(rc==DS_OK,"GetCaps failed: %lx\n",rc);
        if (rc==DS_OK) {
            winetest_trace("  flags=%lx secondary min=%ld max=%ld\n",
                           caps.dwFlags,caps.dwMinSecondarySampleRate,
                           caps.dwMaxSecondarySampleRate);
        }

        IDirectSound_Release(dso);
    }
    return 1;
}

void dsound_out_tests()
{
    HRESULT rc;
    rc=DirectSoundEnumerateA(&dsenum_callback,NULL);
    ok(rc==DS_OK,"DirectSoundEnumerate failed: %ld\n",rc);
}

START_TEST(dsound)
{
    dsound_out_tests();
}
