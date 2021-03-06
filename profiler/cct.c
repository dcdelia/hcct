#include <stdio.h>
#include <stdlib.h>
#include <asm/unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#define _GNU_SOURCE
#include <errno.h>
extern char *program_invocation_name;
extern char *program_invocation_short_name;

#include "cct.h"

#if USE_MALLOC==0
#include "pool.h"   
#endif

// global parameters set by hcct_getenv()
char* dumpPath;
UINT16 getenv_done;

// TLS
__thread UINT16 _TLS_cct_stack_idx;
__thread UINT32 _TLS_cct_nodes;
__thread cct_node_t *_TLS_cct_stack[STACK_MAX_DEPTH];
__thread cct_node_t *_TLS_cct_root;
#if USE_MALLOC==0
// custom allocator
__thread pool_t *_TLS_cct_pool;
__thread void *_TLS_cct_free_list;
#endif

#if CCT_TRACK_SPACE==1
__thread UINT64 _TLS_stream_length;
__thread FILE*  _TLS_track_log;
#endif

#if BURSTING==1
void __attribute__((no_instrument_function)) init_bursting();

extern UINT32 sampling_interval;
extern UINT32 burst_length;
extern UINT16 burst_on;

extern __thread UINT16 _TLS_aligned;
extern __thread UINT64 _TLS_exhaustive_enter_events;
extern __thread hcct_stack_node_t _TLS_shadow_stack[STACK_MAX_DEPTH];
extern __thread UINT16 _TLS_shadow_stack_idx; // legal values: 0 to shadow_stack_idx-1
#elif PROFILE_TIME==1
void __attribute__((no_instrument_function)) init_sampling();

extern UINT32 sampling_interval;

extern __thread UINT64 _TLS_thread_tics;
extern __thread hcct_stack_node_t _TLS_shadow_stack[STACK_MAX_DEPTH];
extern __thread UINT16 _TLS_shadow_stack_idx; // legal values: 0 to shadow_stack_idx-1
#endif

// get parameters from environment variables

static void __attribute__((no_instrument_function)) hcct_getenv() {
    dumpPath = getenv("DUMPPATH");
    if (dumpPath == NULL || dumpPath[0] == '\0')
        dumpPath = NULL;

    getenv_done = 1;
}

void hcct_init() {
    // hcct_init might have been invoked already once before trace_begin() is executed
    if (_TLS_cct_root != NULL) return;

#if BURSTING==1
    if (sampling_interval == 0) init_bursting();
#elif PROFILE_TIME==1
    if (sampling_interval == 0) init_sampling();
#endif

    if (getenv_done == 0) hcct_getenv(); // will be executed only once

    _TLS_cct_stack_idx = 0;
    _TLS_cct_nodes = 1;

#if SHOW_MESSAGES==1
    pid_t tid = syscall(__NR_gettid);
    printf("[cct] initializing data structures for thread %d\n", tid);
#endif

#if USE_MALLOC==1        
    _TLS_cct_stack[0] = (cct_node_t*) malloc(sizeof (cct_node_t));
    if (_TLS_cct_stack[0] == NULL) {
        printf("[cct] error while initializing cct root node... Quitting!\n");
        exit(1);
    }
#else
    // initialize custom memory allocator    
    _TLS_cct_pool = pool_init(PAGE_SIZE, sizeof (cct_node_t), &_TLS_cct_free_list);
    if (_TLS_cct_pool == NULL) {
        printf("[cct] error while initializing allocator... Quitting!\n");
        exit(1);
    }

    pool_alloc(_TLS_cct_pool, _TLS_cct_free_list, _TLS_cct_stack[0], cct_node_t);
    if (_TLS_cct_stack[0] == NULL) {
        printf("[cct] error while initializing cct root node... Quitting!\n");
        exit(1);
    }
#endif

    _TLS_cct_stack[0]->first_child = NULL;
    _TLS_cct_stack[0]->next_sibling = NULL;
    _TLS_cct_stack[0]->routine_id = 0;
    _TLS_cct_stack[0]->call_site = 0;
    _TLS_cct_stack[0]->counter = 1;
    _TLS_cct_root = _TLS_cct_stack[0];

#if CCT_TRACK_SPACE == 1
    _TLS_stream_length = 0;
    pid_t tid = syscall(__NR_gettid);
    
    char dumpFileName[BUFLEN + 1];
    if (dumpPath == NULL) {
        sprintf(dumpFileName, "%s/%s-%d.log", DEFAULT_DUMP_PATH, program_invocation_short_name, tid);
    } else {
        sprintf(dumpFileName, "%s/%s-%d.log", dumpPath, program_invocation_short_name, tid);
    }
    
    _TLS_track_log = fopen(dumpFileName, "w+");
    if (_TLS_track_log == NULL) exit((printf("[cct] ERROR: cannot create space-tracking log file %s\n", dumpFileName), 1));    
#endif   
}

#if BURSTING==1

void hcct_align() {
    // reset CCT internal stack
    _TLS_cct_stack_idx = 0;

#if UPDATE_ALONG_TREE==1
    // updates all nodes related to routines actually in the the stack
    int i;
    for (i = 0; i < _TLS_shadow_stack_idx; ++i)
        hcct_enter(_TLS_shadow_stack[i].routine_id, _TLS_shadow_stack[i].call_site);
    _TLS_aligned = 1;
#else
    // uses hcct_enter only on current routine and on nodes that have to be created along the path
    cct_node_t *parent, *node;
    int i;

    // scan shadow stack
#if 0
    for (i = 0; i < _TLS_shadow_stack_idx - 1;) {
        parent = _TLS_cct_stack[_TLS_cct_stack_idx++];
        for (node = parent->first_child; node != NULL; node = node->next_sibling)
            if (node->routine_id == _TLS_shadow_stack[i].routine_id &&
                    node->call_site == _TLS_shadow_stack[i].call_site) break;
        if (node != NULL) {
            // no need to update counter here
            _TLS_cct_stack[_TLS_cct_stack_idx] = node;
            ++i;
        } else {
            // I have to create additional nodes in the next for iteration
            --_TLS_cct_stack_idx;
            break;
        }
    }
#else
    // should be better
    parent = _TLS_cct_stack[_TLS_cct_stack_idx];
    for (i = 0; i < _TLS_shadow_stack_idx - 1;) {
        for (node = parent->first_child; node != NULL; node = node->next_sibling)
            if (node->routine_id == _TLS_shadow_stack[i].routine_id &&
                    node->call_site == _TLS_shadow_stack[i].call_site) break;
        if (node != NULL) {
            // no need to update counter here
            _TLS_cct_stack[++_TLS_cct_stack_idx] = node;
            parent = node;
            ++i;
        } else {
            // I have to create additional nodes in the next for iteration
            break;
        }
    }

#endif

    // update counters only for current routine and for new nodes created along the path
    for (; i < _TLS_shadow_stack_idx; ++i)
        hcct_enter(_TLS_shadow_stack[i].routine_id, _TLS_shadow_stack[i].call_site);

    _TLS_aligned = 1;
#endif
}
#elif PROFILE_TIME==1

void hcct_align(UINT32 increment) {
    // reset CCT internal stack
    _TLS_cct_stack_idx = 0;

    // walk the shadow stack
    int i;
    for (i = 0; i < _TLS_shadow_stack_idx - 1; ++i)
        hcct_enter(_TLS_shadow_stack[i].routine_id, _TLS_shadow_stack[i].call_site, 0);
    hcct_enter(_TLS_shadow_stack[i].routine_id, _TLS_shadow_stack[i].call_site, increment);

}
#endif

#if PROFILE_TIME==0
void hcct_enter(ADDRINT routine_id, ADDRINT call_site)
#else

void hcct_enter(ADDRINT routine_id, ADDRINT call_site, UINT32 increment)
#endif
{
#if CCT_TRACK_SPACE == 1
    ++_TLS_stream_length;
#endif                
    cct_node_t *parent = _TLS_cct_stack[_TLS_cct_stack_idx++];
    if (parent == NULL) { // for instance, after trace_end() some events may still occur!   
#if SHOW_MESSAGES==1    
        printf("[cct] rtn_enter event while CCT not initialized yet for thread %ld\n", syscall(__NR_gettid));
#endif

        hcct_init();

        parent = _TLS_cct_stack[_TLS_cct_stack_idx];
        if (parent == NULL) {
            printf("[hcct] unable to initialize HCCT... Quitting!\n");
            exit(1);
        }
    }

    cct_node_t *node;
    for (node = parent->first_child; node != NULL; node = node->next_sibling)
        if (node->routine_id == routine_id &&
                node->call_site == call_site) break;

    if (node != NULL) {
        _TLS_cct_stack[_TLS_cct_stack_idx] = node;
#if PROFILE_TIME==0
        node->counter++;
#else
        node->counter += increment;
#endif
        return;
    }

    ++_TLS_cct_nodes;

#if CCT_TRACK_SPACE==1
    if (_TLS_cct_nodes % 100000 == 0) {
        fprintf(_TLS_track_log, "[%p] Number of nodes: %.1f M - Number of events: %.1f M\n", &_TLS_cct_nodes, _TLS_cct_nodes / 1000000.0, _TLS_stream_length / 1000000.0);
        fflush(_TLS_track_log);
    }
#endif    

#if USE_MALLOC==1
    node = (cct_node_t*) malloc(sizeof (cct_node_t));
#else    
    pool_alloc(_TLS_cct_pool, _TLS_cct_free_list, node, cct_node_t);
#endif
    _TLS_cct_stack[_TLS_cct_stack_idx] = node;
    node->routine_id = routine_id;
    node->call_site = call_site;
#if PROFILE_TIME==0
    node->counter = 1;
#else
    node->counter = increment;
#endif
    node->first_child = NULL;
    node->next_sibling = parent->first_child;
    parent->first_child = node;
}

void hcct_exit() {
    _TLS_cct_stack_idx--;
}

#if DUMP_TREE==1

static void __attribute__((no_instrument_function)) hcct_dump_aux(FILE* out,
        cct_node_t* root, unsigned long *nodes, UINT64* cct_enter_events, cct_node_t* parent) {
    if (root == NULL) return;

    (*nodes)++;
    (*cct_enter_events) += root->counter;
    cct_node_t* ptr;

    // syntax: v <node id> <parent id> <counter> <routine_id> <call_site>
    // addresses in hexadecimal notation (useful for addr2line, and also to save bytes))
    fprintf(out, "v %lx %lx %lu %lx %lx\n", (unsigned long) root, (unsigned long) (parent),
            root->counter, root->routine_id, root->call_site);

    for (ptr = root->first_child; ptr != NULL; ptr = ptr->next_sibling)
        hcct_dump_aux(out, ptr, nodes, cct_enter_events, root);
}
#endif

#if USE_MALLOC==1

static void __attribute__((no_instrument_function)) free_cct(cct_node_t* node) {
    cct_node_t *ptr, *tmp;
    for (ptr = node->first_child; ptr != NULL;) {
        tmp = ptr->next_sibling;
        free_cct(ptr);
        ptr = tmp;
    }
    free(node);
}
#endif

void hcct_dump() {
    pid_t tid;

#if SHOW_MESSAGES==1
    tid = syscall(__NR_gettid);
    printf("[cct] removing data structures for thread %d\n", tid);
#endif

#if DUMP_TREE==1
    unsigned long nodes = 0;
    UINT64 cct_enter_events = 0;
    FILE* out;
    tid = syscall(__NR_gettid);

    int ds;
    char dumpFileName[BUFLEN + 1];
    if (dumpPath == NULL) {
        sprintf(dumpFileName, "%s/%s-%d.tree", DEFAULT_DUMP_PATH, program_invocation_short_name, tid);
    } else {
        sprintf(dumpFileName, "%s/%s-%d.tree", dumpPath, program_invocation_short_name, tid);
    }
    ds = open(dumpFileName, O_EXCL | O_CREAT | O_WRONLY, 0660);
    if (ds == -1) exit((printf("[cct] ERROR: cannot create output file %s\n", dumpFileName), 1));
    out = fdopen(ds, "w");

#if BURSTING==1
    // c <tool> <sampling_interval> <burst_length> <exhaustive_enter_events>    
    fprintf(out, "c cct-burst %lu %lu %llu\n", sampling_interval, burst_length, _TLS_exhaustive_enter_events);
#elif PROFILE_TIME==1
    // c <tool> <sampling_interval> <thread_tics>
    fprintf(out, "c cct-time %lu %llu\n", sampling_interval, _TLS_thread_tics);
#else
    // c <tool>
    fprintf(out, "c cct \n"); // do not remove the white space between cct and \n           
#endif

    // c <command> <process/thread id> <working directory>
    char* cwd = getcwd(NULL, 0);
    fprintf(out, "c %s %d %s\n", program_invocation_name, tid, cwd);
    free(cwd);

    hcct_dump_aux(out, _TLS_cct_root, &nodes, &cct_enter_events, NULL);
    cct_enter_events--; // root node is a dummy node with counter 1

    // p <nodes> <enter_events>
    fprintf(out, "p %lu %llu\n", nodes, cct_enter_events); // #nodes used for a sanity check in the analysis tool
    fclose(out);
#endif

#if USE_MALLOC==1
    // recursively free the CCT
    free_cct(_TLS_cct_root);
#else
    // free memory used by custom allocator
    pool_cleanup(_TLS_cct_pool);
#endif

    // for instance, after trace_end() some events may still occur! 
    _TLS_cct_root = NULL; // see first instruction in hcct_init()
    _TLS_cct_stack_idx = 0;
    _TLS_cct_stack[0] = NULL; // see if parent==NULL in hcct_enter()

#if SHOW_MESSAGES==1
    printf("[cct] dump completed for thread %ld\n", syscall(__NR_gettid));
#endif
 
#if CCT_TRACK_SPACE == 1
    fclose(_TLS_track_log);
#endif    

}
