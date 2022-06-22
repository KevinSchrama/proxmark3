

#ifndef __QUICKCMD_H
#define __QUICKCMD_H

#include <stdint.h>
#include <stdio.h>
#include "util.h"

//#define NUMCARDS 11
#define THREADPRINT     1
#define GUIPRINT        0

/****************************************************************************/
/*!
    @brief Data structure for the data arguments of threads and the control of threads.
    @param available boolean to indicate available UID from Spider.
    @param stopThread boolean to indicate if threads need to stop.
    @param endurance_test_size integer to indicate number of cards for endurance test.
    @param testtype integer to indicate the type of test: 1 card type test, 2 endurance test, 3 configuration specific test, 4 change config.
    @param required_config integer to the required config, related to search table of the config structure.
 */
/****************************************************************************/
typedef struct {
    bool available;
    bool stopThread;
    int endurance_test_size;
    int testtype;
    int required_config;
} UIDthread_arg_t;

/****************************************************************************/
/*!
    @brief Data structure for the Spider availabilty check thread.
    @param available bool to indicate if a usable Spider is connected for use.
    @param stopThread bool to indicate to stop the availability thread.
    @param quitProgram bool to indicate to quit the program for the availability thread to stop.
    @param spiderInputReady bool to indicate if the spiderThread is ready to receive input from Spider.
 */
/****************************************************************************/
typedef struct {
    bool available;
    bool stopThread;
    bool quitProgram;
    bool spiderInputReady;
} availability_arg_t;

/* Cards struct */
/****************************************************************************/
/*!
    @brief Data structure for the cards.
    @param UID Character array of the UID of the card.
    @param name Character array of the name of the card.
    @param simFunction Function to call for simulating the card.
    @param num_tries Integer to keep count of number of times card was tested during test.
    @param simulate bool to indicate if cards needs to be simulated during test.
    @param detected bool to indicate if card is read during test.
 */
/****************************************************************************/
typedef struct {
    const char *UID;
    const char *name;
    void (*simFunction)(void);
    int num_tries;
    bool simulate;
    bool detected;
} card_t;

/* Config struct */
/****************************************************************************/
/*!
    @brief Data structure for the cards.
    @param name Character array of the name of the config.
    @param testtype type of test the specific config can be used for.
 */
/****************************************************************************/
typedef struct {
    const char *name;
    unsigned char testtype; // nog geen toepassing voor geïmplementeerd
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
int Simulate(int sim);
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