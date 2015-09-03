/*
 * Regsvr32 definitions
 *
 * Copyright 2014 Hugh McMaster
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

/* Exit codes */
#define INVALID_ARG                           1
#define LOADLIBRARY_FAILED                    3
#define GETPROCADDRESS_FAILED                 4
#define DLLSERVER_FAILED                      5

/* Resource strings */
#define STRING_HEADER                      1000
#define STRING_USAGE                       1001
#define STRING_UNRECOGNIZED_SWITCH         1002
#define STRING_DLL_LOAD_FAILED             1003
#define STRING_PROC_NOT_IMPLEMENTED        1004
#define STRING_REGISTER_FAILED             1005
#define STRING_REGISTER_SUCCESSFUL         1006
#define STRING_UNREGISTER_FAILED           1007
#define STRING_UNREGISTER_SUCCESSFUL       1008
#define STRING_INSTALL_FAILED              1009
#define STRING_INSTALL_SUCCESSFUL          1010
#define STRING_UNINSTALL_FAILED            1011
#define STRING_UNINSTALL_SUCCESSFUL        1012
