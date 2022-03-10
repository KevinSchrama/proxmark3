#include "quickcmd.h"

#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>

#ifdef _WIN32
#else
#include <err.h>
#include <libevdev-1.0/libevdev/libevdev.h>
#endif

#include <errno.h>
#include <time.h>

#include "comms.h" //SendCommand
#include "util.h" //sprint_hex
#include "ui.h" //PrintAndLogEx
#include "util_posix.h" //msleep
#include "pm3_cmd.h"
#include "cmdlf.h"
#include "cmdlfem410x.h"
#include "cmdlfnoralsy.h"

// Wait time for UID in ms
#define WAIT_TIME 5000

#define COUNTOF(x)  (int) ( ( sizeof(x) / sizeof((x)[0]) ) )

UIDthread_arg_t thread_args;
static pthread_t spider_thread;
static const char *keycodes[64 * 1024] = { 0 }; // hack
cardtypes_s cardtypes_t;

static void setupCardTypes(void);
static void setupKeyCodes(void);
void ulltohexstring(char *Des, unsigned long long int Src);

void stopSim(void);
void checkUID(int index);

char getDevice(void);

void Sim14A(uint8_t i);
void SimiClass(void);
void SimHID(void);
void SimEM410x(void);
void SimParadox(void);
void SimNoralsy(void);

void* spiderThread(void* p);

time_t time_begin;
time_t time_end;

void* spiderThread(void* p){
    UIDthread_arg_t *args = (UIDthread_arg_t *)p;

    setupKeyCodes();
    setupCardTypes();

    char *buf = malloc(40);
    
    unsigned long long int bufint = 0;

    char *eventDevice = malloc(25);
    sprintf(eventDevice, "/dev/input/event%c", getDevice());

    const int fd = open(eventDevice, O_RDONLY | O_NONBLOCK);
# if defined(_WIN32)
    if (fd < 0){
        printf("ERROR: cannot open device %s [%s]", eventDevice, strerror(errno));
        exit(1);
    }
# else
    if (fd < 0) errx(EXIT_FAILURE, "ERROR: cannot open device %s [%s]", eventDevice, strerror(errno));
# endif

    struct libevdev *dev;
    struct input_event ev;

    int err = libevdev_new_from_fd(fd, &dev);
# if defined(_WIN32)
    if (err < 0){
        printf("ERROR: cannot associate event device [%s]", strerror(-err));
        exit(1);
    }
# else
    if (err < 0) errx(EXIT_FAILURE, "ERROR: cannot associate event device [%s]", strerror(-err));
# endif
    

    printf("Device %s is open and associated w/ libevent\n", eventDevice);
    do {
        

        err = libevdev_next_event(dev, LIBEVDEV_READ_FLAG_NORMAL, &ev);

        if (err == 0 && ev.type == EV_KEY && ev.value == EV_KEY)
        {
            strcat(buf, keycodes[ev.code]);
            if(ev.code == KEY_ENTER){
                bufint = strtoull(buf,NULL,10);
                ulltohexstring(args->UID, bufint);
                memset(buf, 0, 40);
                //printf("UID: %s\n", args->UID);
                args->UID_available = true;
            }
        }
    } while ((err == 1 || err == 0 || err == -EAGAIN) && !args->stopThread);

    close(fd);
    pthread_exit(NULL);
    return NULL;
}

void initSpidercomms(void){
    thread_args.UID_available = false;
    thread_args.stopThread = false;
    thread_args.UID = malloc(40);
    memset(thread_args.UID, 0, 40);
    pthread_create(&spider_thread, NULL, spiderThread, &thread_args);
    msleep(100);
}

void stopSpidercomms(void){
    thread_args.stopThread = true;
    pthread_join(spider_thread, NULL);
    memset(&spider_thread, 0, sizeof(pthread_t));
}

void checkUID(int index){
    int i = 0;
    while(!thread_args.UID_available && i < WAIT_TIME){
        msleep(1);
        i++;
    }
    if(thread_args.UID_available){
        if(strcmp(cardtypes_t.cardUID[index], thread_args.UID) == 0){
            cardtypes_t.detected[index] = 1;
            PrintAndLogEx(SUCCESS, "UID detected: %s", thread_args.UID);
        }else{
            PrintAndLogEx(ERR, "UID not detected: %s", thread_args.UID);
        }
        thread_args.UID_available = false;
    }
    cardtypes_t.num_tries[index]++;
}

void stopSim(void){
    clearCommandBuffer();
    SendCommandNG(CMD_BREAK_LOOP, NULL, 0);
    msleep(500);
}

void Sim14A(uint8_t i){
    int uid_len = 7;
    uint8_t uid[10] = {0x4B, 0x65, 0x76, 0x69, 0x6E, 0x00, i%10};
    if(i != 0)
        PrintAndLogEx(INFO, "TESTING ISO 14443A sim with UID %s", sprint_hex(uid, uid_len));

    struct {
        uint8_t tagtype;
        uint16_t flags;
        uint8_t uid[10];
        uint8_t exitAfter;
    } PACKED payload;

    if(i != 0){
        payload.tagtype = i%10;
    }else{
        payload.tagtype = 1;
    }
    payload.flags = 4;
    payload.exitAfter = 0;
    memcpy(payload.uid, uid, uid_len);

    clearCommandBuffer();
    SendCommandNG(CMD_HF_ISO14443A_SIMULATE, (uint8_t *)&payload, sizeof(payload));
}

void SimiClass(void){
    uint8_t csn[8] = {0x4B, 0x65, 0x76, 0x69, 0x6E, 0xB9, 0x33, 0x14};
    PrintAndLogEx(INFO, "TESTING iClass sim with UID %s", sprint_hex(csn, 8));
    clearCommandBuffer();
    SendCommandMIX(CMD_HF_ICLASS_SIMULATE, 0, 0, 1, csn, 8); //Simulate iClass
}

void SimHID(void){
    lf_hidsim_t payload;
    payload.hi2 = 0;
    payload.hi = 0x30;
    payload.lo = 0x06ec0c86;
    payload.longFMT = (0x30 > 0xFFF);
    PrintAndLogEx(INFO, "TESTING HID sim with UID 10 06 EC 0C 86");

    clearCommandBuffer();
    SendCommandNG(CMD_LF_HID_SIMULATE, (uint8_t *)&payload,  sizeof(payload));
}

void SimEM410x(void){
    int clock = 64;
    int uid_len = 5;
    int gap = 20;
    uint8_t uid[] = {0x0F, 0x03, 0x68, 0x56, 0x8B};

    PrintAndLogEx(INFO, "TESTING EM410x sim with UID %s", sprint_hex(uid, uid_len));

    em410x_construct_emul_graph(uid, clock, gap);
    CmdLFSim("");
}

void SimParadox(void){
    uint8_t raw[] = {0x0f, 0x55, 0x55, 0x56, 0x95, 0x59, 0x6a, 0x6a, 0x99, 0x99, 0xa5, 0x9a};

    uint8_t bs[sizeof(raw) * 8];
    bytes_to_bytebits(raw, sizeof(raw), bs); 

    uint8_t clk = 50, high = 10, low = 8;

    lf_fsksim_t *payload = calloc(1, sizeof(lf_fsksim_t) + sizeof(bs));
    payload->fchigh = high;
    payload->fclow =  low;
    payload->separator = 0;
    payload->clock = clk;
    memcpy(payload->data, bs, sizeof(bs));

    clearCommandBuffer();
    SendCommandNG(CMD_LF_FSK_SIMULATE, (uint8_t *)payload,  sizeof(lf_fsksim_t) + sizeof(bs));
    free(payload);
}

void SimNoralsy(void){
    uint32_t id = 1337;
    uint16_t year = 2000;

    uint8_t bs[96];
    memset(bs, 0, sizeof(bs));

    if (getnoralsyBits(id, year, bs) != PM3_SUCCESS) {
        PrintAndLogEx(ERR, "Error with tag bitstream generation.");
    }

    lf_asksim_t *payload = calloc(1, sizeof(lf_asksim_t) + sizeof(bs));
    payload->encoding = 1;
    payload->invert = 0;
    payload->separator = 1;
    payload->clock = 32;
    memcpy(payload->data, bs, sizeof(bs));

    clearCommandBuffer();
    SendCommandNG(CMD_LF_ASK_SIMULATE, (uint8_t *)payload,  sizeof(lf_asksim_t) + sizeof(bs));
    free(payload);
}

void testCycle(void){
    time_begin = time(NULL);
    for(uint8_t i = 0; i <= NUMCARDS; i++){
        Simulate(i);
    }
    for(uint8_t i = 0; i <= NUMCARDS; i++){
        if(cardtypes_t.detected[i])
            continue;
        Simulate(i);
    }
    time_end = time(NULL);
}

void Simulate(int sim){
    switch(sim){
        case 0:
            Sim14A(0); //buffer simulation
            stopSim();
            return;
            break;
        case 1:
            Sim14A(1);
            break;
        case 2:
            Sim14A(2);
            break;
        case 3:
            Sim14A(6);
            break;
        case 4:
            Sim14A(7);
            break;
        case 5:
            Sim14A(8);
            break;
        case 6:
            Sim14A(9);
            break;
        case 7:
            SimiClass();
            break;
        case 8:
            SimParadox();
            break;
        case 9:
            SimHID();
            break;
        case 10:
            SimEM410x();
            break;
        case 11:
            SimNoralsy();
            break;
        default:
            PrintAndLogEx(ERR, "Not a valid sim number!");
            return;
    }
    checkUID(sim);
    stopSim();
}

void printResults(void){
    PrintAndLogEx(INFO, "============================================");
    PrintAndLogEx(INFO, "UID's detected:");
    PrintAndLogEx(INFO, "Num ---     Card UID     --- Number of tries");
    int detect_count = 0;
    for (int i = 1; i <= NUMCARDS; i++){
        if(cardtypes_t.detected[i]){
            PrintAndLogEx(SUCCESS, "%3i --- %16s --- %i", i, cardtypes_t.cardUID[i], cardtypes_t.num_tries[i]);
            detect_count++;
        }
    }
    PrintAndLogEx(INFO, "============================================");
    PrintAndLogEx(INFO, "============================================");
    PrintAndLogEx(INFO, "Missing UID's:");
    PrintAndLogEx(INFO, "Num --- Card UID");
    int count = 0;
    for (int i = 1; i <= NUMCARDS; i++){
        if(!cardtypes_t.detected[i]){
            PrintAndLogEx(INFO, _RED_("%3i --- %16s"), i, cardtypes_t.cardUID[i]);
            count++;
        }
    }
    if(count == 0){
        PrintAndLogEx(SUCCESS, _GREEN_("Nothing missed, test succeeded!"));
    }
    PrintAndLogEx(INFO, "============================================");
    PrintAndLogEx(INFO, "Testing took %d seconds.", (time_end - time_begin));
    PrintAndLogEx(INFO, "============================================");

    FILE *ptr;
    ptr = fopen("/home/pi/proxmark3/client/testlog.txt", "a");
    if(ptr == NULL){
        PrintAndLogEx(ERR, "Couldn't open logfile");
        return;
    }
    fprintf(ptr, "%d,%d,%ld\n", detect_count, count, (time_end-time_begin));
    fclose(ptr);
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

void ulltohexstring(char *Des, unsigned long long int Src){
    int temp, i = 0;
    char hex[40] = {0};
    while(Src != 0){
        temp = Src % 16;
        if(temp < 10){
            temp = temp + 48;
        }else{
            temp = temp + 55;
        }
        hex[i++] = temp;
        Src = Src / 16;
    }
    size_t l = strlen(hex);
    Des[l] = '\0';
    for(int j = 0; j < l; j++){
        Des[j] = hex[l - 1 - j];
    }
}

static void setupCardTypes(void){
    for (int i = 0; i <= NUMCARDS; i++){
        cardtypes_t.cardUID[i] = 0;
        cardtypes_t.detected[i] = 0;
        cardtypes_t.num_tries[i] = 0;
    }
    cardtypes_t.cardUID[0] = 0;
    cardtypes_t.cardUID[1] = "4B6576696E0001";      //ISO14443A - 1
    cardtypes_t.cardUID[2] = "4B6576696E0002";      //ISO14443A - 2
    cardtypes_t.cardUID[3] = "4B6576696E0006";      //ISO14443A - 6
    cardtypes_t.cardUID[4] = "4B6576696E0007";      //ISO14443A - 7
    cardtypes_t.cardUID[5] = "4B6576696E0008";      //ISO14443A - 8
    cardtypes_t.cardUID[6] = "4B6576696E0009";      //ISO14443A - 9
    cardtypes_t.cardUID[7] = "4B6576696EB93314";    //iClass
    cardtypes_t.cardUID[8] = "218277AACB";          //paradox
    cardtypes_t.cardUID[9] = "1006EC0C86";          //hid
    cardtypes_t.cardUID[10] = "F0368568B";          //em410x
    cardtypes_t.cardUID[11] = "FEFE";               //noralsy
}

char getDevice(void){
    FILE *fp;
    int searchEvent = 0;
    fp = fopen("/proc/bus/input/devices", "r");
    if(fp == NULL)
        return '0';
    char buf[512];
    while(!feof(fp)){
        fgets(buf, 512, fp);
        if(strstr(buf, "Spider") != NULL && searchEvent == 0){
            searchEvent = 1;
        }
        if(searchEvent != 0){
            if(buf[0] == 'H'){
                for(int i = 0; i < sizeof(buf); i++){
                    if(buf[i] == 'e' && buf[i+1] == 'v' && buf[i+2] == 'e' && buf[i+3] == 'n' && buf[i+4] == 't'){
                        return buf[i+5];
                    }else if(buf[i+4] == '\n'){
                        return '0';
                    }
                }
                return '0';
            }
        }
    }
    fclose(fp);
    return '0';
}