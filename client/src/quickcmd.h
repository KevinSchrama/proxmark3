

#ifndef __QUICKCMD_H
#define __QUICKCMD_H

#include <stdint.h>

#define NUMCARDS 11

typedef struct {
    bool UID_available;
    bool stopThread;
    char UID[40];
} UIDthread_arg_t;

struct cardtypes_s {
    const char *cardUID[(NUMCARDS+1)] = {0};
    int num_tries[(NUMCARDS+1)] = {0};
    bool detected[(NUMCARDS+1)] = {0};
} cardtypes_t;

extern UIDthread_arg_t UIDthread;

void Simulate(int sim);
void initSpidercomms(void);
void stopSpidercomms(void);
void printResults(void);






#endif