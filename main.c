/**
 * @file main.c
 * @author Yannnick Zickler
 * @brief A C implementation of the game of chess, consisting of
 *      1. main.c,      implementing main() and logging;
 *      2. tui.c,       implementing the text-based graphical user interface; and
 *      3. backend.c,   implementing the actual game of chess.
 * @version 1.1
 * @date 1.9.2025
 *
 * @copyright do whatever you want
 *
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "main.h"
#include "tui.h"
#include "backend.h"


const enum Verbosity verbosity = verbose;

void init_log() {
    remove("log.txt");
}

void log_msg(char *msg, enum Verbosity type) {
    if (type < verbosity)
        return;

    time_t t = time(NULL);
    struct tm tm = *localtime(&t);

    char prefix[8];

    switch(type) {
        case verbose:   strcpy(prefix, "verbose"); break;
        case log:       strcpy(prefix, "  log  "); break;
        case error:     strcpy(prefix, " error "); break;
    }

    FILE *f = fopen("log.txt", "a");

    // Append some text to the file
    fprintf(f,
        "[%s] [%02d.%02d.%d %02d:%02d:%02d]: %s\n", 
        prefix,
        tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900, tm.tm_hour, tm.tm_min, tm.tm_sec,
        msg
    );

    fclose(f); 
}

int main() {
    init_log();
    log_msg("Starting session", 1);

    tui_main();

    return 0;
}
