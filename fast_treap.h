#ifndef FAST_TREAP_H
#define FAST_TREAP_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <limits.h>
#include <libpmem.h>
#include <libvmmalloc.h>
#include <pthread.h>
#include <set>
//#include "ralloc.hpp"

struct treapNode_adv {
    int key;
    int value;
    int priority;
    struct treapNode_adv *left;
    struct treapNode_adv *right;
    //    struct treapNode_adv *parent; // TEST
    int version;
//    pthread_mutex_t lock;
    bool subtree_writing;
};


struct Treap_adv {
//    Always have a pointer at root
    struct treapNode_adv *root;
    int latest_key;
//    int version_global;
    bool pq_writing;
    int version_global_commit; // positive int
//    std::set<int> pq; // min heap
};

void traverse_adv(struct treapNode_adv *n);
struct treapNode_adv *search_adv(struct Treap_adv *t, int key);
int adjustPositionCheckLeft(struct treapNode_adv *waiting_node, struct treapNode_adv *new_root_node, int key, int my_version);
int adjustPositionCheckRight(struct treapNode_adv *waiting_node, struct treapNode_adv *new_root_node, int key, int my_version) ;
int update_adv(struct Treap_adv *t, int key, int new_value);
int insert_adv(struct Treap_adv *t, struct treapNode_adv *new_node);
int randomInsert_adv(struct Treap_adv *t, int key, int value, int priority);
void treePrint_adv(struct treapNode_adv *root, int level);
int64_t getTreapReadNodeNumber();
void resetTreapReadNodeNumber();
int64_t getTreapWriteNodeNumber();
void resetTreapWriteNodeNumber();
void resetCounter();
int64_t getCounter();
int64_t getLockNumber();
void resetLockNumber();

int searchwithInsert_adv(struct Treap_adv *t, int key, int value, int priority, FILE* metadata);

int IsBST_adv(struct treapNode_adv *n, int min, int max, struct treapNode_adv *min_node, struct treapNode_adv *max_node);
int recover_adv(struct Treap_adv *t, FILE* metadata);
int recover_visit_node(struct treapNode_adv *n);
void filter_func(struct treapNode_adv *n);

int adjustPositionCheckLeft_failure(struct treapNode_adv *waiting_node, struct treapNode_adv *new_root_node, int key);
int adjustPositionCheckRight_failure(struct treapNode_adv *waiting_node, struct treapNode_adv *new_root_node, int key);
int searchwithInsert_adv_failure(struct Treap_adv *t, int key, int value, int priority, FILE* metadata);
#endif
