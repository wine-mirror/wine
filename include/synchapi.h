/*
 * Copyright 2018 Daniel Lehman
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

#ifndef _SYNCHAPI_H
#define _SYNCHAPI_H

#ifdef __cplusplus
extern "C" {
#endif

BOOL WINAPI WaitOnAddress(volatile void*, void*, SIZE_T, DWORD);
void WINAPI WakeByAddressAll(void*);
void WINAPI WakeByAddressSingle(void*);

typedef RTL_BARRIER SYNCHRONIZATION_BARRIER;
typedef PRTL_BARRIER PSYNCHRONIZATION_BARRIER;
typedef PRTL_BARRIER LPSYNCHRONIZATION_BARRIER;

#define SYNCHRONIZATION_BARRIER_FLAGS_SPIN_ONLY  0x1
#define SYNCHRONIZATION_BARRIER_FLAGS_BLOCK_ONLY 0x2
#define SYNCHRONIZATION_BARRIER_FLAGS_NO_DELETE  0x4

BOOL WINAPI InitializeSynchronizationBarrier(SYNCHRONIZATION_BARRIER *,LONG, LONG);
BOOL WINAPI DeleteSynchronizationBarrier(SYNCHRONIZATION_BARRIER *);
BOOL WINAPI EnterSynchronizationBarrier(SYNCHRONIZATION_BARRIER*, DWORD);

#ifdef __cplusplus
}
#endif

#endif  /* _SYNCHAPI_H */
