/**
 * @file main.c
 * @author Yannnick Zickler
 * @brief A computer programm trying to find good chess moves
 * @version 0.0
 * @date 1.9.2025
 *
 */


#include <stdio.h>
#include <stdlib.h>

// TODO: delete
#include <ncurses.h>

#include "engine.h"
#include "main.h"
#include "backend.h"

/*** Constants ***/

struct Node root;

const int material_worth[15] = {
    0,
    1000, 1000, 3000, 3000, 5000, 9000, 1000000,
    -1000, -1000, -3000, -3000, -5000, -9000, -1000000
};

// Controlling squares more squares = better position. Central squares > on the side
const int positional_worth[64] = {
    121, 121, 121, 121, 121, 121, 121, 121,
    121, 123, 123, 123, 123, 123, 123, 121,
    121, 123, 127, 127, 127, 127, 123, 121,
    121, 123, 127, 129, 129, 127, 123, 121,
    121, 123, 127, 129, 129, 127, 123, 121,
    121, 123, 127, 127, 127, 127, 123, 121,
    121, 123, 123, 123, 123, 123, 123, 121,
    121, 121, 121, 121, 121, 121, 121, 121,
};

int static_eval(struct Position *pos) {
    enum Game_state crnt_state = pos->state;
    int evaluation = 0;

    uint8_t white_from[128]; uint8_t white_to[128];
    uint8_t black_from[128]; uint8_t black_to[128];

    pos->state = white;
    int white_num_legal_moves = gen_legal_moves(pos, 0, white_from, white_to);
    
    pos->state = black;
    int black_num_legal_moves = gen_legal_moves(pos, 0, black_from, black_to);

    // Sum up material
    for(int square = 0; square < 64; square++) {
        evaluation += material_worth[ (int)pos->board[square] ];
    }

    // Sum up positional advantage of controlled squares
    for(int move = 0; move < white_num_legal_moves; move ++) {
        evaluation += positional_worth[ white_to[move] ];
    }

    for(int move = 0; move < black_num_legal_moves; move ++) {
        evaluation -= positional_worth[ black_to[move] ];
    }

    // Sump up pos advantage of squares pieces stand on
    for(int square = 0; square < 64; square ++) {
        if ( is_white(pos->board[square]) )
            evaluation += positional_worth[ square ];
    }

    for(int square = 0; square < black_num_legal_moves; square ++) {
        if ( is_black(pos->board[square]) )
            evaluation -= positional_worth[ square ];
    }

    pos->state = crnt_state;

    return evaluation;
}

void printf_pos(struct Position* pos) {
    for(int y = 7; y >= 0; y--) {
        for(int x = 0; x < 8; x++) {
            enum Piece piece = pos->board[x+8*y];
            
            switch (piece) {
                case p: case p_passant: 
                        printf("p"); break;
                case n: printf("n"); break;
                case b: printf("b"); break;
                case r: printf("r"); break;
                case q: printf("q"); break;
                case k: printf("k"); break;

                case P: case P_passant: 
                        printf("P"); break;
                case N: printf("N"); break;
                case B: printf("B"); break;
                case R: printf("R"); break;
                case Q: printf("Q"); break;
                case K: printf("K"); break;
                case o: printf(".");break;
            }
        }

        printf("\n");
    }
}

void printf_node(struct Node* node) {
    printf("parent: %p\namount children: %d\nevaluation: %d\n", node->parent, node->amount_children, node->node_content.eval);
    printf("Position:\n\n");

    printf_pos(&node->node_content.position);
    printf("\n_________________________\n\n");
}

/*
* create children of single node containing all possible positions
*/
void create_children(struct Node* parent) {
    uint8_t from[128];
    uint8_t to[128];

    int amount_children = gen_legal_moves( &parent->node_content.position, 0, from, to );

    struct Node* children = malloc( sizeof(struct Node) * amount_children );

    for(int i = 0; i < amount_children; i++) {
        struct Node *child = &children[i];

        child->parent = parent;
        child->children = NULL;
        child->amount_children = 0;

        // TODO: handle promotions
        unsafe_play_move_to(&parent->node_content.position, &child->node_content.position, from[i], to[i], o);
        child->node_content.eval = static_eval(&child->node_content.position);

        child->node_content.did_update_eval = 0;
    }

    parent->amount_children = amount_children;
    parent->children = children;
}

const int max_depth_search_tree = 2;

/*
* Recursively generate all possible positions up to a certain depth
*/
void recursive_generate(struct Node* node, int depth) {
    if (depth > max_depth_search_tree)
        return;

    create_children(node);
    for (int i = 0; i < node->amount_children; i++) {
        recursive_generate( &node->children[i], depth+1 );
    }
}

/*
* Update evaluation of position based on the eval of the best possible next move
*/
void update_eval(struct Node* node) {
    int eval_best_move = node->children[0].node_content.eval;
    int index_best_move = 0;

    enum Game_state color = node->node_content.position.state;

    for(int i = 0; i < node->amount_children; i++) {
        struct Node* child = &node->children[i];

        if (color == white && child->node_content.eval > eval_best_move) {
            eval_best_move = child->node_content.eval;
            index_best_move = i;
        }

        else if (color == black && child->node_content.eval < eval_best_move) {
            eval_best_move = child->node_content.eval;
            index_best_move = i;
        }
    }

    node->node_content.eval = eval_best_move;
    node->node_content.did_update_eval = 1;
}

/*
* Update all positions in a position tree recursively
*/
void recursive_update(struct Node* node) {
    // if already updated: return
    if (node->node_content.did_update_eval)
        return;

    // if no children, static eval is fine
    else if (node->amount_children == 0) {
        node->node_content.did_update_eval = 1;
        return;
    }

    // else, update children first, then self
    else {
        for(int i = 0; i < node->amount_children; i++) {
            recursive_update(&node->children[i]);
        }

        update_eval(node);

    }

}

struct Position* choose_move(struct Node* parent) {
    log_msg("(engine) Trying to find best move...", verbose);

    log_msg("(engine) Generating decision tree...", verbose);
    recursive_generate(parent, 0);

    log_msg("(engine) Updating evaluations...", verbose);
    recursive_update(parent);

    enum Game_state color = parent->node_content.position.state;

    int eval_best_move;
    int index_best_move = 0;

    if (color == white)
        eval_best_move = -1000000;
    else if (color == black)
        eval_best_move = 1000000;

    for(int i = 0; i < parent->amount_children; i++) {
        struct Node* child = &parent->children[i];

        if (color == white && child->node_content.eval > eval_best_move) {
            eval_best_move = child->node_content.eval;
            index_best_move = i;
        }

        else if (color == black && child->node_content.eval < eval_best_move) {
            eval_best_move = child->node_content.eval;
            index_best_move = i;
        }
    }

    log_msg("(engine) Found best move!", verbose);
    char msg[64];
    snprintf(msg, 64, "(engine) Position now evaluated at: %f!", (float)(eval_best_move)/1000);
    log_msg(msg, verbose);


    return &parent->children[index_best_move].node_content.position;
}


void init_engine() {
    struct Node root = {
        NULL,
        NULL,
        0
    };

    root.node_content.did_update_eval = 0;
}

/*
int main() {
    init_log();

    struct Game game;
    init_game(&game, &starting_position, 128);

    struct Node root = {
        NULL,
        NULL,
        0,
        {
            game.positions[0],
            static_eval(&game.positions[0])
        }
    };

    create_children(&root);
    eval_children(&root);

    for(int i = 0; i < 20; i++){
        printf_node(&root.children[i]);
    }

    printf("Best move:\n");

    printf_pos( choose_move(&root) );

    return 0;
}
*/
