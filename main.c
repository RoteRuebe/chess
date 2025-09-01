/**
 * @file main.c
 * @author Yannnick Zickler
 * @brief A C implementation of the game of chess, consisting of
 *      1. main.c,      implementing main() and some high-level logic behind the tui;
 *      2. tui.c,       implementing the text-based graphical user interface; and
 *      3. backend.c,   implementing the actual game of chess.
 * @version 1.0
 * @date 1.9.2025
 *
 * @copyright do whatever you want
 *
 */

#include <curses.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>

#include "backend.h"
#include "tui.h"

struct Game game;

enum {
    in_main_menu,
    in_game,
    in_about_page
} state = in_main_menu;


void main_menu_func() {
    curs_set(0);
    noecho();

    header(&header_state);
    int ret = menu(&main_menu_state);

    switch (ret) {
        case new_game:
            init_game(&game);
            main_menu_state.continue_enabled = 1;
            // fall through
        case cont:
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
    boardscr(&boardscr_state, &(game.position[game.halfmove]));
            
    curs_set(1);
    echo();

    char inp[64];
    input(&input_state, inp);
    
    if( !strcmp(inp, "menu") ) {
        clear();
        refresh();
        state = in_main_menu;
    }
    else if ( !strcmp(inp, "quit") || !strcmp(inp, "exit") ) {
        exit(0);
    }
    else if( !eval_algebraic(&game, inp) ) {
            ret = 0;
    }

    // Clear input buffer
    for(int i = 0; i < 64; i++) {
        inp[i] = '\0';
    }

    return ret;
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

            error = !game_func();
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
    init_game(&game);

    init_curses();
    setup_wins();

    for(;;) {
        loop();
    }

    return 0;
}
