/* xterm.c */

/* This "driver" is designed to go on top of an existing driver
   to provide support for features only present if using an
   xterm or compatible program for your console output. 
   Currently, it supports resizing and separating debug messages from
   program output.
   It does not currently support changing the title bar.
*/

#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <errno.h>

#include "console.h"
#include "options.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(console)

char console_xterm_prog[80];

static BOOL wine_create_console(FILE **master, FILE **slave, int *pid);
FILE *wine_openpty(int *master, int *slave, char *name,
                        struct termios *term, struct winsize *winsize);

/* The console -- I chose to keep the master and slave
 * (UNIX) file descriptors around in case they are needed for
 * ioctls later.  The pid is needed to destroy the xterm on close
 */
typedef struct _XTERM_CONSOLE {
        FILE    *master;                 /* xterm side of pty */
        FILE    *slave;                  /* wine side of pty */
        int     pid;                     /* xterm's pid, -1 if no xterm */
} XTERM_CONSOLE;

static XTERM_CONSOLE xterm_console;

CONSOLE_device chain;
FILE *old_in, *old_out;

void XTERM_Start()
{
   /* Here, this is a supplementary driver so we should remember to call
      the chain. */
   chain.init = driver.init;
   driver.init = XTERM_Init;

   chain.close = driver.close;
   driver.close = XTERM_Close;

   chain.resizeScreen = driver.resizeScreen;
   driver.resizeScreen = XTERM_ResizeScreen;

   /* Read in driver configuration */
   PROFILE_GetWineIniString("console", "XtermProg",
      "xterm", console_xterm_prog, 79); 

}

void XTERM_Init()
{
   wine_create_console(&xterm_console.master, &xterm_console.slave,
      &xterm_console.pid);

   old_in = driver.console_in;
   driver.console_in = xterm_console.slave;

   old_out = driver.console_out;
   driver.console_out = xterm_console.slave;

   /* Then call the chain... */
   if (chain.init)
      chain.init();
}

void XTERM_Close()
{
   /* Call the chain first... */
   if (chain.close)
      chain.close();

   driver.console_in = old_in;
   driver.console_out = old_out;

   /* make sure a xterm exists to kill */
   if (xterm_console.pid != -1) {
      kill(xterm_console.pid, SIGTERM);
   }
}

void XTERM_ResizeScreen(int x, int y)
{
   char temp[100];

   /* Call the chain first, there shoudln't be any... */
   if (chain.resizeScreen)
      chain.resizeScreen(x, y);

   sprintf(temp, "\x1b[8;%d;%dt", y, x);
   CONSOLE_WriteRawString(temp);

   CONSOLE_NotifyResizeScreen(x, y);
}


static BOOL wine_create_console(FILE **master, FILE **slave, int *pid)
{
        /* There is definately a bug in this routine that causes a lot
           of garbage to be written to the screen, but I can't find it...
        */
        struct termios term;
        char buf[1024];
        char c = '\0';
        int status = 0;
        int i;
        int tmaster, tslave;
        char xterm_resolution[10];

        sprintf(xterm_resolution, "%dx%d", driver.x_res,
           driver.y_res);

        if (tcgetattr(0, &term) < 0) return FALSE;
        term.c_lflag |= ICANON;
        term.c_lflag &= ~ECHO;
        if (wine_openpty(&tmaster, &tslave, NULL, &term, NULL) < 0)
           return FALSE;
        *master = fdopen(tmaster, "r+");
        *slave = fdopen(tslave, "r+");

        if ((*pid=fork()) == 0) {
                tcsetattr(fileno(*slave), TCSADRAIN, &term);
                sprintf(buf, "-Sxx%d", fileno(*master));
                execlp(console_xterm_prog, console_xterm_prog, buf, "-fg",
                   "white", "-bg", "black", "-g",
                   xterm_resolution, NULL);
                ERR("error creating xterm (file not found?)\n");
                exit(1);
        }

        /* most xterms like to print their window ID when used with -S;
         * read it and continue before the user has a chance...
         * NOTE: this is the reason we started xterm with ECHO off,
         * we'll turn it back on below
         */

        for (i=0; c!='\n'; (status=fread(&c, 1, 1, *slave)), i++) {
                if (status == -1 && c == '\0') {
                                /* wait for xterm to be created */
                        usleep(100);
                }
                if (i > 10000) {
                        WARN("can't read xterm WID\n");
                        kill(*pid, SIGKILL);
                        return FALSE;
                }
        }
        term.c_lflag |= ECHO;
        tcsetattr(fileno(*master), TCSADRAIN, &term);

        return TRUE;
}
