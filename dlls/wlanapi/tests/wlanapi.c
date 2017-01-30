/*
 * Unit test suite for wlanapi functions
 *
 * Copyright 2017 Bruno Jesus
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

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wlanapi.h>

#include "wine/test.h"

START_TEST(wlanapi)
{
  HANDLE handle;
  DWORD neg_version;

  /* Windows checks the service before validating the client version so this
   * call will always result in error, no need to free the handle. */
  if (WlanOpenHandle(0, NULL, &neg_version, &handle) == ERROR_SERVICE_NOT_ACTIVE)
  {
      win_skip("No wireless service running\n");
      return;
  }
}
