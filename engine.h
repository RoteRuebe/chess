#pragma once

#include "backend.h"

extern struct Node root;

struct Node_content {
    struct Position position;
    int eval;
    int did_update_eval;
};

struct Node {
    struct Node *parent;
    struct Node *children;
    int amount_children;

    struct Node_content node_content;
};

struct Position* choose_move(struct Node* parent);


void init_engine();