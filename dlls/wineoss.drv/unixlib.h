/*
 * Copyright 2022 Huw Davies
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

#include "../mmdevapi/unixlib.h"

NTSTATUS oss_midi_release(void *args);
NTSTATUS oss_midi_out_message(void *args);
NTSTATUS oss_midi_in_message(void *args);
NTSTATUS oss_midi_notify_wait(void *args);

#ifdef _WIN64
NTSTATUS oss_wow64_midi_out_message(void *args);
NTSTATUS oss_wow64_midi_in_message(void *args);
NTSTATUS oss_wow64_midi_notify_wait(void *args);
#endif

#define OSS_CALL(func, params) WINE_UNIX_CALL(func, params)
