#ifndef BACKEND_H
#define BACKEND_H

// o for empty square, capital letters = white pieces, p/P_passant for pawns that can be taken en passant
enum Piece {
    o,
    P, P_passant, N, B, R, Q, K,
    p, p_passant, n, b, r, q, k,
};

// Used for Game_state and some return values
// Missuse since used for so many different things?
enum Game_state {
    white, black,
    both, neither,
    white_win, black_win, draw
};

enum Line {
    up,
    up_right,
    right,
    down_right,
    down,
    down_left,
    left,
    up_left
};

/*
* A single chess position consisting of the board and some meta-information
*/
struct Position {
    enum Piece board[64];

    enum Game_state state;
    
    uint8_t white_can_castle_king  : 1;
    uint8_t white_can_castle_queen : 1;

    uint8_t black_can_castle_king  : 1;
    uint8_t black_can_castle_queen : 1;
};

/*
* A game of chess consisting of a series of positions.
*/
struct Game {
    struct Position position[256];

    uint8_t halfmove;
};

int is_white(enum Piece piece);
int is_black(enum Piece piece);

/*
* Check if move is legal against (already generated) arrays of all legal moves
*/
int is_legal(uint8_t* legal_from, uint8_t* legal_to, int num_legal_moves, int from, int to);

void put_piece(struct Position* pos, uint8_t square, enum Piece piece);
enum Piece get_piece_at(struct Position* pos, uint8_t square);

/*
* Generate all legal moves. They are stored pairwise in "from" and "to" arrays
* returns: number of legal moves
*/
int gen_legal_moves(struct Position* pos, uint8_t* from, uint8_t* to);

/*
* Initialize game to starting position
*/
void init_game(struct Game* game);

/*
* Play move, checking for legality
* returns: success of operation
*/
int play_move(struct Game* game, uint8_t from, uint8_t to, enum Piece promote);

/*
* Play move
*/
void unsafe_play_move(struct Game* game, uint8_t from, uint8_t to, enum Piece promote);

/*
* evaluate a move using algebraic notation (e.a. Nf3 e4 ...)
* returns: success of operation
*/
int eval_algebraic(struct Game* game, char s[8]);

#endif