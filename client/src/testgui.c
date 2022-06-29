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
#include <libusb-1.0/libusb.h>

#include <errno.h>
#include <time.h>
#include <stdarg.h>

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

#define PRINT_BUFFER_SIZE 2048
#define MAX_BUFFER_SIZE 46

#define COUNTOF(x)  (int) ( ( sizeof(x) / sizeof((x)[0]) ) )

#define NORMAL_MODE 0
#define BOOTLOADER_MODE 1

#define DATA_SIZE 58
#define SETUP_SIZE 6

/* Thread variables and arguments */

UIDthread_arg_t thread_args;
availability_arg_t availability_args;
static pthread_t spider_thread;
pthread_t cardtype_test_thread;
static pthread_t endurance_test_thread;
static pthread_t config_program_thread;
static pthread_t availability_thread;
static pthread_t specific_test_thread;
pthread_mutex_t thread_mutex;
pthread_mutex_t gtk_mutex;

/* Spider input variables and functions */

static const char *keycodes[64 * 1024] = { 0 };
static const char *shiftkeycodes[64 * 1024] = { 0 };

static void setupKeyCodes(void);
void ulltohexstring(char *Des, unsigned long long int Src);

/* Simulation */

void stopSim(void);
int checkUID(char *check_uid);

/* Spider functions */

char getDevice(void);
char* getConfig(void);

/* Change configuration of Spider */

int sendConfig(int config_num);
int switchMode(int desired_mode);
int uploadConfig(int config_num);

/* Initiate thread functions */

void initThreadArgs(void);
void initSpidercomms(void);
void initCardtypeTestThread(void);
void initEnduranceTestThread(void);
void initConfigProgramThread(void);
void initAvailabilityThread(void);
void initSpecificTestThread(void);

/* Simulation functions */

void SimBUF(void);
void SimMfClas1k(void);
void SimMfUltra(void);
void SimMfMini(void);
void SimNTAG215(void);
void SimMfClas4k(void);
void SimFM11RF005SH(void);
void SimiClass(void);
void SimHID(void);
void SimEM410x(void);
void SimParadox(void);
void SimNoralsy(void);
void SimAwid(void);
void SimMifare1K(void);

/* Threads */

void* spiderThread(void* p);
void* cardtypeTestThread(void* p);
void* enduranceTestThead(void* p);
void* configProgramThread(void* p);
void* availabilityThread(void* p);
void* specificTestThread(void* p);

/* GUI functions */

void destroy(GtkWidget *window);
void on_radio1_toggled(GtkWidget *radio);
void on_radio2_toggled(GtkWidget *radio);
void on_radio3_toggled(GtkWidget *radio);
void on_radio4_toggled(GtkWidget *radio);
void on_test1_HFcards_toggled(GtkWidget *check);
void on_test1_LFcards_toggled(GtkWidget *check);
void on_startbutton1_clicked(GtkWidget *startbutton);
void on_resetbutton1_clicked(GtkWidget *resetbutton);
void on_clearbutton1_clicked(GtkWidget *clearbutton);
void on_closebutton1_clicked(GtkWidget *closebutton);
void on_closebutton2_clicked(GtkWidget *closebutton);
void on_scrollbutton1_clicked(GtkWidget *scrollbutton);
void on_scrolledwindow1_size_allocate(GtkWidget* scrolledwindow);
void on_endtest_entry1_focus_in_event(GtkWidget* entry);
void on_numpadbutton_clicked(GtkWidget* button);
gboolean on_window2_delete_event(GtkWidget *widget, GdkEvent *event, gpointer data);
gboolean printtologscreen(void *text);
gboolean progressbarUpdate(void *p);
gboolean resetTests(void *p);
gboolean printtoresultwindow(void *p);
gboolean showResults(void *p);
gboolean updateSpiderInfo(void *p);
gboolean callDestroy(void *p);

/* Print to result textview buffer */

void printResultTextview(GtkTextBuffer *textviewbuffer, const char *text, ...);

/* Time variables */

time_t time_begin;
time_t time_end;

/* Test variables */

int testType = 1;
int numcards = 0;

/* GTK gui widgets */

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
                GtkWidget *endtest_card1;
                GtkWidget *endtest_card2;
                GtkWidget *endtest_card3;
                GtkWidget *endtest_card4;
                GtkWidget *endtest_card5;
                GtkWidget *endtest_card6;
                GtkWidget *endtest_card7;
                GtkWidget *endtest_card8;
                GtkWidget *endtest_card9;
                GtkWidget *endtest_card10;
                GtkWidget *endtest_card11;
                GtkWidget *endtest_card12;
                GtkWidget *endtest_entry1;
        GtkWidget *fixed3;
            GtkWidget *radio3;
            GtkWidget *fixedoptions3;
                GtkWidget *testconfig_radio1;
                GtkWidget *testconfig_radio2;
                GtkWidget *testconfig_radio3;
            GtkWidget *radio4;
            GtkWidget *fixedoptions4;
                GtkWidget *config_silentmode;
                GtkWidget *config_radio1;
                GtkWidget *config_radio2;
                GtkWidget *config_radio3;
                GtkWidget *config_radio4;
                GtkWidget *config_radio5;
        GtkWidget *startbutton1;
        GtkWidget *resetbutton1;
        GtkWidget *textview3;
        GtkWidget *stack1;
            GtkWidget *scrolledwindow1;
            GtkWidget *grid3;
                GtkWidget *numpadbutton1;
                GtkWidget *numpadbutton2;
                GtkWidget *numpadbutton3;
                GtkWidget *numpadbutton4;
                GtkWidget *numpadbutton5;
                GtkWidget *numpadbutton6;
                GtkWidget *numpadbutton7;
                GtkWidget *numpadbutton8;
                GtkWidget *numpadbutton9;
                GtkWidget *numpadbutton0;
                GtkWidget *numpadbutton10;
                GtkWidget *numpadbutton11;
        GtkWidget *clearbutton1;
        GtkWidget *scrollbutton1;
        GtkWidget *closebutton1;
        GtkWidget *progressbar1;
        GtkWidget *spider_info_label;

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
        GtkWidget *closebutton2;

GtkTextBuffer *textviewbuf1;
GtkTextBuffer *textviewbuf2;
GtkTextBuffer *textviewbuf3;

GtkAdjustment *adjustment1;

GtkTextIter iter1;

GtkCssProvider *cssprovider1;

/* List of cards */
card_t cards[] = {
    {"buffer",              "Buffer simulation",            SimBUF,         0,  false, false}, // 0
    {"4B6576696E0001",      "Mifare Classic 1k",            SimMfClas1k,    0,  false, false}, // 1
    {"4B6576696E0002",      "Mifare Ultralight",            SimMfUltra,     0,  false, false}, // 2
    {"4B6576696E0006",      "Mifare Mini",                  SimMfMini,      0,  false, false}, // 3
    {"4B6576696E0007",      "NTAG 215",                     SimNTAG215,     0,  false, false}, // 4
    {"4B6576696E0008",      "Mifare Classic 4k",            SimMfClas4k,    0,  false, false}, // 5
    {"4B6576696E0009",      "FM11RF005SH",                  SimFM11RF005SH, 0,  false, false}, // 6
    {"4B6576696EB93314",    "iClass",                       SimiClass,      0,  false, false}, // 7
    {"0F0368568B",          "EM410x",                       SimEM410x,      0,  false, false}, // 8
    {"04F60A73",            "AWID",                         SimAwid,        0,  false, false}, // 9
    {"218277AACB",          "Paradox",                      SimParadox,     0,  false, false}, // 10
    {"1006EC0C86",          "HID",                          SimHID,         0,  false, false}, // 11
    {"5A65657247656865",    "Mifare Classic Block Read",    SimMifare1K,    0,  false, false}, // read block, not UID but memory read
    {NULL, NULL, NULL, 0, false, false},
    {NULL, NULL, NULL, 0, false, false},
    {NULL, NULL, NULL, 0, false, false},
    {NULL, NULL, NULL, 0, false, false},
    {NULL, NULL, NULL, 0, false, false},
    {NULL, NULL, NULL, 0, false, false},
    {NULL, NULL, NULL, 0, false, false}
};

/* List of configs */
config_t config[] = {
    {"Generic V2.0", 0x00},
    {"Generic V2.0 full-hex", 0x00},
    {"mifare-only", 0x00},
    {"mifare-ultralight-ntag-only", 0x00},
    {"mifare-read-block", 0x00},
    {"test-config", 0x00},
    {NULL, 0}
};

bool silent_mode = false;

bool scroll_lock = true;

uint8_t running_threads = 0;

#define THREAD_SPIDER           (1<<0)
#define THREAD_CARDTYPE         (1<<1)
#define THREAD_ENDURANCE        (1<<2)
#define THREAD_CONFIG           (1<<3)
#define THREAD_AVAILABILITY     (1<<4)
#define THREAD_SPECIFIC         (1<<5)

/* Config index */

#define CONFIG_GENERIC_V2_0                0
#define CONFIG_GENERIC_FULL                1
#define CONFIG_MIFARE_ONLY                 2
#define CONFIG_MIFARE_ULTRALIGHT_NTAG_ONLY 3
#define CONFIG_MIFARE_READ_BLOCK           4
#define CONFIG_OTHER_CONFIG                5

/* Progressbar arguments */
struct progressbar_args {
    int cardcount_arg;
    int numcards_args;
};

/* Result textview arguments */
struct textview_args {
    GtkTextBuffer *textviewbuffer;
    char text[PRINT_BUFFER_SIZE];
};

/****************************************************************************/
/*!
    @brief Main entry of the GUI, initiates and starts GUI.
    @param void
    @return 
 */
/****************************************************************************/
void main_gui(void){
    if(pthread_mutex_init(&thread_mutex, NULL) != 0){
        if(g_debugMode) g_print("\n mutex init 'thread_mutex' has failed\n");
        return;
    }
    if(pthread_mutex_init(&gtk_mutex, NULL) != 0){
        if(g_debugMode) g_print("\n mutex init 'gtk_mutex' has failed\n");
        return;
    }

    int err = libusb_init(NULL);
    if(err) return;

    initAvailabilityThread();

    gtk_init(NULL, NULL);

    builder = gtk_builder_new_from_file("/home/pi/proxmark3/client/src/gui.glade");

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
    endtest_card1 = GTK_WIDGET(gtk_builder_get_object(builder, "endtest_card1"));
    endtest_card2 = GTK_WIDGET(gtk_builder_get_object(builder, "endtest_card2"));
    endtest_card3 = GTK_WIDGET(gtk_builder_get_object(builder, "endtest_card3"));
    endtest_card4 = GTK_WIDGET(gtk_builder_get_object(builder, "endtest_card4"));
    endtest_card5 = GTK_WIDGET(gtk_builder_get_object(builder, "endtest_card5"));
    endtest_card6 = GTK_WIDGET(gtk_builder_get_object(builder, "endtest_card6"));
    endtest_card7 = GTK_WIDGET(gtk_builder_get_object(builder, "endtest_card7"));
    endtest_card8 = GTK_WIDGET(gtk_builder_get_object(builder, "endtest_card8"));
    endtest_card9 = GTK_WIDGET(gtk_builder_get_object(builder, "endtest_card9"));
    endtest_card10 = GTK_WIDGET(gtk_builder_get_object(builder, "endtest_card10"));
    endtest_card11 = GTK_WIDGET(gtk_builder_get_object(builder, "endtest_card11"));
    endtest_card12 = GTK_WIDGET(gtk_builder_get_object(builder, "endtest_card12"));
    endtest_entry1 = GTK_WIDGET(gtk_builder_get_object(builder, "endtest_entry1"));
    fixed3 = GTK_WIDGET(gtk_builder_get_object(builder, "fixed3"));
    radio3 = GTK_WIDGET(gtk_builder_get_object(builder, "radio3"));
    fixedoptions3 = GTK_WIDGET(gtk_builder_get_object(builder, "fixedoptions3"));
    testconfig_radio1 = GTK_WIDGET(gtk_builder_get_object(builder, "testconfig_radio1"));
    testconfig_radio2 = GTK_WIDGET(gtk_builder_get_object(builder, "testconfig_radio2"));
    testconfig_radio3 = GTK_WIDGET(gtk_builder_get_object(builder, "testconfig_radio3"));
    radio4 = GTK_WIDGET(gtk_builder_get_object(builder, "radio4"));
    fixedoptions4 = GTK_WIDGET(gtk_builder_get_object(builder, "fixedoptions4"));
    config_silentmode = GTK_WIDGET(gtk_builder_get_object(builder, "config_silentmode"));
    config_radio1 = GTK_WIDGET(gtk_builder_get_object(builder, "config_radio1"));
    config_radio2 = GTK_WIDGET(gtk_builder_get_object(builder, "config_radio2"));
    config_radio3 = GTK_WIDGET(gtk_builder_get_object(builder, "config_radio3"));
    config_radio4 = GTK_WIDGET(gtk_builder_get_object(builder, "config_radio4"));
    config_radio5 = GTK_WIDGET(gtk_builder_get_object(builder, "config_radio5"));
    startbutton1 = GTK_WIDGET(gtk_builder_get_object(builder, "startbutton1"));
    resetbutton1 = GTK_WIDGET(gtk_builder_get_object(builder, "resetbutton1"));
    progressbar1 = GTK_WIDGET(gtk_builder_get_object(builder, "progressbar1"));
    textview3 = GTK_WIDGET(gtk_builder_get_object(builder, "textview3"));
    stack1 = GTK_WIDGET(gtk_builder_get_object(builder, "stack1"));
    scrolledwindow1 = GTK_WIDGET(gtk_builder_get_object(builder, "scrolledwindow1"));
    grid3 = GTK_WIDGET(gtk_builder_get_object(builder, "grid3"));
    numpadbutton1 = GTK_WIDGET(gtk_builder_get_object(builder, "numpadbutton1"));
    numpadbutton2 = GTK_WIDGET(gtk_builder_get_object(builder, "numpadbutton2"));
    numpadbutton3 = GTK_WIDGET(gtk_builder_get_object(builder, "numpadbutton3"));
    numpadbutton4 = GTK_WIDGET(gtk_builder_get_object(builder, "numpadbutton4"));
    numpadbutton5 = GTK_WIDGET(gtk_builder_get_object(builder, "numpadbutton5"));
    numpadbutton6 = GTK_WIDGET(gtk_builder_get_object(builder, "numpadbutton6"));
    numpadbutton7 = GTK_WIDGET(gtk_builder_get_object(builder, "numpadbutton7"));
    numpadbutton8 = GTK_WIDGET(gtk_builder_get_object(builder, "numpadbutton8"));
    numpadbutton9 = GTK_WIDGET(gtk_builder_get_object(builder, "numpadbutton9"));
    numpadbutton0 = GTK_WIDGET(gtk_builder_get_object(builder, "numpadbutton0"));
    numpadbutton10 = GTK_WIDGET(gtk_builder_get_object(builder, "numpadbutton10"));
    numpadbutton11 = GTK_WIDGET(gtk_builder_get_object(builder, "numpadbutton11"));
    clearbutton1 = GTK_WIDGET(gtk_builder_get_object(builder, "clearbutton1"));
    scrollbutton1 = GTK_WIDGET(gtk_builder_get_object(builder, "scrollbutton1"));
    spider_info_label = GTK_WIDGET(gtk_builder_get_object(builder, "spider_info_label"));
    closebutton1 = GTK_WIDGET(gtk_builder_get_object(builder, "closebutton1"));

    window2 = GTK_WIDGET(gtk_builder_get_object(builder, "window2"));

    grid2 = GTK_WIDGET(gtk_builder_get_object(builder, "grid2"));
    label1 = GTK_WIDGET(gtk_builder_get_object(builder, "label1"));
    label2 = GTK_WIDGET(gtk_builder_get_object(builder, "label2"));
    textview1 = GTK_WIDGET(gtk_builder_get_object(builder, "textview1"));
    textview2 = GTK_WIDGET(gtk_builder_get_object(builder, "textview2"));
    closebutton2 = GTK_WIDGET(gtk_builder_get_object(builder, "closebutton2"));
    fill6 = GTK_WIDGET(gtk_builder_get_object(builder, "fill6"));
    fill7 = GTK_WIDGET(gtk_builder_get_object(builder, "fill7"));
    fill8 = GTK_WIDGET(gtk_builder_get_object(builder, "fill8"));

    textviewbuf1 = GTK_TEXT_BUFFER(gtk_builder_get_object(builder, "textviewbuf1"));
    textviewbuf2 = GTK_TEXT_BUFFER(gtk_builder_get_object(builder, "textviewbuf2"));
    textviewbuf3 = GTK_TEXT_BUFFER(gtk_builder_get_object(builder, "textviewbuf3"));

    adjustment1 = GTK_ADJUSTMENT(gtk_builder_get_object(builder, "adjustment1"));
    
    gtk_text_buffer_get_end_iter(textviewbuf3, &iter1);

    cssprovider1 = gtk_css_provider_new();
    gtk_css_provider_load_from_path(cssprovider1, "/home/pi/proxmark3/client/src/guistyle.css", NULL);
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(), GTK_STYLE_PROVIDER(cssprovider1), GTK_STYLE_PROVIDER_PRIORITY_USER);

    if(g_debugMode) g_print("Debug mode: %d\n", g_debugMode);

    gtk_window_fullscreen(GTK_WINDOW(window1));
    gtk_widget_show(window1);

    gtk_main();
    
    libusb_exit(NULL);

    if(g_debugMode) g_print("Einde programma\n");
}

// GUI functions /////////////////////////////////////////////////////////////////////////////////////////////////
/****************************************************************************/
/*!
    @brief Destroy window, also called when close button on window is pressed
    @param window pointer to widget that called destroy
    @return 
 */
/****************************************************************************/
void destroy (GtkWidget *window){
    if(g_debugMode) g_print("Destroy function\n");
    stopSim();
    availability_args.stopThread = true;
    if(running_threads && THREAD_AVAILABILITY) {
        pthread_join(availability_thread, NULL);
        running_threads &= (0xFF - THREAD_AVAILABILITY);
    }
    stopThreads();
    
    if(g_debugMode) g_print("Mutex destroy\n");
    pthread_mutex_destroy(&thread_mutex);
    pthread_mutex_destroy(&gtk_mutex);
    
    gtk_main_quit();
}

/****************************************************************************/
/*!
    @brief Called on radio1 radio button pressed, sets testType to toggled button and disables all other options.
    @param radio pointer to widget that called function
    @return 
 */
/****************************************************************************/
void on_radio1_toggled (GtkWidget *radio){
    testType = 1;
    gtk_widget_set_sensitive(fixedoptions1, TRUE);
    gtk_widget_set_sensitive(fixedoptions2, FALSE);
    gtk_widget_set_sensitive(fixedoptions3, FALSE);
    gtk_widget_set_sensitive(fixedoptions4, FALSE);
}

/****************************************************************************/
/*!
    @brief Called on radio2 radio button pressed, sets testType to toggled button and disables all other options.
    @param radio pointer to widget that called function
    @return 
 */
/****************************************************************************/
void on_radio2_toggled (GtkWidget *radio){
    testType = 2;
    gtk_widget_set_sensitive(fixedoptions2, TRUE);
    gtk_widget_set_sensitive(fixedoptions1, FALSE);
    gtk_widget_set_sensitive(fixedoptions3, FALSE);
    gtk_widget_set_sensitive(fixedoptions4, FALSE);
}

/****************************************************************************/
/*!
    @brief Called on radio3 radio button pressed, sets testType to toggled button and disables all other options.
    @param radio pointer to widget that called function
    @return 
 */
/****************************************************************************/
void on_radio3_toggled (GtkWidget *radio){
    testType = 3;
    gtk_widget_set_sensitive(fixedoptions3, TRUE);
    gtk_widget_set_sensitive(fixedoptions1, FALSE);
    gtk_widget_set_sensitive(fixedoptions2, FALSE);
    gtk_widget_set_sensitive(fixedoptions4, FALSE);
}

/****************************************************************************/
/*!
    @brief Called on radio4 radio button pressed, sets testType to toggled button and disables all other options.
    @param radio pointer to widget that called function
    @return 
 */
/****************************************************************************/
void on_radio4_toggled (GtkWidget *radio){
    testType = 4;
    gtk_widget_set_sensitive(fixedoptions4, TRUE);
    gtk_widget_set_sensitive(fixedoptions1, FALSE);
    gtk_widget_set_sensitive(fixedoptions2, FALSE);
    gtk_widget_set_sensitive(fixedoptions3, FALSE);
}

/****************************************************************************/
/*!
    @brief Called on test1_HFcards toggle button pressed, enables or disables all option button under HF cards.
    @param check pointer to widget that called function
    @return 
 */
/****************************************************************************/
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

/****************************************************************************/
/*!
    @brief Called on test1_LFcards toggle button pressed, enables or disables all option button under LF cards.
    @param check pointer to widget that called function
    @return 
 */
/****************************************************************************/
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

/****************************************************************************/
/*!
    @brief Called when startbutton is pressed and chooses test type and starts it.
    @param startbutton pointer to widget that called function
    @return 
 */
/****************************************************************************/
void on_startbutton1_clicked (GtkWidget *startbutton){
    gtk_widget_set_sensitive(startbutton1, FALSE);
    if(g_debugMode) g_print("Test started\n");

    pthread_mutex_lock(&gtk_mutex);
    availability_args.available = false;
    pthread_mutex_unlock(&gtk_mutex);

    char *configname = NULL;
    int i;

    gtk_text_buffer_set_text(textviewbuf1, "", -1);
    gtk_text_buffer_set_text(textviewbuf2, "", -1);

    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progressbar1), 0);

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
            i = 0;
            numcards = 0;
            while(cards[i].UID){
                if(strcmp(cards[i].UID, "buffer") != 0){
                    if(cards[i].simulate)
                        numcards++;
                }
                i++;
            }
            if(numcards == 0){
                on_resetbutton1_clicked(resetbutton1);
                printTextviewBuffer(GUIPRINT, "Can't run test without cards...");
                break;
            }
            printTextviewBuffer(GUIPRINT, "Simulating %d different cards", numcards);

            gtk_text_buffer_set_text(textviewbuf1, "", -1);
            gtk_text_buffer_set_text(textviewbuf2, "", -1);

            i = 0;
            configname = getConfig();
            while(config[i].name){
                if(strcmp(configname, config[i].name) == 0){
                    break;
                }
                i++;
            }
            if(config[i].name == NULL){
                printTextviewBuffer(GUIPRINT, "Config not recognized, programming usable config");
                while(uploadConfig(CONFIG_GENERIC_FULL) && !thread_args.stopThread);
            }
            
            initThreadArgs();
            initCardtypeTestThread();
            initSpidercomms();
            break;
        case 2:
            if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(endtest_card1))) thread_args.testtype = 1;
            if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(endtest_card2))) thread_args.testtype = 2;
            if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(endtest_card3))) thread_args.testtype = 3;
            if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(endtest_card4))) thread_args.testtype = 4;
            if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(endtest_card5))) thread_args.testtype = 5;
            if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(endtest_card6))) thread_args.testtype = 6;
            if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(endtest_card7))) thread_args.testtype = 7;
            if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(endtest_card8))) thread_args.testtype = 8;
            if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(endtest_card9))) thread_args.testtype = 9;
            if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(endtest_card10))) thread_args.testtype = 10;
            if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(endtest_card11))) thread_args.testtype = 11;
            if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(endtest_card12))) thread_args.testtype = 12;

            const char *testsize = gtk_entry_get_text(GTK_ENTRY(endtest_entry1));

            if(strlen(testsize) != 0){
                thread_args.endurance_test_size = atoi(testsize);
                if(g_debugMode) printf("%d, %d\n", thread_args.endurance_test_size, strlen(testsize));
            }else{
                thread_args.endurance_test_size = 10;
            }
            
            if(thread_args.endurance_test_size == 0){
                printTextviewBuffer(GUIPRINT, "No valid test size received, try again...");
                gtk_entry_set_text (GTK_ENTRY(endtest_entry1), "");
                on_resetbutton1_clicked(resetbutton1);
            }else{
                numcards = thread_args.endurance_test_size;
                printTextviewBuffer(GUIPRINT, "Simulating %s for %d times", cards[thread_args.testtype].name, thread_args.endurance_test_size);
            }

            i = 0;
            configname = getConfig();
            while(config[i].name){
                if(strcmp(configname, config[i].name) == 0){
                    break;
                }
                i++;
            }
            if(config[i].name == NULL){
                printTextviewBuffer(GUIPRINT, "Config not recognized, programming usable config");
                while(uploadConfig(CONFIG_GENERIC_FULL) && !thread_args.stopThread);
            }
            
            initThreadArgs();
            initSpidercomms();
            initEnduranceTestThread();
            break;
        case 3:
            if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(testconfig_radio1))) thread_args.testtype = 1;    // Mifare Classic only
            if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(testconfig_radio2))) thread_args.testtype = 2;    // Mifare Ultralight only
            if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(testconfig_radio3))) {                            // Mifare Classic block read
                thread_args.testtype = 3;
                cards[12].simulate = true;
                numcards = 1;
            }else{
                cards[1].simulate = true;
                cards[2].simulate = true;
                cards[3].simulate = true;
                cards[4].simulate = true;
                cards[5].simulate = true;
                cards[6].simulate = true;
                cards[7].simulate = true;
                cards[8].simulate = true;
                cards[9].simulate = true;
                cards[10].simulate = true;
                cards[11].simulate = true;
                numcards = 2;
            }

            initThreadArgs();
            initSpidercomms();
            initSpecificTestThread();
            break;
        case 4:
            if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(config_radio1))) thread_args.required_config = CONFIG_GENERIC_V2_0;
            if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(config_radio2))) thread_args.required_config = CONFIG_GENERIC_FULL;
            if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(config_radio3))) thread_args.required_config = CONFIG_MIFARE_ONLY;
            if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(config_radio4))) thread_args.required_config = CONFIG_MIFARE_ULTRALIGHT_NTAG_ONLY;
            if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(config_radio5))) thread_args.required_config = CONFIG_MIFARE_READ_BLOCK;

            silent_mode = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(config_silentmode));

            printTextviewBuffer(GUIPRINT, silent_mode ? "Configuring Spider with new config in silent mode" : "Configuring Spider with new config");

            initThreadArgs();
            initConfigProgramThread();
            break;

    }
    gtk_stack_set_visible_child(GTK_STACK(stack1), scrolledwindow1);
    free(configname);

}

/****************************************************************************/
/*!
    @brief Called when resetbutton is pressed and resets all threads, variables and results.
    @param resetbutton pointer to widget that called function
    @return 
 */
/****************************************************************************/
void on_resetbutton1_clicked(GtkWidget *resetbutton){
    gtk_widget_set_sensitive(resetbutton1, FALSE);
    if(g_debugMode) printf("Reset all tests\n");

    stopThreads();
    stopSim();

    int i = 0;
    while(cards[i].UID){
        cards[i].num_tries = 0;
        cards[i].detected = false;
        cards[i].simulate = false;
        i++;
    }

    thread_args.stopThread = false;
    pthread_mutex_lock(&gtk_mutex);
    availability_args.available = true;
    pthread_mutex_unlock(&gtk_mutex);

    gtk_widget_set_sensitive(startbutton1, TRUE);
    gtk_widget_set_sensitive(resetbutton1, TRUE);
}

/****************************************************************************/
/*!
    @brief Called when clearbutton is pressed and clear the log screen.
    @param clearbutton pointer to widget that called function
    @return 
 */
/****************************************************************************/
void on_clearbutton1_clicked(GtkWidget *clearbutton){
    gtk_widget_set_sensitive(clearbutton, FALSE);

    gtk_text_buffer_set_text(textviewbuf3, "", -1);
    gtk_text_buffer_get_end_iter(textviewbuf3, &iter1);
    
    gtk_widget_set_sensitive(clearbutton, TRUE);
}

/****************************************************************************/
/*!
    @brief Called when closebutton of window 1 is pressed and close screen.
    @param closebutton pointer to widget that called function
    @return 
 */
/****************************************************************************/
void on_closebutton1_clicked(GtkWidget *closebutton){
    gtk_widget_set_sensitive(closebutton, FALSE);

    destroy(window1);
    
    gtk_widget_set_sensitive(closebutton, TRUE);
}

/****************************************************************************/
/*!
    @brief Called when closebutton of window 2 is pressed and close the result screen.
    @param closebutton pointer to widget that called function
    @return 
 */
/****************************************************************************/
void on_closebutton2_clicked(GtkWidget *closebutton){
    gtk_widget_set_sensitive(closebutton, FALSE);

    gtk_widget_hide(window2);
    
    gtk_widget_set_sensitive(closebutton, TRUE);
}

/****************************************************************************/
/*!
    @brief Called when scrollbutton is pressed to lock scrolling and start auto scroll of log window.
    @param scrollbutton pointer to widget that called function
    @return 
 */
/****************************************************************************/
void on_scrollbutton1_clicked(GtkWidget *scrollbutton){
    gtk_widget_set_sensitive(scrollbutton, FALSE);

    scroll_lock = !scroll_lock;

    if(scroll_lock){
        gtk_widget_set_name(scrollbutton, "scrolllocked");
    }else{
        gtk_widget_set_name(scrollbutton, "scrollunlocked");
    }
    
    gtk_widget_set_sensitive(scrollbutton, TRUE);
}

/****************************************************************************/
/*!
    @brief Called when testresult window (window2) is closed to not destroy it but hide it.
    @param widget pointer to widget that called function
    @param event
    @param data
    @return gboolean, always TRUE.
 */
/****************************************************************************/
gboolean on_window2_delete_event(GtkWidget *widget, GdkEvent *event, gpointer data){
    gtk_widget_hide(window2);
    return TRUE;
}

/****************************************************************************/
/*!
    @brief Called when text in textview is bigger than what fits to scroll window to newest text.
    @param scrolledwindow pointer to widget that called function
    @return 
 */
/****************************************************************************/
void on_scrolledwindow1_size_allocate(GtkWidget* scrolledwindow){
    if(scroll_lock){
        gtk_adjustment_set_value(adjustment1, gtk_adjustment_get_upper(adjustment1) - gtk_adjustment_get_page_size(adjustment1));
    }
}

void on_endtest_entry1_focus_in_event(GtkWidget* entry){

    gtk_stack_set_visible_child(GTK_STACK(stack1), grid3);
}

void on_numpadbutton_clicked(GtkWidget* button){
    gtk_widget_set_sensitive(button, FALSE);
    const char *entrybuf = gtk_entry_get_text(GTK_ENTRY(endtest_entry1));
    char *buffer = calloc(1, 10);

    if(button == numpadbutton1){
        if(g_debugMode) g_print("numpadbutton1 pressed\n");
        sprintf(buffer, "%s1", entrybuf);
    }else if(button == numpadbutton2){
        if(g_debugMode) g_print("numpadbutton2 pressed\n");
        sprintf(buffer, "%s2", entrybuf);
    }else if(button == numpadbutton3){
        if(g_debugMode) g_print("numpadbutton3 pressed\n");
        sprintf(buffer, "%s3", entrybuf);
    }else if(button == numpadbutton4){
        if(g_debugMode) g_print("numpadbutton4 pressed\n");
        sprintf(buffer, "%s4", entrybuf);
    }else if(button == numpadbutton5){
        if(g_debugMode) g_print("numpadbutton5 pressed\n");
        sprintf(buffer, "%s5", entrybuf);
    }else if(button == numpadbutton6){
        if(g_debugMode) g_print("numpadbutton6 pressed\n");
        sprintf(buffer, "%s6", entrybuf);
    }else if(button == numpadbutton7){
        if(g_debugMode) g_print("numpadbutton7 pressed\n");
        sprintf(buffer, "%s7", entrybuf);
    }else if(button == numpadbutton8){
        if(g_debugMode) g_print("numpadbutton8 pressed\n");
        sprintf(buffer, "%s8", entrybuf);
    }else if(button == numpadbutton9){
        if(g_debugMode) g_print("numpadbutton9 pressed\n");
        sprintf(buffer, "%s9", entrybuf);
    }else if(button == numpadbutton0){
        if(g_debugMode) g_print("numpadbutton0 pressed\n");
        sprintf(buffer, "%s0", entrybuf);
    }else if(button == numpadbutton10){
        if(g_debugMode) g_print("numpadbutton10 pressed\n");
        sprintf(buffer, "%s", entrybuf);
        gtk_stack_set_visible_child(GTK_STACK(stack1), scrolledwindow1);
    }else if(button == numpadbutton11){
        if(g_debugMode) g_print("numpadbutton11 pressed\n");
    }

    gtk_entry_set_text(GTK_ENTRY(endtest_entry1), buffer);

    gtk_widget_set_sensitive(button, TRUE);
}
// GUI functions /////////////////////////////////////////////////////////////////////////////////////////////////

/****************************************************************************/
/*!
    @brief Initiate thread arguments
    @param void
    @return 
 */
/****************************************************************************/
void initThreadArgs(void){
    thread_args.available = false;
    thread_args.stopThread = false;
}

// Spider communiction thread ////////////////////////////////////////////////////////////////////////////////////
/****************************************************************************/
/*!
    @brief Initiate and start spider communication thread.
    @param void
    @return 
 */
/****************************************************************************/
void initSpidercomms(void){
    running_threads |= THREAD_SPIDER;
    pthread_create(&spider_thread, NULL, spiderThread, &thread_args);
}

/****************************************************************************/
/*!
    @brief Thread to receive keyboard input from Spider and check if received input is a known UID
    @param p pointer to thread arguments passed through from initiation.
    @return void*
 */
/****************************************************************************/
void* spiderThread(void* p){
    if(g_debugMode) printf("Spider communication thread started\n");
    UIDthread_arg_t *args = (UIDthread_arg_t *)p;

    setupKeyCodes();

    char *buf = calloc(1, UID_LENGTH);

    int shift = 0;

    int cardcount = 0;
    int detected_card = 0;

    unsigned long long int bufint = 0;

    char *eventDevice = malloc(25);
    sprintf(eventDevice, "/dev/input/event%c", getDevice());

    int fd = open(eventDevice, O_RDONLY | O_NONBLOCK);
    while(fd < 0) fd = open(eventDevice, O_RDONLY | O_NONBLOCK);
    //if (fd < 0) errx(EXIT_FAILURE, "ERROR: cannot open device %s [%s]", eventDevice, strerror(errno));
    availability_args.spiderInputReady = true;

    struct libevdev *dev;
    struct input_event ev;

    int err = libevdev_new_from_fd(fd, &dev);
    if (err < 0) errx(EXIT_FAILURE, "ERROR: cannot associate event device [%s]", strerror(-err));

    if(g_debugMode) printf("Device %s is open and associated w/ libevent\n", eventDevice);
    free(eventDevice);
    
    do {
        
        err = libevdev_next_event(dev, LIBEVDEV_READ_FLAG_NORMAL, &ev);

        if (err == 0 && ev.type == EV_KEY && ev.value == EV_KEY){
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
                if(buf[0] != '<'){
                    for(; num < strlen(buf); num++){
                         if(buf[num] == ','){
                             count++;
                             if(count == 2){
                                 break;
                             }
                         }
                    }
                    num++;
                    if(num >= UID_LENGTH || count == 0) {
                        num = 0;
                        bufint = strtoull(buf,NULL,10);
                        ulltohexstring(buf, bufint);
                    }
                    detected_card = checkUID(&buf[num]);
                    if(detected_card){
                        pthread_mutex_lock(&thread_mutex);
                        printTextviewBuffer(THREADPRINT, "UID detected: %s from %s", &buf[num], cards[detected_card].name);
                        cardcount++;
                        updateProgressbar(cardcount, numcards);
                        args->available = true;
                        pthread_mutex_unlock(&thread_mutex);
                    }else{
                        printTextviewBuffer(THREADPRINT, "Unknown card detected: %s", &buf[num]);
                    }
                }
                memset(buf, '\0', UID_LENGTH);
            }
        }
        
        pthread_mutex_lock(&thread_mutex);
        if(args->stopThread) {
            pthread_mutex_unlock(&thread_mutex);
            if(g_debugMode) printf("Stop spider thread\n"); 
            break;
        }
        pthread_mutex_unlock(&thread_mutex);
    } while (err == 1 || err == 0 || err == -EAGAIN);
    close(fd);
    free(buf);

    availability_args.spiderInputReady = false;

    pthread_exit(NULL);
    return NULL;
}
// Spider communiction thread ////////////////////////////////////////////////////////////////////////////////////

// Test thread for card types ///////////////////////////////////////////////////////////////////////////////////////////////
/****************************************************************************/
/*!
    @brief Initiate and start Card type test thread.
    @param void
    @return 
 */
/****************************************************************************/
void initCardtypeTestThread(void){
    running_threads |= THREAD_CARDTYPE;
    pthread_create(&cardtype_test_thread, NULL, cardtypeTestThread, &thread_args);
}

/****************************************************************************/
/*!
    @brief Thread to simulate the given cards and show the results if they were recognised by spiderThread.
    @param p pointer to thread arguments passed through from initiation.
    @return void*
 */
/****************************************************************************/
void* cardtypeTestThread(void *p){
    if(g_debugMode) printf("Cardtype test thread started\n");
    UIDthread_arg_t *args = (UIDthread_arg_t *)p;
    
    while(!availability_args.spiderInputReady){
        msleep(10);
    }

    time_begin = time(NULL);
    int i = 0;
    while(cards[i].UID){
        if(i == 0 || cards[i].simulate) {
            cards[i].num_tries++;
            if(i != 0) printTextviewBuffer(THREADPRINT, "Simulating %s", cards[i].name);
            Simulate(i);
        }
        i++;
        if(args->stopThread) {pthread_exit(NULL); return NULL;}
    }
    i = 0;
    while(cards[i].UID){
        if(i == 0 || (cards[i].simulate && !cards[i].detected)) {
            cards[i].num_tries++;
            if(i != 0) printTextviewBuffer(THREADPRINT, "Simulating %s again", cards[i].name);
            Simulate(i);
        }
        i++;
        if(args->stopThread) {pthread_exit(NULL); return NULL;}
    }
    time_end = time(NULL);
    
    printTextviewBuffer(THREADPRINT, "End of test, test took %d seconds...", time_end-time_begin);

    g_idle_add(showResults, NULL);
    g_idle_add(resetTests, NULL);

    pthread_exit(NULL);
    return NULL;
}
// Test thread for card types ///////////////////////////////////////////////////////////////////////////////////////////////

// Test thread for endurance test ///////////////////////////////////////////////////////////////////////////////////////////
/****************************************************************************/
/*!
    @brief Initiate and start Endurance test thread.
    @param void
    @return 
 */
/****************************************************************************/
void initEnduranceTestThread(void){
    running_threads |= THREAD_ENDURANCE;
    pthread_create(&endurance_test_thread, NULL, enduranceTestThead, &thread_args);
}

/****************************************************************************/
/*!
    @brief Thread to simulate one card a given amount of times.
    @param p pointer to thread arguments passed through from initiation.
    @return void*
 */
/****************************************************************************/
void* enduranceTestThead(void* p){
    if(g_debugMode) printf("Endurance test thread started\n");
    UIDthread_arg_t *args = (UIDthread_arg_t *)p;

    int cardcount = 0;

    while(!availability_args.spiderInputReady){
        msleep(10);
    }

    time_begin = time(NULL);
    Simulate(0);
    int i = 1;
    while(i <= args->endurance_test_size){
        printTextviewBuffer(THREADPRINT, "Simulation %d of %s", i, cards[args->testtype].name);
        if(Simulate(args->testtype)) cardcount++;
        i++;
        if(args->stopThread) break;
    }
    time_end = time(NULL);
    
    printTextviewBuffer(THREADPRINT, "End of test, test took %d seconds...\nCard was detected %d times out of %d simulations", time_end-time_begin, cardcount, args->endurance_test_size);

    g_idle_add(resetTests, NULL);

    pthread_exit(NULL);
    return NULL;
}
// Test thread for endurance test ///////////////////////////////////////////////////////////////////////////////////////////

// Test thread for specific test type ///////////////////////////////////////////////////////////////////////////////////////
/****************************************************************************/
/*!
    @brief Initiate and start a conifg specific test thread.
    @param void
    @return 
 */
/****************************************************************************/
void initSpecificTestThread(void){
    running_threads |= THREAD_SPECIFIC;
    pthread_create(&specific_test_thread, NULL, specificTestThread, &thread_args);
}

/****************************************************************************/
/*!
    @brief Thread for a config specific test.
    @param p pointer to thread arguments passed through from initiation.
    @return void*
 */
/****************************************************************************/
void* specificTestThread(void* p){
    UIDthread_arg_t *args = (UIDthread_arg_t *)p;
    if(g_debugMode) printf("Specific test thread started: %d\n", args->testtype);

    int i = 0;
    int wrong_cards = 0;
    int correct_cards = 0;

    while(cards[i].UID){
        if(cards[i].simulate || i == 0){
            if(g_debugMode) printf("Simulating %s, %s\n", cards[i].name, cards[i].UID);
            cards[i].num_tries++;
            if(i != 0) printTextviewBuffer(THREADPRINT, "Simulating %s", cards[i].name);
            Simulate(i);
        }
        i++;
        if(args->stopThread) {
            pthread_exit(NULL); 
            return NULL;
        }
    }

    i = 0;
    switch(args->testtype){
        case 1:
            while(cards[i].UID){
                if(cards[i].detected){
                    if((strcmp(cards[i].name, "Mifare Classic 1k") != 0) && (strcmp(cards[i].name, "Mifare Classic 4k") != 0)){
                        wrong_cards++;
                    }else{
                        correct_cards++;
                    }
                }
                i++;
            }
            if(wrong_cards == 0 && correct_cards > 0) {
                printTextviewBuffer(THREADPRINT, "Mifare Classic only test succesful");
            }else{
                if(correct_cards == 0){
                    printTextviewBuffer(THREADPRINT, "Mifare Classic only test failed: no correct cards detected");
                }else{
                    printTextviewBuffer(THREADPRINT, "Mifare Classic only test failed: %d card(s) detected while not supposed to", wrong_cards);
                }
            }
            g_idle_add(showResults, NULL);
            break;
        case 2:
            while(cards[i].UID){
                if(cards[i].detected){
                    if((strcmp(cards[i].name, "Mifare Ultralight") != 0) && (strcmp(cards[i].name, "NTAG 215") != 0)){
                        wrong_cards++;
                    }else{
                        correct_cards++;
                    }
                }
                i++;
            }
            if(wrong_cards == 0 && correct_cards > 0) {
                printTextviewBuffer(THREADPRINT, "Mifare Ultralight and NTAG 215 only test succesful");
            }else{
                if(correct_cards == 0){
                    printTextviewBuffer(THREADPRINT, "Mifare Ultralight and NTAG 215 only test failed: no correct cards detected");
                }else{
                    printTextviewBuffer(THREADPRINT, "Mifare Ultralight and NTAG 215 only test failed: %d card(s) detected while not supposed to", wrong_cards);
                }
            }
            g_idle_add(showResults, NULL);
            break;
        case 3:
            while(cards[i].UID){
                if(cards[i].detected){
                    if(strcmp(cards[i].name, "Mifare Classic Block Read") == 0){
                        correct_cards++;
                    }else{
                        wrong_cards++;
                    }
                }
                i++;
            }
            if(correct_cards == 1){
                printTextviewBuffer(THREADPRINT, "Mifare Classic read block from memory test succesful");
            }else{
                printTextviewBuffer(THREADPRINT, "Mifare Classic read block from memory test failed, block from memory not received");
            }
            break;
    }

    g_idle_add(resetTests, NULL);

    pthread_exit(NULL);
    return NULL;
}
// Test thread for specific test type ///////////////////////////////////////////////////////////////////////////////////////

// Config programmer thread /////////////////////////////////////////////////////////////////////////////////////////////////
/****************************************************************************/
/*!
    @brief Initiate and start the Config Program thread.
    @param void
    @return 
 */
/****************************************************************************/
void initConfigProgramThread(void){
    running_threads |= THREAD_CONFIG;
    pthread_create(&config_program_thread, NULL, configProgramThread, &thread_args);
}

/****************************************************************************/
/*!
    @brief Thread to program a Spider with a given config.
    @param p pointer to thread arguments passed through from initiation.
    @return void*
 */
/****************************************************************************/
void* configProgramThread(void* p){
    if(g_debugMode) printf("Config program thread started\n");
    UIDthread_arg_t *args = (UIDthread_arg_t *)p;
    int err = 0;
    int success = 0;
    int tries = 0;

    do{
        err = switchMode(BOOTLOADER_MODE);
        if(err < 0 && g_debugMode) printf("Switch error %d: %s\n", err, libusb_strerror(err));
        tries++;
        if(args->stopThread || tries >= 5) goto exit_config_thread;
    }while(err != 0);

    tries = 0;
    do{
        err = sendConfig(args->required_config);
        if(err < 0 && g_debugMode) printf("Send config error %d: %s\n", err, libusb_strerror(err));
        tries++;
        if(args->stopThread || tries >= 5) goto exit_config_thread;
    }while(err != 0);
    success = 1;

exit_config_thread:
    tries = 0;
    do{
        err = switchMode(NORMAL_MODE);
        if(err < 0 && g_debugMode) printf("Switch error %d: %s\n", err, libusb_strerror(err));
        tries++;
    }while(err != 0 && tries < 10);

    if(success){
        printTextviewBuffer(THREADPRINT, "Spider configured succesfully with [%s]", getConfig());
    }else{
        printTextviewBuffer(THREADPRINT, "Configuration failed or got interrupted");
    }

    if(g_debugMode) printf("Config succesfully uploaded\n");

    g_idle_add(resetTests, NULL);

    pthread_exit(NULL);
    return NULL;
}
// Config programmer thread /////////////////////////////////////////////////////////////////////////////////////////////////

// Spider availability thread ///////////////////////////////////////////////////////////////////////////////////////////////
/****************************************************************************/
/*!
    @brief Initiate and start the availability thread.
    @param void
    @return 
 */
/****************************************************************************/
void initAvailabilityThread(void){
    running_threads |= THREAD_AVAILABILITY;
    availability_args.stopThread = false;
    availability_args.available = true;
    pthread_create(&availability_thread, NULL, availabilityThread, &availability_args);
}

#define MAX_TIME_BOOT_MODE 10

/****************************************************************************/
/*!
    @brief Thread to check if a Spider is connected and to check the state of the Spider.
    @param p pointer to thread arguments passed through from initiation.
    @return void*
 */
/****************************************************************************/
void* availabilityThread(void* p){
    availability_arg_t *args = (availability_arg_t *)p;
    struct libusb_device_descriptor descriptor = {0};
    libusb_device **list;
    ssize_t cnt = 0;
    bool state = false;
    int old_state = -1;
    int new_state = 0;
    int boot_state = 0;
    time_t boot_begin = 0;

    while(!args->stopThread){
        // check if Spider is connected
        cnt = libusb_get_device_list(NULL, &list);
        if(cnt < 0) return 0;
        for(int i = 0; i < cnt; i++){
            libusb_get_device_descriptor(list[i], &descriptor);
            if(descriptor.idVendor == 0x1da6){
                new_state = 1;
                break;
            }
            new_state = 0;
        }
        if(new_state != old_state){
            old_state = new_state;
            if(old_state){
                if(descriptor.idProduct == 0x0000){
                    boot_state = 1;
                    boot_begin = time(NULL);
                }else{
                    boot_state = 0;
                }
                state = true;
                if(g_debugMode) printf("Spider connected!\n");
                g_idle_add(updateSpiderInfo, &state);
            }else{
                state = false;
                if(g_debugMode) printf("Spider not connected!\n");
                g_idle_add(updateSpiderInfo, &state);
            }
        }
        libusb_free_device_list(list, 1);

        if(boot_state == 1 && (time(NULL) - boot_begin >= MAX_TIME_BOOT_MODE)){
            printTextviewBuffer(THREADPRINT, "Set Spider back to normal mode from bootloader mode");
            boot_state = 0;
            switchMode(NORMAL_MODE);
        }

        pthread_mutex_lock(&gtk_mutex);
        if(args->quitProgram){
            g_idle_add(callDestroy, NULL);
            args->stopThread = true;
        }
        pthread_mutex_unlock(&gtk_mutex);
    }

    pthread_exit(NULL);
    return NULL;
}
// Spider availability thread ///////////////////////////////////////////////////////////////////////////////////////////////

/****************************************************************************/
/*!
    @brief Stop all threads and wait for each thread to join.
    @param void
    @return
 */
/****************************************************************************/
void stopThreads(void){
    if(g_debugMode) printf("Stop threads\n");
    pthread_mutex_lock(&thread_mutex);
    thread_args.stopThread = true;
    pthread_mutex_unlock(&thread_mutex);
    if(g_debugMode) printf("Threads join\n");

    if(g_debugMode) printf("running_threads: %02X\n", running_threads);

    if(running_threads & THREAD_ENDURANCE) {
        pthread_join(endurance_test_thread, NULL);
        running_threads &= (0xFF - THREAD_ENDURANCE);
        if(g_debugMode) printf("Endurance test thread stopped\n");
    }

    if(running_threads & THREAD_CARDTYPE) {
        pthread_join(cardtype_test_thread, NULL);
        running_threads &= (0xFF - THREAD_CARDTYPE);
        if(g_debugMode) printf("Cardtype test thread stopped\n");
    }

    if(running_threads & THREAD_CONFIG) {
        pthread_join(config_program_thread, NULL);
        running_threads &= (0xFF - THREAD_CONFIG);
        if(g_debugMode) printf("Config program thread stopped\n");
    }

    if(running_threads & THREAD_SPECIFIC) {
        pthread_join(specific_test_thread, NULL);
        running_threads &= (0xFF - THREAD_SPECIFIC);
        if(g_debugMode) printf("Specific test thread stopped\n");
    }

    if(running_threads & THREAD_SPIDER) {
        pthread_join(spider_thread, NULL);
        running_threads &= (0xFF - THREAD_SPIDER);
        if(g_debugMode) printf("Spider communication thread stopped\n");
    }
}

/****************************************************************************/
/*!
    @brief Compare given UID with list of known UID's.
    @param check_uid char pointer of UID to compare.
    @return integer with number to index of the known and correctly compared to UID, returns 0 if unknown.
 */
/****************************************************************************/
int checkUID(char *check_uid){
    int i = 0;
    //printf("Check UID: %s\n", check_uid);
    while(cards[i].UID){
        if(strcmp(cards[i].UID, check_uid) == 0){
            cards[i].detected = true;
            return i;
        }
        i++;
    }
    return 0;
}

// Print to textviewbuffer //////////////////////////////////////////////////////////////////////////////////////
/****************************************************************************/
/*!
    @brief Print text to textview buffer.
    @param thread To indicate if the call came from a thread or from the gui as they need to be handled differently.
    @param text char pointer to text that needs be printed.
    @param ... specifier arguments
    @return 
 */
/****************************************************************************/
void printTextviewBuffer(int thread, const char *text, ...){
    char buffer[PRINT_BUFFER_SIZE] = {0};

    va_list args;
    va_start(args, text);
    vsnprintf(buffer, sizeof(buffer), text, args);
    va_end(args);
    strcat(buffer, "\n");

    if(thread){
        g_idle_add(printtologscreen, buffer);
    }else{
        gtk_text_buffer_insert(textviewbuf3, &iter1, (const gchar*)buffer, -1);
    }

/*
    int num = 0;
    int count = 0;

    va_list args;
    va_start(args, text);
    num = vsnprintf(buffer, sizeof(buffer), text, args);
    va_end(args);
    strcat(buffer, "\n");

    if(num > MAX_BUFFER_SIZE - 2){
        while(count < num){
            if(g_debugMode) printf("num: %d, count: %d\n", num, count);
            memmove(&buffer[count + MAX_BUFFER_SIZE], &buffer[count + MAX_BUFFER_SIZE - 2], sizeof(buffer) - (count + MAX_BUFFER_SIZE));
            buffer[count + MAX_BUFFER_SIZE - 2] = '\n';
            buffer[count + MAX_BUFFER_SIZE - 1] = '\0';
            if(g_debugMode) printf("%s", &buffer[count]);

            if(thread){
                g_idle_add(printtologscreen, &buffer[count]);
            }else{
                gtk_text_buffer_insert(textviewbuf3, &iter1, (const gchar*)&buffer[count], -1);
            }
            count += MAX_BUFFER_SIZE;
        }
    }else{
        if(thread){
            g_idle_add(printtologscreen, buffer);
        }else{
            gtk_text_buffer_insert(textviewbuf3, &iter1, (const gchar*)buffer, -1);
        }
    }
*/
}

/****************************************************************************/
/*!
    @brief Handler for print to textview buffer if call came from a thread.
    @param text void pointer to text to print.
    @return gboolean to indicatie if source needs to be removed after call.
 */
/****************************************************************************/
gboolean printtologscreen(void *text){
    const gchar *buffer = text;
    gtk_text_buffer_get_end_iter(textviewbuf3, &iter1);
    gtk_text_buffer_insert(textviewbuf3, &iter1, buffer, -1);
    return G_SOURCE_REMOVE;
}
// Print to textviewbuffer ///////////////////////////////////////////////////////////////////////////////////////

/****************************************************************************/
/*!
    @brief Print text to result window textview buffers.
    @param textviewbuffer textviewbuffer to print text to.
    @param text char pointer to text that needs be printed.
    @param ... specifier arguments
    @return 
 */
/****************************************************************************/
void printResultTextview(GtkTextBuffer *textviewbuffer, const char *text, ...){
    struct textview_args *args = g_slice_alloc(sizeof(*args));
    args->textviewbuffer = textviewbuffer;
    memset(args->text, 0, PRINT_BUFFER_SIZE);

    va_list arguments;
    va_start(arguments, text);
    vsnprintf(args->text, PRINT_BUFFER_SIZE - 2, text, arguments);
    va_end(arguments);
    strcat(args->text, "\r\n");

    g_idle_add(printtoresultwindow, args);
}

/****************************************************************************/
/*!
    @brief Handler for print to textview buffer if call came from a thread.
    @param p void pointer to text to print.
    @return gboolean to indicatie if source needs to be removed after call.
 */
/****************************************************************************/
gboolean printtoresultwindow(void *p){
    struct textview_args *args = p;
    GtkTextIter iter;
    gtk_text_buffer_get_end_iter(args->textviewbuffer, &iter);
    gtk_text_buffer_insert(args->textviewbuffer, &iter, args->text, -1);
    return G_SOURCE_REMOVE;
}

// Update progressbar ////////////////////////////////////////////////////////////////////////////////////////////
/****************************************************************************/
/*!
    @brief Update progressbar with count/number.
    @param count integer for a certain amount out of number.
    @param number integer for desired amount.
    @return
 */
/****************************************************************************/
void updateProgressbar(int count, int number){
    struct progressbar_args *args = g_slice_alloc(sizeof(*args));
    args->cardcount_arg = count;
    args->numcards_args = number;
    g_idle_add(progressbarUpdate, args);
}

/****************************************************************************/
/*!
    @brief Handler for update of progressbar.
    @param p void pointer to struct with to count and number.
    @return gboolean to indicatie if source needs to be removed after call.
 */
/****************************************************************************/
gboolean progressbarUpdate(void *p){
    struct progressbar_args *args = p;
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progressbar1), (gdouble)((double)args->cardcount_arg/(double)args->numcards_args));
    return G_SOURCE_REMOVE;
}
// Update progressbar ////////////////////////////////////////////////////////////////////////////////////////////

/****************************************************************************/
/*!
    @brief Handler to call resetbutton clicked function from threads.
    @param p void pointer, but not used.
    @return gboolean to indicatie if source needs to be removed after call.
 */
/****************************************************************************/
gboolean resetTests(void *p){
    on_resetbutton1_clicked(resetbutton1);
    return G_SOURCE_REMOVE;
}

/****************************************************************************/
/*!
    @brief Handler to show window with results from tests.
    @param p void pointer, but not used.
    @return gboolean to indicatie if source needs to be removed after call.
 */
/****************************************************************************/
gboolean showResults(void *p){
    printResults();
    gtk_widget_show(window2);
    gtk_window_move(GTK_WINDOW(window2), 50, 50);
    return G_SOURCE_REMOVE;
}

/****************************************************************************/
/*!
    @brief Handler to update info about Spider if connected and currect config.
    @param p void pointer to bool if a Spider is connected.
    @return gboolean to indicatie if source needs to be removed after call.
 */
/****************************************************************************/
gboolean updateSpiderInfo(void *p){
    bool *available = p;
    char *configname = calloc(1, 64);
    if(*available){
        sprintf(configname, "Spider connected: [%s]", getConfig());
        gtk_label_set_text(GTK_LABEL(spider_info_label), configname);
        pthread_mutex_lock(&gtk_mutex);
        if(availability_args.available){
            gtk_widget_set_sensitive(startbutton1, TRUE);
        }
        pthread_mutex_unlock(&gtk_mutex);
        gtk_widget_set_sensitive(spider_info_label, TRUE);
    }else{
        gtk_label_set_text(GTK_LABEL(spider_info_label), "No Spider connected");
        gtk_widget_set_sensitive(startbutton1, FALSE);
        gtk_widget_set_sensitive(spider_info_label, FALSE);
    }
    free(configname);
    return G_SOURCE_REMOVE;
}

// quit program functions ///////////////////////////////////////////////////////////////////////////////////////////
/****************************************************************************/
/*!
    @brief Call to exit program, it will set quitProgram to true, availabilityThread will call destroy function.
    @param void
    @return
 */
/****************************************************************************/
void exitProgram(void){
    pthread_mutex_lock(&gtk_mutex);
    availability_args.quitProgram = true;
    pthread_mutex_unlock(&gtk_mutex);
}

/****************************************************************************/
/*!
    @brief Handler to call destroy function from threads.
    @param p void pointer, but not used.
    @return gboolean to indicatie if source needs to be removed after call.
 */
/****************************************************************************/
gboolean callDestroy(void *p){
    printf("Exit program\n");
    destroy(NULL);
    return G_SOURCE_REMOVE;
}
// quit program functions ///////////////////////////////////////////////////////////////////////////////////////////

/****************************************************************************/
/*!
    @brief Function will call the simulation function of the specified card, wait for 5 seconds if the card is read from Spider and send a stop sim command.
    @param sim index to card to simulate in list of cards[]
    @return 
 */
/****************************************************************************/
int Simulate(int sim){
    int ret = 0;
    cards[sim].simFunction();
    int i = 0;
    if(sim != 0){
        while(!thread_args.available && i < WAIT_TIME){
            msleep(1);
            i++;
            if(thread_args.stopThread) break;
        }
    }
    if(thread_args.available) ret = 1;
    stopSim();
    pthread_mutex_lock(&thread_mutex);
    thread_args.available = false;
    pthread_mutex_unlock(&thread_mutex);
    return ret;
}

// Simulation functions ////////////////////////////////////////////////////////////////////////////////////////////////////////
/****************************************************************************/
/*!
    @brief Send command to proxmark3 to stop simulation.
    @param void
    @return 
 */
/****************************************************************************/
void stopSim(void){
    clearCommandBuffer();
    SendCommandNG(CMD_BREAK_LOOP, NULL, 0);
    msleep(500);
}

/****************************************************************************/
/*!
    @brief Simulate a buffer card, because the first card to simulate is not fast enough detected by Spider.
    @param void
    @return 
 */
/****************************************************************************/
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

/****************************************************************************/
/*!
    @brief Simulate a Mifare Classic 1k card.
    @param void
    @return 
 */
/****************************************************************************/
void SimMfClas1k(void){
    int uid_len = 7;
    uint8_t uid[10] = {0x4B, 0x65, 0x76, 0x69, 0x6E, 0x00, 0x01};
    if(g_debugMode) PrintAndLogEx(INFO, "TESTING ISO 14443A (Mifare Classic 1k) sim with UID %s", sprint_hex(uid, uid_len));

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

/****************************************************************************/
/*!
    @brief Simulate a Mifare Ultralight card.
    @param void
    @return 
 */
/****************************************************************************/
void SimMfUltra(void){
    int uid_len = 7;
    uint8_t uid[10] = {0x4B, 0x65, 0x76, 0x69, 0x6E, 0x00, 0x02};
    if(g_debugMode) PrintAndLogEx(INFO, "TESTING ISO 14443A (Mifare Ultralight) sim with UID %s", sprint_hex(uid, uid_len));

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

/****************************************************************************/
/*!
    @brief Simulate a Mifare Mini card.
    @param void
    @return 
 */
/****************************************************************************/
void SimMfMini(void){
    int uid_len = 7;
    uint8_t uid[10] = {0x4B, 0x65, 0x76, 0x69, 0x6E, 0x00, 0x06};
    if(g_debugMode) PrintAndLogEx(INFO, "TESTING ISO 14443A (Mifare Mini) sim with UID %s", sprint_hex(uid, uid_len));

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

/****************************************************************************/
/*!
    @brief Simulate a NTAG card.
    @param void
    @return 
 */
/****************************************************************************/
void SimNTAG215(void){
    int uid_len = 7;
    uint8_t uid[10] = {0x4B, 0x65, 0x76, 0x69, 0x6E, 0x00, 0x07};
    if(g_debugMode) PrintAndLogEx(INFO, "TESTING ISO 14443A (NTAG 215) sim with UID %s", sprint_hex(uid, uid_len));

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

/****************************************************************************/
/*!
    @brief Simulate a Mifare Classic 4k card.
    @param void
    @return 
 */
/****************************************************************************/
void SimMfClas4k(void){
    int uid_len = 7;
    uint8_t uid[10] = {0x4B, 0x65, 0x76, 0x69, 0x6E, 0x00, 0x08};
    if(g_debugMode) PrintAndLogEx(INFO, "TESTING ISO 14443A (Mifare Classic 4k) sim with UID %s", sprint_hex(uid, uid_len));

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

/****************************************************************************/
/*!
    @brief Simulate a FM11RF005SH card.
    @param void
    @return 
 */
/****************************************************************************/
void SimFM11RF005SH(void){
    int uid_len = 7;
    uint8_t uid[10] = {0x4B, 0x65, 0x76, 0x69, 0x6E, 0x00, 0x09};
    if(g_debugMode) PrintAndLogEx(INFO, "TESTING ISO 14443A (FM11RF005SH) sim with UID %s", sprint_hex(uid, uid_len));

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

/****************************************************************************/
/*!
    @brief Simulate a iClass card.
    @param void
    @return 
 */
/****************************************************************************/
void SimiClass(void){
    uint8_t csn[8] = {0x4B, 0x65, 0x76, 0x69, 0x6E, 0xB9, 0x33, 0x14};
    if(g_debugMode) PrintAndLogEx(INFO, "TESTING iClass sim with UID %s", sprint_hex(csn, 8));
    clearCommandBuffer();
    SendCommandMIX(CMD_HF_ICLASS_SIMULATE, 0, 0, 1, csn, 8); //Simulate iClass
}

/****************************************************************************/
/*!
    @brief Simulate a HID card.
    @param void
    @return 
 */
/****************************************************************************/
void SimHID(void){
    lf_hidsim_t payload;
    payload.hi2 = 0;
    payload.hi = 0x30;
    payload.lo = 0x06ec0c86;
    payload.longFMT = (0x30 > 0xFFF);
    if(g_debugMode) PrintAndLogEx(INFO, "TESTING HID sim with UID 10 06 EC 0C 86");

    clearCommandBuffer();
    SendCommandNG(CMD_LF_HID_SIMULATE, (uint8_t *)&payload,  sizeof(payload));
}

/****************************************************************************/
/*!
    @brief Simulate a EM410x card.
    @param void
    @return 
 */
/****************************************************************************/
void SimEM410x(void){
    int clock = 64;
    int uid_len = 5;
    int gap = 20;
    uint8_t uid[] = {0x0F, 0x03, 0x68, 0x56, 0x8B};

    if(g_debugMode) PrintAndLogEx(INFO, "TESTING EM410x sim with UID %s", sprint_hex(uid, uid_len));

    em410x_construct_emul_graph(uid, clock, gap);
    CmdLFSim("");
}

/****************************************************************************/
/*!
    @brief Simulate a Paradox card.
    @param void
    @return 
 */
/****************************************************************************/
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

    if(g_debugMode) PrintAndLogEx(INFO, "TESTING Paradox sim with UID 21 82 77 AA CB");

    clearCommandBuffer();
    SendCommandNG(CMD_LF_FSK_SIMULATE, (uint8_t *)payload,  sizeof(lf_fsksim_t) + sizeof(bs));
    free(payload);
}

/****************************************************************************/
/*!
    @brief Simulate a Noralsy card.
    @param void
    @return 
 */
/****************************************************************************/
void SimNoralsy(void){
    uint32_t id = 1337;
    uint16_t year = 2000;

    uint8_t bs[96];
    memset(bs, 0, sizeof(bs));

    if (getnoralsyBits(id, year, bs) != PM3_SUCCESS) {
        if(g_debugMode) PrintAndLogEx(ERR, "Error with tag bitstream generation.");
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

/****************************************************************************/
/*!
    @brief Simulate an AWID card.
    @param void
    @return 
 */
/****************************************************************************/
void SimAwid(void){
    uint8_t bs[] = {0,0,0,0,0,0,0,1,0,0,0,1,1,1,0,1,1,0,0,0,0,1,1,1,1,1,0,1,1,1,0,1,0,0,0,1,0,1,0,0,1,0,0,0,1,1,1,0,0,0,1,0,1,0,0,0,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1};
    lf_fsksim_t *payload = calloc(1, sizeof(lf_fsksim_t) + sizeof(bs));
    payload->fchigh = 10;
    payload->fclow =  8;
    payload->separator = 1;
    payload->clock = 50;
    memcpy(payload->data, bs, sizeof(bs));

    if(g_debugMode) PrintAndLogEx(INFO, "TESTING Awid sim with UID 04 F6 0A 73");

    clearCommandBuffer();
    SendCommandNG(CMD_LF_FSK_SIMULATE, (uint8_t *)payload,  sizeof(lf_fsksim_t) + sizeof(bs));
    free(payload);
}

/****************************************************************************/
/*!
    @brief Simulate a Mifare Classic 1k card that is able to let its memory be read.
    @param void
    @return 
 */
/****************************************************************************/
void SimMifare1K(void){
    uint8_t uid[] = {0x4B, 0x65, 0x76, 0x69, 0x6E, 0x00, 0x01};

    struct {
        uint16_t flags;
        uint8_t exitAfter;
        uint8_t uid[10];
        uint16_t atqa;
        uint8_t sak;
    } PACKED payload;

    payload.flags = 0x0104; //for 1k, 0x0404 for 4k
    payload.exitAfter = 0;
    memcpy(payload.uid, uid, 7);
    payload.atqa = 0;
    payload.sak = 0;

    clearCommandBuffer();
    SendCommandNG(CMD_HF_MIFARE_SIMULATE, (uint8_t *)&payload, sizeof(payload));
}
// Simulation functions /////////////////////////////////////////////////////////////////////////////////////////////////////

/****************************************************************************/
/*!
    @brief Print the results of a card type test.
    @param void
    @return 
 */
/****************************************************************************/
void printResults(void){
    if(g_debugMode){
        PrintAndLogEx(INFO, "========================================================");
        PrintAndLogEx(INFO, "Testing took %d seconds.", (time_end - time_begin));
        PrintAndLogEx(INFO, "========================================================");
        PrintAndLogEx(INFO, "UID's detected:");
        PrintAndLogEx(INFO, "#  | Card UID         | Card Name         | Tries");
    }
    int detect_count = 0;
    int not_detect_count = 0;
    int i = 1;
    while(cards[i].UID){
        if(cards[i].detected){
            if(g_debugMode) PrintAndLogEx(SUCCESS, "%-2i | %-16s | %-17s | %i", i, cards[i].UID, cards[i].name, cards[i].num_tries);
            printResultTextview(textviewbuf1, "%s: %s", cards[i].name, cards[i].UID);
            detect_count++;
        }else if(cards[i].simulate){
            not_detect_count++;
        }
        i++;
    }
    if(g_debugMode){
        PrintAndLogEx(INFO, "========================================================");
        PrintAndLogEx(INFO, "========================================================");
    }
    if(not_detect_count != 0){
        if(g_debugMode){
            PrintAndLogEx(INFO, "Missing UID's:");
            PrintAndLogEx(INFO, "#  | Card UID         | Card Name");
        }
        i = 1;
        while(cards[i].UID){
            if(!cards[i].detected && cards[i].simulate){
                if(g_debugMode) PrintAndLogEx(INFO, _RED_("%-2i") " | " _RED_("%-16s") " | " _RED_("%s"), i, cards[i].UID, cards[i].name);
                printResultTextview(textviewbuf2, "%s: %s", cards[i].name, cards[i].UID);
            }
            i++;
        }
    }else{
        if(g_debugMode) PrintAndLogEx(SUCCESS, _GREEN_("No cards not detected"));
        printResultTextview(textviewbuf2, "No cards not detected");
    }
    if(g_debugMode) PrintAndLogEx(INFO, "========================================================");
}

/****************************************************************************/
/*!
    @brief Setup Key Codes for spiderThread to receive input from Spider.
    @param void
    @return 
 */
/****************************************************************************/
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

/****************************************************************************/
/*!
    @brief Convert decimal value to a hexadecimal string.
    @param Des char pointer to output hexadecimal string to.
    @param Src unsigned long long integer to convert to hexadecimal string.
    @return 
 */
/****************************************************************************/
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
        if(i%2 == 1){
            hex[i] = '0';
        }
        Src = Src / 16;
    }
    size_t l = strlen(hex);
    Des[l] = '\0';
    for(int j = 0; j < l; j++){
        Des[j] = hex[l - 1 - j];
    }
}

/****************************************************************************/
/*!
    @brief Search through input devices for Spider.
    @param void
    @return char of number on which event device the Spider is, default is '0'
 */
/****************************************************************************/
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

/****************************************************************************/
/*!
    @brief Search on which usb port the proxmark3 is located.
    @param void
    @return char pointer with "/dev/ttyACMx" with x the port number of the proxmark3, return NULL if not found or failed.
 */
/****************************************************************************/
char *FindProxmark(void){
    FILE *fp;
    char *buf = calloc(1, 64);
    for(int i = 0; i <= 9 ; i++){
        sprintf(buf, "/sys/class/tty/ttyACM%d/../../../manufacturer", i);
        fp = fopen(buf, "r");
        if(fp == NULL){
            continue;
        }
        memset(buf, 0, 64);
        fgets(buf, 20, fp);
        if(strcmp(buf, "proxmark.org\n") == 0){
            fclose(fp);
            sprintf(buf, "/dev/ttyACM%d", i);
            return buf;
        }
        if(g_debugMode) printf("Correct file not found\n");
        fclose(fp);
    }
    printf("ERROR: No compatible device found\n");
    if(fp != NULL)
        fclose(fp);
    return NULL;
}

/****************************************************************************/
/*!
    @brief Send config file to Spider if in bootloader mode.
    @param config_num index of config in list of config_t struct.
    @return error number
 */
/****************************************************************************/
int sendConfig(int config_num){
    int err = 0;
    FILE *file;
    libusb_device_handle *handle = NULL;
    unsigned char *data = calloc(1, 64);
    unsigned char *buf = calloc(1, 64);
    char *filename = calloc(1, 128);

    handle = libusb_open_device_with_vid_pid(NULL, 0x1da6, 0x0000); // Open spider if in bootloader mode
    while(handle == NULL){
        handle = libusb_open_device_with_vid_pid(NULL, 0x1da6, 0x0000); // Open spider if in bootloader mode
        if(thread_args.stopThread) return 1;
    }

    if(g_debugMode) printf("Check if kernel driver active\n");
    if(libusb_kernel_driver_active(handle, 0) == 1){
        if(g_debugMode) printf("Kernel driver active, detach kernel driver\n");
        err = libusb_detach_kernel_driver(handle, 0);
        if(err < 0) {
            if(g_debugMode) printf("Couldn't detach kernel driver: %d, %s\n", err, libusb_strerror(err));
            libusb_close(handle);
            return err;
        }
    }

    err = libusb_claim_interface(handle, 0);
    if(err < 0){ 
        if(g_debugMode) printf("Claim error: %s\n", libusb_strerror(err));
        libusb_close(handle);
        return err;
    }

    sprintf(filename, "/home/pi/proxmark3/client/spider-configs/%s.readerconfig", config[config_num].name);
    file = fopen(filename, "r");

    if(file == NULL){
        if(g_debugMode) printf("Couldn't open config file: %s\n", filename);
        return -99;
    }

    free(filename);

    int size = 0;
    int count = 0;
    int temp = 0;
    int length = 0;
    unsigned char *mem = calloc(1, 3000);

    int transferred = 0;

    while(1){
        temp = fgetc(file);
        if(temp == EOF){
            mem[size] = 0;
            break;
        }
        if(temp != 10 && temp != 13){
            if(temp == ':'){
                for(int i = 0; i < 6; i++){
                    fgetc(file);
                }
                if(fgetc(file) == '0'){
                    if(fgetc(file) == '0'){
                        for(int i = 0; i < 32; i++){
                            temp = fgetc(file);
                            mem[size] = temp;
                            size++;
                        }
                    }
                }
            }
        }
    }

    for(int i = 0; i < size; i++){
        if(i % 2 == 0){
            if(mem[i] >= 48 && mem[i] < 58){
                mem[count] = (mem[i] - 48) * 16;
            }else if(mem[i] >= 65 && mem[i] < 71){
                mem[count] = (mem[i] - 55) * 16;
            }else if(mem[i] >= 97 && mem[i] < 103){
                mem[count] = (mem[i] - 87) * 16;
            }
        }else{
            if(mem[i] >= 48 && mem[i] < 58){
                mem[count] += (mem[i] - 48);
            }else if(mem[i] >= 65 && mem[i] < 71){
                mem[count] += (mem[i] - 55);
            }else if(mem[i] >= 97 && mem[i] < 103){
                mem[count] += (mem[i] - 87);
            }
            count++;
        }
    }
    mem[count] = 0;
    size = count;
    count = 0;

    fclose(file);

    if(g_debugMode) printf("Size of config file: %d bytes\n", size);

    // Beeper volume is the 8th byte in the config file
    if(silent_mode) {
        for(int i = 0; i < 512 ; i++){
            if(mem[i] == 0x8F){
                if(mem[i+1] == 0x01){
                    mem[i+2] = 0;
                    break;
                }
            }
        }
    }

    if(g_debugMode) {
        int j = 0;
        while(j < size){
            if(j%16 == 0) printf("\n");
            printf("%4d: %02X ", j, mem[j]);
            j++;
        }
        printf("\n");
    }

    data[0] = 0x05;
    data[1] = 0x00;
    data[2] = 0x00;
    data[3] = 0xF0;
    data[4] = 0x00;

    while(count < size){
        if((count + DATA_SIZE) > size){
            length = size - count;
            memset(&data[6], 0, DATA_SIZE);
            if(g_debugMode) printf("Short\n");
        }else{
            length = DATA_SIZE;
            if(g_debugMode) printf("Normal\n");
        }
        memcpy(&data[6], &mem[count], length);
        data[5] = length;
        temp = count;
        count += length;
        data[2] = 0;
        while(temp > 0xFF){
            data[2]++;
            temp = temp - 256;
        }
        data[1] = temp;

        if(g_debugMode){ 
            for(int i = 0; i < 64; i++){
                printf("%02X ", data[i]);
            }
            printf("\n");
        }

        err = libusb_bulk_transfer(handle, 0x01, data, 64, &transferred, 500);
        if(err < 0){
            if(g_debugMode) printf("Bulk transfer error %d: %s\n", err, libusb_strerror(err));
            libusb_close(handle);
            return err;
        }
        if(g_debugMode) printf("Transferred %d bytes\n", transferred);

        err = libusb_bulk_transfer(handle, 0x81, buf, 64, &transferred, 500);
        if(err < 0){
            if(g_debugMode) printf("Bulk read error %d: %s\n", err, libusb_strerror(err));
            libusb_close(handle);
            return err;
        }

        if(memcmp(buf, data, 64) == 0){
            if(g_debugMode) printf("Send block succesful\n");
        }
    }

    memset(data, 0, 64);
    data[0] = 0x06;
    err = libusb_bulk_transfer(handle, 0x01, data, 64, &transferred, 500);
    if(err){
        if(g_debugMode) printf("Bulk transfer error %d: %s\n", err, libusb_strerror(err));
    }
    memset(buf, 0, 64);
    err = libusb_bulk_transfer(handle, 0x81, buf, 64, &transferred, 500);
    if(err){
        if(g_debugMode) printf("Bulk read error %d: %s\n", err, libusb_strerror(err));
    }

    if(g_debugMode) {
        for(int i = 0; i < 7; i++){
            printf("%02X ", buf[i]);
        }
        printf("\n");
    }



    err = libusb_release_interface(handle, 0);
    if(err){
        if(g_debugMode) printf("Release error: %s\n", libusb_strerror(err));
    }
    
    free(data);
    free(buf);
    free(mem);

    libusb_close(handle);
    return 0;
}

/****************************************************************************/
/*!
    @brief Switch mode of Spider.
    @param desired_mode the desired mode to switch to: 0 for normal mode, 1 for bootloader mode
    @return error number
 */
/****************************************************************************/
int switchMode(int desired_mode){
// Mode 0: Normal mode
// Mode 1: Bootloader mode
    int err = 0;
    int current_mode;
    unsigned char *data = calloc(1, 64);
    libusb_device_handle *handle = NULL;
    data[0] = 0x08;
    data[1] = desired_mode;


    handle = libusb_open_device_with_vid_pid(NULL, 0x1da6, 0x0110); // Open spider if in normale mode
    current_mode = 0;
    if(handle == NULL){
        handle = libusb_open_device_with_vid_pid(NULL, 0x1da6, 0x0000); // Open spider if in bootloader mode
        current_mode = 1;
        if(handle == NULL){
            if(g_debugMode) printf("Spider not connected\n");
            libusb_close(handle);
            return -4;
        }
    }

    if(desired_mode == current_mode){
        if(g_debugMode) printf("Spider already in desired mode\n");
        libusb_close(handle);
        return 0;
    }

    if(g_debugMode) printf("Check if kernel driver active\n");
    if(libusb_kernel_driver_active(handle, 0) == 1){
        if(g_debugMode) printf("Kernel driver active, detach kernel driver\n");
        err = libusb_detach_kernel_driver(handle, 0);
        if(err < 0) {
            if(g_debugMode) printf("Couldn't detach kernel driver: %d, %s\n", err, libusb_strerror(err));
            libusb_close(handle);
            return err;
        }
    }

    err = libusb_claim_interface(handle, 0);
    if(err < 0){ 
        if(g_debugMode) printf("Claim error: %s\n", libusb_strerror(err));
        libusb_close(handle);
        return err; 
    }

    switch(desired_mode){
        case 0: // Spider in bootloader mode
            if(g_debugMode) printf("Switch to normale mode\n");
            err = libusb_bulk_transfer(handle, 0x01, data, 64, NULL, 500);
            if(err < 0) {
                if(g_debugMode) printf("Bulk transfer error %d: %s\n", err, libusb_strerror(err)); 
                libusb_release_interface(handle, 0);
                libusb_close(handle);
                return err;
            }
            break;
        case 1: // Spider in normal mode
            if(g_debugMode) printf("Switch to bootloader mode\n");
            err = libusb_control_transfer(handle, 0x21, 0x09, 0x0200, 0, data, 8, 500);
            if(err < 0) {
                if(g_debugMode) printf("Control transfer error %d: %s\n", err, libusb_strerror(err)); 
                libusb_release_interface(handle, 0);
                libusb_close(handle);
                return err;
            }
            break;
    }

    err = libusb_release_interface(handle, 0);
    if(err){
        if(g_debugMode) printf("Release error: %s\n", libusb_strerror(err));
    }

    free(data);

    libusb_close(handle);
    return 0;
}

/****************************************************************************/
/*!
    @brief Read config name from Spider from device descriptor.
    @param void
    @return char pointer with config name, returns NULL if failed or not connected.
 */
/****************************************************************************/
char* getConfig(void){
    libusb_device_handle *handle = NULL;
    unsigned char *data = calloc(1, 64);

    handle = libusb_open_device_with_vid_pid(NULL, 0x1da6, 0x0110); // Open spider if in normal mode
    if(handle == NULL){
        handle = libusb_open_device_with_vid_pid(NULL, 0x1da6, 0x0000); // Open spider if in bootloader mode
        if(handle == NULL){
            if(g_debugMode) printf("Spider not found\n");
            return NULL;
        }
    }

    libusb_get_string_descriptor_ascii(handle, 7, data, 64);

    if(g_debugMode) printf("Current config: %s\n", data);

    libusb_close(handle);

    return (char*)data;
}

/****************************************************************************/
/*!
    @brief Switch mode, upload config and switch back.
    @param config_num index of config in list of config_t struct.
    @return error number
 */
/****************************************************************************/
int uploadConfig(int config_num){
    int err = 0;
    err = switchMode(BOOTLOADER_MODE);
    if(err) return err;
    err = sendConfig(config_num);
    if(err) return err;
    err = switchMode(NORMAL_MODE);
    return err;
}
