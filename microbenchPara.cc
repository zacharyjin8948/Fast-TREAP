#include "microbench.h"
#include "fast_treap.h"

static __thread __uint128_t g_lehmer64_state; // Thread safe
static pthread_barrier_t barrier;
static unsigned int first_seed;
static int correctness = 1;

struct readThreadParams {
    long num_ops;
    FILE* metadata;
    int threadId;
    struct Treap_adv *T_treap_adv;
    int *keys;
    int isLookup; // 0 for insert; 1 for lookup/update(need to shuffle keys)
};

static void initSeed(unsigned int first_seed) {
    srand(first_seed);
    g_lehmer64_state = rand();
}

static uint64_t lehmer64() {
    g_lehmer64_state *= 0xda942042e4dd58b5;
    return g_lehmer64_state >> 64;
}

// A utility function to swap to integers(int64_t)
void swap(int *a, int *b)
{
    int temp = *a;
    *a = *b;
    *b = temp;
}

// Source from: https://www.geeksforgeeks.org/shuffle-a-given-array-using-fisher-yates-shuffle-algorithm/
void shuffleArray(int arr[], int n){
    // Start from the last element and swap one by one. We don't
    // need to run for the first element that's why i > 0
    for (int i = n-1; i > 0; i--){
        // Pick a random index from 0 to i
        int j = lehmer64() % (i+1);
     
        // Swap arr[i] with the element at random index
        swap(&arr[i], &arr[j]);
    }

}

//INT64_MIN
//INT64_MAX
int IsTreap(struct treapNode_adv *adv_node, int min, int max, int max_priority){
    if(adv_node == NULL)
        return 1;
    
    if(adv_node->key < min || adv_node->key > max || adv_node->priority > max_priority){
        printf("min:%d max:%d max_priority:%d\n", min, max, max_priority);
        treePrint_adv(adv_node, 0);
        return 0;
    }
        
    
    return IsTreap(adv_node->left, min, adv_node->key, adv_node->priority) && IsTreap(adv_node->right, adv_node->key, max, adv_node->priority);
    
}

int IsStopWriting(struct treapNode_adv *adv_node, int min, int max){
    if(adv_node == NULL)
        return 1;
    
    if(adv_node->subtree_writing == true)
        return 0;
    
    return IsStopWriting(adv_node->left, min, adv_node->key) && IsStopWriting(adv_node->right, adv_node->key, max);
    
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

void *multithreading(void *arg){
    
    struct readThreadParams *params = (struct readThreadParams*)arg;
    long num_ops = params->num_ops;
    FILE* metadata = params->metadata;
    int threadId = params->threadId;
    struct Treap_adv *T_treap_adv = params->T_treap_adv;
    int *keys = params->keys;
    int isLookup = params->isLookup;
    
    unsigned int my_seed = (unsigned) time(NULL);
//    unsigned int my_seed = (unsigned) 41;
    initSeed(my_seed+threadId);
//    initSeed((unsigned int) threadId+1);
    
    if(isLookup == 0){
//        resetCounter();
//        traverse_adv(T_treap_adv->root);
//        printf("No. of nodes before start: %ld\n", getCounter());
        if(threadId == 0)
            system("sudo /home/blepers/linux/tools/perf/perf record -a -g &");
    }else{
        shuffleArray(keys, num_ops);
//        printf("Thread %d: End of shuffling. Start. \n", threadId);
    }
    int key;
    int priority;
    
    declare_timer;
    pthread_barrier_wait(&barrier);
    struct treapNode_adv * found_node;
    start_timer {
        for(int i = 0; i < num_ops; i++){
            if(isLookup != 0){
                key = keys[i];
//                update_adv(T_treap_adv, key, i);  // Updates
                found_node = search_adv((struct Treap_adv *)T_treap_adv, key); // Lookups
                if(!found_node)
                    printf("SHOULD NOT HAVE THIS: key not found? \n");
            }else{
                key = lehmer64();
                priority = lehmer64();
                searchwithInsert_adv(T_treap_adv, key, i, priority, metadata);
            }

        }

    }stop_timer("FAST TREAP(Random keys): Thread %d : Doing %ld ops (%f ops/s)", threadId, num_ops, num_ops/(((double)elapsed)/1000000.));
    if(threadId == 0)
        system("sudo killall -INT -w perf");
    if(threadId == 0)
        printf("Average number of lock per insert: %f\n", getLockNumber()*1.0/num_ops);
        
    
    return NULL;
}


int main(int argc, char **argv) {
    int test[] = {1, 0, 0, 0};
    int autochecking = 1;
    int loading = 0;
    int latencycheck = 0;
    int perfcheck = 0;
    int previous = 0;
    int numThread = atoi(argv[3]); // n writers
    
    first_seed = (unsigned) time(NULL);
//    first_seed = (unsigned) 41;
    
    initSeed(first_seed);
    
#ifdef RALLOC
    printf("Init val: %d\n", RP_init("test1", 8*1024*1024*1024ULL));
#endif

#ifdef RALLOC
    struct Treap_adv *T_treap_adv = (struct Treap_adv *) RP_malloc(sizeof(struct Treap_adv));
    T_treap_adv->root = NULL;
    struct Treap_adv *T_treap_adv_fail = (struct Treap_adv *) RP_malloc(sizeof(struct Treap_adv));
    T_treap_adv_fail->root = NULL;
    struct Treap_adv *T_treap_adv_recover = (struct Treap_adv *) RP_malloc(sizeof(struct Treap_adv));
#else
    struct Treap_adv *T_treap_adv = (struct Treap_adv *) malloc(sizeof(struct Treap_adv));
    T_treap_adv->root = NULL;
    struct Treap_adv *T_treap_adv_fail = (struct Treap_adv *) malloc(sizeof(struct Treap_adv));
    T_treap_adv_fail->root = NULL;
    struct Treap_adv *T_treap_adv_recover = (struct Treap_adv *) malloc(sizeof(struct Treap_adv));
    T_treap_adv_recover->root = NULL;
#endif
    
    // Save root in a persistent file (Abandoned, chose to save root using ralloc)
    FILE* metadata;
    
    int64_t num_ops = atoi(argv[1]);
    int64_t insert_update_percentage = atoi(argv[2]);
    
    declare_timer;
    int key;
    int priority;
    
#ifdef RALLOC
    int *keys = (int *)RP_malloc(num_ops  * sizeof(int));
#else
    int *keys = (int *)malloc(num_ops * sizeof(int));
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
    
//    printf("\n Start Benchmarking with %ld ops...\n\n", num_ops);
    
    if(previous){ // Find previous root in metadata file if exists
        printf("Find previous root ...\n");
#ifdef RALLOC
        T_treap_adv_recover = (struct Treap_adv *)RP_get_root_c((uint64_t) 1);
        T_treap_adv_recover->root = (struct treapNode_adv *)RP_get_root_c((uint64_t) 2);
        RP_recover();
#endif
        
//        metadata = fopen("./metadata","r");
//        char buff[20];
//        fscanf(metadata,"%s", buff);
//        sscanf(buff, "%p", (struct Treap_adv **)&T_treap_adv_recover);
        printf("Root address: %p\n", T_treap_adv_recover);

        if(T_treap_adv_recover->root){
            printf("latest_key: %d\n", T_treap_adv_recover->latest_key);
            printf("root's key: %d\n", T_treap_adv_recover->root->key);
            printf("root's left child's key:%p\n", T_treap_adv_recover->root->left);
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
            int ret;
            while(i < num_ops){
                key = lehmer64();
                priority = lehmer64();
                ret = searchwithInsert_adv(T_treap_adv, key, i, priority, metadata); // Inserts with searching duplicates first
                if (ret == 1){
                    keys[i] = key;
                    i++;
                }
                
            }
            system("sleep 2");
            printf("End of loading.\n");
            // Place shuffle process inside ech threading function
        }
        
        resetTreapReadNodeNumber();
        resetTreapWriteNodeNumber();
        resetLockNumber();

        if(perfcheck){
            system("sleep 2");
//            system("sudo /home/blepers/linux/tools/perf/perf stat -e unc_m_pmm_bandwidth.read,unc_m_pmm_bandwidth.write -a &");
                system("sudo /home/blepers/linux/tools/perf/perf record -a -g &");
            system("sleep 2");
        }
        
        /* multithreading starts */
        pthread_t *tid = (pthread_t *)malloc(numThread * sizeof(pthread_t));
        
        pthread_barrier_init(&barrier, NULL, numThread);
        
        for(int i=0; i<numThread; i++ ){
            /* params for each thread */
            struct readThreadParams *readParams = (struct readThreadParams *)malloc(sizeof(*readParams));
            readParams->num_ops = num_ops*1.0/numThread; // NEED TO CHANGE WHEN LOOKUP
            readParams->metadata = metadata;
            readParams->threadId = i;
            readParams->T_treap_adv = T_treap_adv;
            readParams->keys = keys;
            readParams->isLookup = 0; // now all insert threads
            
            pthread_create( &tid[i], NULL, multithreading, readParams);
        }
            
        /* multithreading ends */
        for(int i=0; i<numThread; i++ )
            pthread_join( tid[i], NULL );
        
//        printf("Average number of node read per insert: %f\n", getTreapReadNodeNumber()*1.0/(num_ops*insert_update_percentage*1.0/100));
//        resetTreapReadNodeNumber();
//        printf("Average number of node written per insert: %f\n", getTreapWriteNodeNumber()*1.0/(num_ops*insert_update_percentage*1.0/100));
//        resetTreapWriteNodeNumber();
        
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
//
//        if(T_treap_adv->root->left->key)
//            printf("%d\n", T_treap_adv->root->left->key);
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
            system("sudo /home/blepers/linux/tools/perf/perf stat -e unc_m_pmm_bandwidth.read,unc_m_pmm_bandwidth.write -a &");
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

    
//    system("sudo /home/blepers/linux/tools/perf/perf record -a -g &");
//    system("sudo killall perf");
    
    resetCounter();
    traverse_adv(T_treap_adv->root);
    printf("Final counting: %ld\n", getCounter());
    
    
    
//    printf("\n Finished\n");
    
    
    if(autochecking){
        printf("\n Auto Checking...\n");
        // TODO: try rebuilding TREAP: use T_treap_adv_recover
//        if(loading){
//            printf("Loading %ld records ...\n", num_ops);
//            int i = 0;
//            int ret;
//            while(i < num_ops){
//                key = lehmer64();
//                priority = lehmer64();
//                ret = searchwithInsert_adv(T_treap_adv, key, i, priority, metadata); // Inserts with searching duplicates first
//                if (ret == 1){
//                    keys[i] = key;
//                    i++;
//                }
//
//            }
//            printf("End of loading.\n");
//        }
        
        
//        if(T_treap_adv->root != NULL && T_treap_adv_recover->root != NULL){
            if(!IsTreap(T_treap_adv->root, INT_MIN, INT_MAX, T_treap_adv->root->priority))
                printf(" Treap_adv not good\n\n");
            if(!IsStopWriting(T_treap_adv->root, INT_MIN, INT_MAX))
                printf(" Treap_adv not stopping\n\n");
//            else{
//                treeAutoCheck(T_treap_adv->root, T_treap_adv_recover->root);
//                printf(" Correctness:%d\n\n\n", correctness);
//            }
//        }
    }
    
#ifdef RALLOC
    RP_close();
#endif
    
    
    return 0;
}

