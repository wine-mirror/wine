/*
 * Msiexec (msiexec.h)
 *
 * Copyright 2004 Vincent Béron
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

typedef HRESULT (*DLLREGISTERSERVER)(void);
typedef HRESULT (*DLLUNREGISTERSERVER)(void);

/* Logging flags */

#define LOG_STATUS_MESSAGES		0x00000001
#define LOG_NONFATAL_WARNINGS		0x00000002
#define LOG_ALL_ERROR_MESSAGES		0x00000004
#define LOG_STARTUP_OF_ACTIONS		0x00000008
#define LOG_ACTION_SPECIFIC_RECORDS	0x00000010
#define LOG_USER_REQUESTS		0x00000020
#define LOG_INITIAL_USER_INTERFACE_PARAMETERS	0x00000040
#define LOG_OUT_OF_MEMORY		0x00000080
#define LOG_TERMINAL_PROPERTIES		0x00000100
#define LOG_VERBOSE_OUTPUT		0x00000200
#define LOG_APPEND_TO_EXISTING_FILE	0x00000400
#define LOG_FLUSH_EACH_LINE		0x00000800
#define LOG_WILDCARD			0x000001ff
