/*
 * Wine porting definitions
 *
 */

#ifndef __WINE_WINE_PORT_H
#define __WINE_WINE_PORT_H

#include "config.h"

#include <sys/types.h>
#include <sys/time.h>

/* Types */

#if !defined(HAVE_GETNETBYADDR) && !defined(HAVE_GETNETBYNAME)
struct netent {
  char         *n_name;
  char        **n_aliases;
  int           n_addrtype;
  unsigned long n_net;
};
#endif /* !defined(HAVE_GETNETBYADDR) && !defined(HAVE_GETNETBYNAME) */

#if !defined(HAVE_GETPROTOBYNAME) && !defined(HAVE_GETPROTOBYNUMBER)
struct  protoent {
  char  *p_name;
  char **p_aliases;
  int    p_proto;
};
#endif /* !defined(HAVE_GETPROTOBYNAME) && !defined(HAVE_GETPROTOBYNUMBER) */

#ifndef HAVE_STATFS
# ifdef __BEOS__
#  define STATFS_HAS_BFREE
struct statfs {
  long   f_bsize;  /* block_size */
  long   f_blocks; /* total_blocks */
  long   f_bfree;  /* free_blocks */
};
# else /* defined(__BEOS__) */
struct statfs;
# endif /* defined(__BEOS__) */
#endif /* !defined(HAVE_STATFS) */

/* Functions */

#if !defined(HAVE_CLONE) && defined(linux)
int clone(int (*fn)(void *arg), void *stack, int flags, void *arg);
#endif /* !defined(HAVE_CLONE) && defined(linux) */

#ifndef HAVE_GETNETBYADDR
struct netent *getnetbyaddr(unsigned long net, int type);
#endif /* defined(HAVE_GETNETBYNAME) */

#ifndef HAVE_GETNETBYNAME
struct netent *getnetbyname(const char *name);
#endif /* defined(HAVE_GETNETBYNAME) */

#ifndef HAVE_GETPROTOBYNAME
struct protoent *getprotobyname(const char *name);
#endif /* !defined(HAVE_GETPROTOBYNAME) */

#ifndef HAVE_GETPROTOBYNUMBER
struct protoent *getprotobynumber(int proto);
#endif /* !defined(HAVE_GETPROTOBYNUMBER) */

#ifndef HAVE_GETSERVBYPORT
struct servent *getservbyport(int port, const char *proto);
#endif /* !defined(HAVE_GETSERVBYPORT) */

#ifndef HAVE_GETSOCKOPT
int getsockopt(int socket, int level, int option_name, void *option_value, size_t *option_len);
#endif /* !defined(HAVE_GETSOCKOPT) */

#ifndef HAVE_MEMMOVE
void *memmove(void *dest, const void *src, unsigned int len);
#endif /* !defined(HAVE_MEMMOVE) */

#ifndef HAVE_INET_NETWORK
unsigned long inet_network(const char *cp);
#endif /* !defined(HAVE_INET_NETWORK) */

#ifndef HAVE_SETTIMEOFDAY
int settimeofday(struct timeval *tp, void *reserved);
#endif /* !defined(HAVE_SETTIMEOFDAY) */

#ifndef HAVE_STATFS
int statfs(const char *name, struct statfs *info);
#endif /* !defined(HAVE_STATFS) */

#ifndef HAVE_STRNCASECMP
int strncasecmp(const char *str1, const char *str2, size_t n);
#endif /* !defined(HAVE_STRNCASECMP) */

#ifndef HAVE_STRERROR
const char *strerror(int err);
#endif /* !defined(HAVE_STRERROR) */

#ifndef HAVE_STRCASECMP
int strcasecmp(const char *str1, const char *str2);
#endif /* !defined(HAVE_STRCASECMP) */

#ifndef HAVE_USLEEP
int usleep (unsigned int useconds);
#endif /* !defined(HAVE_USLEEP) */

#endif /* !defined(__WINE_WINE_PORT_H) */
