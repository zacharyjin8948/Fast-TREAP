#include "fast_treap.h"

static int64_t num_read_treap_adv;
static int64_t num_write_treap_adv;
static int64_t num_lock;
static int64_t counter = 0;
std::set<int> pq; // min heap

void traverse_adv(struct treapNode_adv *n) {  // pre-order
    if(n == NULL)
        return;
    
//    printf("Address: 0x%lx Key: %d Value: %d priority: %2d\n", (int64_t)n, n->key, n->value, n->priority);
//    n->visited++;
    counter++;
    traverse_adv(n->left);
    traverse_adv(n->right);
    
}

struct treapNode_adv *search_adv(struct Treap_adv *t, int key) {
    struct treapNode_adv *n;
    volatile int my_version;
    if(t == NULL) // Treap not exists
        return 0;
retrylookup:
    if(!pq.empty())
        my_version = *(pq.begin())-1;
    else
        my_version = t->version_global_commit;
    n = t->root;

    while(n) {
        num_read_treap_adv++;
        if(my_version < n->version){
            goto retrylookup;
        }

        if(n->key == key) { // Found!
            return (struct treapNode_adv *)n;
        } else if(n->key > key) {
            n = n->left;
        } else {
            n = n->right;
        }
    }
    return 0;
}


int adjustPositionCheckLeft(struct treapNode_adv *waiting_node, struct treapNode_adv *new_root_node, int key, int my_version) {
    struct treapNode_adv *waiting;
    struct treapNode_adv *new_root;
    struct treapNode_adv *new_root_child;
    struct treapNode_adv *parent;

    waiting = waiting_node;
    new_root = new_root_node;
    parent = NULL;
    
    while(1){
        if(new_root == NULL || new_root->left == NULL){
            waiting->right = NULL;
            num_write_treap_adv++;
            waiting->version = my_version;
            
            if(parent) // && parent->subtree_writing;
                parent->subtree_writing = false;
            return 1;
        }
        while(!__sync_bool_compare_and_swap(&(new_root->subtree_writing), false, true)) {} // TODO
        num_lock++;
        
        if(parent) // && parent->subtree_writing;
            parent->subtree_writing = false;

        new_root_child = new_root->left;
        if(new_root->left->key < key){
            if(new_root->left != new_root_child){  // Check if old parent is still parent (Not used? set here just in case)
                new_root->subtree_writing = false;
                printf("hello\n");
                adjustPositionCheckLeft(waiting_node, new_root_node, key, my_version);
            }
            // TODO: may lose
            
            waiting->right = new_root->left;
            waiting->version = my_version;
            
            num_read_treap_adv++;
            num_write_treap_adv++;
            
            adjustPositionCheckRight(new_root, new_root->left, key, my_version);
            new_root->subtree_writing = false;
            
            break;
        }else{ // if(new_root->left->key > key)
            parent = new_root;
            new_root = new_root->left;
            num_read_treap_adv++;
        }
    }
    
    
    return 1;
}


int adjustPositionCheckRight(struct treapNode_adv *waiting_node, struct treapNode_adv *new_root_node, int key, int my_version) {
    struct treapNode_adv *waiting;
    struct treapNode_adv *new_root;
    struct treapNode_adv *new_root_child;
    struct treapNode_adv *parent;

    waiting = waiting_node;
    new_root = new_root_node;
    parent = NULL;
    
    while(1){
        if(new_root == NULL || new_root->right == NULL){
            waiting->left = NULL;
            num_write_treap_adv++;
            waiting->version = my_version;
            
            if(parent) // && parent->subtree_writing;
                parent->subtree_writing = false;
            return 1;
        }
        while(!__sync_bool_compare_and_swap(&(new_root->subtree_writing), false, true)) {} // TODO
        num_lock++;
        
        if(parent) // && parent->subtree_writing;
            parent->subtree_writing = false;

        new_root_child = new_root->right; // volatile
        if(new_root_child->key > key){
            if(new_root->right != new_root_child){  // Check if old parent is still parent (Not used? set here just in case)
                new_root->subtree_writing = false;
                printf("hello2\n");
                adjustPositionCheckRight(waiting_node, new_root_node, key, my_version);
            }
            // TODO: may lose
            
            waiting->left = new_root->right;
            waiting->version = my_version;
            
            num_read_treap_adv++;
            num_write_treap_adv++;
           
            adjustPositionCheckLeft(new_root, new_root->right, key, my_version);
            new_root->subtree_writing = false;
                
            break;
        }else{ // if(new_root->right->key < key)
            parent = new_root;
            new_root = new_root->right;
            num_read_treap_adv++;
        }
    }
    
    return 1;
    
}


int update_adv(struct Treap_adv *t, int key, int new_value){
    struct treapNode_adv *n = search_adv(t,key);
    if(n == 0)  // Node not exists
        return 0;
    else
        n->value = new_value;
    
    pmem_persist(&n, sizeof(struct treapNode_adv));
    
    return 1;
    
}


void treePrint_adv(struct treapNode_adv *root, int level) {  // pre-order
        if(root == NULL)
            return;
    
        for(int i = 0; i < level; i++)
            printf(i == level - 1 ? "        |-" : "          ");
    
        printf("Key: %d, Priority: %d, Sub: %d\n", root->key, root->priority, root->subtree_writing);
        treePrint_adv(root->left, level + 1);
        treePrint_adv(root->right, level + 1);
}

int64_t getTreapReadNodeNumber(){
    return num_read_treap_adv;
}

void resetTreapReadNodeNumber(){
    num_read_treap_adv = 0;
}

int64_t getTreapWriteNodeNumber(){
    return num_write_treap_adv;
}

void resetTreapWriteNodeNumber(){
    num_write_treap_adv = 0;
}

void resetCounter(){
    counter = 0;
}

int64_t getCounter(){
    return counter;
}

int64_t getLockNumber(){
    return num_lock;
}

void resetLockNumber(){
    num_lock = 0;
}

int searchwithInsert_adv(struct Treap_adv *t, int key, int value, int priority, FILE* metadata){
    
    if(t == NULL) // Treap not exists
        return 0;
retryfind:
    volatile int my_version;
    if(!pq.empty())
        my_version = *(pq.begin())-1;
    else
        my_version = t->version_global_commit;
    
    struct treapNode_adv *parent = NULL;
    struct treapNode_adv *n = t->root;
    num_read_treap_adv++;
    
    struct treapNode_adv *target = NULL;
    struct treapNode_adv *pivot = NULL;
    
    int direction_flag = 0;
    
    while(n){
        if(direction_flag == 0){
            
            target = parent;
            if(target){
                while(!__sync_bool_compare_and_swap(&(target->subtree_writing), false, true)) {}
            }
            if(n->priority < priority){
                
                if(n->key > key){
                    // keeping track of n->left ptr
                    direction_flag = -1;
                }else{
                    // keeping track of n->right ptr
                    direction_flag = 1;
                }
                pivot = n;
//                if(target)
//                    while(!__sync_bool_compare_and_swap(&(target->subtree_writing), false, true)) {}
                
            }
            else{
                if(target)
                    target->subtree_writing = false;
            }
        }
        while(!__sync_bool_compare_and_swap(&(n->subtree_writing), false, true)) {}
        num_lock++;
        parent = n;
        
        if(my_version < n->version){
            if(target)
                target->subtree_writing = false;
            parent->subtree_writing = false;
            goto retryfind;
        }
            
        
        if(n->key == key) { // Duplicate key: exit
            if(target)
                target->subtree_writing = false;
            parent->subtree_writing = false;
            return 0;
        } else if(n->key > key) {
            n = n->left;
            parent->subtree_writing = false;
        } else {
            n = n->right;
            parent->subtree_writing = false;
        }
        num_read_treap_adv++;
        
        
    }
    
#ifdef RALLOC
    struct treapNode_adv *new_node = (struct treapNode_adv *) RP_malloc(sizeof(struct treapNode_adv));
#else
    struct treapNode_adv *new_node = (struct treapNode_adv *) malloc(sizeof(struct treapNode_adv));
#endif
    
    if(new_node == NULL)
        return -1;
    
    new_node->key = key;
    new_node->value = value;
    new_node->priority = priority;
    new_node->left = new_node->right = NULL;
    
    new_node->version = __sync_add_and_fetch(&(t->version_global_commit), 1);
    while(!__sync_bool_compare_and_swap(&(t->pq_writing), false, true)) {}
    pq.insert(new_node->version);
    t->pq_writing = false;
    
    new_node->subtree_writing = false;
    
    pmem_persist(&new_node, sizeof(struct treapNode_adv));
    num_write_treap_adv++;
    
    if(t->root == NULL) {
        t->root = new_node;
        t->root->subtree_writing = false; // ? Not Neccessary
        pmem_persist(&t->root, sizeof(struct treapNode_adv));
        num_write_treap_adv++;

//        fprintf(metadata, "%p\n", t);
        
        t->latest_key = key;

        while(!__sync_bool_compare_and_swap(&(t->pq_writing), false, true)) {}
        pq.erase(new_node->version);
        t->pq_writing = false;
        
        pmem_persist(&t, sizeof(struct Treap_adv));
#ifdef RALLOC
        RP_set_root(t, 1);
        RP_set_root(t->root, 2);
#endif
        
        return 1;
    }
//    treePrint_adv(t->root, 0);
//    printf("Key: %d, Priority: %d, Sub: %d\n", new_node->key, new_node->priority, new_node->subtree_writing);
    
    // persist latest key
//    fseek(metadata, 0, SEEK_SET);
//    fprintf(metadata, "%d", key);
    
    t->latest_key = key;
    pmem_persist(&t, sizeof(struct Treap_adv));
    num_write_treap_adv++;
#ifdef RALLOC
    RP_set_root(t, 1);
#endif

    if(direction_flag == 0){ // Reach leaf level
        if(parent == NULL){
            while(!__sync_bool_compare_and_swap(&(t->pq_writing), false, true)) {}
            pq.erase(new_node->version);
            t->pq_writing = false;
            goto retryfind;
        }

        if(parent->key > key){
            parent->left = new_node;
        }else{
            parent->right = new_node;
        }
        parent->version = new_node->version;
        pmem_persist(&parent, sizeof(struct treapNode_adv));
        num_write_treap_adv++;
    }
    
    if(direction_flag == -1){ // keeping track of n->left ptr
        new_node->right = pivot;
        pmem_persist(&new_node, sizeof(struct treapNode_adv)); // ? Not Necessary
        num_write_treap_adv++;
        
        if(target) {
            // check old parent is still parent
            if(target->left != pivot && target->right != pivot){
                target->subtree_writing = false;
                while(!__sync_bool_compare_and_swap(&(t->pq_writing), false, true)) {}
                pq.erase(new_node->version);
                t->pq_writing = false;
                goto retryfind;
            }
                
            if(pivot == target->left)
                target->left = new_node;
            else
                target->right = new_node;
            target->version = new_node->version;
            pmem_persist(&target, sizeof(struct treapNode_adv));
            num_write_treap_adv++;
        } else{
            t->root = new_node;
            pmem_persist(&t, sizeof(struct Treap_adv));
            num_write_treap_adv++;
//            fseek(metadata, 0, SEEK_SET);
//            fprintf(metadata, "%p\n", t);
#ifdef RALLOC
            RP_set_root(t, 1);
            RP_set_root(t->root, 2);
#endif
        }

        parent = NULL;
        while(1){
            while(!__sync_bool_compare_and_swap(&(pivot->subtree_writing), false, true)) {} // TODO
            num_lock++;
            if(parent)
                parent->subtree_writing = false;
            if(pivot->left == NULL || pivot->left->key < new_node->key){
                new_node->left = pivot->left;
                num_read_treap_adv++;
                num_write_treap_adv++;
                    
                adjustPositionCheckRight(pivot, pivot->left, new_node->key, my_version);
                pivot->subtree_writing = false;
                break;
            }
            parent = pivot;
            pivot = pivot->left;
            num_read_treap_adv++;
        }
    }
    
    if(direction_flag == 1){ // keeping track of n->right ptr
        new_node->left = pivot;
        pmem_persist(&new_node, sizeof(struct treapNode_adv)); // ? Not Necessary
        num_write_treap_adv++;
        
        if(target) {
            // check old parent is still parent
            if(target->left != pivot && target->right != pivot){
                target->subtree_writing = false;
                while(!__sync_bool_compare_and_swap(&(t->pq_writing), false, true)) {}
                pq.erase(new_node->version);
                t->pq_writing = false;
                goto retryfind;
            }
            
            if(pivot == target->left)
                target->left = new_node;
            else
                target->right = new_node;
            target->version = new_node->version;
            pmem_persist(&target, sizeof(struct treapNode_adv));
            num_write_treap_adv++;
        } else{
            t->root = new_node;
            pmem_persist(&t, sizeof(struct Treap_adv));
            num_write_treap_adv++;
//            fseek(metadata, 0, SEEK_SET);
//            fprintf(metadata, "%p\n", t);
#ifdef RALLOC
            RP_set_root(t, 1);
            RP_set_root(t->root, 2);
#endif
        }
        
        parent = NULL;
        while(1){
            while(!__sync_bool_compare_and_swap(&(pivot->subtree_writing), false, true)) {} // TODO
            num_lock++;
            if(parent)
                parent->subtree_writing = false;
            if(pivot->right == NULL || pivot->right->key > new_node->key){
                new_node->right = pivot->right;
                num_read_treap_adv++;
                num_write_treap_adv++;
                    
                adjustPositionCheckLeft(pivot, pivot->right, new_node->key, my_version);
                pivot->subtree_writing = false;
                break;
            }
            parent = pivot;
            pivot = pivot->right;
            num_read_treap_adv++;
        }
    }
    
    if(target)
        target->subtree_writing = false;

    while(!__sync_bool_compare_and_swap(&(t->pq_writing), false, true)) {} 
    pq.erase(new_node->version);
    t->pq_writing = false;
    pmem_persist(&t, sizeof(struct Treap_adv));
    return 1;
}



// Next is recovery process


int IsBST_adv(struct treapNode_adv *n, int min, int max, struct treapNode_adv *min_node, struct treapNode_adv *max_node){
    if(n == NULL)
        return 1;

    if(n->key < min){
//        if(n->visited == 1){ // no adjustment happened yet

            if(min_node->left == NULL){
                min_node->left = n;
            }

//        }
        // in the middle of adjustment process

        // Continue adjusting
        adjustPositionCheckRight(max_node, n, min, 1); // TODO

        // End of adjustment
        return 1;

    }else if(n->key > max){
//        if(n->visited == 1){ // no adjustment happened yet

            if(max_node->right == NULL){
                max_node->right = n;
            }

//        }
        // in the middle of adjustment process

        // Continue adjusting
        adjustPositionCheckLeft(min_node, n, max, 1); // TODO

        // End of adjustment
        return 1;
    }

    return IsBST_adv(n->left, min, n->key, min_node, n) && IsBST_adv(n->right, n->key, max, n, max_node);

}

int recover_adv(struct Treap_adv *t, FILE* metadata){

    if(t == NULL) // Treap not found
        return 0;
    
    struct treapNode_adv *n = t->root;
    int key = t->latest_key;
    num_read_treap_adv++;
    
    while(n){
        
        if(n->key == key) { // Key found: Begin to check validity
            
            IsBST_adv(n, INT_MIN, INT_MAX, NULL, NULL); // Check & fix BST property(no need to check priority)
            return 1;
            
        } else if(n->key > key) {
            n = n->left;
        } else {
            n = n->right;
        }
        num_read_treap_adv++;
    }
    return 0; // Last node has not been inserted
}


// 3 functions below is only for simulating failures (return 0)

int adjustPositionCheckLeft_failure(struct treapNode_adv *waiting_node, struct treapNode_adv *new_root_node, int key) {
    struct treapNode_adv *waiting = waiting_node;
    struct treapNode_adv *new_root = new_root_node;

    while(1){
        if(new_root == NULL || new_root->left == NULL){
            waiting->right = NULL;
            return 1;
        }

        if(new_root->left->key < key){
            waiting->right = new_root->left;
            break;
        }else{ // if(new_root->left->key > key)
            new_root = new_root->left;
        }

    }

    adjustPositionCheckRight(new_root, new_root->left, key, 1); // TODO
    return 1;
}


int adjustPositionCheckRight_failure(struct treapNode_adv *waiting_node, struct treapNode_adv *new_root_node, int key) {
    struct treapNode_adv *waiting = waiting_node;
    struct treapNode_adv *new_root = new_root_node;

    while(1){
        if(new_root == NULL || new_root->right == NULL){
            waiting->left = NULL;
            return 1;
        }

        if(new_root->right->key > key){
            waiting->left = new_root->right;
            break;
        }else{ // if(new_root->right->key < key)
            new_root = new_root->right;
        }

    }
    return 0;
    adjustPositionCheckLeft(new_root, new_root->right, key, 1); // TODO
    return 1;

}

int searchwithInsert_adv_failure(struct Treap_adv *t, int key, int value, int priority, FILE* metadata){
    if(t == NULL) // Treap not exists
        return 0;
    
    struct treapNode_adv *parent = NULL;
    struct treapNode_adv *n = t->root;
    num_read_treap_adv++;
    
    struct treapNode_adv *target = NULL;
    struct treapNode_adv *pivot = NULL;
    
    int direction_flag = 0;
    
    while(n){
        
        if(direction_flag == 0){
            
            target = parent;
            if( n->priority < priority){
                
                if(n->key > key){
                    // keeping track of n->left ptr
                    direction_flag = -1;
                }else{
                    // keeping track of n->right ptr
                    direction_flag = 1;
                }
                pivot = n;
            }
        }
        
        parent = n;
        
        if(n->key == key) { // Duplicate key: exit
            return 0;
        } else if(n->key > key) {
            n = n->left;
        } else {
            n = n->right;
        }
        num_read_treap_adv++;
    }
    
#ifdef RALLOC
    struct treapNode_adv *new_node = (struct treapNode_adv *) RP_malloc(sizeof(struct treapNode_adv));
#else
    struct treapNode_adv *new_node = (struct treapNode_adv *) malloc(sizeof(struct treapNode_adv));
#endif
    
    if(new_node == NULL)
        return -1;
    
    new_node->key = key;
    new_node->value = value;
    new_node->priority = priority;
    new_node->left = new_node->right = NULL;
    
    pmem_persist(&new_node, sizeof(struct treapNode_adv));
    num_write_treap_adv++;
    
    
    if(t->root == NULL) {
        t->root = new_node;
        pmem_persist(&t->root, sizeof(struct treapNode_adv));
        num_write_treap_adv++;
        
        fprintf(metadata, "%p\n", t);
//        fprintf(metadata, "%d", key);
        t->latest_key = key;
        pmem_persist(&t, sizeof(struct Treap_adv));
        
        return 1;
    }
    
    // persist latest key
//    fseek(metadata, 15, SEEK_SET);
//    fprintf(metadata, "%d", key);
    t->latest_key = key;
    pmem_persist(&t, sizeof(struct Treap_adv));
    num_write_treap_adv++;
    
    if(direction_flag == 0){ // Reach leaf level
        if(parent->key > key){
            parent->left = new_node;
        }else{
            parent->right = new_node;
        }
        pmem_persist(&parent, sizeof(struct treapNode_adv));
        num_write_treap_adv++;
        return 1;
    }
    
    if(direction_flag == -1){ // keeping track of n->left ptr
        new_node->right = pivot;
        pmem_persist(&new_node, sizeof(struct treapNode_adv));
        num_write_treap_adv++;
        
        if(target) {
            if(pivot == target->left)
                target->left = new_node;
            else
                target->right = new_node;
            pmem_persist(&target, sizeof(struct treapNode_adv));
            num_write_treap_adv++;
        } else{
            t->root = new_node;
            pmem_persist(&t->root, sizeof(struct treapNode_adv));
            num_write_treap_adv++;
            fseek(metadata, 0, SEEK_SET);
            fprintf(metadata, "%p\n", t);
        }
        
        while(1){
            if(pivot->left == NULL || pivot->left->key < new_node->key){
                new_node->left = pivot->left;
                num_read_treap_adv++;
                num_write_treap_adv++;
                break;
            }
            pivot = pivot->left;
            num_read_treap_adv++;
        }
        adjustPositionCheckRight_failure(pivot, pivot->left, new_node->key);
    }
    
    if(direction_flag == 1){ // keeping track of n->right ptr
        new_node->left = pivot;
        pmem_persist(&new_node, sizeof(struct treapNode_adv));
        num_write_treap_adv++;
        
        if(target) {
            if(pivot == target->left)
                target->left = new_node;
            else
                target->right = new_node;
            pmem_persist(&target, sizeof(struct treapNode_adv));
            num_write_treap_adv++;
        } else{
            t->root = new_node;
            pmem_persist(&t->root, sizeof(struct treapNode_adv));
            num_write_treap_adv++;
            fseek(metadata, 0, SEEK_SET);
            fprintf(metadata, "%p\n", t);
        }
        
        while(1){
            if(pivot->right == NULL || pivot->right->key > new_node->key){
                new_node->right = pivot->right;
                num_read_treap_adv++;
                num_write_treap_adv++;
                break;
            }
            pivot = pivot->right;
            num_read_treap_adv++;
        }
        adjustPositionCheckLeft_failure(pivot, pivot->right, new_node->key);
    }
    return 1;

}
