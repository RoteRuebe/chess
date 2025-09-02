/**
 * @file tui.c
 * @author yannnick
 * @brief Handling the text-based graphical interface
 * @version 1.0
 * @date 1.9.2025
 */

#define _XOPEN_SOURCE_EXTENDED
#include <ncurses.h>
#include <stdlib.h>
#include <wchar.h>
#include <locale.h>

#include "tui.h"
#include "main.h"
#include "backend.h"

void quit() {
    log_msg("Exiting", 1);
    endwin();
}

struct Window {
    WINDOW *win;
} header_state, boardscr_state, input_state, about_state, error_win_state, victory_win_state;

void header(struct Window* state) {
    // Escaped backslashes make it look unaligned - its not
    mvwprintw(state->win, 0, 0, "__  _______      ________                      ______            _          ");
    mvwprintw(state->win, 1, 0, "\\ \\/ /__  /     / ____/ /_  ___  __________   / ____/___  ____ _(_)___  ___ ");
    mvwprintw(state->win, 2, 0, " \\  /  / /     / /   / __ \\/ _ \\/ ___/ ___/  / __/ / __ \\/ __ `/ / __ \\/ _ \\");
    mvwprintw(state->win, 3, 0, " / /  / /__   / /___/ / / /  __(__  |__  )  / /___/ / / / /_/ / / / / /  __/");
    mvwprintw(state->win, 4, 0, "/_/  /____/   \\____/_/ /_/\\___/____/____/  /_____/_/ /_/\\__, /_/_/ /_/\\___/");
    mvwprintw(state->win, 5, 0, "                                                       /____/               ");

    wrefresh(state->win);
}

struct Menu {
    WINDOW *win;
    int selected;
    int num_options;
    int continue_enabled;

    char **options;
} main_menu_state;

int menu(struct Menu* state) {
    log_msg("Going into main menu", 2);

    while (1) {
        for (int i = 0; i < state->num_options; i++) {
            if (i == state->selected)
                wattron(state->win, A_REVERSE);

            else if(i == cont && !state->continue_enabled)
                wattron(state->win, A_DIM);

            if(i == exit_game)
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

void boardscr(struct Window* state, struct Position* pos) {
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
            }

            mvwaddwstr(state->win, 21-y*3+2, x*6+3, to_print);
        }
    }

    wrefresh(state->win);
}


void input(struct Window* state, char* input) {
    wclear(state->win);
    box(state->win, 0, 0);
    mvwaddch(state->win, 1, 1, '>');
    wrefresh(state->win);

    mvwscanw(state->win, 1, 3, "%s", input);
}

void about(struct Window* state) {
    log_msg("Opening about page", 2);
    mvwprintw(state->win, 0, 0, "Yet another implementation of the game of chess");
    mvwprintw(state->win, 2, 0, "Written by: Yannick Zickler");
    mvwprintw(state->win, 3, 0, "Version: %d.%d", VERSION_MAJ, VERSION_MIN);

    wattron(state->win, A_ITALIC | A_DIM);
    mvwprintw(state->win, 6, 0, "Press any key to continue");
    wattroff(state->win, A_ITALIC | A_DIM);

    wrefresh(state->win);

    getch();
}

void error_win(struct Window* state) {
    wattron(state->win, COLOR_PAIR(1));
    mvwprintw(state->win, 0, 0, "Invalid Input!");
    wattroff(state->win, COLOR_PAIR(1) || A_REVERSE);

    wrefresh(state->win);
}

void victory_win(struct Window* state, enum Game_state game_state) {
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
    }

    wattron(state->win, A_DIM | A_ITALIC);
    mvwprintw(state->win, 3, 2, "Press any key to continue");
    wattroff(state->win, A_DIM | A_ITALIC);

    wrefresh(state->win);
}

void init_curses() {
    setlocale(LC_ALL, "");
    initscr(); 
    start_color();
    use_default_colors();

    // -1 -> default background color (keeps transperancy)
    init_pair(1, COLOR_RED, -1);

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

    // MAIN MENU
    {
        int height = num_main_menu_options;
        int width = 10;
        int y = 10;
        int x = (COLS - width)/2;

        main_menu_state = (struct Menu) {
            newwin(height, width, y, x),
            1, num_main_menu_options, 0, main_menu_options
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

        error_win_state = (struct Window) {
            newwin(height, width, y, x)
        };
    }
    
    // VICTORY WIN
    {
        int height = 5;
        int width = 30;
        int y = 30;
        int x = 5;

        victory_win_state = (struct Window) {
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
