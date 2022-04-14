

#ifndef __QUICKCMD_H
#define __QUICKCMD_H

#include <stdint.h>
#include <stdio.h>
#include "util.h"

//#define NUMCARDS 11
#define THREADPRINT     1
#define GUIPRINT        0

typedef struct {
    bool available;
    bool stopThread;
    int endurance_test_size;
    int testtype;
    int required_config;
} UIDthread_arg_t;

typedef struct {
    bool available;
    bool stopThread;
    bool quitProgram;
    bool spiderInputReady;
} availability_arg_t;

/* Cards struct */
typedef struct {
    const char *UID;
    const char *name;
    void (*simFunction)(void);
    int num_tries;
    bool simulate;
    bool detected;
} card_t;

/* Config struct */
typedef struct {
    const char *name;
    unsigned char testtype; // nog geen toepassing voor gevonden
} config_t;

// testtype:
// | 8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
// config is available with the following tests:
// bit 0: cardtype test
// bit 1: endurance test
// bit 2: if config is mifare only
// bit 3: if config is mifare ultralight only
// bit 4: if config is mifare read block from card with key
// bit 5-7: nothing

extern UIDthread_arg_t thread_args;
extern availability_arg_t availability_args;

void main_gui(void);
void Simulate(int sim);
void stopThreads(void);
void printResults(void);
char *FindProxmark(void);
void printTextviewBuffer(int thread, const char* text, ...);
void updateProgressbar(int count, int number);
void exitProgram(void);

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