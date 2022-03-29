#include "testgui.h"

#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <err.h>
#include <libevdev-1.0/libevdev/libevdev.h>
#include <gtk/gtk.h>
#include <gtk/gtkx.h>

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

#define UID_LENGTH 50

#define COUNTOF(x)  (int) ( ( sizeof(x) / sizeof((x)[0]) ) )

UIDthread_arg_t thread_args;
static pthread_t spider_thread;
static pthread_t cardtype_test_thread;
pthread_mutex_t thread_mutex;
pthread_mutex_t gtk_mutex;

static const char *keycodes[64 * 1024] = { 0 };
static const char *shiftkeycodes[64 * 1024] = { 0 };

static void setupKeyCodes(void);
void ulltohexstring(char *Des, unsigned long long int Src);

void stopSim(void);
void checkUID(int index);

char getDevice(void);

void SimBUF(void);
void SimMfClas1k(void);
void SimMfUltra(void);
void SimMfMini(void);
void SimNTAG(void);
void SimMfClas4k(void);
void SimFM11RF005SH(void);
void SimiClass(void);
void SimHID(void);
void SimEM410x(void);
void SimParadox(void);
void SimNoralsy(void);
void SimAwid(void);

void* spiderThread(void* p);
void* cardtypeTestThread(void *p);

void on_radio1_toggled (GtkWidget *radio);
void on_radio2_toggled (GtkWidget *radio);
void on_radio3_toggled (GtkWidget *radio);
void on_test1_HFcards_toggled (GtkWidget *check);
void on_test1_LFcards_toggled (GtkWidget *check);
void on_startbutton1_clicked (GtkWidget *startbutton);
void destroy (GtkWidget *window);

time_t time_begin;
time_t time_end;

int testType = 1;
int numcards = 0;
int cardcount = 0;

GtkWidget *window1;
    GtkWidget *grid1;
        GtkWidget *fixed1;
            GtkWidget *radio1;
            GtkWidget *fixedoptions1;
                GtkWidget *test1_HFcards;
                GtkWidget *test1_card1;
                GtkWidget *test1_card2;
                GtkWidget *test1_card3;
                GtkWidget *test1_card4;
                GtkWidget *test1_card5;
                GtkWidget *test1_card6;
                GtkWidget *test1_card7;
                GtkWidget *test1_LFcards;
                GtkWidget *test1_card8;
                GtkWidget *test1_card9;
                GtkWidget *test1_card10;
                GtkWidget *test1_card11;
        GtkWidget *fixed2;
            GtkWidget *radio2;
            GtkWidget *fixedoptions2;
                GtkWidget *check1;
                GtkWidget *check2;
                GtkWidget *check3;
                GtkWidget *check4;
                GtkWidget *check5;
                GtkWidget *check6;
                GtkWidget *check7;
                GtkWidget *check8;
                GtkWidget *check9;
                GtkWidget *check10;
        GtkWidget *fixed3;
            GtkWidget *radio3;
            GtkWidget *fixedoptions3;
                GtkWidget *check11;
                GtkWidget *check12;
                GtkWidget *check13;
                GtkWidget *check14;
                GtkWidget *check15;
                GtkWidget *check16;
                GtkWidget *check17;
                GtkWidget *check18;
                GtkWidget *check19;
                GtkWidget *check20;
        GtkWidget *startbutton1;
        GtkWidget *textview3;
        GtkWidget *progressbar1;
        GtkWidget *fill1;
        GtkWidget *fill2;
        GtkWidget *fill3;
        GtkWidget *fill4;
        GtkWidget *fill5;

GtkBuilder *builder;

GtkWidget *window2;
    GtkWidget *grid2;
        GtkWidget *label1;
        GtkWidget *label2;
        GtkWidget *textview1;
        GtkWidget *textview2;
        GtkWidget *fill6;
        GtkWidget *fill7;
        GtkWidget *fill8;

GtkTextBuffer *textviewbuf1;
GtkTextBuffer *textviewbuf2;
GtkTextBuffer *textviewbuf3;

GtkTextIter iter1;

card_t cards[] = {
    {"buffer",              "Buffer simulation",    SimBUF,         0,  false, false},
    {"4B6576696E0001",      "Mifare Classic 1k",    SimMfClas1k,    0,  false, false},
    {"4B6576696E0002",      "Mifare Ultralight",    SimMfUltra,     0,  false, false},
    {"4B6576696E0006",      "Mifrare Mini",         SimMfMini,      0,  false, false},
    {"4B6576696E0007",      "NTAG",                 SimNTAG,        0,  false, false},
    {"4B6576696E0008",      "Mifare Classic 4k",    SimMfClas4k,    0,  false, false},
    {"4B6576696E0009",      "FM11RF005SH",          SimFM11RF005SH, 0,  false, false},
    {"4B6576696EB93314",    "iClass",               SimiClass,      0,  false, false},
    {"0F0368568B",          "em410x",               SimEM410x,      0,  false, false},
    {"04F60A73",            "awid",                 SimAwid,        0,  false, false},
    {"218277AACB",          "paradox",              SimParadox,     0,  false, false},
    {"1006EC0C86",          "hid",                  SimHID,         0,  false, false},
    {NULL, NULL, NULL, 0, false, false},
    {NULL, NULL, NULL, 0, false, false},
    {NULL, NULL, NULL, 0, false, false},
    {NULL, NULL, NULL, 0, false, false},
    {NULL, NULL, NULL, 0, false, false},
    {NULL, NULL, NULL, 0, false, false},
    {NULL, NULL, NULL, 0, false, false},
    {NULL, NULL, NULL, 0, false, false}
};

void main_gui(void){
    if(pthread_mutex_init(&thread_mutex, NULL) != 0){
        printf("\n mutex init 'thread_mutex' has failed\n");
        return;
    }
    if(pthread_mutex_init(&gtk_mutex, NULL) != 0){
        printf("\n mutex init 'gtk_mutex' has failed\n");
        return;
    }

    gtk_init(NULL, NULL);

    builder = gtk_builder_new_from_file("/home/pi/proxmark3/client/src/gui2.glade");

    window1 = GTK_WIDGET(gtk_builder_get_object(builder, "window1"));
    g_signal_connect(window1, "destroy", G_CALLBACK(destroy), NULL);

    gtk_builder_connect_signals(builder, NULL);

    grid1 = GTK_WIDGET(gtk_builder_get_object(builder, "grid1"));
    fixed1 = GTK_WIDGET(gtk_builder_get_object(builder, "fixed1"));
    radio1 = GTK_WIDGET(gtk_builder_get_object(builder, "radio1"));
    fixedoptions1 = GTK_WIDGET(gtk_builder_get_object(builder, "fixedoptions1"));
    test1_HFcards = GTK_WIDGET(gtk_builder_get_object(builder, "test1_HFcards"));
    test1_card1 = GTK_WIDGET(gtk_builder_get_object(builder, "test1_card1"));
    test1_card2 = GTK_WIDGET(gtk_builder_get_object(builder, "test1_card2"));
    test1_card3 = GTK_WIDGET(gtk_builder_get_object(builder, "test1_card3"));
    test1_card4 = GTK_WIDGET(gtk_builder_get_object(builder, "test1_card4"));
    test1_card5 = GTK_WIDGET(gtk_builder_get_object(builder, "test1_card5"));
    test1_card6 = GTK_WIDGET(gtk_builder_get_object(builder, "test1_card6"));
    test1_card7 = GTK_WIDGET(gtk_builder_get_object(builder, "test1_card7"));
    test1_LFcards = GTK_WIDGET(gtk_builder_get_object(builder, "test1_LFcards"));
    test1_card8 = GTK_WIDGET(gtk_builder_get_object(builder, "test1_card8"));
    test1_card9 = GTK_WIDGET(gtk_builder_get_object(builder, "test1_card9"));
    test1_card10 = GTK_WIDGET(gtk_builder_get_object(builder, "test1_card10"));
    test1_card11 = GTK_WIDGET(gtk_builder_get_object(builder, "test1_card11"));
    fixed2 = GTK_WIDGET(gtk_builder_get_object(builder, "fixed2"));
    radio2 = GTK_WIDGET(gtk_builder_get_object(builder, "radio2"));
    fixedoptions2 = GTK_WIDGET(gtk_builder_get_object(builder, "fixedoptions2"));
    check1 = GTK_WIDGET(gtk_builder_get_object(builder, "check1"));
    check2 = GTK_WIDGET(gtk_builder_get_object(builder, "check2"));
    check3 = GTK_WIDGET(gtk_builder_get_object(builder, "check3"));
    check4 = GTK_WIDGET(gtk_builder_get_object(builder, "check4"));
    check5 = GTK_WIDGET(gtk_builder_get_object(builder, "check5"));
    check6 = GTK_WIDGET(gtk_builder_get_object(builder, "check6"));
    check7 = GTK_WIDGET(gtk_builder_get_object(builder, "check7"));
    check8 = GTK_WIDGET(gtk_builder_get_object(builder, "check8"));
    check9 = GTK_WIDGET(gtk_builder_get_object(builder, "check9"));
    check10 = GTK_WIDGET(gtk_builder_get_object(builder, "check10"));
    fixed3 = GTK_WIDGET(gtk_builder_get_object(builder, "fixed3"));
    radio3 = GTK_WIDGET(gtk_builder_get_object(builder, "radio3"));
    fixedoptions3 = GTK_WIDGET(gtk_builder_get_object(builder, "fixedoptions3"));
    check11 = GTK_WIDGET(gtk_builder_get_object(builder, "check11"));
    check12 = GTK_WIDGET(gtk_builder_get_object(builder, "check12"));
    check13 = GTK_WIDGET(gtk_builder_get_object(builder, "check13"));
    check14 = GTK_WIDGET(gtk_builder_get_object(builder, "check14"));
    check15 = GTK_WIDGET(gtk_builder_get_object(builder, "check15"));
    check16 = GTK_WIDGET(gtk_builder_get_object(builder, "check16"));
    check17 = GTK_WIDGET(gtk_builder_get_object(builder, "check17"));
    check18 = GTK_WIDGET(gtk_builder_get_object(builder, "check18"));
    check19 = GTK_WIDGET(gtk_builder_get_object(builder, "check19"));
    check20 = GTK_WIDGET(gtk_builder_get_object(builder, "check20"));
    startbutton1 = GTK_WIDGET(gtk_builder_get_object(builder, "startbutton1"));
    progressbar1 = GTK_WIDGET(gtk_builder_get_object(builder, "progressbar1"));
    textview3 = GTK_WIDGET(gtk_builder_get_object(builder, "textview3"));
    fill1 = GTK_WIDGET(gtk_builder_get_object(builder, "fill1"));
    fill2 = GTK_WIDGET(gtk_builder_get_object(builder, "fill2"));
    fill3 = GTK_WIDGET(gtk_builder_get_object(builder, "fill3"));
    fill4 = GTK_WIDGET(gtk_builder_get_object(builder, "fill4"));
    fill5 = GTK_WIDGET(gtk_builder_get_object(builder, "fill5"));

    window2 = GTK_WIDGET(gtk_builder_get_object(builder, "window2"));
    g_signal_connect(window2, "destroy", G_CALLBACK(destroy), NULL);

    grid2 = GTK_WIDGET(gtk_builder_get_object(builder, "grid2"));
    label1 = GTK_WIDGET(gtk_builder_get_object(builder, "label1"));
    label2 = GTK_WIDGET(gtk_builder_get_object(builder, "label2"));
    textview1 = GTK_WIDGET(gtk_builder_get_object(builder, "textview1"));
    textview2 = GTK_WIDGET(gtk_builder_get_object(builder, "textview2"));
    fill6 = GTK_WIDGET(gtk_builder_get_object(builder, "fill6"));
    fill7 = GTK_WIDGET(gtk_builder_get_object(builder, "fill7"));
    fill8 = GTK_WIDGET(gtk_builder_get_object(builder, "fill8"));

    textviewbuf1 = GTK_TEXT_BUFFER(gtk_builder_get_object(builder, "textviewbuf1"));
    textviewbuf2 = GTK_TEXT_BUFFER(gtk_builder_get_object(builder, "textviewbuf2"));
    textviewbuf3 = GTK_TEXT_BUFFER(gtk_builder_get_object(builder, "textviewbuf3"));

    gtk_widget_show(window1);

    gtk_main();

    stopThreads();

    g_print("Einde programma\n");
}

// GUI functions /////////////////////////////////////////////////////////////////////////////////////////////////
void destroy (GtkWidget *window){
    g_print("Destroy function\n");
    stopSim();
    gtk_main_quit();
}

void on_radio1_toggled (GtkWidget *radio){
    testType = 1;
    gtk_widget_set_sensitive(fixedoptions1, TRUE);
    gtk_widget_set_sensitive(fixedoptions2, FALSE);
    gtk_widget_set_sensitive(fixedoptions3, FALSE);
}

void on_radio2_toggled (GtkWidget *radio){
    testType = 2;
    gtk_widget_set_sensitive(fixedoptions2, TRUE);
    gtk_widget_set_sensitive(fixedoptions1, FALSE);
    gtk_widget_set_sensitive(fixedoptions3, FALSE);
}

void on_radio3_toggled (GtkWidget *radio){
    testType = 3;
    gtk_widget_set_sensitive(fixedoptions3, TRUE);
    gtk_widget_set_sensitive(fixedoptions1, FALSE);
    gtk_widget_set_sensitive(fixedoptions2, FALSE);
}

void on_test1_HFcards_toggled (GtkWidget *check){
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(test1_card1), gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(check)));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(test1_card2), gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(check)));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(test1_card3), gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(check)));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(test1_card4), gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(check)));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(test1_card5), gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(check)));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(test1_card6), gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(check)));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(test1_card7), gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(check)));
    if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(check))){
        gtk_widget_set_sensitive(test1_card1, TRUE);
        gtk_widget_set_sensitive(test1_card2, TRUE);
        gtk_widget_set_sensitive(test1_card3, TRUE);
        gtk_widget_set_sensitive(test1_card4, TRUE);
        gtk_widget_set_sensitive(test1_card5, TRUE);
        gtk_widget_set_sensitive(test1_card6, TRUE);
        gtk_widget_set_sensitive(test1_card7, TRUE);
    }else{
        gtk_widget_set_sensitive(test1_card1, FALSE);
        gtk_widget_set_sensitive(test1_card2, FALSE);
        gtk_widget_set_sensitive(test1_card3, FALSE);
        gtk_widget_set_sensitive(test1_card4, FALSE);
        gtk_widget_set_sensitive(test1_card5, FALSE);
        gtk_widget_set_sensitive(test1_card6, FALSE);
        gtk_widget_set_sensitive(test1_card7, FALSE);
    }
}

void on_test1_LFcards_toggled (GtkWidget *check){
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(test1_card8), gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(check)));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(test1_card9), gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(check)));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(test1_card10), gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(check)));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(test1_card11), gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(check)));
    if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(check))){
        gtk_widget_set_sensitive(test1_card8, TRUE);
        gtk_widget_set_sensitive(test1_card9, TRUE);
        gtk_widget_set_sensitive(test1_card10, TRUE);
        gtk_widget_set_sensitive(test1_card11, TRUE);
    }else{
        gtk_widget_set_sensitive(test1_card8, FALSE);
        gtk_widget_set_sensitive(test1_card9, FALSE);
        gtk_widget_set_sensitive(test1_card10, FALSE);
        gtk_widget_set_sensitive(test1_card11, FALSE);
    }
}

void on_startbutton1_clicked (GtkWidget *startbutton){
    gtk_widget_set_sensitive(startbutton1, FALSE);
    g_print("Test started\n");

    switch(testType){
        case 1:
            if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(test1_HFcards))){
                cards[1].simulate = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(test1_card1));   //MF Classic 1k
                cards[2].simulate = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(test1_card2));   //MF_ULTRALIGHT
                cards[3].simulate = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(test1_card3));   //MF_MINI
                cards[4].simulate = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(test1_card4));   //NTAG
                cards[5].simulate = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(test1_card5));   //MF_CLASSIC_4K
                cards[6].simulate = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(test1_card6));   //FM11RF005SH
                cards[7].simulate = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(test1_card7));   //ICLASS
            }
            if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(test1_LFcards))){
                cards[8].simulate = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(test1_card8));   //EM410X
                cards[9].simulate = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(test1_card9));   //AWID
                cards[10].simulate = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(test1_card10)); //PARADOX
                cards[11].simulate = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(test1_card11)); //HID
            }
            int i = 0;
            numcards = 0;
            while(cards[i].UID){
                if(strcmp(cards[i].UID, "buffer") != 0){
                    if(cards[i].simulate)
                        numcards++;
                }
                i++;
            }
            break;
        case 2:
            break;
        case 3:
            break;
    }

    initThreadArgs();
    initSpidercomms();
    initCardtypeTestThread();
}
// GUI functions /////////////////////////////////////////////////////////////////////////////////////////////////

// Init thread arguments ////////////////////////////////////////////////////////////////////////////////////////
void initThreadArgs(void){
    thread_args.UID_available = false;
    thread_args.stopThread = false;
    thread_args.cardUID = malloc(UID_LENGTH);
    memset(thread_args.cardUID, 0, UID_LENGTH);
}
// Init thread arguments ////////////////////////////////////////////////////////////////////////////////////////

// Spider communiction thread ////////////////////////////////////////////////////////////////////////////////////
// Init spider communication thread
void initSpidercomms(void){
    pthread_create(&spider_thread, NULL, spiderThread, &thread_args);
    msleep(100);
}

void* spiderThread(void* p){
    UIDthread_arg_t *args = (UIDthread_arg_t *)p;

    setupKeyCodes();

    char *buf = malloc(UID_LENGTH);

    int shift = 0;

    char *eventDevice = malloc(25);
    sprintf(eventDevice, "/dev/input/event%c", getDevice());

    const int fd = open(eventDevice, O_RDONLY | O_NONBLOCK);
    if (fd < 0) errx(EXIT_FAILURE, "ERROR: cannot open device %s [%s]", eventDevice, strerror(errno));

    struct libevdev *dev;
    struct input_event ev;

    int err = libevdev_new_from_fd(fd, &dev);
    if (err < 0) errx(EXIT_FAILURE, "ERROR: cannot associate event device [%s]", strerror(-err));
    
    gtk_text_buffer_get_start_iter(textviewbuf3, &iter1);

    g_print("Device %s is open and associated w/ libevent\n", eventDevice);
    do {
        

        err = libevdev_next_event(dev, LIBEVDEV_READ_FLAG_NORMAL, &ev);

        if (err == 0 && ev.type == EV_KEY && ev.value == EV_KEY)
        {
            if(ev.code == KEY_LEFTSHIFT){
                shift = 1;
            }else if(shift == 1){
                strcat(buf, shiftkeycodes[ev.code]);
                shift = 0;
            }else{
                strcat(buf, keycodes[ev.code]);
            }
            if(ev.code == KEY_ENTER || ev.code == KEY_KPENTER){
                static int num;
                static int count;
                num = 0;
                count = 0;
                for(; num < strlen(buf); num++){
                     if(buf[num] == ','){
                         count++;
                         if(count == 2){
                             break;
                         }
                     }
                }
                num++;
                pthread_mutex_lock(&thread_mutex);
                if(count > 0){
                    //memcpy(args->cardUID, &buf[num], UID_LENGTH - num);
                    strcpy(args->cardUID, &buf[num]);
                    strcat(buf, "\r\n");
                    //pthread_mutex_lock(&gtk_mutex);
                    gtk_text_buffer_insert(textviewbuf3, &iter1, (const gchar*)&buf[num], -1);
                    //pthread_mutex_unlock(&gtk_mutex);
                    memset(buf, '\0', UID_LENGTH);
                }else{
                    memcpy(args->cardUID, buf, UID_LENGTH);
                    strcat(buf, "\r\n");
                    //pthread_mutex_lock(&gtk_mutex);
                    gtk_text_buffer_insert(textviewbuf3, &iter1, (const gchar*)&buf[num], -1);
                    //pthread_mutex_unlock(&gtk_mutex);
                    memset(buf, '\0', UID_LENGTH);
                }

                args->UID_available = true;
                pthread_mutex_unlock(&thread_mutex);
            }
        }
    } while ((err == 1 || err == 0 || err == -EAGAIN) && !args->stopThread);

    free(eventDevice);

    close(fd);
    pthread_exit(NULL);
    return NULL;
}

// Spider communiction thread ////////////////////////////////////////////////////////////////////////////////////

// Test thread for card types ///////////////////////////////////////////////////////////////////////////////////////////////
void initCardtypeTestThread(void){
    pthread_create(&cardtype_test_thread, NULL, cardtypeTestThread, &thread_args);
    msleep(100);
}

void* cardtypeTestThread(void *p){
    UIDthread_arg_t *args = (UIDthread_arg_t *)p;
    time_begin = time(NULL);
    int i = 0;
    while(cards[i].UID){
        if(i == 0 || cards[i].simulate) Simulate(i);
        i++;
        if(args->stopThread) {pthread_exit(NULL); return NULL;}
    }
    i = 0;
    while(cards[i].UID){
        if(i == 0 || (cards[i].simulate && !cards[i].detected)) Simulate(i);
        i++;
        if(args->stopThread) {pthread_exit(NULL); return NULL;}
    }
    time_end = time(NULL);

    printResults();

    //////////////////////////////////////////////////////////////////////////////
    // Toevoegen switchen van scherm na simuleren
    //////////////////////////////////////////////////////////////////////////////

    pthread_exit(NULL);
    return NULL;
}
// Test thread for card types ///////////////////////////////////////////////////////////////////////////////////////////////

// Stop threads
void stopThreads(void){
    g_print("Stop threads\n");
    pthread_mutex_lock(&thread_mutex);
    thread_args.stopThread = true;
    pthread_mutex_unlock(&thread_mutex);
    g_print("Threads join\n");
    pthread_join(cardtype_test_thread, NULL);
    pthread_join(spider_thread, NULL);
    
    g_print("Mutex destroy\n");
    pthread_mutex_destroy(&thread_mutex);
    pthread_mutex_destroy(&gtk_mutex);
}

// Check the UID ////////////////////////////////////////////////////////////////////////////////////////////////
void checkUID(int index){
    int i = 0;
    while(!thread_args.UID_available && i < WAIT_TIME){
        msleep(1);
        i++;
        if(thread_args.stopThread) return;
    }
    pthread_mutex_lock(&thread_mutex);
    if(thread_args.UID_available){
        if(strcmp(cards[index].UID, thread_args.cardUID) == 0){
            cards[index].detected = 1;
            PrintAndLogEx(SUCCESS, "UID detected: %s", thread_args.cardUID);
            cardcount++;
            //pthread_mutex_lock(&gtk_mutex);
            //gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progressbar1), (gdouble)((double)cardcount/(double)numcards));
            //pthread_mutex_unlock(&gtk_mutex);
        }
        thread_args.UID_available = false;
    }else{
        PrintAndLogEx(ERR, "Expected UID (%s) not detected!", cards[index].UID);
    }
    pthread_mutex_unlock(&thread_mutex);
    cards[index].num_tries++;
}
// Check the UID ////////////////////////////////////////////////////////////////////////////////////////////////

void Simulate(int sim){
    cards[sim].simFunction();
    if(strcmp(cards[sim].UID, "buffer") != 0)
        checkUID(sim);
    stopSim();
}

// Simulation functions ////////////////////////////////////////////////////////////////////////////////////////////////////////
void stopSim(void){
    clearCommandBuffer();
    SendCommandNG(CMD_BREAK_LOOP, NULL, 0);
    msleep(500);
}

void SimBUF(void){
    int uid_len = 7;
    uint8_t uid[10] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

    struct {
        uint8_t tagtype;
        uint16_t flags;
        uint8_t uid[10];
        uint8_t exitAfter;
    } PACKED payload;

    payload.tagtype = 1;
    payload.flags = 4;
    payload.exitAfter = 0;
    memcpy(payload.uid, uid, uid_len);

    clearCommandBuffer();
    SendCommandNG(CMD_HF_ISO14443A_SIMULATE, (uint8_t *)&payload, sizeof(payload));
}

void SimMfClas1k(void){
    int uid_len = 7;
    uint8_t uid[10] = {0x4B, 0x65, 0x76, 0x69, 0x6E, 0x00, 0x01};
    PrintAndLogEx(INFO, "TESTING ISO 14443A (Mifare Classic 1k) sim with UID %s", sprint_hex(uid, uid_len));

    struct {
        uint8_t tagtype;
        uint16_t flags;
        uint8_t uid[10];
        uint8_t exitAfter;
    } PACKED payload;

    payload.tagtype = 1;
    payload.flags = 4;
    payload.exitAfter = 0;
    memcpy(payload.uid, uid, uid_len);

    clearCommandBuffer();
    SendCommandNG(CMD_HF_ISO14443A_SIMULATE, (uint8_t *)&payload, sizeof(payload));
}

void SimMfUltra(void){
    int uid_len = 7;
    uint8_t uid[10] = {0x4B, 0x65, 0x76, 0x69, 0x6E, 0x00, 0x02};
    PrintAndLogEx(INFO, "TESTING ISO 14443A (Mifare Ultralight) sim with UID %s", sprint_hex(uid, uid_len));

    struct {
        uint8_t tagtype;
        uint16_t flags;
        uint8_t uid[10];
        uint8_t exitAfter;
    } PACKED payload;

    payload.tagtype = 2;
    payload.flags = 4;
    payload.exitAfter = 0;
    memcpy(payload.uid, uid, uid_len);

    clearCommandBuffer();
    SendCommandNG(CMD_HF_ISO14443A_SIMULATE, (uint8_t *)&payload, sizeof(payload));
}

void SimMfMini(void){
    int uid_len = 7;
    uint8_t uid[10] = {0x4B, 0x65, 0x76, 0x69, 0x6E, 0x00, 0x06};
    PrintAndLogEx(INFO, "TESTING ISO 14443A (Mifare Mini) sim with UID %s", sprint_hex(uid, uid_len));

    struct {
        uint8_t tagtype;
        uint16_t flags;
        uint8_t uid[10];
        uint8_t exitAfter;
    } PACKED payload;

    payload.tagtype = 6;
    payload.flags = 4;
    payload.exitAfter = 0;
    memcpy(payload.uid, uid, uid_len);

    clearCommandBuffer();
    SendCommandNG(CMD_HF_ISO14443A_SIMULATE, (uint8_t *)&payload, sizeof(payload));
}

void SimNTAG(void){
    int uid_len = 7;
    uint8_t uid[10] = {0x4B, 0x65, 0x76, 0x69, 0x6E, 0x00, 0x07};
    PrintAndLogEx(INFO, "TESTING ISO 14443A (NTAG) sim with UID %s", sprint_hex(uid, uid_len));

    struct {
        uint8_t tagtype;
        uint16_t flags;
        uint8_t uid[10];
        uint8_t exitAfter;
    } PACKED payload;

    payload.tagtype = 7;
    payload.flags = 4;
    payload.exitAfter = 0;
    memcpy(payload.uid, uid, uid_len);

    clearCommandBuffer();
    SendCommandNG(CMD_HF_ISO14443A_SIMULATE, (uint8_t *)&payload, sizeof(payload));
}

void SimMfClas4k(void){
    int uid_len = 7;
    uint8_t uid[10] = {0x4B, 0x65, 0x76, 0x69, 0x6E, 0x00, 0x08};
    PrintAndLogEx(INFO, "TESTING ISO 14443A (Mifare Classic 4k) sim with UID %s", sprint_hex(uid, uid_len));

    struct {
        uint8_t tagtype;
        uint16_t flags;
        uint8_t uid[10];
        uint8_t exitAfter;
    } PACKED payload;

    payload.tagtype = 8;
    payload.flags = 4;
    payload.exitAfter = 0;
    memcpy(payload.uid, uid, uid_len);

    clearCommandBuffer();
    SendCommandNG(CMD_HF_ISO14443A_SIMULATE, (uint8_t *)&payload, sizeof(payload));
}

void SimFM11RF005SH(void){
    int uid_len = 7;
    uint8_t uid[10] = {0x4B, 0x65, 0x76, 0x69, 0x6E, 0x00, 0x09};
    PrintAndLogEx(INFO, "TESTING ISO 14443A (FM11RF005SH) sim with UID %s", sprint_hex(uid, uid_len));

    struct {
        uint8_t tagtype;
        uint16_t flags;
        uint8_t uid[10];
        uint8_t exitAfter;
    } PACKED payload;

    payload.tagtype = 9;
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
    uint8_t raw[] = {0x0f, 0x55, 0x55, 0x56, 0x95, 0x59, 0x6a, 0x6a, 0x99, 0x99, 0xa5, 0x9a}; //0f55555695596a6a9999a59a

    uint8_t bs[sizeof(raw) * 8];
    bytes_to_bytebits(raw, sizeof(raw), bs); 

    uint8_t clk = 50, high = 10, low = 8;

    lf_fsksim_t *payload = calloc(1, sizeof(lf_fsksim_t) + sizeof(bs));
    payload->fchigh = high;
    payload->fclow =  low;
    payload->separator = 0;
    payload->clock = clk;
    memcpy(payload->data, bs, sizeof(bs));

    PrintAndLogEx(INFO, "TESTING Paradox sim with UID 21 82 77 AA CB");

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

void SimAwid(void){
    uint8_t bs[] = {0,0,0,0,0,0,0,1,0,0,0,1,1,1,0,1,1,0,0,0,0,1,1,1,1,1,0,1,1,1,0,1,0,0,0,1,0,1,0,0,1,0,0,0,1,1,1,0,0,0,1,0,1,0,0,0,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1};
    lf_fsksim_t *payload = calloc(1, sizeof(lf_fsksim_t) + sizeof(bs));
    payload->fchigh = 10;
    payload->fclow =  8;
    payload->separator = 1;
    payload->clock = 50;
    memcpy(payload->data, bs, sizeof(bs));

    PrintAndLogEx(INFO, "TESTING Awid sim with UID 04 F6 0A 73");

    clearCommandBuffer();
    SendCommandNG(CMD_LF_FSK_SIMULATE, (uint8_t *)payload,  sizeof(lf_fsksim_t) + sizeof(bs));
    free(payload);
}
// Simulation functions /////////////////////////////////////////////////////////////////////////////////////////////////////

void printResults(void){
    PrintAndLogEx(INFO, "========================================================");
    PrintAndLogEx(INFO, "Testing took %d seconds.", (time_end - time_begin));
    PrintAndLogEx(INFO, "========================================================");
    PrintAndLogEx(INFO, "UID's detected:");
    PrintAndLogEx(INFO, "#  | Card UID         | Card Name         | Tries");
    int detect_count = 0;
    int not_detect_count = 0;
    int i = 1;
    while(cards[i].UID){
        if(cards[i].detected){
            PrintAndLogEx(SUCCESS, "%-2i | %-16s | %-17s | %i", i, cards[i].UID, cards[i].name, cards[i].num_tries);
            detect_count++;
        }else if(cards[i].simulate){
            not_detect_count++;
        }
        i++;
    }
    PrintAndLogEx(INFO, "========================================================");
    PrintAndLogEx(INFO, "========================================================");
    if(not_detect_count != 0){
        PrintAndLogEx(INFO, "Missing UID's:");
        PrintAndLogEx(INFO, "#  | Card UID         | Card Name");
        i = 1;
        while(cards[i].UID){
            if(!cards[i].detected && cards[i].simulate)
                PrintAndLogEx(INFO, _RED_("%-2i") " | " _RED_("%-16s") " | " _RED_("%s"), i, cards[i].UID, cards[i].name);
            i++;
        }
    }else{
        PrintAndLogEx(SUCCESS, _GREEN_("Nothing missed, test succeeded!"));
    }
    PrintAndLogEx(INFO, "========================================================");

    FILE *ptr;
    ptr = fopen("/home/pi/proxmark3/client/testlog.txt", "a");
    if(ptr == NULL){
        PrintAndLogEx(ERR, "Couldn't open logfile");
        return;
    }
    fprintf(ptr, "\n%d,%d,%ld", detect_count, not_detect_count, (time_end-time_begin));
    fclose(ptr);
}

static void setupKeyCodes(void){
    for (int i = 0; i < COUNTOF(keycodes); i++)
        keycodes[i] = 0;
    
    for (int i = 0; i < COUNTOF(shiftkeycodes); i++)
        shiftkeycodes[i] = 0;

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
    keycodes[KEY_A] = "a";
    keycodes[KEY_B] = "b";
    keycodes[KEY_C] = "c";
    keycodes[KEY_D] = "d";
    keycodes[KEY_E] = "e";
    keycodes[KEY_F] = "f";
    keycodes[KEY_G] = "g";
    keycodes[KEY_H] = "h";
    keycodes[KEY_I] = "i";
    keycodes[KEY_J] = "j";
    keycodes[KEY_K] = "k";
    keycodes[KEY_L] = "l";
    keycodes[KEY_M] = "m";
    keycodes[KEY_N] = "n";
    keycodes[KEY_O] = "o";
    keycodes[KEY_P] = "p";
    keycodes[KEY_Q] = "q";
    keycodes[KEY_R] = "r";
    keycodes[KEY_S] = "s";
    keycodes[KEY_T] = "t";
    keycodes[KEY_U] = "u";
    keycodes[KEY_V] = "v";
    keycodes[KEY_W] = "w";
    keycodes[KEY_X] = "x";
    keycodes[KEY_Y] = "y";
    keycodes[KEY_Z] = "z";
    keycodes[KEY_ENTER] = "\0";
    keycodes[KEY_KPENTER] = "\0";
    keycodes[KEY_COMMA] = ",";

    shiftkeycodes[KEY_A] = "A";
    shiftkeycodes[KEY_B] = "B";
    shiftkeycodes[KEY_C] = "C";
    shiftkeycodes[KEY_D] = "D";
    shiftkeycodes[KEY_E] = "E";
    shiftkeycodes[KEY_F] = "F";
    shiftkeycodes[KEY_G] = "G";
    shiftkeycodes[KEY_H] = "H";
    shiftkeycodes[KEY_I] = "I";
    shiftkeycodes[KEY_J] = "J";
    shiftkeycodes[KEY_K] = "K";
    shiftkeycodes[KEY_L] = "L";
    shiftkeycodes[KEY_M] = "M";
    shiftkeycodes[KEY_N] = "N";
    shiftkeycodes[KEY_O] = "O";
    shiftkeycodes[KEY_P] = "P";
    shiftkeycodes[KEY_Q] = "Q";
    shiftkeycodes[KEY_R] = "R";
    shiftkeycodes[KEY_S] = "S";
    shiftkeycodes[KEY_T] = "T";
    shiftkeycodes[KEY_U] = "U";
    shiftkeycodes[KEY_V] = "V";
    shiftkeycodes[KEY_W] = "W";
    shiftkeycodes[KEY_X] = "X";
    shiftkeycodes[KEY_Y] = "Y";
    shiftkeycodes[KEY_Z] = "Z";
    shiftkeycodes[KEY_COMMA] = "<";
    shiftkeycodes[KEY_DOT] = ">";
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

char *FindProxmark(void){
    FILE *fp;
    char *buf = malloc(20);
    char *pwd = malloc(50);
    for(int i = 0; i <= 9 ; i++){
        sprintf(pwd, "/sys/class/tty/ttyACM%d/../../../manufacturer", i);
        fp = fopen(pwd, "r");
        if(fp == NULL){
            continue;
        }
        fgets(buf, 20, fp);
        if(strcmp(buf, "proxmark.org\n") == 0){
            fclose(fp);
            sprintf(buf, "/dev/ttyACM%d", i);
            return buf;
        }
        printf("Correct file not found\n");
        fclose(fp);
    }
    printf("ERROR: No compatible device found\n");
    if(fp != NULL)
        fclose(fp);
    return NULL;
}