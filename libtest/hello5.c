/*
 * This example demonstrates dynamical loading of (internal) Win32 DLLS.
 *
 * Copyright 1998 Marcus Meissner
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

#include <stdio.h>
#include <windows.h>

int PASCAL WinMain (HINSTANCE inst, HINSTANCE prev, LPSTR cmdline, int show)
{
	SYSTEM_INFO	si;
	void (CALLBACK *fnGetSystemInfo)(LPSYSTEM_INFO si);
	HMODULE	kernel32;

	kernel32 = LoadLibrary("KERNEL32");
	if (!kernel32) {
		fprintf(stderr,"FATAL: could not load KERNEL32!\n");
		return 0;
	}
	fnGetSystemInfo = (void*)GetProcAddress(kernel32,"GetSystemInfo");
	if (!fnGetSystemInfo) {
		fprintf(stderr,"FATAL: could not find GetSystemInfo!\n");
		return 0;
	}
	fnGetSystemInfo(&si);
	fprintf(stderr,"QuerySystemInfo returns:\n");
	fprintf(stderr,"	wProcessorArchitecture: %d\n",si.u.s.wProcessorArchitecture);
	fprintf(stderr,"	dwPageSize: %ld\n",si.dwPageSize);
	fprintf(stderr,"	lpMinimumApplicationAddress: %p\n",si.lpMinimumApplicationAddress);
	fprintf(stderr,"	lpMaximumApplicationAddress: %p\n",si.lpMaximumApplicationAddress);
	fprintf(stderr,"	dwActiveProcessorMask: %ld\n",si.dwActiveProcessorMask);
	fprintf(stderr,"	dwNumberOfProcessors: %ld\n",si.dwNumberOfProcessors);
	fprintf(stderr,"	dwProcessorType: %ld\n",si.dwProcessorType);
	fprintf(stderr,"	dwAllocationGranularity: %ld\n",si.dwAllocationGranularity);
	fprintf(stderr,"	wProcessorLevel: %d\n",si.wProcessorLevel);
	fprintf(stderr,"	wProcessorRevision: %d\n",si.wProcessorRevision);
	return 0;
}
