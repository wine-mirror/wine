#include <sys/types.h>
#include <sys/time.h>

#ifndef HAVE_USLEEP

unsigned int usleep (unsigned int useconds)
{
    struct timeval delay;

    delay.tv_sec = 0;
    delay.tv_usec = useconds;

    select( 0, 0, 0, 0, &delay );
    return 0;
}

#endif /* HAVE_USLEEP */



