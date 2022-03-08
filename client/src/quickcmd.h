

#ifndef __QUICKCMD_H
#define __QUICKCMD_H

#include <stdint.h>
#include <stdio.h>
#include "util.h"

#define NUMCARDS 11

typedef struct {
    bool UID_available;
    bool stopThread;
    char *UID;
} UIDthread_arg_t;

typedef struct {
    const char *cardUID[(NUMCARDS+1)];
    int num_tries[(NUMCARDS+1)];
    bool detected[(NUMCARDS+1)];
} cardtypes_s;

extern UIDthread_arg_t UIDthread;

void Simulate(int sim);
void testCycle(void);
void initSpidercomms(void);
void stopSpidercomms(void);
void printResults(void);






#endif