/*  $Revision: 1.1 $
**
**  OS-9 system-dependant routines for editline library.
*/
#include "editline.h"
#include <sgstat.h>
#include <modes.h>


void
rl_ttyset(Reset)
    int			Reset;
{
    static struct sgbuf	old;
    struct sgbuf	new;


    if (Reset == 0) {
        _gs_opt(0, &old);
        _gs_opt(0, &new);
        new.sg_backsp = 0;	new.sg_delete = 0;	new.sg_echo = 0;
        new.sg_alf = 0;		new.sg_nulls = 0;	new.sg_pause = 0;
        new.sg_page = 0;	new.sg_bspch = 0;	new.sg_dlnch = 0;
        new.sg_eorch = 0;	new.sg_eofch = 0;	new.sg_rlnch = 0;
        new.sg_dulnch = 0;	new.sg_psch = 0;	new.sg_kbich = 0;
        new.sg_kbach = 0;	new.sg_bsech = 0;	new.sg_bellch = 0;
        new.sg_xon = 0;		new.sg_xoff = 0;	new.sg_tabcr = 0;
        new.sg_tabsiz = 0;
        _ss_opt(0, &new);
        rl_erase = old.sg_bspch;
        rl_kill = old.sg_dlnch;
        rl_eof = old.sg_eofch;
        rl_intr = old.sg_kbich;
        rl_quit = -1;
    }
    else
        _ss_opt(0, &old);
}

void
rl_add_slash(path, p)
    char	*path;
    char	*p;
{
    (void)strcat(p, access(path, S_IREAD | S_IFDIR) ? " " : "/");
}
