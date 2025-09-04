/**
 * @file tui.c
 * @author yannnick
 * @brief Handling the text-based graphical interface
 * @version 1.2
 * @date 1.9.2025
 */

#define _XOPEN_SOURCE_EXTENDED

#include <ncurses.h>
#include <stdlib.h>
#include <wchar.h>
#include <locale.h>
#include <string.h>

#include "tui.h"
#include "main.h"
#include "backend.h"

#include "engine.h"

struct Game game;

/*** Definitions ***/

enum State {
    in_main_menu,
    in_sub_menu,
    in_game_human,
    in_game_engine,
    in_game_human_with_error,
    in_game_engine_with_error,
    in_about_page,
    in_victory_screen
};

struct Window {
    WINDOW *win;
} header_state, boardscr_state, input_state, about_state, error_state, victory_state;

struct Menu {
    WINDOW *win;
    int selected;
    int num_options;
    int continue_enabled;

    enum State prev_state;

    char **options;
} main_menu_state, sub_menu_state;    

char *main_menu_options[] = {
    "Continue",
    "New Game",
    "Load Game",
    "Save Game",
    "About",
    "Quit"
};
const int num_main_menu_options = 6;

char *sub_menu_options[] = {
    "Against Human",
    "Against Engine"
};
const int num_sub_menu_options = 2;

enum Main_menu_return {
    option_continue,
    option_new_game,
    option_load_game,
    option_save_game,
    option_about,
    option_quit
};

enum Sub_menu_return {
    option_against_human,
    option_against_engine
};

/*** Initializing functions ***/

void quit() {
    log_msg("(tui) Exiting", log);
    endwin();
}

void init_curses() {
    setlocale(LC_ALL, "");
    initscr(); 
    start_color();
    use_default_colors();

    // -1 -> default background color (keeps transperancy)
    init_pair(1, COLOR_RED, -1);
    init_pair(2, COLOR_WHITE, COLOR_BLACK);

    atexit(quit);
    cbreak(); 
    noecho();
    curs_set(0);
}

void setup_wins() {
    // HEADER
    {
        int height = 6;
        int width = 80;

        // keep centered
        int y = 2;
        int x = (COLS - width)/2;

        header_state = (struct Window) {
            newwin(height, width, y, x)
        };
    }

    // MENUS
    {
        int height = num_main_menu_options;
        int width = 15;
        int y = 10;
        int x = (COLS - width)/2;
        WINDOW *win = newwin(height, width, y, x);

        main_menu_state = (struct Menu) {
            win,
            1, num_main_menu_options, 0, in_game_human, main_menu_options
        };

        sub_menu_state = (struct Menu) {
            win,
            0, num_sub_menu_options, 1, in_game_human, sub_menu_options
        };
    }
    keypad(main_menu_state.win, TRUE);

    // BOARD SCREEN
    {
        int height = 26;
        int width = 52;
        int y = 2;
        int x = 5;

        boardscr_state = (struct Window) {
            newwin(height, width, y, x)
        };
    }

    // INPUT FIELD
    {
        int height = 3;
        int width = 20;
        int y = 30;
        int x = 5;

        input_state = (struct Window) {
            newwin(height, width, y, x)
        };
    }

    // ERROR_WIN
    {
        int height = 3;
        int width = 20;
        int y = 31;
        int x = 30;

        error_state = (struct Window) {
            newwin(height, width, y, x)
        };
    }
    
    // VICTORY WIN
    {
        int height = 5;
        int width = 30;
        int y = 30;
        int x = 5;

        victory_state = (struct Window) {
            newwin(height, width, y, x)
        };
    }
 
    // ABOUT PAGE
    {
        int height = 20;
        int width = COLS-10;
        int y = 2;
        int x = 5;

        about_state = (struct Window) {
            newwin(height, width, y, x)
        };
    }
}

/*** Functions drawing the windows ***/

void draw_header(struct Window* state) {
    // Escaped backslashes make it look unaligned - its not
    mvwprintw(state->win, 0, 0, "__  _______      ________                      ______            _          ");
    mvwprintw(state->win, 1, 0, "\\ \\/ /__  /     / ____/ /_  ___  __________   / ____/___  ____ _(_)___  ___ ");
    mvwprintw(state->win, 2, 0, " \\  /  / /     / /   / __ \\/ _ \\/ ___/ ___/  / __/ / __ \\/ __ `/ / __ \\/ _ \\");
    mvwprintw(state->win, 3, 0, " / /  / /__   / /___/ / / /  __(__  |__  )  / /___/ / / / /_/ / / / / /  __/");
    mvwprintw(state->win, 4, 0, "/_/  /____/   \\____/_/ /_/\\___/____/____/  /_____/_/ /_/\\__, /_/_/ /_/\\___/");
    mvwprintw(state->win, 5, 0, "                                                       /____/               ");

    wrefresh(state->win);
}

int draw_menu(struct Menu* state) {
    while (1) {
        for (int i = 0; i < state->num_options; i++) {
            if (i == state->selected)
                wattron(state->win, A_REVERSE);

            else if(i == option_continue && !state->continue_enabled)
                wattron(state->win, A_DIM);

            if(i == option_quit)
                wattron(state->win, COLOR_PAIR(1));
            
            mvwprintw(state->win, i, 0, state->options[i]);
            wattroff(state->win, A_REVERSE | A_DIM | COLOR_PAIR(1));
        }
        wrefresh(state->win);


        int ch;

        ch = wgetch(state->win);
        
        switch (ch) {
            case KEY_UP:
                if (state->selected > 0)
                    state->selected--;
            break;
            case KEY_DOWN:
                if (state->selected < state->num_options - 1)
                    state->selected++;
            break;
            case '\n':
                return state->selected;
        }

        if (!state->continue_enabled && state->selected == 0)
            state->selected = 1;
    }
}

void draw_board(struct Window* state, struct Position* pos) {
    for(int i = 0; i < 9; i++)
        mvwvline(state->win, 1, 6*i, ACS_VLINE, 23);

    for(int i = 0; i < 9; i++)
        mvwhline(state->win, 3*i, 1, ACS_HLINE, 47);

    mvwprintw(state->win, 25, 0, "   a     b     c     d     e     f     g     h");

    for(int i = 1; i <= 8; i++) {
        mvwaddch(state->win, 25-i*3, 50, i+48);
    }

    for(int y = 0; y < 8; y++) {
        for(int x = 0; x < 8; x++) {

            if ( (x+y) % 2 == 0 ) {
                wattron(state->win, COLOR_PAIR(2));
                char string[] = "     ";
                mvwaddstr(state->win, 20-y*3+2, x*6+1, string);
                mvwaddstr(state->win, 21-y*3+2, x*6+1, string);
            }

            enum Piece piece = pos->board[x+8*y];
            wchar_t to_print[2] = {L' ', '\0'};
            
            
            switch (piece) {
                case p: case p_passant: 
                        to_print[0] = L'\u2659'; break;
                case n: to_print[0] = L'\u2658'; break;
                case b: to_print[0] = L'\u2657'; break;
                case r: to_print[0] = L'\u2656'; break;
                case q: to_print[0] = L'\u2655'; break;
                case k: to_print[0] = L'\u2654'; break;

                case P: case P_passant: 
                        to_print[0] = L'\u265F'; break;
                case N: to_print[0] = L'\u265E'; break;
                case B: to_print[0] = L'\u265D'; break;
                case R: to_print[0] = L'\u265C'; break;
                case Q: to_print[0] = L'\u265B'; break;
                case K: to_print[0] = L'\u265A'; break;
                case o: break;
            }

            mvwaddwstr(state->win, 21-y*3+2, x*6+3, to_print);

            wattroff(state->win, COLOR_PAIR(2));
        }
    }

    wrefresh(state->win);

}

void draw_input(struct Window* state, char* input) {
    wclear(state->win);
    box(state->win, 0, 0);
    mvwaddch(state->win, 1, 1, '>');
    wrefresh(state->win);

    mvwscanw(state->win, 1, 3, "%s", input);
}

void draw_about(struct Window* state) {
    log_msg("(tui) Opening about page", verbose);
    mvwprintw(state->win, 0, 0, "Yet another implementation of the game of chess");
    mvwprintw(state->win, 2, 0, "Written by: Yannick Zickler");
    mvwprintw(state->win, 3, 0, "Version: %d.%d", VERSION_MAJ, VERSION_MIN);

    wattron(state->win, A_ITALIC | A_DIM);
    mvwprintw(state->win, 6, 0, "Press any key to continue");
    wattroff(state->win, A_ITALIC | A_DIM);

    wrefresh(state->win);

    getch();
}

void draw_error(struct Window* state) {
    wattron(state->win, COLOR_PAIR(1));
    mvwprintw(state->win, 0, 0, "Invalid Input!");
    wattroff(state->win, COLOR_PAIR(1) || A_REVERSE);

    wrefresh(state->win);
}

void draw_victory(struct Window* state, enum Game_state game_state) {
    box(state->win, 0, 0);

    switch (game_state) {
        case white_win:
            mvwprintw(state->win, 1, 2, "White has won the game!");
        break;
        case black_win:
            mvwprintw(state->win, 1, 2, "Black has won the game!");
        break;
        case draw:
            mvwprintw(state->win, 1, 2, "It's a draw!");
        break;

        default:
            log_msg("Error in draw_victory(): Game has not yet finished", error);
            exit(1);
    }

    wattron(state->win, A_DIM | A_ITALIC);
    mvwprintw(state->win, 3, 2, "Press any key to continue");
    wattroff(state->win, A_DIM | A_ITALIC);

    wrefresh(state->win);
}

/*** Implementing tui logic ***/

void invoke_chess_engine() {
    root.node_content.position = game.positions[game.halfmove];
    game.halfmove++;
    
    game.positions[game.halfmove] = *choose_move(&root);
}

enum State handle_main_menu() {
    curs_set(0);
    noecho();

    draw_header(&header_state);
    enum Main_menu_return ret = (enum Main_menu_return)draw_menu(&main_menu_state);

    switch (ret) {
        case option_new_game:
            return in_sub_menu;
        break;

        case option_continue:
            log_msg("(tui) Continuing game", verbose);
            return main_menu_state.prev_state;
        break;

        case option_load_game:
            return in_main_menu;
        break;

        case option_save_game:
            return in_main_menu;
        break;

        case option_about:
            return in_about_page;
        break;

        case option_quit:
            exit(0);
    }            
}

enum State handle_sub_menu() {
    log_msg("START!", verbose);
    enum Sub_menu_return ret = (enum Sub_menu_return) draw_menu(&sub_menu_state);

    switch (ret) {
        case option_against_human:
            return in_game_human;
        case option_against_engine:
            return in_game_engine;
    }

    log_msg("DONE!", verbose);
}

enum State handle_game(enum State state) {
    endwin();

    int caused_error = 0;

    if (state == in_game_engine_with_error || state == in_game_human_with_error)
        draw_error(&error_state);

    draw_board(&boardscr_state, &(game.positions[game.halfmove]));

    char inp[16];
    for(int i = 0; i < 16; i ++) {
        inp[i] = '\0';
    }

    draw_input(&input_state, inp);

    char msg[64];
    snprintf(msg, 64, "(tui) Interpreting input \'%s\'", inp);
    log_msg(msg, verbose);
    
    if( !strcmp(inp, "menu") || !strcmp(inp, "quit") || !strcmp(inp, "exit") ) {
        clear();
        refresh();
        main_menu_state.prev_state = state;
        main_menu_state.continue_enabled = 1;
        return in_main_menu;
    }

    else if( !eval_algebraic(&game, inp) ) {
        caused_error = 1;
    }

    else if (state == in_game_engine || state == in_game_engine_with_error) {
        struct Position *last_pos = &game.positions[game.halfmove];
        update_state(last_pos);

        if (last_pos->state != white && last_pos->state != black) {
            main_menu_state.continue_enabled = 0;
            return in_victory_screen;
        }

        draw_board(&boardscr_state, &(game.positions[game.halfmove]));

        curs_set(0);
        invoke_chess_engine();
        curs_set(1);
    }

    // Clear input buffer
    for(int i = 0; i < 64; i++) {
        inp[i] = '\0';
    }

    if (caused_error) {
        if (state == in_game_engine || state == in_game_engine_with_error)
            return in_game_engine_with_error;

        else if (state == in_game_human || state == in_game_human_with_error)
            return in_game_human_with_error;
    }
    else {
        if (state == in_game_engine || state == in_game_engine_with_error)
            return in_game_engine;

        else if (state == in_game_human || state == in_game_human_with_error)
            return in_game_human;
    }
}

void handle_victory() {
    draw_board(&boardscr_state, &(game.positions[game.halfmove]));
    draw_victory(&victory_state, game.positions[game.halfmove].state);
    getch();
}

void loop() {
    static enum State state = in_main_menu;

    switch(state) {
        case (in_main_menu):
            state = handle_main_menu();
        break;

        case (in_sub_menu):
            state = handle_sub_menu();

            // For sure we want to start a new game
            init_game(&game, &starting_position, 512);
        break;

        case (in_game_engine):
        case (in_game_engine_with_error):
        case (in_game_human):
        case (in_game_human_with_error):
            curs_set(1);
            echo();

            state = handle_game(state);
            
            curs_set(0);
            noecho();
        break;

        case (in_victory_screen):
            handle_victory();
            state = in_main_menu;
        break;

        case (in_about_page):
            draw_about(&about_state);
            state = in_main_menu;
        break;
    }

    clear();
    refresh();
}

void tui_loop() {
    for(;;) {
        loop();
    }
}

void init_tui() {
    init_curses();
    setup_wins();

    init_game(&game, &starting_position, 512);
}