/* tty.c */

/* This is the console driver for TTY-based consoles, i.e. consoles
   without cursor placement, etc. It's also a pretty decent starting
   point for other driers.
*/

/* When creating new drivers, you need to assign all the functions that
   that driver supports into the driver struct. If it is a supplementary
   driver, it should make sure to perserve the old values. */

#include <stdio.h>
#include "console.h"
#include "config.h"
#include "wintypes.h" /* FALSE */
#include "windows.h" /* _lread16() */
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
   printf("%c", output);
}

void TTY_GetKeystroke(char *ch, char *scan)
{
   /* All we have are character input things, nothing for extended */
   /* This is just the TTY driver, after all. We'll cope. */
   _lread16(0, ch, 0);
}


