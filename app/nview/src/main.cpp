#include <stdlib.h>
#include <pthread.h>
#include <ncurses.h>
#include <signal.h>
#include <string.h>

#include "lib/vcm/vcm.h"
#include "lib/telemetry/TelemetryViewer.h"
#include "lib/dls/dls.h"
#include "common/types.h"

#include "listview.h"

// view telemetry values live
// run as val_view [-f path_to_config_file]

using namespace vcm;
using namespace shm;
using namespace dls;


#define NUM_SIGNALS 5
int signals[NUM_SIGNALS] = {
    SIGINT,
    SIGTERM,
    SIGSEGV,
    SIGFPE,
    SIGABRT
};

TelemetryViewer tlm;
VCM* veh;

WINDOW* win;
listview* lv;

bool killed = false;



void sighandler(int) {
    killed = true;

    // signal the shared memory control to force us to awaken
    // tlm.sighandler();
    tlm.sighandler();
}


void setup_ncurses() {
    initscr();
    start_color();
    clear();
    noecho();
    curs_set(0);
    cbreak();
    keypad(stdscr, TRUE);
    WINDOW* win = newwin(0, 0, 0, 0);
    refresh();

    lv = listview_new();
    listview_add_column(lv, (char*) "Measurement");
    listview_add_column(lv, (char*) "Value");
    listview_add_column(lv, (char*) "Type");
    listview_add_column(lv, (char*) "Sign");
    listview_add_column(lv, (char*) "Endianness");
    listview_add_column(lv, (char*) "Hex");
    listview_add_column(lv, (char*) "Last Update (ms)");

    // for(size_t i = 0; i < veh->measurements.size(); i++) {
    for(size_t i = 0; i < 53; i++) {
        listview_add_row(lv, veh->measurements[i].c_str(), "test", "int", "signed", "big", "0xDEADBEEF", "173");
    }


    listview_update_cell_background_color(lv, 2, 1, cyan);

    listview_render(win, lv);
    wrefresh(win);
    refresh();

    while(1) {};

    int row = 0;
    int max_row = veh->measurements.size() - 1;
    int col = 0;
    int max_col = 6;
    while(1) {
        listview_update_cell_background_color(lv, row, col, black);
        switch(getch()) {
            case KEY_LEFT:
                if(col > 0) {
                    col++;
                }
                break;
            case KEY_RIGHT:
                if(col < max_col) {
                    col++;
                }
                break;
            case KEY_UP:
                if(row > 0) {
                    row--;
                }
                break;
            case KEY_DOWN:
                if(row < max_row) {
                    row++;
                }
                break;
        }
        listview_update_cell_background_color(lv, row, col, cyan);
        listview_render(win, lv);
        wrefresh(win);
        refresh();
    }
}

int main(int argc, char* argv[]) {
    MsgLogger logger("nview");

    logger.log_message("starting nview");

    std::string config_file = "";

    for(int i = 1; i < argc; i++) {
        if(!strcmp(argv[i], "-f")) {
            if(i + 1 > argc) {
                logger.log_message("Must specify a path to the config file after using the -f option");
                printf("Must specify a path to the config file after using the -f option\n");
                return -1;
            } else {
                config_file = argv[++i];
            }
        } else {
            std::string msg = "Invalid argument: ";
            msg += argv[i];
            logger.log_message(msg.c_str());
            printf("Invalid argument: %s\n", argv[i]);
            return -1;
        }
    }

    if(config_file == "") {
        veh = new VCM(); // use default config file
    } else {
        veh = new VCM(config_file); // use specified config file
    }

    if(FAILURE == veh->init()) {
        logger.log_message("failed to initialize VCM");
        printf("failed to initialize VCM\n");
        exit(-1);
    }

    setup_ncurses();

    while(1) {};

    if(FAILURE == tlm.init(veh)) {
        logger.log_message("failed to initialize telemetry viewer");
        printf("failed to initialize telemetry viewer\n");
        exit(-1);
    }

    // add signal handlers
    for(int i = 0; i < NUM_SIGNALS; i++) {
        signal(signals[i], sighandler);
    }

    tlm.add_all();
    tlm.set_update_mode(TelemetryViewer::BLOCKING_UPDATE);
}
