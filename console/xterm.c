/* xterm.c */

/* This "driver" is designed to go on top of an existing driver
   to provide support for features only present if using an
   xterm or compatible program for your console output. It should
   inlcude such features as resizing, separation of output from the
   standard wine console, and a configurable title bar for
   Win32 console. */
/* Right now, it doesn't have any "special" features */

#include <signal.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <errno.h>

#include "windows.h"
#include "console.h"
#include "debug.h"

static BOOL32 wine_create_console(FILE **master, FILE **slave, int *pid);
static FILE *wine_openpty(FILE **master, FILE **slave, char *name,
                        struct termios *term, struct winsize *winsize);

/* The console -- I chose to keep the master and slave
 * (UNIX) file descriptors around in case they are needed for
 * ioctls later.  The pid is needed to destroy the xterm on close
 */
typedef struct _XTERM_CONSOLE {
        FILE    *master;                 /* xterm side of pty */
        FILE    *slave;                  /* wine side of pty */
        int     pid;                    /* xterm's pid, -1 if no xterm */
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

/**
 *  It looks like the openpty that comes with glibc in RedHat 5.0
 *  is buggy (second call returns what looks like a dup of 0 and 1
 *  instead of a new pty), this is a generic replacement.
 */
/** Can't we determine this using autoconf?
 */

static FILE *wine_openpty(FILE **master, FILE **slave, char *name,
                        struct termios *term, struct winsize *winsize)
{
        FILE *fdm, *fds;
        char *ptr1, *ptr2;
        char pts_name[512];

        strcpy (pts_name, "/dev/ptyXY");
        for (ptr1 = "pqrstuvwxyzPQRST"; *ptr1 != 0; ptr1++) {
                pts_name[8] = *ptr1;
                for (ptr2 = "0123456789abcdef"; *ptr2 != 0; ptr2++) {
                        pts_name[9] = *ptr2;

                        if ((fdm = fopen(pts_name, "r+")) == NULL) {
                                if (errno == ENOENT)
                                        return (FILE *) -1;
                                else
                                        continue;
                        }
                        pts_name[5] = 't';
                        if ((fds = fopen(pts_name, "r+")) == NULL) {
                                pts_name[5] = 'p';
                                continue;
                        }
                        *master = fdm;
                        *slave = fds;

                        if (term != NULL)
                                tcsetattr((*slave)->_fileno, TCSANOW, term);
                        if (winsize != NULL)
                                ioctl((*slave)->_fileno, TIOCSWINSZ, winsize);

                        if (name != NULL)
                                strcpy(name, pts_name);
                        return fds;
                }
        }
        return (FILE *) -1;
}

static BOOL32 wine_create_console(FILE **master, FILE **slave, int *pid)
{
        struct termios term;
        char buf[1024];
        char c = '\0';
        int status = 0;
        int i;

        if (tcgetattr(0, &term) < 0) return FALSE;
        term.c_lflag |= ICANON;
        term.c_lflag &= ~ECHO;
        if (wine_openpty(master, slave, NULL, &term, NULL) < 0)
           return FALSE;

        if ((*pid=fork()) == 0) {
                tcsetattr((*slave)->_fileno, TCSADRAIN, &term);
                sprintf(buf, "-Sxx%d", (*master)->_fileno);
                execlp("xterm", "xterm", buf, NULL);
                ERR(console, "error creating AllocConsole xterm\n");
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
                        WARN(console, "can't read xterm WID\n");
                        kill(*pid, SIGKILL);
                        return FALSE;
                }
        }
        term.c_lflag |= ECHO;
        tcsetattr((*master)->_fileno, TCSADRAIN, &term);

        return TRUE;
}
