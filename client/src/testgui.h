

#ifndef __QUICKCMD_H
#define __QUICKCMD_H

#include <stdint.h>
#include <stdio.h>
#include "util.h"

//#define NUMCARDS 11

typedef struct {
    bool UID_available;
    bool stopThread;
    char *cardUID;
} UIDthread_arg_t;

typedef struct {
    const char *UID;
    const char *name;
    void (*simFunction)(void);
    int num_tries;
    bool simulate;
    bool detected;
} card_t;

extern UIDthread_arg_t thread_args;

void main_gui(void);
void Simulate(int sim);
void initThreadArgs(void);
void initSpidercomms(void);
void initCardtypeTestThread(void);
void stopThreads(void);
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