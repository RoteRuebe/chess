/**
 * @file backend.c
 * @brief the meat and potatoes
 * @version 1.0
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

#define X 0
#define Y 1

static struct Position starting_position = {
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

// differences of squares a knights jump away
static int8_t knights_jump[8][2] = {
    {1, 2}, {-1, 2}, {-1, -2}, {1, -2},
    {2, 1}, {-2, 1}, {-2, -1}, {-2, 1}
};

static int8_t kings_move[8][2] = {
    {0, 1}, {1, 0}, {1, 1},
    {0, -1}, {-1, 0}, {-1, -1},
    {1, -1}, {-1, 1}
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

void _gen_legal_moves_Pawn(struct Position* pos, int square, int* index_move, uint8_t* from, uint8_t* to) {
    // One square
    if (pos->board[square+8] == o) {
        from[*index_move] = square;
        to[*index_move] = square + 8;
        *(index_move) = *(index_move) + 1;

        // Two squares
        if (
            (8 <= square && square < 16) &&
            (pos->board[square+16] == o)
        ) {
            from[*index_move] = square;
            to[*index_move] = square + 16;
            *(index_move) = *(index_move) + 1;
        }
    }
    // Diagonal or en-passant
    if (
        is_black(pos->board[square+7]) ||
        pos->board[square-1] == p_passant
    ) {
        from[*index_move] = square;
        to[*index_move] = square + 7;
        *(index_move) = *(index_move) + 1;
    }
    if (
        is_black(pos->board[square+9]) ||
        pos->board[square+1] == p_passant
    ) {
        from[*index_move] = square;
        to[*index_move] = square+9;
        *(index_move) = *(index_move) + 1;
    }
}

void _gen_legal_moves_pawn(struct Position* pos, int square, int* index_move, uint8_t* from, uint8_t* to) {
    if (pos->board[square-8] == o) {
        from[*index_move] = square;
        to[*index_move] = square - 8;
        *(index_move) = *(index_move) + 1;

        if (
            (48 <= square && square < 55) &&
            (pos->board[square-16] == o)
        ) {
            from[*index_move] = square;
            to[*index_move] = square - 16;
            *(index_move) = *(index_move) + 1;
        }
    }
    if (
        is_white(pos->board[square-7])||
        pos->board[square+1] == P_passant
    ) {
        from[*index_move] = square;
        to[*index_move] = square - 7;
        *(index_move) = *(index_move) + 1;
    }
    if (
        is_white(pos->board[square-9]) ||
        pos->board[square-1] == P_passant
    ) {
        from[*index_move] = square;
        to[*index_move] = square - 9;
        *(index_move) = *(index_move) + 1;
    }
}

void _gen_legal_moves_castling(struct Position* pos, int square, int* index_move, uint8_t* from, uint8_t* to) {
    // White
    if( pos->board[square] == K ) {
        // Kingside
        if (
            ( pos->white_can_castle_king ) &&
            ( pos->board[5] == o && pos->board[6] == o )
        ) {
            from[*index_move] = square;
            to[*index_move] = 6;
            *(index_move) = *(index_move) + 1;
        }

        // Queenside
        if (
            ( pos->white_can_castle_queen ) &&
            ( pos->board[3] == o && pos->board[2] == o && pos->board[1] == o )
        ) {
            from[*index_move] = square;
            to[*index_move] = 2;
            *(index_move) = *(index_move) + 1;
        }
    }

    // Black
    else if( pos->board[square] == k ) {
        if (
            ( pos->black_can_castle_king ) &&
            ( pos->board[61] == o && pos->board[62] == o )
        ) {
            from[*index_move] = square;
            to[*index_move] = 62;
            *(index_move) = *(index_move) + 1;
        }

        if (
            ( pos->black_can_castle_queen ) &&
            ( pos->board[59] == o && pos->board[58] == o && pos->board[57] == o )
        ) {
            from[*index_move] = square;
            to[*index_move] = 58;
            *(index_move) = *(index_move) + 1;
        }
    }
}

void _gen_legal_moves_knight_king(struct Position* pos, int square, int* index_move, uint8_t* from, uint8_t* to) {
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
            from[*index_move] = square;
            to[*index_move] = new_square;
            *(index_move) = *(index_move) + 1;
        }
    }
}

void _gen_legal_moves_other(struct Position* pos, int square, int* index_move, uint8_t* from, uint8_t* to) {
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
                from[*index_move] = square;
                to[*index_move] = new_square;
                *(index_move) = *(index_move) + 1;
            }

            // Opposite color
            else if (
                ( is_white(piece) && is_black(pos->board[new_square]) ) ||
                ( is_black(piece) && is_white(pos->board[new_square]) )
            ) {
                from[*index_move] = square;
                to[*index_move] = new_square;
                *(index_move) = *(index_move) + 1;
                break;
            }
        }
    }
}

int gen_legal_moves(struct Position* pos, uint8_t* from, uint8_t* to) {
    enum Game_state color = pos->state;

    if ( !(color == white || color == black) ) {
        printf("Error in gen_legal_moves(): Generated moves on finished game\n");
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
                _gen_legal_moves_Pawn(pos, square, &index_move, from, to);

                break;

            case p: case p_passant:
                _gen_legal_moves_pawn(pos, square, &index_move, from, to);
                break;

            case K: case k:
                _gen_legal_moves_castling(pos, square, &index_move, from, to);
                // fall-through
            case N: case n:
                _gen_legal_moves_knight_king(pos, square, &index_move, from, to);
            break;

            case B: case R: case Q: case b: case r: case q:
                _gen_legal_moves_other(pos, square, &index_move, from, to);
        }
    }
    return index_move;
}


/*** Methods of struct Game ***/

void init_game(struct Game* game) {
    game->halfmove = 0;

    memcpy(&(game->position[0].board), &starting_position, sizeof(struct Position));
}

void _force_move(struct Game* game, uint8_t from, uint8_t to) {
    struct Position *crnt_pos = &(game->position[game->halfmove]);
    struct Position *next_pos = &(game->position[game->halfmove + 1]);

    memcpy(next_pos, crnt_pos, sizeof(struct Position));
    next_pos->state = !crnt_pos->state;

    next_pos->board[to] = next_pos->board[from];
    next_pos->board[from] = 0;

    game->halfmove++;
}

void unsafe_play_move(struct Game* game, uint8_t from, uint8_t to, enum Piece promote) {
    _force_move(game, from, to);

    struct Position *new_position = &( game->position[game->halfmove] );
    enum Piece piece = new_position->board[to];
    enum Game_state color = new_position->state;

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
                new_position->white_can_castle_queen = 0;
            else if (from == 63)
                new_position->white_can_castle_king = 0;
        break;
    }

    // Unflag en passant pawns
    for(int square = 0; square < 64; square++) {
        if (color == white && new_position->board[square] == P_passant)
            put_piece(new_position, square, P);
        else if (color == black && new_position->board[square] == p_passant)
            put_piece(new_position, square, p);
    }
}

int play_move(struct Game* game, uint8_t from, uint8_t to, enum Piece promote) {
    uint8_t legal_from[128];
    uint8_t legal_to[128];

    int num_legal_moves = gen_legal_moves( &(game->position[game->halfmove]), legal_from, legal_to);

    if( is_legal(legal_from, legal_to, num_legal_moves, from, to) ) {
        unsafe_play_move(game, from, to, promote);
        return 1;
    }
    else
        return 0;
}

int eval_algebraic(struct Game* game, char s[8]) {
    struct Position *crnt_pos = &(game->position[game->halfmove]);
    enum Game_state color = crnt_pos->state;

    uint8_t legal_from[128];
    uint8_t legal_to[128];

    int num_legal_moves = gen_legal_moves( crnt_pos, legal_from, legal_to );

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
    int start, end;

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
        unsafe_play_move(game, legal_moves[0], end, promote_to);
        return 1;
    }

    else {
        for(int move=0; move<num_moves; move++) {
            if ( rank != -1 && legal_moves[move] % 8 != rank)
                continue;

            if ( file != -1 && legal_moves[move] / 8 != file )
                continue;

            unsafe_play_move(game, legal_moves[move], end, promote_to);
            return 1;
        }
    }

    return 0;
}

/*
void load_pgn(struct Game* game, FILE* file) {
    char content[1024];
    char move[8];
    fgets(content, 1024, file);

    int cursor_file = 0;
    int cursor_move = 0;
    enum Game_state whose_move = neither;
    int started = 0;

    while(content[cursor_file] != '*') {
        char x = content[cursor_file];

        if (x == '.')
            whose_move = white;

        else if (x == ' ' && whose_move == white && started == 0)
            started = 1;

        else if (x == ' ' && whose_move == white && started == 1) {
            eval_algebraic(game, move);
            for(int i = 0; i < 8; i++)
                move[i] = '\0';
            cursor_move = 0;

            whose_move = black;
        }

        else if (x == ' ' && whose_move == black) {
            eval_algebraic(game, move);
            for(int i = 0; i < 8; i++)
                move[i] = '\0';
            cursor_move = 0;

            whose_move = neither;
            started = 0;
        }

        if (started) {
            if (x != ' ') {
                move[cursor_move] = content[cursor_file];
                cursor_move++;
            }
        }

        cursor_file++;
    }

    fclose(file);
    return;
}
*/
