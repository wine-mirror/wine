/*  $Revision: 1.2 $
**
**  A "micro-shell" to test editline library.
**  If given any arguments, commands aren't executed.
*/
#include <stdio.h>
#if	defined(HAVE_STDLIB)
#include <stdlib.h>
#endif	/* defined(HAVE_STDLIB) */

const char version_string[] = "4.321"; 


extern char	*readline();
extern void	add_history();

#if	!defined(HAVE_STDLIB)
extern int	chdir();
extern int	free();
extern int	strncmp();
extern int	system();
extern void	exit();
#endif	/* !defined(HAVE_STDLIB) */


#if	defined(NEED_PERROR)
void
perror(s)
    char	*s;
{
    extern int	errno;

    (voidf)printf(stderr, "%s: error %d\n", s, errno);
}
#endif	/* defined(NEED_PERROR) */


/* ARGSUSED1 */
int
main(ac, av)
    int		ac;
    char	*av[];
{
    char	*p;
    int		doit;

    doit = ac == 1;
    while ((p = readline("testit> ")) != NULL) {
	(void)printf("\t\t\t|%s|\n", p);
	if (doit)
	    if (strncmp(p, "cd ", 3) == 0) {
		if (chdir(&p[3]) < 0)
		    perror(&p[3]);
	    }
	    else if (system(p) != 0)
		perror(p);
	add_history(p);
	free(p);
    }
    exit(0);
    /* NOTREACHED */
}
