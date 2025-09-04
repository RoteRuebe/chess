/**
 * @file backend.c
 * @brief the meat and potatoes
 * @version 1.2
 * @date 1.9.2025
 *
 * TODO: checks, checkmate, sys-arguments(?), import FEN, export PGN;
 */

/*  internally, each square is assigned only one number:

      (a)  (b)  (c)  (d)  (e)  (f)  (g)  (h)
    +----+----+----+----+----+----+----+----+
  7 | 56 | 57 | 58 | 59 | 60 | 61 | 62 | 63 | (8)
    +----+----+----+----+----+----+----+----+
  6 | 48 | 49 | 50 | 51 | 52 | 53 | 54 | 55 | (7)
    +----+----+----+----+----+----+----+----+
  5 | 40 | 41 | 42 | 43 | 44 | 45 | 46 | 47 | (6)
    +----+----+----+----+----+----+----+----+
  4 | 32 | 33 | 34 | 35 | 36 | 37 | 38 | 39 | (5)
    +----+----+----+----+----+----+----+----+
  3 | 24 | 25 | 26 | 27 | 28 | 29 | 30 | 31 | (4)
    +----+----+----+----+----+----+----+----+
  2 | 16 | 17 | 18 | 19 | 20 | 21 | 22 | 23 | (3)
    +----+----+----+----+----+----+----+----+
  1 |  8 |  9 | 10 | 11 | 12 | 13 | 14 | 15 | (2)
    +----+----+----+----+----+----+----+----+
  0 |  0 |  1 |  2 |  3 |  4 |  5 |  6 |  7 | (1)
    +----+----+----+----+----+----+----+----+
       0    1    2    3    4    5    6    7
*/

/*** Definitions ***/

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "backend.h"
#include "main.h"

#define X 0
#define Y 1

/*** Constants ***/

const int max_pgn_content_size = 2048;

// differences of squares a knights jump away
const int8_t knights_jump[8][2] = {
    {1, 2}, {-1, 2}, {-1, -2}, {1, -2},
    {2, 1}, {-2, 1}, {-2, -1}, {2, -1}
};

const int8_t kings_move[8][2] = {
    {0, 1}, {1, 0}, {1, 1},
    {0, -1}, {-1, 0}, {-1, -1},
    {1, -1}, {-1, 1}
};

const struct Position starting_position = {
    {
        R,N,B,Q,K,B,N,R,
        P,P,P,P,P,P,P,P,
        o,o,o,o,o,o,o,o,
        o,o,o,o,o,o,o,o,
        o,o,o,o,o,o,o,o,
        o,o,o,o,o,o,o,o,
        p,p,p,p,p,p,p,p,
        r,n,b,q,k,b,n,r
    }, white, 1, 1, 1, 1
};


/*** Misc Functions ***/

int convert_algebraic(char s[2]) {
    return (s[0]-97) + (s[1]-49)*8;
}

enum Piece convert_character_piece(char x, enum Game_state color) {
    if (color == white) {
        switch(x) {
            case 'N': return N;
            case 'B': return B;
            case 'R': return R;
            case 'Q': return Q;
            case 'K': return K;
        }
    }

    else if (color == black) {
        switch(x) {
            case 'N': return n;
            case 'B': return b;
            case 'R': return r;
            case 'Q': return q;
            case 'K': return k;
        }
    }

    return o;
}

int is_white(enum Piece piece) {
    return P <= piece && piece <= K;
}

int is_black(enum Piece piece) {
    return p <= piece && piece <= k;
}

int is_legal(uint8_t* legal_from, uint8_t* legal_to, int num_legal_moves, int from, int to) {
    for(int i = 0; i < num_legal_moves; i++) {
        if (legal_from[i] == from && legal_to[i] == to)
            return 1;
    }

    return 0;
}

/*** Methods of struct Position ***/

void put_piece(struct Position* pos, uint8_t square, enum Piece piece) {
    pos->board[square] = piece;
}

enum Piece get_piece_at(struct Position* pos, uint8_t square) {
    return pos->board[square];
}

void update_state(struct Position* pos) {
    int was_in_check = in_check(pos);

    uint8_t from[128];
    uint8_t to[128];
    int num_legal_moves = gen_legal_moves(pos, 0, from, to);

    if(num_legal_moves > 0)
        return;

    log_msg("(backend) Game has concluded", log);

    if (was_in_check) {
        if (pos->state == black)
            pos->state = white_win;
        else if (pos->state == white)
            pos->state = black_win;
    }
    else
        pos->state = draw;
}

int in_check(struct Position* pos) {
    int ret = 0;
    int king_square;

    enum Game_state color = pos->state;

    for (int square = 0; square < 64; square++) {
        if(
            (color == white && pos->board[square] == K) ||
            (color == black && pos->board[square] == k)
        ) {
            king_square = square;
        }
    }

    // Check if opposite color could take king
    pos->state = !pos->state;
    uint8_t from[128];
    uint8_t to[128];
    int num_legal_moves = gen_legal_moves(pos, 1, from, to);

    for (int i = 0; i < num_legal_moves; i++) {
        if (to[i] == king_square) {
            ret = 1;
        }
    }

    pos->state = !pos->state;

    return ret;
}

void _check_for_check(struct Position* pos, uint8_t new_from, uint8_t new_to, int allow_checks, int* index_move, uint8_t* from, uint8_t* to) {
    // Try and play the move to see, if player still in check
    if (!allow_checks) { 
        struct Game try_game;
        init_game(&try_game, pos, 2);

        unsafe_play_move(&try_game, new_from, new_to, o);

        try_game.positions[1].state = !try_game.positions[1].state;
        
        if (in_check( &try_game.positions[1]) )
            return;

        delete_game(&try_game);
    }

    from[*index_move] = new_from;
    to[*index_move] = new_to;
    *(index_move) = *(index_move) + 1;
}

void _gen_legal_moves_Pawn(struct Position* pos, int square, int allow_checks, int* index_move, uint8_t* from, uint8_t* to) {
    int y = square / 8;
    int x = square % 8;

    // One square
    if (pos->board[square+8] == o) {
        _check_for_check(pos, square, square+8, allow_checks, index_move, from, to);

        // Two squares
        if ( y == 1 && pos->board[square+16] == o ) {
            _check_for_check(pos, square, square+16, allow_checks, index_move, from, to);
        }
    }
    // Diagonal or en-passant
    if ( x != 0 ) {
        if(
            is_black(pos->board[square+7]) ||
            pos->board[square-1] == p_passant
        ) {
            _check_for_check(pos, square, square+7, allow_checks, index_move, from, to);
        }
    }

    if ( x != 7 ) {
        if (
            is_black(pos->board[square+9]) ||
            pos->board[square+1] == p_passant
        ) {
            _check_for_check(pos, square, square+9, allow_checks, index_move, from, to);
        }
    }
}

void _gen_legal_moves_pawn(struct Position* pos, int square, int allow_checks, int* index_move, uint8_t* from, uint8_t* to) {
    int y = square / 8;
    int x = square % 8;

    if (pos->board[square-8] == o) {
        _check_for_check(pos, square, square-8, allow_checks, index_move, from, to);
        if ( y == 6 && pos->board[square-16] == o) {
            _check_for_check(pos, square, square-16, allow_checks, index_move, from, to);
        }
    }
    if ( x != 7 ) {
        if (
            is_white(pos->board[square-7])||
            pos->board[square+1] == P_passant
        ) {
            _check_for_check(pos, square, square-7, allow_checks, index_move, from, to);
        }
    }
    
    if ( x != 0 ) {
        if (
            is_white(pos->board[square-9]) ||
            pos->board[square-1] == P_passant
        ) {
            _check_for_check(pos, square, square-9, allow_checks, index_move, from, to);
        }
    }
}

void _gen_legal_moves_castling(struct Position* pos, int square, int allow_checks, int* index_move, uint8_t* from, uint8_t* to) {
    uint8_t legal_from[128];
    uint8_t legal_to[128];

    int kingside_controlled_by_enemy = 0;
    int queenside_controlled_by_enemy = 0;

    // Safeguard; else endless recursion
    // TODO: its now unsafe to call gen_legal_moves with allow_checks off for chess engine
    //      for efficency, because it could try to castle through check. Make seperate
    //      castling_through_check flag?
    if(!allow_checks) {
        // To check if squares are controlled by enemy
        pos->state = !pos->state;
        int num_legal_moves = gen_legal_moves(pos, 1, legal_from, legal_to);
        pos->state = !pos->state;

        for(int i = 0; i < num_legal_moves; i++) {
            int x = legal_to[i];
            if (pos->board[square] == K) {
                if (x == 4 || x == 5 || x == 6) {
                    kingside_controlled_by_enemy = 1;
                }
                else if (x == 4 || x == 3 || x == 2 || x == 1)
                    queenside_controlled_by_enemy = 1;
            }

            else if (pos->board[square] == k) {
                if (x == 60 || x == 61 || x == 62)
                    kingside_controlled_by_enemy = 1;
                else if (x == 60 || x == 59 || x == 58 || x == 57)
                    queenside_controlled_by_enemy = 1;
            }
        }
    }

    // White
    if( pos->board[square] == K ) {
        // Kingside
        if (
            ( pos->white_can_castle_king ) &&
            ( pos->board[5] == o && pos->board[6] == o ) &&
            !kingside_controlled_by_enemy
        ) {
            _check_for_check(pos, square, 6, allow_checks, index_move, from, to);
        }

        // Queenside
        if (
            ( pos->white_can_castle_queen ) &&
            ( pos->board[3] == o && pos->board[2] == o && pos->board[1] == o ) &&
            !queenside_controlled_by_enemy
        ) {
            _check_for_check(pos, square, 2, allow_checks, index_move, from, to);
        }
    }

    // Black
    else if( pos->board[square] == k ) {
        if (
            ( pos->black_can_castle_king ) &&
            ( pos->board[61] == o && pos->board[62] == o ) &&
            !kingside_controlled_by_enemy
        ) {
            _check_for_check(pos, square, 62, allow_checks, index_move, from, to);
        }

        if (
            ( pos->black_can_castle_queen ) &&
            ( pos->board[59] == o && pos->board[58] == o && pos->board[57] == o ) &&
            !queenside_controlled_by_enemy
        ) {
            _check_for_check(pos, square, 58, allow_checks, index_move, from, to);
        }
    }
}

void _gen_legal_moves_knight_king(struct Position* pos, int square, int allow_checks,  int* index_move, uint8_t* from, uint8_t* to) {
    int x = square % 8;
    int y = square / 8;

    enum Piece piece = pos->board[square];

    for(int i = 0; i < 8; i++) {
        int dx;
        int dy;

        if (piece == N || piece == n) {
            dx = knights_jump[i][X];
            dy = knights_jump[i][Y];
        }
        else if (piece == K || piece == k) {
            dx = kings_move[i][X];
            dy = kings_move[i][Y];
        }

        int new_square = square + dx + 8*dy;

        // Out of bounds
        if ( !(0 <= x+dx && x+dx < 8 && 0 <= y+dy && y+dy < 8) )
            continue;

        // Same color
        else if (
            ( is_white(piece) && is_white(pos->board[new_square]) ) ||
            ( is_black(piece) && is_black(pos->board[new_square]) )
        ) {
            continue;
        }

        else {
            _check_for_check(pos, square, new_square, allow_checks, index_move, from, to);
        }
    }
}

void _gen_legal_moves_other(struct Position* pos, int square, int allow_checks, int* index_move, uint8_t* from, uint8_t* to) {
    int x = square % 8;
    int y = square / 8;

    enum Piece piece = pos->board[square];

    for(int line = 0; line < 8; line ++) {
        // Bishops only on diagonals rooks only on ranks
        if ( (piece == B) && (line % 2 == 0) )      continue;
        else if ( (piece == b) && (line % 2 == 0) ) continue;

        else if ( (piece == R) && (line % 2 == 1) ) continue;
        else if ( (piece == r) && (line % 2 == 1) ) continue;

        int dx = 0;
        int dy = 0;

        while (1) {
            if (line == up_right || line == right || line == down_right)
                dx++;
            else if (line == down_left || line == left || line == up_left)
                dx--;

            if (line == up_left || line == up || line == up_right)
                dy++;
            else if (line == down_right || line == down || line == down_left)
                dy--;

            int new_square = square + dx + 8*dy;

            // Out of bounds
            if ( !(0 <= x+dx && x+dx < 8 && 0 <= y+dy && y+dy < 8) )
                break;

            // Same color
            else if (
                ( is_white(piece) && is_white(pos->board[new_square]) ) ||
                ( is_black(piece) && is_black(pos->board[new_square]) )
            ) {
                break;
            }

            // Empty
            else if (pos->board[new_square] == o) {
                _check_for_check(pos, square, new_square, allow_checks, index_move, from, to);
            }

            // Opposite color
            else if (
                ( is_white(piece) && is_black(pos->board[new_square]) ) ||
                ( is_black(piece) && is_white(pos->board[new_square]) )
            ) {
                _check_for_check(pos, square, new_square, allow_checks, index_move, from, to);
                break;
            }
        }
    }
}

int gen_legal_moves(struct Position* pos, int allow_checks, uint8_t* from, uint8_t* to) {
    enum Game_state color = pos->state;

    if ( !(color == white || color == black) ) {
        log_msg("Error in gen_legal_moves(): Generated moves on finished game", error);
        exit(1);
    }

    int index_move = 0;

    for (int square = 0; square < 64; square++) {
        enum Piece piece = pos->board[square];

        if (piece == o)                             continue;
        else if (color == white && is_black(piece)) continue;
        else if (color == black && is_white(piece)) continue;

        switch (piece) {
            case P: case P_passant:
                _gen_legal_moves_Pawn(pos, square, allow_checks, &index_move, from, to);

                break;

            case p: case p_passant:
                _gen_legal_moves_pawn(pos, square, allow_checks, &index_move, from, to);
                break;

            case K: case k:
                _gen_legal_moves_castling(pos, square, allow_checks, &index_move, from, to);
                // fall-through
            case N: case n:
                _gen_legal_moves_knight_king(pos, square, allow_checks, &index_move, from, to);
            break;

            case B: case R: case Q: case b: case r: case q:
                _gen_legal_moves_other(pos, square, allow_checks, &index_move, from, to);
        }
    }
    return index_move;
}


/*** Methods of struct Game ***/

void init_game(struct Game* game, const struct Position* pos, int max_moves) {
    game->halfmove = 0;
    game->max_moves = max_moves;

    game->positions = malloc( sizeof(struct Position) * max_moves );

    memcpy(&(game->positions[0].board), pos, sizeof(struct Position));
}

void delete_game(struct Game* game) {
    free(game->positions);
}

void _force_move(struct Position* crnt_position, struct Position* new_position, uint8_t from, uint8_t to) {
    memcpy(new_position, crnt_position, sizeof(struct Position));
    new_position->state = !crnt_position->state;

    new_position->board[to] = new_position->board[from];
    new_position->board[from] = 0;
}

void unsafe_play_move_to(struct Position* crnt_position, struct Position* new_position, uint8_t from, uint8_t to, enum Piece promote) {
    _force_move(crnt_position, new_position, from, to);

    enum Piece piece = new_position->board[to];
    enum Game_state color = new_position->state;

    // if enemy has eaten a rook, you cannot castle that way
    if (color == white) {
        if (to == 0)
            new_position->white_can_castle_queen = 0;
        else if (to == 7)
            new_position->white_can_castle_king = 0;
    }
    else if (color == black) {
        if (to == 56)
            new_position->black_can_castle_queen = 0;
        else if (to == 63)
            new_position->black_can_castle_king = 0;
    }

    switch (piece) {
        case P:
            // flag en passant pawn if moved two squares
            if (
                ( 8 <= from && from < 16 ) &&
                ( 24 <= to && to < 32 )
            ) {
                put_piece(new_position, to, P_passant);
            }

            // Promote
            else if (56 <= to && to < 64) {
                put_piece(new_position, to, promote);
            }

            // remove enemy pawn taken en passant
            if (new_position->board[to-8] == p_passant)
                put_piece(new_position, to-8, o);
        break;

        case p:
            if (
                ( 48 <= from && from < 56 ) &&
                ( 32 <= to && to < 40 )
            ) {
                put_piece(new_position, to, p_passant);
            }

            else if (0 <= to && to < 8)
                put_piece(new_position, to, promote);

            if (new_position->board[to+8] == P_passant)
                put_piece(new_position, to+8, o);

        break;

        case K:
            // Castle
            if(from == 4 && to == 6) {
                put_piece(new_position, 7, o);
                put_piece(new_position, 5, R);
            }
            else if (from == 4 && to == 2) {
                put_piece(new_position, 0, o);
                put_piece(new_position, 3, R);
            }
            new_position->white_can_castle_king = 0;
            new_position->white_can_castle_queen = 0;
        break;

        case k:
            if(from == 60 && to == 62) {
                put_piece(new_position, 63, o);
                put_piece(new_position, 61, r);
            }
            else if (from == 60 && to == 58) {
                put_piece(new_position, 56, o);
                put_piece(new_position, 59, r);
            }
            new_position->black_can_castle_king = 0;
            new_position->black_can_castle_queen = 0;
        break;

        case R:
            // No castling that side if rook moved
            if (from == 0)
                new_position->white_can_castle_queen = 0;
            else if (from == 7)
                new_position->white_can_castle_king = 0;
        break;

        case r:
            if (from == 56)
                new_position->black_can_castle_queen = 0;
            else if (from == 63)
                new_position->black_can_castle_king = 0;
        break;

        default: break;
    }

    // Unflag en passant pawns
    for(int square = 0; square < 64; square++) {
        if (color == white && new_position->board[square] == P_passant)
            put_piece(new_position, square, P);
        else if (color == black && new_position->board[square] == p_passant)
            put_piece(new_position, square, p);
    }
}

void unsafe_play_move(struct Game* game, uint8_t from, uint8_t to, enum Piece promote) {
    unsafe_play_move_to( &game->positions[game->halfmove], &game->positions[game->halfmove+1], from, to, promote );

    game->halfmove++;
}

int play_move(struct Game* game, uint8_t from, uint8_t to, enum Piece promote) {
    char msg[64];
    snprintf(msg, 64, "(backend) Playing move %d to %d", from, to);
    log_msg(msg, verbose);

    uint8_t legal_from[128];
    uint8_t legal_to[128];

    int num_legal_moves = gen_legal_moves( &(game->positions[game->halfmove]), 0, legal_from, legal_to);

    if( is_legal(legal_from, legal_to, num_legal_moves, from, to) ) {
        unsafe_play_move(game, from, to, promote);
        return 1;
    }
    else
        return 0;
}

// TODO: when two moves are possible but which one to do is not further specified, this
// function will simply use the first possible it can find
int eval_algebraic(struct Game* game, char s[8]) {
    char msg[64];
    snprintf(msg, 64, "(backend) Interpreting move \'%s\'", s);
    log_msg(msg, verbose);

    struct Position *crnt_pos = &(game->positions[game->halfmove]);
    enum Game_state color = crnt_pos->state;

    uint8_t legal_from[128];
    uint8_t legal_to[128];

    int num_legal_moves = gen_legal_moves( crnt_pos, 0, legal_from, legal_to );

    // quick and dirty
    if( !strcmp(s, "O-O") || !strcmp(s, "0-0")) {
        if (color == white)
            s = "Kg1\0\0\0\0\0";
        else if (color == black)
            s = "Kg8\0\0\0\0\0";
    }
    if( !strcmp(s, "O-O-O") || !strcmp(s, "0-0-0")) {
        if (color == white)
            s = "Kc1\0\0\0\0\0";
        else if (color == black)
            s = "Kc8\0\0\0\0\0";
    }

    enum Piece piece, promote_to = o;
    int end;

    // First character is piece (if lowercase, assume pawn)
    if(isupper(s[0])) {
        piece = convert_character_piece(s[0], color);
    }
    else if (color == white)
        piece = P;
    else if (color == black)
        piece = p;

    // Find last two letters -> end square or promotion if uppercase
    int index_end = 0;
    for (index_end=8; index_end>=0; index_end--) {
        if( isupper(s[index_end]))
            promote_to = s[index_end];

        if ( islower(s[index_end]) ) {
            end = convert_algebraic(s+index_end);
            break;
        }
    }

    // Everything before end specifies start square, Piece uppercase therefore ignored
    int rank = -1;
    int file = -1;

    for(int i = 0; i < index_end; i++) {
        if (islower(s[i]) && s[i] != 'x')
            rank = s[i];
        if (isdigit(s[i]))
            file = s[i];
    }

    // Convert ascii -> int
    if (rank != -1)
        rank = rank-97;
    if (file != -1)
        file = file-49;

    // Store starting squares of all legal moves
    int num_moves = 0;
    int legal_moves[3];
    for (int i = 0; i<64; i++) {
        enum Piece piece_at_square = crnt_pos->board[i];
        if(
            (piece_at_square == piece) ||
            (piece_at_square == P_passant && piece == P) ||
            (piece_at_square == p_passant && piece == p)
        ) {
            if( is_legal(legal_from, legal_to, num_legal_moves, i, end) ) {
                legal_moves[num_moves] = i;
                num_moves++;
            }
        }
    }

    promote_to = convert_character_piece(promote_to, color);

    // If legal moves > 1, check specifiers
    if (num_moves == 1) {
        char msg[64];
        snprintf(msg, 64, "(backend) Found suitor %d to %d", legal_moves[0], end);
        log_msg(msg, verbose);

        unsafe_play_move(game, legal_moves[0], end, promote_to);
        return 1;
    }

    else {
        for(int move=0; move<num_moves; move++) {
            if ( rank != -1 && legal_moves[move] % 8 != rank)
                continue;

            if ( file != -1 && legal_moves[move] / 8 != file )
                continue;

            snprintf(msg, 64, "(backend) Found suitor %d to %d", legal_moves[move], end);
            log_msg(msg, verbose);

            unsafe_play_move(game, legal_moves[move], end, promote_to);
            return 1;
        }
    }

    return 0;
}

void load_pgn(struct Game* game, FILE* file) {
    if(file == NULL) {
        log_msg("Error in load_pgn(): file does not exist", error);
        exit(1);
    }
    
    log_msg("(backend) Loading pgn file", log);

    char content[max_pgn_content_size];
    char move[8] = {0};
    fgets(content, max_pgn_content_size, file);
    
    int cursor_file = 0;
    int cursor_move = 0;

    enum {
        start,
        information,
        comment,
        before_white,
        whites_move,
        before_black, 
        blacks_move,
        submit_black,
        new_line,
        result,
        abort
    } state = start;

    for(; cursor_file < max_pgn_content_size; cursor_file++) {
        
        char x = content[cursor_file];

        switch (state) {
            case start:
                if (x == '[')       state = information;
                else if (x == '{')  state = comment;
                else if (x == '.')  state = before_white;
                else if (x == '\n') state = new_line;                    
                else if (x == '-')  state = result;
                else if (x == '\0') state = abort;
                else if (x == '*')  state = abort;
                break;

            case information:
                if (x == ']')
                    state = start;
                break;

            case comment:
                if (x == '}')
                    state = start;
                break;

            case before_white:
                if (x == ' ')
                    state = whites_move;

                break;
                    
            case whites_move:
                if (x != ' ') {
                    move[cursor_move] = content[cursor_file];
                    cursor_move ++;
                }

                else {
                    state = before_black;
                }
                break;

            // Submit whites move
            case before_black:
                if ( !eval_algebraic(game, move) ) {
                    log_msg("Error in load_pgn(): Illegal move", error);
                    exit(0);
                }
                for (int i = 0; i < 8; i++) {
                    move[i] = '\0';
                }
                move[0] = x;
                state = blacks_move;
                cursor_move = 1;
                break;

            case blacks_move:
                if (x != ' ') {
                    move[cursor_move] = content[cursor_file];
                    cursor_move ++;
                }

                else {
                    state = submit_black;
                }
                break;

            case submit_black:
                if ( !eval_algebraic(game, move) ) {
                    log_msg("Error in load_pgn(): Illegal move", error);
                    exit(0);
                }
                for (int i = 0; i < 8; i++) {
                    move[i] = '\0';
                }
                cursor_move = 0;

                state = start;
                cursor_file --; // enough time to read, for comment -> ugly asf
                break;

            case new_line:
                fgets(content, max_pgn_content_size, file);
                cursor_file = -1; // incremented at start of loop
                state = start;
                break;

            case result:
            case abort:
                fclose(file);
                return;
        }
    }

    return;
}
