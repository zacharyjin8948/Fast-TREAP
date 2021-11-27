#include "microbench.h"
#include "fast_treap.h"

static __thread __uint128_t g_lehmer64_state; // Thread safe
static unsigned int first_seed;
static int correctness = 1;

static void initSeed(unsigned int first_seed) {
    srand(first_seed);
    g_lehmer64_state = rand();
}

static uint64_t lehmer64() {
    g_lehmer64_state *= 0xda942042e4dd58b5;
    return g_lehmer64_state >> 64;
}

// A utility function to swap to integers(int64_t)
void swap(int64_t *a, int64_t *b)
{
    int64_t temp = *a;
    *a = *b;
    *b = temp;
}

// Source from: https://www.geeksforgeeks.org/shuffle-a-given-array-using-fisher-yates-shuffle-algorithm/
void shuffleArray(int64_t arr[], int64_t n){
    // Start from the last element and swap one by one. We don't
    // need to run for the first element that's why i > 0
    for (int64_t i = n-1; i > 0; i--){
        // Pick a random index from 0 to i
        int64_t j = lehmer64() % (i+1);
     
        // Swap arr[i] with the element at random index
        swap(&arr[i], &arr[j]);
    }

}

//INT64_MIN
//INT64_MAX
int IsTreap(struct treapNode_adv *adv_node, int min, int max, int max_priority){
    if(adv_node == NULL)
        return 1;
    
    if(adv_node->key < min || adv_node->key > max || adv_node->priority > max_priority)
        return 0;
    
    return IsTreap(adv_node->left, min, adv_node->key, adv_node->priority) && IsTreap(adv_node->right, adv_node->key, max, adv_node->priority);
    
}

void treeAutoCheck(struct treapNode_adv *adv_node, struct treapNode_adv *classic_node){ // in-order
    if(adv_node == NULL || classic_node == NULL){
        if(adv_node == NULL && classic_node == NULL){
            return;
        }else{
            correctness = 0;
            return;
        }
    }
    
    treeAutoCheck(adv_node->left, classic_node->left);
    
    if(!(adv_node->key == classic_node->key) || !(adv_node->value == classic_node->value) || !(adv_node->priority == classic_node->priority))
        correctness = 0;
    
    treeAutoCheck(adv_node->right, classic_node->right);
    return;
}



int main(int argc, char **argv) {
    int test[] = {0, 0, 0, 0};
    int autochecking = 0;
    int loading = 0;
    int latencycheck = 0;
    int perfcheck = 0;
    int previous = 1;
    
//    first_seed = (unsigned) time(NULL);
    first_seed = (unsigned) 41;
    
    initSeed(first_seed);
    
    int64_t num_ops = atoi(argv[1]);
    int64_t insert_update_percentage = atoi(argv[2]);
    test[0]=atoi(argv[3]);
    previous=atoi(argv[4]);
    
#ifdef RALLOC
    printf("Init val: %d\n", RP_init("test1", 8*1024*1024*1024ULL));
#endif

#ifdef RALLOC
    struct Treap_adv *T_treap_adv;
    struct Treap_adv *T_treap_adv_fail;
    struct Treap_adv *T_treap_adv_recover;
    
    if(!previous){
        T_treap_adv = (struct Treap_adv *) RP_malloc(sizeof(struct Treap_adv));
        T_treap_adv->root = NULL;
        T_treap_adv_fail = (struct Treap_adv *) RP_malloc(sizeof(struct Treap_adv));
        T_treap_adv_fail->root = NULL;
        T_treap_adv_recover = (struct Treap_adv *) RP_malloc(sizeof(struct Treap_adv));
        printf("Root address: %p\n", T_treap_adv_recover);
    }
    
#else
    struct Treap_adv *T_treap_adv = (struct Treap_adv *) malloc(sizeof(struct Treap_adv));
    T_treap_adv->root = NULL;
    struct Treap_adv *T_treap_adv_fail = (struct Treap_adv *) malloc(sizeof(struct Treap_adv));
    T_treap_adv_fail->root = NULL;
    struct Treap_adv *T_treap_adv_recover = (struct Treap_adv *) malloc(sizeof(struct Treap_adv));
#endif
    
//    printf("Root address: %p\n", T_treap_adv);
//    printf("Root address: %p\n", T_treap_adv_recover);
    
    // Save root in a persistent file
    FILE* metadata;
    
    declare_timer;
    int key;
    int priority;
    
#ifdef RALLOC
    int64_t *keys = (int64_t *)RP_malloc(num_ops * sizeof(int64_t));
#else
    int64_t *keys = (int64_t *)malloc(num_ops * sizeof(int64_t));
#endif
    
    uint64_t a,b;
    uint64_t *latencies;
    FILE* file;
    
    if(latencycheck){
//        CHECK LATENCY:
        latencies = (uint64_t*) calloc(sizeof(*latencies), num_ops);
        if ((file = fopen("./latency_log.txt","w")) == NULL){
            printf("Error when opening file");
            return 0;
        }
    }

    
    printf("\n Start Benchmarking with %ld ops...\n\n", num_ops);
    if(previous){ // Find previous root in metadata file if exists
        printf("Find previous root ...\n");
#ifdef RALLOC
//        struct Treap_adv *T_treap_adv_recover = (struct Treap_adv *) RP_malloc(sizeof(struct Treap_adv));
        T_treap_adv_recover = (struct Treap_adv *)RP_get_root_c((uint64_t) 1);
        T_treap_adv_recover->root = (struct treapNode_adv *)RP_get_root_c((uint64_t) 2);
//        T_treap_adv_recover = (struct Treap_adv *)RP_get_root<struct Treap_adv *>((uint64_t) 1);
//
//        T_treap_adv_recover->root = (struct treapNode_adv *)RP_get_root<struct treapNode_adv>((uint64_t) 2);
        
//            GarbageCollection gc;
//            gc();
        
//        printf("%zu\n",RP_recover().size());
        printf("Root recovery completed!\n");
//        filter_func((struct treapNode_adv *)RP_get_root_c((uint64_t) 2));
        RP_recover();
#endif
        
//        metadata = fopen("./metadata","r");
//        char buff[20];
//        fscanf(metadata,"%s", buff);
//
//        sscanf(buff, "%p", (struct Treap_adv **)&T_treap_adv_recover);
        printf("Root address: %p\n", T_treap_adv_recover);
//        T_treap_adv_recover = (struct Treap_adv *) RP_malloc(sizeof(struct Treap_adv));
//        printf("Root address: %p\n", T_treap_adv_recover);

        if(T_treap_adv_recover->root){
            printf("latest_key: %d\n", T_treap_adv_recover->latest_key);
            printf("root's key: %d\n", T_treap_adv_recover->root->key);
            printf("root's left child's key:%d\n", T_treap_adv_recover->root->left->key);
        }
            
//        fclose(metadata);

    }
    
    
    /* 1. Benchmark N Fast_treap(Adv_treap) ops with uniformly random keys */
    /* Timer starts */
    if(test[0]){
        initSeed(first_seed);
        T_treap_adv->root = NULL;
        memset(keys, 0, num_ops * sizeof(keys[0]));
        
        if ((metadata = fopen("./metadata","w+")) == NULL){
            printf("Error when opening file");
            return 0;
        }
        
        if(loading){
            printf("Loading %ld records ...\n", num_ops);
            int i = 0;
            while(i < num_ops){
                key = lehmer64();
                priority = lehmer64();
//                if(!search_adv(T_treap_adv, key)){ // Checking key duplicates first
//                    randomInsert_adv(T_treap_adv, key, i, priority); // Inserts
//                    i++;
//                }
                searchwithInsert_adv(T_treap_adv, key, i, priority, metadata); // Inserts with searching duplicates first
                keys[i] = key;
                i++;
            }
            printf("End of loading. Start Shuffling keys. \n");
            shuffleArray(keys, num_ops);
            printf("End of shuffling. Start. \n");
        }
        
        resetTreapReadNodeNumber();
        resetTreapWriteNodeNumber();

        if(perfcheck){
            system("sleep 5");
            system("sudo perf stat -e unc_m_pmm_bandwidth.read,unc_m_pmm_bandwidth.write -a &");
        }
        
        system("sleep 5");
        
        start_timer {
            for(int i = 0; i < num_ops; i++){
                if(lehmer64() % 100 >= insert_update_percentage){
                    key = keys[lehmer64()%(num_ops-1)];
                    update_adv(T_treap_adv, key, i);  // Updates
//                    if(!search_adv(T_treap_adv, keys[i])){ // Lookups
//                    }
                }else{
                    if(latencycheck){
                        rdtscll(a);
                    }
                    key = lehmer64();
                    priority = lehmer64();
                    searchwithInsert_adv(T_treap_adv, key, i, priority, metadata);
                    if(latencycheck){
                        rdtscll(b);
                        latencies[i] = b -a;
                    }
                    
                }

            }

        } stop_timer("FAST TREAP(Random keys): Doing %ld ops (%f ops/s)", num_ops, num_ops/(((double)elapsed)/1000000.));
        printf("Average number of node read per insert: %f\n", getTreapReadNodeNumber()*1.0/(num_ops*insert_update_percentage*1.0/100));
        resetTreapReadNodeNumber();
        printf("Average number of node written per insert: %f\n", getTreapWriteNodeNumber()*1.0/(num_ops*insert_update_percentage*1.0/100));
        resetTreapWriteNodeNumber();
        
        if(latencycheck){
            for(int i = 0; i < num_ops; i++)
                fprintf(file, "%lu\n", latencies[i]);
            fclose(file);
        }
        
        if(perfcheck){
            system("sudo killall -INT -w perf");
        }
        fclose(metadata);

//        metadata = fopen("./metadata","r");
//        char buff[20];
//        fscanf(metadata,"%s", buff);
//
//        sscanf(buff, "%p", (struct Treap_adv **)&T_treap_adv_recover);
//
//        if(T_treap_adv_recover->root->left->key)
//            printf("%d\n", T_treap_adv_recover->latest_key);
//
//        fclose(metadata);
        
    }
    

    /* 2. Benchmark N Fast_treap(Adv_treap) ops with increasing keys */
    /* Timer starts */
    if(test[1]){
        initSeed(first_seed);
        T_treap_adv->root = NULL;
        memset(keys, 0, num_ops * sizeof(keys[0]));

        if ((metadata = fopen("./metadata","w+")) == NULL){
            printf("Error when opening file");
            return 0;
        }

        if(loading){
            printf("Loading %ld records ...\n", num_ops);
            int i = 0;
            while(i < num_ops){
                key = lehmer64();
                priority = lehmer64();
//                if(!search_adv(T_treap_adv, key)){ // Checking key duplicates first
//                    randomInsert_adv(T_treap_adv, key, i, priority); // Inserts
//                    i++;
//                }
                searchwithInsert_adv(T_treap_adv, key, i, priority, metadata); // Inserts with searching duplicates first
                keys[i] = i;
                i++;
            }
            printf("End of loading. Start. \n");
        }

        resetTreapReadNodeNumber();
        resetTreapWriteNodeNumber();

        if(perfcheck){
            system("sleep 5");
            system("sudo perf stat -e unc_m_pmm_bandwidth.read,unc_m_pmm_bandwidth.write -a &");
        }

        system("sleep 5");

        start_timer {
            for(int i = 0; i < num_ops; i++){
                if(lehmer64() % 100 >= insert_update_percentage){
                    key = keys[lehmer64()%(num_ops-1)];
                    update_adv(T_treap_adv, key, i);  // Updates
                }else{
                    if(latencycheck){
                        rdtscll(a);
                    }
                    key = lehmer64();
                    priority = lehmer64();
                    searchwithInsert_adv(T_treap_adv, i, i, priority, metadata);
                    if(latencycheck){
                        rdtscll(b);
                        latencies[i] = b -a;
                    }

                }
            }

        } stop_timer("FAST TREAP(Increasing keys): Doing %ld ops (%f ops/s)", num_ops, num_ops/(((double)elapsed)/1000000.));
        printf("Average number of node read per insert: %f\n", getTreapReadNodeNumber()*1.0/(num_ops*insert_update_percentage*1.0/100));
        resetTreapReadNodeNumber();
        printf("Average number of node written per insert: %f\n", getTreapWriteNodeNumber()*1.0/(num_ops*insert_update_percentage*1.0/100));
        resetTreapWriteNodeNumber();

        if(latencycheck){
            for(int i = 0; i < num_ops; i++)
                fprintf(file, "%lu\n", latencies[i]);
            fclose(file);
        }

        if(perfcheck){
            system("sudo killall -INT -w perf");
        }

        fclose(metadata);

//        metadata = fopen("./metadata","r");
//        char buff[20];
//        fscanf(metadata,"%s", buff);

//        sscanf(buff, "%p", (struct Treap_adv **)&T_treap_adv_recover);

//        if(T_treap_adv_recover->root->left->key)
//            printf("%d\n", T_treap_adv_recover->latest_key);

//        fclose(metadata);
    }







    /* 3. Benchmark N Fast_treap(Adv_treap) ops with uniformly random keys(WITH FAILURE) */
    /* Timer starts */
    if(test[2]){
        initSeed(first_seed);
        T_treap_adv_fail->root = NULL;
        memset(keys, 0, num_ops * sizeof(keys[0]));

        if ((metadata = fopen("./metadata","w+")) == NULL){
            printf("Error when opening file");
            return 0;
        }

        if(loading){
            printf("Loading %ld records ...\n", num_ops);
            int i = 0;
            while(i < num_ops-1){
                key = lehmer64();
                priority = lehmer64();
                searchwithInsert_adv(T_treap_adv, key, i, priority, metadata); // Inserts with searching duplicates first
                keys[i] = key;
                i++;
            }

            // TODO: last operation is bad
            while(i<num_ops){
                key = lehmer64();
                priority = lehmer64();

                searchwithInsert_adv_failure(T_treap_adv, key, i, priority, metadata); // Inserts with failure

            }

            printf("End of loading. Start Shuffling keys. \n");
        }

        system("sleep 5");
        start_timer {
            recover_adv(T_treap_adv_fail, metadata);

        } stop_timer("FAST TREAP(Random keys): Doing %ld ops RECOVERY (%f ops/s)", num_ops, num_ops/(((double)elapsed)/1000000.));


    }
    
    
    
//    system("sudo perf record -a -g &");
//    system("sudo killall perf");
    
    
//    traverse_re(T_treap_re->root);
//    treePrint_re(T_treap_re->root, 0);
//    treePrint_adv(T_treap_adv->root, 0);
    
    
    printf("\n Finished\n");
    sleep(5);
    
    if(autochecking){
        if(T_treap_adv->root != NULL && T_treap_adv_recover->root != NULL){
            printf("\n Auto Checking...\n");
            if(!IsTreap(T_treap_adv->root, INT_MIN, INT_MAX, T_treap_adv->root->priority))
                printf(" Treap_adv not good\n\n");
            else{
                treeAutoCheck(T_treap_adv->root, T_treap_adv_recover->root);
                printf(" Correctness:%d\n\n\n", correctness);
            }
        }
    }
    

//    RP_simulate_crash();
    printf("crash!\n");
    
    
#ifdef RALLOC
    RP_close();
//    system("rm /pmem4/ralloc_pool/*");
    
#endif
    return 0;
}

