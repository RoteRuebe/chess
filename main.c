/**
 * @file main.c
 * @author Yannnick Zickler
 * @brief A C implementation of the game of chess, consisting of
 *      1. main.c,      implementing main() and some high-level logic behind the tui;
 *      2. tui.c,       implementing the text-based graphical user interface; and
 *      3. backend.c,   implementing the actual game of chess.
 * @version 1.1
 * @date 1.9.2025
 *
 * @copyright do whatever you want
 *
 */

#include <curses.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <time.h>

#include "main.h"
#include "tui.h"
#include "backend.h"

struct Game game;

enum {
    in_main_menu,
    in_game,
    in_about_page,
    in_victory_screen
} state = in_main_menu;


void log_msg(char *msg, int type) {
    if (type > verbosity)
        return;

    time_t t = time(NULL);
    struct tm tm = *localtime(&t);

    char prefix[8];

    switch(type) {
        case 2: strcpy(prefix, "verbose"); break;
        case 1: strcpy(prefix, "log    "); break;
        case 0: strcpy(prefix, "error  "); break;
    }

    FILE *fptr;

    fptr = fopen("log.txt", "a");

    // Append some text to the file
    fprintf(fptr,
        "[%s] [%02d.%02d.%d %02d:%02d:%02d]: %s\n", 
        prefix,
        tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900, tm.tm_hour, tm.tm_min, tm.tm_sec,
        msg
    );

    // Close the file
    fclose(fptr); 
}


void main_menu_func() {
    curs_set(0);
    noecho();

    header(&header_state);
    int ret = menu(&main_menu_state);

    switch (ret) {
        case new_game:
            log_msg("Starting new game", 1);
            init_game(&game, &starting_position, 128);
            main_menu_state.continue_enabled = 1;
            // fall through
        case cont:
            log_msg("Continuing new game", 2);
            state = in_game;
        break;

        case about_page:
            state = in_about_page;
        break;

        case exit_game:
            exit(0);
    }            
}

int game_func() {
    int ret = 1;
    boardscr(&boardscr_state, &(game.positions[game.halfmove]));

    char inp[16];
    for(int i = 0; i < 16; i ++) {
        inp[i] = '\0';
    }

    input(&input_state, inp);

    char msg[64];
    snprintf(msg, 64, "Interpreting input \'%s\'", inp);
    log_msg(msg, 2);
    
    if( !strcmp(inp, "menu") || !strcmp(inp, "quit") || !strcmp(inp, "exit") ) {
        clear();
        refresh();
        state = in_main_menu;
    }

    else if( !eval_algebraic(&game, inp) ) {
        ret = 0;
    }

    else {
        struct Position *last_pos = &game.positions[game.halfmove];
        update_state(last_pos);

        if (last_pos->state != white && last_pos->state != black)
            state = in_victory_screen;
    }

    // Clear input buffer
    for(int i = 0; i < 64; i++) {
        inp[i] = '\0';
    }

    return ret;
}

void victory_screen_func() {
    boardscr(&boardscr_state, &(game.positions[game.halfmove]));
    victory_win(&victory_win_state, game.positions[game.halfmove].state);
    getch();
}

void loop() {
    int ret;
    char inp[64];
    static int error = 0;

    switch(state) {
        case (in_main_menu):
            main_menu_func();
        break;

        case (in_game):
            if (error)
                error_win(&error_win_state);

            curs_set(1);
            echo();
            
            error = !game_func();
            
            curs_set(0);
            noecho();
        break;

        case (in_victory_screen):
            victory_screen_func();
            state = in_main_menu;
        break;

        case (in_about_page):
            about(&about_state);
            state = in_main_menu;
        break;
    }

    clear();
    refresh();
}

int main() {
    log_msg("Starting session", 1);

    init_curses();
    setup_wins();

    for(;;) {
        loop();
    }

    return 0;
}
