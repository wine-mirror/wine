/*
 * X events handling functions
 * 
 * Copyright 1993 Alexandre Julliard
 * 
 */

#include <unistd.h>
#include "message.h"
#include "winsock.h"
#include "debugtools.h"

DECLARE_DEBUG_CHANNEL(event)

/**********************************************************************/

EVENT_DRIVER *EVENT_Driver = NULL;

/* EVENT_WaitNetEvent() master fd sets */
static fd_set __event_io_set[3];
static int    __event_max_fd = 0;
static int    __wakeup_pipe[2];

/***********************************************************************
 *		EVENT_Init
 *
 * Initialize network IO.
 */
BOOL EVENT_Init(void)
{
    int i;
    for( i = 0; i < 3; i++ )
        FD_ZERO( __event_io_set + i );

    /* this pipe is used to be able to wake-up the scheduler(WaitNetEvent) by
     a 32 bit thread, this will become obsolete when the input thread will be
     implemented */
    pipe(__wakeup_pipe);

    /* make the pipe non-blocking */
    fcntl(__wakeup_pipe[0], F_SETFL, O_NONBLOCK);
    fcntl(__wakeup_pipe[1], F_SETFL, O_NONBLOCK);

    FD_SET( __wakeup_pipe[0], &__event_io_set[EVENT_IO_READ] );
    __event_max_fd = __wakeup_pipe[0];
    __event_max_fd++;

    return EVENT_Driver->pInit();
}

/***********************************************************************
 *		EVENT_AddIO 
 */
void EVENT_AddIO(int fd, unsigned io_type)
{
    FD_SET( fd, &__event_io_set[io_type] );
    if( __event_max_fd <= fd ) __event_max_fd = fd + 1;
}

/***********************************************************************
 *		EVENT_DeleteIO 
 */
void EVENT_DeleteIO(int fd, unsigned io_type)
{
    FD_CLR( fd, &__event_io_set[io_type] );
}

/***********************************************************************
 *		EVENT_WakeUp
 *
 * Wake up the scheduler (EVENT_WaitNetEvent). Use by 32 bit thread
 * when thew want signaled an event to a 16 bit task. This function
 * will become obsolete when an Asynchronous thread will be implemented
 */
void EVENT_WakeUp(void)
{
    if (write (__wakeup_pipe[1], "A", 1) != 1)
        ERR_(event)("unable to write in wakeup_pipe\n");
}

/***********************************************************************
 *           EVENT_ReadWakeUpPipe
 *
 * Empty the wake up pipe
 */
void EVENT_ReadWakeUpPipe(void)
{
    char tmpBuf[10];
    ssize_t ret;

    /* Flush the wake-up pipe, it's just dummy data for waking-up this
     thread. This will be obsolete when the input thread will be done */
    while ( (ret = read(__wakeup_pipe[0], &tmpBuf, 10)) == 10 );
}

/***********************************************************************
 * 		EVENT_WaitNetEvent
 *
 * Sleep until a network event arrives, or until woken.
 */
void EVENT_WaitNetEvent( void )
{
    int num_pending;
    fd_set io_set[3];

    memcpy( io_set, __event_io_set, sizeof(io_set) );

    num_pending = select( __event_max_fd, &io_set[EVENT_IO_READ],
                          &io_set[EVENT_IO_WRITE],
                          &io_set[EVENT_IO_EXCEPT], NULL );

    if ( num_pending == -1 )
    {
        /* Error - signal, invalid arguments, out of memory */
        return;
    }

    /* Flush the wake-up pipe, it's just dummy data for waking-up this
     thread. This will be obsolete when the input thread will be done */
    if ( FD_ISSET( __wakeup_pipe[0], &io_set[EVENT_IO_READ] ) )
    {
        num_pending--;
        EVENT_ReadWakeUpPipe();
    }

    /*  Winsock asynchronous services */
    WINSOCK_HandleIO( &__event_max_fd, num_pending, io_set, __event_io_set );
}


/***********************************************************************
 *		EVENT_Synchronize
 *
 * Synchronize with the X server. Should not be used too often.
 */
void EVENT_Synchronize( BOOL bProcessEvents )
{
    EVENT_Driver->pSynchronize( bProcessEvents );
}

/**********************************************************************
 *		EVENT_CheckFocus
 */
BOOL EVENT_CheckFocus(void)
{
  return EVENT_Driver->pCheckFocus();
}

/***********************************************************************
 *		EVENT_QueryPointer
 */
BOOL EVENT_QueryPointer(DWORD *posX, DWORD *posY, DWORD *state)
{
  return EVENT_Driver->pQueryPointer(posX, posY, state);
}

/***********************************************************************
 *		EVENT_DummyMotionNotify
 *
 * Generate a dummy MotionNotify event. Used to force a WM_SETCURSOR message.
 */
void EVENT_DummyMotionNotify(void)
{
  EVENT_Driver->pDummyMotionNotify();
}

