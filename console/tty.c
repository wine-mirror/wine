/*
 * Copyright 1999 - Joseph Pranevich
 *
 * This is the console driver for TTY-based consoles, i.e. consoles
 * without cursor placement, etc. It's also a pretty decent starting
 * point for other drivers.
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

/* When creating new drivers, you need to assign all the functions that
   that driver supports into the driver struct. If it is a supplementary
   driver, it should make sure to perserve the old values. */

#include "config.h"

#include <stdio.h>

#include "console.h"
#include "windef.h"
void TTY_Start()
{
   /* This should be the root driver so we can ignore anything
      already in the struct. */

   driver.norefresh = FALSE;

   driver.write = TTY_Write;
   driver.getKeystroke = TTY_GetKeystroke;
}

void TTY_Write(char output, int fg, int bg, int attribute)
{
   /* We can discard all extended information. */
   fprintf(driver.console_out, "%c", output);
}

void TTY_GetKeystroke(char *scan, char *ch)
{
   /* All we have are character input things, nothing for extended */
   /* This is just the TTY driver, after all. We'll cope. */
   *ch = fgetc(driver.console_in);
}


