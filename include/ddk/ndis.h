/*
 * ndis.h
 *
 * Copyright 2015 Austin English
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
#ifndef _NDIS_
#define _NDIS_

typedef void *NDIS_HANDLE, *PNDIS_HANDLE;
typedef int   NDIS_STATUS, *PNDIS_STATUS;

typedef struct _NDIS_SPIN_LOCK
{
  KSPIN_LOCK SpinLock;
  KIRQL      OldIrql;
} NDIS_SPIN_LOCK, *PNDIS_SPIN_LOCK;

#define NDIS_STATUS_FAILURE                                ((NDIS_STATUS) STATUS_UNSUCCESSFUL)

NDIS_STATUS WINAPI NdisAllocateMemoryWithTag(void **, UINT, ULONG);
void WINAPI NdisAllocateSpinLock(NDIS_SPIN_LOCK *);

#endif /* _NDIS_ */
