/***************************************************************************
 * Copyright 1995, Technion, Israel Institute of Technology
 * Electrical Eng, Software Lab.
 * Author:    Michael Veksler.
 ***************************************************************************
 * File:      shm_semaph.h
 * Purpose:   Handle semaphores for shared memory operations.
 ***************************************************************************
 */

#ifndef __WINE_SHM_SEMAPH_H
#define __WINE_SHM_SEMAPH_H
/* IMPORTANT: If possible, restrict usage of these functions. */

#ifdef CONFIG_IPC

typedef int shm_sem;

void shm_read_wait(shm_sem semid);
void shm_write_wait(shm_sem semid);
void shm_write_signal(shm_sem semid);
void shm_read_signal(shm_sem semid);
void shm_sem_init(shm_sem *semptr);
void shm_sem_done(shm_sem *semptr);

#endif  /* CONFIG_IPC */

#endif /* __WINE_SHM_SEMAPH_H */
