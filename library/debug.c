/*
 * Management of the debugging channels
 *
 * Copyright 2000 Alexandre Julliard
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

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>

struct dll
{
    struct dll   *next;        /* linked list of dlls */
    struct dll   *prev;
    char * const *channels;    /* array of channels */
    int           nb_channels; /* number of channels in array */
};

static struct dll *first_dll;

struct debug_option
{
    struct debug_option *next;       /* next option in list */
    unsigned char        set;        /* bits to set */
    unsigned char        clear;      /* bits to clear */
    char                 name[14];   /* channel name, or empty for "all" */
};

static struct debug_option *first_option;
static struct debug_option *last_option;


static int cmp_name( const void *p1, const void *p2 )
{
    const char *name = p1;
    const char * const *chan = p2;
    return strcmp( name, *chan + 1 );
}

/* apply a debug option to the channels of a given dll */
static void apply_option( struct dll *dll, const struct debug_option *opt )
{
    if (opt->name[0])
    {
        char **dbch = bsearch( opt->name, dll->channels, dll->nb_channels,
                               sizeof(*dll->channels), cmp_name );
        if (dbch) **dbch = (**dbch & ~opt->clear) | opt->set;
    }
    else /* all */
    {
        int i;
        for (i = 0; i < dll->nb_channels; i++)
            dll->channels[i][0] = (dll->channels[i][0] & ~opt->clear) | opt->set;
    }
}

/* register a new set of channels for a dll */
void *__wine_dbg_register( char * const *channels, int nb )
{
    struct debug_option *opt = first_option;
    struct dll *dll = malloc( sizeof(*dll) );
    if (dll)
    {
        dll->channels = channels;
        dll->nb_channels = nb;
        dll->prev = NULL;
        if ((dll->next = first_dll)) dll->next->prev = dll;
        first_dll = dll;

        /* apply existing options to this dll */
        while (opt)
        {
            apply_option( dll, opt );
            opt = opt->next;
        }
    }
    return dll;
}


/* unregister a set of channels; must pass the pointer obtained from wine_dbg_register */
void __wine_dbg_unregister( void *channel )
{
    struct dll *dll = channel;
    if (dll)
    {
        if (dll->next) dll->next->prev = dll->prev;
        if (dll->prev) dll->prev->next = dll->next;
        else first_dll = dll->next;
        free( dll );
    }
}


/* add a new debug option at the end of the option list */
void wine_dbg_add_option( const char *name, unsigned char set, unsigned char clear )
{
    struct dll *dll = first_dll;
    struct debug_option *opt;

    if (!(opt = malloc( sizeof(*opt) ))) return;
    opt->next  = NULL;
    opt->set   = set;
    opt->clear = clear;
    strncpy( opt->name, name, sizeof(opt->name) );
    opt->name[sizeof(opt->name)-1] = 0;
    if (last_option) last_option->next = opt;
    else first_option = opt;
    last_option = opt;

    /* apply option to all existing dlls */
    while (dll)
    {
        apply_option( dll, opt );
        dll = dll->next;
    }
}
