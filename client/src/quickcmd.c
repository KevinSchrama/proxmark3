#include "quickcmd.h"

#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <err.h>
#include <errno.h>
#include <libevdev-1.0/libevdev/libevdev.h>
#include "comms.h" //SendCommand
#include "util.h" //sprint_hex
#include "ui.h" //PrintAndLogEx
#include "util_posix.h" //msleep
#include "pm3_cmd.h"

#define WAIT_TIME 0

#define COUNTOF(x)  (int) ( ( sizeof(x) / sizeof((x)[0]) ) )

UIDthread_arg_t UIDthread;
static pthread_t spider_thread;
static const char *keycodes[64 * 1024] = { 0 }; // hack
cardtypes_s cardtypes_t;

static void setupCardTypes(void);
static void setupKeyCodes(void);
void ulltohexstring(char *UID, unsigned long long int quo);

void stopSim(void);
void checkUID(int index);

void Sim14A(uint8_t i);
void SimiClass(void);
void SimHID(void);

void* spiderThread(void* p);


void* spiderThread(void* p){
    UIDthread_arg_t *args = (UIDthread_arg_t *)p;

    setupKeyCodes();
    setupCardTypes();

    char *buf = malloc(40);
    
    unsigned long long int bufint = 0;

    const char *eventDevice = "/dev/input/event0";

    const int fd = open(eventDevice, O_RDONLY | O_NONBLOCK);
    if (fd < 0) errx(EXIT_FAILURE, "ERROR: cannot open device %s [%s]", eventDevice, strerror(errno));

    struct libevdev *dev;
    struct input_event ev;

    int err = libevdev_new_from_fd(fd, &dev);
    if (err < 0) errx(EXIT_FAILURE, "ERROR: cannot associate event device [%s]", strerror(-err));

    printf("Device %s is open and associated w/ libevent\n", eventDevice);
    do {
        

        err = libevdev_next_event(dev, LIBEVDEV_READ_FLAG_NORMAL, &ev);

        if (err == 0 && ev.type == EV_KEY && ev.value == EV_KEY)
        {
            strcat(buf, keycodes[ev.code]);
            if(ev.code == KEY_ENTER){
                bufint = strtoull(buf,NULL,10);
                ulltohexstring(args->UID, bufint);
                memset(buf, '\0', 40);
                printf("UID: %s\n", args->UID);
                args->UID_available = true;
            }
        }
    } while ((err == 1 || err == 0 || err == -EAGAIN) && !args->stopThread);

    close(fd);
    pthread_exit(NULL);
    return NULL;
}

void initSpidercomms(void){
    UIDthread.UID_available = false;
    UIDthread.stopThread = false;
    memset(UIDthread.UID, 0, 40);
    pthread_create(&spider_thread, NULL, spiderThread, &UIDthread);
}

void stopSpidercomms(void){
    UIDthread.stopThread = true;
    pthread_join(spider_thread, NULL);
    memset(&spider_thread, 0, sizeof(pthread_t));
}

void checkUID(int index){
    int i = 0;
    while(!UIDthread.UID_available && i < 500){
        msleep(10);
        i++;
    }
    if(UIDthread.UID_available){
        if(strcmp(cardtypes_t.cardUID[index], UIDthread.UID) == 0){
            cardtypes_t.detected[index] = 1;
        }
    }
    cardtypes_t.num_tries[index]++;
}

void stopSim(void){
    SendCommandNG(CMD_BREAK_LOOP, NULL, 0);
    msleep(100);
}

void Sim14A(uint8_t i){
    int uid_len = 7;
    uint8_t uid[10] = {0x4B, 0x65, 0x76, 0x69, 0x6E, 0x00, i};
    PrintAndLogEx(SUCCESS, "TESTING ISO 14443A sim with UID %s", sprint_hex(uid, uid_len));

    struct {
        uint8_t tagtype;
        uint16_t flags;
        uint8_t uid[10];
        uint8_t exitAfter;
    } PACKED payload;

    payload.tagtype = i;
    payload.flags = 4;
    payload.exitAfter = 0;
    memcpy(payload.uid, uid, uid_len);

    clearCommandBuffer();
    SendCommandNG(CMD_HF_ISO14443A_SIMULATE, (uint8_t *)&payload, sizeof(payload));
    msleep(WAIT_TIME);
}

void SimiClass(void){
    uint8_t csn[8] = {0x4B, 0x65, 0x76, 0x69, 0x6E, 0xB9, 0x33, 0x14};
    PrintAndLogEx(SUCCESS, "TESTING iClass sim with UID %s", sprint_hex(csn, 8));
    clearCommandBuffer();
    SendCommandMIX(CMD_HF_ICLASS_SIMULATE, 0, 0, 1, csn, 8); //Simulate iClass
    msleep(WAIT_TIME);
}

void SimHID(void){
    lf_hidsim_t payload;
    payload.hi2 = 0;
    payload.hi = 0x30;
    payload.lo = 0x06ec0c86;
    payload.longFMT = (0x30 > 0xFFF);
    PrintAndLogEx(SUCCESS, "TESTING HID sim with UID 10 06 EC 0C 86");

    clearCommandBuffer();
    SendCommandNG(CMD_LF_HID_SIMULATE, (uint8_t *)&payload,  sizeof(payload));
    msleep(WAIT_TIME);
}

void Simulate(int sim){
    switch(sim){
        case 0:
            SimHID();
            break;
        case 1:
            Sim14A(sim);
            break;
        case 2:
            Sim14A(sim);
            break;
        case 3:
            SimiClass();
            break;
        case 4:
            break;
        case 5:
            break;
        case 6:
            Sim14A(sim);
            break;
        case 7:
            Sim14A(sim);
            break;
        case 8:
            Sim14A(sim);
            break;
        case 9:
            Sim14A(sim);
            break;
        default:
            PrintAndLogEx(ERR, "Not a valid sim number!");
            return;
    }
    checkUID(sim);
    stopSim();
}

void printResults(void){
    PrintAndLogEx(INFO, "============================================\n");
    PrintAndLogEx(INFO, "UID's detected:\n");
    PrintAndLogEx(INFO, "Num ---     Card UID     --- Number of tries\n");
    for (int i = 0; i < COUNTOF(cardtypes_t.cardUID); i++){
        if(cardtypes_t.detected[i]){
            PrintAndLogEx(SUCCESS, "%3i --- %16s --- %i\n", i, cardtypes_t.cardUID[i], cardtypes_t.num_tries[i]);
        }
    }
    PrintAndLogEx(INFO, "============================================\n");
    PrintAndLogEx(INFO, "============================================\n");
    PrintAndLogEx(INFO, "Missing UID's:\n");
    PrintAndLogEx(INFO, "Num --- Card UID\n");
    for (int i = 0; i < COUNTOF(cardtypes_t.cardUID); i++){
        if(!cardtypes_t.detected[i]){
            PrintAndLogEx(SUCCESS, "%3i --- %16s\n", i, cardtypes_t.cardUID[i]);
        }
    }
    PrintAndLogEx(INFO, "============================================\n");
}

static void setupKeyCodes(void){
    for (int i = 0; i < COUNTOF(keycodes); i++)
        keycodes[i] = 0;

    // these from /usr/include/linux/input-event-codes.h

    keycodes[KEY_1] = "1";
    keycodes[KEY_2] = "2";
    keycodes[KEY_3] = "3";
    keycodes[KEY_4] = "4";
    keycodes[KEY_5] = "5";
    keycodes[KEY_6] = "6";
    keycodes[KEY_7] = "7";
    keycodes[KEY_8] = "8";
    keycodes[KEY_9] = "9";
    keycodes[KEY_0] = "0";
    keycodes[KEY_ENTER] = "\0";
}

void ulltohexstring(char *UID, unsigned long long int quo){
    int temp, i = 0;
    char hex[40] = {0};
    while(quo != 0){
        temp = quo % 16;
        if(temp < 10){
            temp = temp + 48;
        }else{
            temp = temp + 55;
        }
        hex[i++] = temp;
        quo = quo / 16;
    }
    size_t l = strlen(hex);
    UID[l] = '\0';
    for(int j = 0; j < l; j++){
        UID[j] = hex[l - 1 - j];
    }
}

static void setupCardTypes(void){
    for (int i = 0; i < COUNTOF(cardtypes_t.cardUID); i++){
        cardtypes_t.cardUID[i] = 0;
        cardtypes_t.detected[i] = 0;
        cardtypes_t.num_tries[i] = 0;
    }
    cardtypes_t.cardUID[1] = "1006ec0c86";
    cardtypes_t.cardUID[2] = "f0368568b";
    cardtypes_t.cardUID[3] = "fefe";
    cardtypes_t.cardUID[4] = "218277aacb";
    cardtypes_t.cardUID[5] = "4b6576696eb93314";
    cardtypes_t.cardUID[6] = "4b6576696e0006";
    cardtypes_t.cardUID[7] = "4b6576696e0007";
    cardtypes_t.cardUID[8] = "4b6576696e0008";
    cardtypes_t.cardUID[9] = "4b6576696e0009";
    cardtypes_t.cardUID[10] = "4b6576696e000a";
    cardtypes_t.cardUID[11] = "4b6576696e000b";
}