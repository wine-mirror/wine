/*
 * Copyright 2023 Piotr Caban for CodeWeavers
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

#include <corecrt.h>

#if defined(__x86_64__) && _MSVCR_VER>=140

#include "msvcrt.h"

BOOL WINAPI DllMain(HINSTANCE hinst, DWORD reason, VOID *reserved)
{
  switch (reason)
  {
  case DLL_PROCESS_ATTACH:
      return msvcrt_init_handler4();
  case DLL_THREAD_ATTACH:
      msvcrt_attach_handler4();
      break;
  case DLL_PROCESS_DETACH:
      if (reserved) break;
      msvcrt_free_handler4();
      break;
  }
  return TRUE;
}

#endif
