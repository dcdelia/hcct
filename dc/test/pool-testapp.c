#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "pool.h"

#define MAX_BLOCKS  25
#define OPERATIONS  (MAX_BLOCKS*10)
#define PAGE_SIZE   10
#define SEED        971

typedef struct {
    long x, y;
} rec_t;

int main() {
    
    int i;
    rec_t* rec[MAX_BLOCKS];

    void* f;
    pool_t* p = pool_init(PAGE_SIZE, sizeof(rec_t), &f);

    pool_dump(p, f, 1);

    for (i=0; i < MAX_BLOCKS; ++i) {
        pool_alloc(p, f, rec[i], rec_t);
        memset(rec[i], 0xFF, sizeof(rec_t));
        pool_dump(p, f, 1);
    }

    srand(SEED);

    for (i=0; i < OPERATIONS; ++i) {
        int j = rand() % MAX_BLOCKS;
        if (rec[j]) {
            pool_free(rec[j], f);
            memset((char*)rec[j]+sizeof(void*), 0x00, sizeof(rec_t)-sizeof(void*));
            rec[j] = NULL;
            pool_dump(p, f, 1);
        }
        else {
            pool_alloc(p, f, rec[j], rec_t);
            memset(rec[j], 0xFF, sizeof(rec_t));
            pool_dump(p, f, 1);
        }
    }

    pool_dump(p, f, 1);

    return 0;
}
