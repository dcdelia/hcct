#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "symbols.h"

#define CLEANUP_VERSION 0

// auxiliary methods (for internal use only)
static int parseMemoryMapLine(char* line, ADDRINT *start, ADDRINT *end, ADDRINT *offset, char** pathname);
static void dumpMapToFileIterator(gpointer key, gpointer data, gpointer p);
#if CLEANUP_VERSION != 0
static int freeSymbolsComparator(const void* a, const void* b);
#endif
static void freeSymbolsIterator(gpointer key, gpointer data, gpointer p);
static regions_list_t* createRegionsList(char* program);
static void freeRegionsList(regions_list_t* list);
static sym_t* getFunctionFromAddress(ADDRINT addr, regions_list_t* list, char* program);
static void freeSym(sym_t* sym);

syms_map_t* readMapFromFile(FILE* file) {
    char buf[BUFLEN + 1];
    char *s;

    syms_map_t* map = malloc(sizeof(syms_map_t));
    map->regions_list = NULL; // not available in offline (i.e. using pre-computed information) mode

    // two dirty macros :-)
    #define brokenFile      { printf("[ERROR] Broken symbols file\n"); exit(1); }
    #define wrongFormat     { printf("[ERROR] Wrong format for symbols file!\n"); exit(1); }

    /* Retrieve header*/
    if (fgets(buf, BUFLEN, file) == NULL) brokenFile

    s = strtok(buf, " ");
    if (strcmp(s, "[program]")) wrongFormat
    s = strtok(NULL, "\n");
    map->program = strdup(s);

    if (fgets(buf, BUFLEN, file) == NULL) brokenFile
    s = strtok(buf, " ");
    if (strcmp(s, "[maps_file]")) wrongFormat
    s = strtok(NULL, "\n");
    map->mapfile = strdup(s);

    UINT32 symbols = 0;
    if (fgets(buf, BUFLEN, file) == NULL) brokenFile
    symbols = strtoul(buf, NULL, 0);

    /* Process all the symbols*/
    map->addresses = g_hash_table_new(g_direct_hash, g_direct_equal);

    while (symbols-- > 0) {
        ADDRINT address = 0;
        sym_t* sym = malloc(sizeof(sym_t));

        if (fgets(buf, BUFLEN, file) == NULL) brokenFile
        address = (ADDRINT)strtoul(buf, NULL, 16);

        if (fgets(buf, BUFLEN, file) == NULL) brokenFile
        sym->name = strdup(strtok(buf, "\n"));

        if (fgets(buf, BUFLEN, file) == NULL) brokenFile
        sym->file = strdup(strtok(buf, "\n"));

        if (fgets(buf, BUFLEN, file) == NULL) brokenFile
        sym->image = strdup(strtok(buf, "\n"));

        if (fgets(buf, BUFLEN, file) == NULL) brokenFile
        sym->offset = (ADDRINT)strtoul(buf, NULL, 16);

        g_hash_table_insert(map->addresses, GINT_TO_POINTER((int)address), sym);
    }

    if (fgets(buf, BUFLEN, file) != NULL) brokenFile

    #undef brokenFile
    #undef wrongFormat

    return map;
}

syms_map_t* initializeSymbols(char* program, char* mapfile) {
    syms_map_t* map = malloc(sizeof(syms_map_t));

    map->program = strdup(program);
    map->mapfile = strdup(mapfile);

    map->addresses = g_hash_table_new(g_direct_hash, g_direct_equal);

    if (strcmp(mapfile, "none")) {
        map->regions_list = createRegionsList(mapfile);
    } else {
        map->regions_list = NULL;
        printf("Warning: no maps file available - addresses might not be resolved!\n");
    }

    return map;
}

void manuallyAddSymbol(syms_map_t* syms_map, ADDRINT address, sym_t* sym) {
    g_hash_table_insert(syms_map->addresses, GINT_TO_POINTER((int)address), sym);
}

sym_t* getSolvedSymbol(ADDRINT addr, syms_map_t* sym_map) {
    return (sym_t*)g_hash_table_lookup(sym_map->addresses, GINT_TO_POINTER((int)addr));
}

sym_t* solveAddress(ADDRINT addr, syms_map_t* sym_map) {
    sym_t* sym = (sym_t*)g_hash_table_lookup(sym_map->addresses, GINT_TO_POINTER((int)addr));
    if (sym == NULL) {
        sym = getFunctionFromAddress(addr, sym_map->regions_list, sym_map->program);
        g_hash_table_insert(sym_map->addresses, GINT_TO_POINTER((int)addr), sym);
    }
    return sym;
}

void dumpMapToFile(syms_map_t* sym_map, FILE* out) {
    // header
    fprintf(out, "[program] %s\n", sym_map->program);
    fprintf(out, "[maps_file] %s\n", sym_map->mapfile);
    fprintf(out, "%lu\n", (UINT32)g_hash_table_size(sym_map->addresses));

    g_hash_table_foreach(sym_map->addresses, (GHFunc)dumpMapToFileIterator, out);
}

void freeSymbols(syms_map_t* sym_map) {
    if (sym_map == NULL) return;

    guint num_addresses = g_hash_table_size(sym_map->addresses);

    if (num_addresses > 0) { // g_hash_table_foreach would make the program crash!
        #if CLEANUP_VERSION == 0
        g_hash_table_foreach(sym_map->addresses, (GHFunc)freeSymbolsIterator, NULL);
        #elif CLEANUP_VERSION == 1
        // I wrote this complicated code for future versions, so I will keep it here :-)
        void** dump = malloc(num_addresses * sizeof(void*));

        void** pair = malloc(2*sizeof(void*));
        pair[0] = dump;
        pair[1] = 0;  // super dirty - works for IA32 only :-)

        g_hash_table_foreach(sym_map->addresses, (GHFunc)freeSymbolsIterator, pair);

        qsort(dump, num_addresses, sizeof(void*), freeSymbolsComparator);
        int index = 0;
        freeSym(dump[index]);
        for (index = 1 ; index < num_addresses ; ++index) {
            if (dump[index] != dump[index-1]) { // don't free memory twice!
                freeSym(dump[index]);
            }
        }

        free(pair);
        free(dump);
        #else // equivalent to version 1 - much better code
        sym_t** dump = malloc(num_addresses * sizeof(sym_t*));
        sym_t** cur_ptr = dump;
        g_hash_table_foreach(sym_map->addresses, (GHFunc)freeSymbolsIterator, &cur_ptr);
        guint entries = cur_ptr - dump;
        if (entries != num_addresses) {
            printf("[ERROR] Code is broken!\n"); exit(1);
        }
        qsort(dump, entries, sizeof(sym_t*), freeSymbolsComparator);
        int index = 0;
        freeSym(dump[index]);
        for (index = 1 ; index < entries ; ++index) {
            if (dump[index] != dump[index-1]) { // don't free memory twice!
                freeSym(dump[index]);
            }
        }
        free(dump);
        #endif

    }

    freeRegionsList(sym_map->regions_list);
    g_hash_table_destroy(sym_map->addresses);
    free(sym_map->mapfile);
    free(sym_map->program);
    free(sym_map);

    printf("Symbols have been deleted from memory.\n");
}

int compareSymbols(sym_t* first, sym_t* second) {
    // aspetti in gioco: SYM_UNRESOLVED_IMAGE, SYM_UNKNOWN, SYM_UNRESOLVED_NAME
    int names = strcmp(first->name, second->name);
    if (!names) { // symbols have the same name
        if (!strcmp(first->name, SYM_UNRESOLVED_NAME)) {
            // both are partially solved: compare images and then offsets
            int images = strcmp(first->image, second->image);
            if (!images) {
                // same image: compare offsets
                return first->offset - second->offset;
            } else {
                // different images
                return images;
            }
        } else if (!strcmp(first->name, SYM_UNKNOWN)) {
            return 0; // they both weren't solved
        } else {
            // both are completely solved: compare file
            return strcmp(first->file, second->file);
        }
    } else {
        /* Multiple scenarios here:
         * 1) Both symbols are completely solved, and they correspond to different functions
         * 2) One symbol is completely solved, the other is either partially solved or unknown
         * 3) One symbol is partially solved, the other is unknown
        */
        return names;
    }
}

/* Create a data structure for mapped memory regions */
static regions_list_t* createRegionsList(char* filename) {
    FILE* maps_file;

    maps_file = fopen(filename, "r");
    if (maps_file == NULL) {
        printf("Warning: unable to load file containing memory mapped regions!\n");
        free(filename);
        return NULL;
    }

    // parse file
    char buf[BUFLEN + 1];

    regions_list_t* first = (regions_list_t*) malloc(sizeof (regions_list_t));
    first->next = NULL;

    if (fgets(buf, BUFLEN, maps_file) == NULL) {
        printf("Warning: file containing memory mapped regions is broken!\n");
        free(filename);
        free(first);
        return NULL;
    }

    int ret;
    ret = parseMemoryMapLine(buf, &(first->start), &(first->end), &(first->offset), &(first->pathname));
    if (ret) {
        printf("Error: map file seems to be broken!\n");
        exit(1);
    }

    regions_list_t *next = first, *tmp;

    while (fgets(buf, BUFLEN, maps_file) != NULL) {
        tmp = (regions_list_t*) malloc(sizeof(regions_list_t));
        ret = parseMemoryMapLine(buf, &(tmp->start), &(tmp->end), &(tmp->offset), &(tmp->pathname));
        if (ret) {
            printf("Error: map file seems to be broken!\n");
            exit(1);
        }
        next->next = tmp;
        next = tmp;
    }

    next->next = NULL;

    fclose(maps_file);

    return first;
}

/* Free memory in use by a map data structure */
static void freeRegionsList(regions_list_t* list) {
    regions_list_t *l1, *l2;

    for (l1 = list; l1 != NULL;) {
        l2 = l1->next;
        free(l1->pathname);
        free(l1);
        l1 = l2;
    }
}

/* Try to solve an address */
static sym_t* getFunctionFromAddress(ADDRINT addr, regions_list_t* list, char* program) {
    FILE* fp;
    char buf[BUFLEN + 1], command[BUFLEN + 1];
    sym_t* sym = malloc(sizeof (sym_t));

    sprintf(command, "addr2line --e %s -f -C %lx", program, addr);
    fp = popen(command, "r");
    if (fp == NULL) {
        printf("Error: unable to execute addr2line!\n");
        exit(1);
    }

    if (fgets(buf, BUFLEN, fp) != NULL) {
        if (buf[0] != '?' && buf[1] != '?') { // addr2line worked!
            buf[strlen(buf) - 1] = '\0';
            sym->name = strdup(buf);
            if (fgets(buf, BUFLEN, fp) != NULL) { // addr2line will produce two output lines
                buf[strlen(buf) - 1] = '\0';
                sym->file = strdup(buf);
                sym->image = strdup(program);
                pclose(fp);
                // get offset
                for (; list != NULL; list = list->next) {
                    if (addr >= list->start && addr <= list->end) {
                        sym->offset = addr - list->start;
                    }
                }
                return sym;
            } else {
                printf("Error: addr2line does not work!\n");
                exit(1);
            }
        } // else: try to solve it later
    } else {
        printf("Error: addr2line does not work!\n");
        exit(1);
    }

    pclose(fp);

    // try to solve the address using the map file
    // try to solve the address using the map file
    for (; list != NULL; list = list->next) {
        if (addr >= list->start && addr <= list->end) {
            // get offset
            sym->offset = addr - list->start;

            if (list->pathname[0] != '[' && strcmp(list->pathname, "<unknown>")) { // avoid [heap], [stack] ...
                sprintf(command, "addr2line -e %s -f -C %lx 2> /dev/null", list->pathname, sym->offset);
                fp = popen(command, "r");
                if (fp == NULL) {
                    printf("Error: unable to execute addr2line!\n");
                    exit(1);
                }

                // addr2line on a shared library
                if (fgets(buf, BUFLEN, fp) != NULL) {
                    if (buf[0] != '?' && buf[1] != '?') { // addr2line worked!
                        buf[strlen(buf) - 1] = '\0';
                        sym->name = strdup(buf);
                        if (fgets(buf, BUFLEN, fp) != NULL) { // addr2line will produce two output lines
                            buf[strlen(buf) - 1] = '\0';
                            sym->file = strdup(buf);
                            sym->image = strdup(list->pathname);
                            pclose(fp);
                            return sym;
                        } else {
                            printf("Error: addr2line does not work!\n");
                            exit(1);
                        }
                    }
                }

                // addr2line not executed, address not recognized or invalid format
                sym->name = strdup(SYM_UNRESOLVED_NAME);
                sym->file = strdup(SYM_UNRESOLVED_FILE);
                if (!strcmp(list->pathname, "<unknown>")) {
                    sym->file = strdup(SYM_UNRESOLVED_IMAGE);
                } else {
                    sym->image = strdup(list->pathname);
                }

                pclose(fp);
                return sym;
            } else { // [heap], [stack], [...] or <unknown> (?)
                sym->name = strdup(SYM_UNRESOLVED_NAME);
                sym->file = strdup(SYM_UNRESOLVED_FILE);
                if (!strcmp(list->pathname, "<unknown>")) {
                    sym->file = strdup(SYM_UNRESOLVED_IMAGE);
                } else {
                    sym->image = strdup(list->pathname);
                }
                return sym;
            }
        }
    }

    // address is not in the map!
    sym->name = strdup(SYM_UNKNOWN);
    sym->file = strdup(SYM_UNKNOWN);
    sym->image = strdup(SYM_UNKNOWN);
    sym->offset = 0;

    return sym;
}

/* Free memory in use by a symbol data structure */
static void freeSym(sym_t* sym) {
    if (sym == NULL) return;
    free(sym->name);
    free(sym->file);
    free(sym->image);
    free(sym);
}


#if CLEANUP_VERSION == 0
static void freeSymbolsIterator(gpointer key, gpointer data, gpointer p) {
    freeSym((sym_t*) data);
}
#else
static int freeSymbolsComparator(const void* a, const void* b) {
    return a - b;
}

static void freeSymbolsIterator(gpointer key, gpointer data, gpointer p) {
    #if CLEANUP_VERSION == 1
    sym_t* sym = (sym_t*) data;
    void** pair = (void**) p; // TODO ???
    // pair contains a pointer to the dump array and the index to write at
    void** dump = pair[0];
    int index = (int)(pair[1]);
    dump[index] = sym;
    pair[1] = (void*)(index + 1);
    #else
    sym_t*** ptr_to_cur_ptr = (sym_t***)p;
    sym_t** cur_ptr = *ptr_to_cur_ptr;
    *cur_ptr = (sym_t*)data;
    *ptr_to_cur_ptr = cur_ptr + 1;
    #endif
}
#endif

static void dumpMapToFileIterator(gpointer key, gpointer data, gpointer p) {
    FILE* out = (FILE*) p;
    ADDRINT address = (ADDRINT) key;
    sym_t* sym = (sym_t*) data;

    fprintf(out, "%lx\n", address);
    fprintf(out, "%s\n", sym->name);
    fprintf(out, "%s\n", sym->file);
    fprintf(out, "%s\n", sym->image);
    fprintf(out, "%lx\n", sym->offset);
}

/* process a line from a file with syntax as in /proc/<PID>/maps files */
static int parseMemoryMapLine(char* line, ADDRINT *start, ADDRINT *end, ADDRINT *offset, char** pathname) {

    ADDRINT tmp;
    char *s;

    // syntax: <start_address-end_address> <perms> <offset> <dev> <inode> <pathname>

    // start address
    s = strtok(line, "-");
    tmp = strtoul(s, NULL, 16);
    if (tmp == 0) return 1;
    *start = tmp;

    // end address
    s = strtok(NULL, " ");
    tmp = strtoul(s, NULL, 16);
    if (tmp == 0) return 1;
    *end = tmp;

    s = strtok(NULL, " "); // skip permissions column

    // offset
    s = strtok(NULL, " ");
    tmp = strtoul(s, NULL, 16);
    *offset = tmp;

    s = strtok(NULL, " "); // skip dev column
    s = strtok(NULL, " "); // skip inode column

    // pathname
    s = strtok(NULL, "\0");
    if (strlen(s) == 1) {
        *pathname = strdup("<unknown>");
    } else {
        s[strlen(s) - 1] = '\0';
        *pathname = strdup(s);
    }

    return 0;
}

