#include "quickcmd.h"

#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include <unistd.h>
#include "comms.h" //SendCommand
#include "util.h" //sprint_hex
#include "ui.h" //PrintAndLogEx
#include "util_posix.h" //msleep
#include "pm3_cmd.h"

#define WAIT_TIME 1000

static pthread_t spider_thread;

void* spiderThread(void* p){
    
}

int initSpider(){
    pthread_create(&spider_thread, NULL, spiderThread, &)
}

void StopSim(void){
    SendCommandNG(CMD_BREAK_LOOP, NULL, 0);
    msleep(500);
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

void chooseSim(int sim){
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
            break;
    }
}