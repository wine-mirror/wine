/*
 * Copyright 2014 Dmitry Timoshkov
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

#ifndef __WINE_SCHEDSVC_PRIVATE_H__
#define __WINE_SCHEDSVC_PRIVATE_H__

void schedsvc_auto_start(void) DECLSPEC_HIDDEN;
void add_job(const WCHAR *name) DECLSPEC_HIDDEN;
void remove_job(const WCHAR *name) DECLSPEC_HIDDEN;
void check_task_state(void) DECLSPEC_HIDDEN;
void add_process_to_queue(HANDLE hproc) DECLSPEC_HIDDEN;
void update_process_status(DWORD pid) DECLSPEC_HIDDEN;
BOOL get_next_runtime(LARGE_INTEGER *rt) DECLSPEC_HIDDEN;
void check_task_time(void) DECLSPEC_HIDDEN;
void load_at_tasks(void) DECLSPEC_HIDDEN;
void check_missed_task_time(void) DECLSPEC_HIDDEN;

#endif /* __WINE_SCHEDSVC_PRIVATE_H__ */
