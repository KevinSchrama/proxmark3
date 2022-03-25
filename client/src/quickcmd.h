

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
    char *CardNum;
} UIDthread_arg_t;

typedef struct {
    const char *cardUID[(NUMCARDS+1)];
    const char *cardType[(NUMCARDS+1)];
    char *CardNum[(NUMCARDS+1)];
    int num_tries[(NUMCARDS+1)];
    bool detected[(NUMCARDS+1)];
} cardtypes_s;

extern UIDthread_arg_t thread_args;

void startgui(void);
void Simulate(int sim);
void testCycle(void);
void initSpidercomms(void);
void stopSpidercomms(void);
void printResults(void);
char *FindProxmark(void);

#define MF_CLASSIC_1K       0x0001
#define MF_ULTRALIGHT       0x0002
#define MF_MINI             0x0004
#define NTAG                0x0008
#define MF_CLASSIC_4K       0x0010
#define FM11RF005SH         0x0020
#define ICLASS              0x0040
#define EM410X              0x0080
#define AWID                0x0100
#define PARADOX             0x0200
#define HID                 0x0400



#endif