#ifndef TUI_H
#define TUI_H

#define VERSION_MAJ 1
#define VERSION_MIN 0

#include "backend.h"

/* 
* struct for all windows with no extra state
*/
struct Window {
    WINDOW *win;
};

void header(struct Window* state);

static char *main_menu_options[] = {
    "Continue",
    "New Game",
    "Load Game",
    "Save Game",
    "About",
    "Quit"
};
static int num_main_menu_options = 6;

enum main_menu_ret {
    cont,
    new_game,
    load_game,
    save_game,
    about_page,
    exit_game
};

struct Menu {
    WINDOW *win;
    int selected;
    int num_options;
    int continue_enabled;

    char **options;
};

int menu(struct Menu* state);

void boardscr(struct Window* state, struct Position* pos);

void input(struct Window* state, char* input);

void about(struct Window* state);

void error_win(struct Window* state);

void init_curses();
void setup_wins();

extern struct Menu main_menu_state;
extern struct Window header_state, boardscr_state, input_state, about_state, error_win_state;

#endif