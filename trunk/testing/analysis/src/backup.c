typedef struct sym_pair_s sym_pair_t;
struct sym_pair_s {
    hcct_sym_t* routine;
    hcct_sym_t* call_site;    
};

static int compareSymbolSimilarity(hcct_sym_t* first, hcct_sym_t* second) {
    // for now let's assume they are equal if they match name and file when
    // those fields can be solved; otherwise check image and offset
    if (strcmp(first->name, SYM_UNRESOLVED_NAME)) {
        int names = strcmp(first->name, second->name);
        if (!names) {
            return strcmp(first->file, second->file);
        } else return names;
    } else {
        if (!strcmp(first->image, second->image) && (first->offset == second->offset)) {
            return 0;
        } else {
            return -1;
        }
    }
}

static int distinctRoutinesCompare(const void* a, const void* b) {
    sym_pair_t* first = (sym_pair_t*) a;
    sym_pair_t* second = (sym_pair_t*) b;
    
    return compareSymbolSimilarity(first->routine, second->routine);        
}

static void distinctRoutinesAndCallSitesAuxOld(hcct_node_t* node, sym_pair_t* tmp_array, int* index) {
    int offset = *index;
    //printf("%d\n", offset);    
    //printf("$p\n", pair);    
    tmp_array[offset].routine = node->routine_sym;
    tmp_array[offset].call_site = node->call_site_sym;
    ++(*index);
        
    hcct_node_t* child;
    for (child = node->first_child; child != NULL; child = child->next_sibling)
        distinctRoutinesAndCallSitesAuxOld(child, tmp_array, index);
}

static void distinctRoutinesAndCallSitesOld(hcct_tree_t* tree, UINT32* routines, UINT32* call_sites) {
    UINT32 nodes = tree->nodes;
    int index = 0;
    sym_pair_t* syms_array = malloc(nodes * sizeof(sym_pair_t));
       
    distinctRoutinesAndCallSitesAuxOld(tree->root, syms_array, &index);
    
    if (index != nodes) {
        printf("ERRORE POTENTE! :-)\n");
    }
        
    qsort(syms_array, nodes, sizeof(sym_pair_t), distinctRoutinesCompare);
    // I probably should use GHashLib here: comparator is broken!
    
    *routines = 0; // one is <dummy>
    printf("Routine -1: %s\n", syms_array[0].routine->name);
    for (index = 1; index < nodes; ++index) 
        if (compareSymbolSimilarity(syms_array[index].routine, syms_array[index-1].routine)) {
            printf("Routine %lu: %s\n", *routines, syms_array[index].routine->name);
            ++(*routines);            
        }    
    
    for (index = 0; index <= 200; index+=10) {
        //printf("[%d] %s [%s]\n", index, syms_array[index].routine->name, syms_array[index].routine->file);
    }
    
    //printf("--> %d\n", compareSymbolSimilarity(syms_array[120].routine, syms_array[200].routine));
    
    *call_sites = 1;
    // TODO this code is all broken       
    
    free(syms_array);
}

static int compareSymbols(hcct_sym_t* first, hcct_sym_t* second) {
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
    
        /*
    int first_unresolved = strcmp(first->name, SYM_UNRESOLVED_NAME);
    int second_unresolved = strcmp(second->name, SYM_UNRESOLVED_NAME);
    
    if (first_unresolved) {
        if (second_unresolved) {
            // both symbols were unresolved: compare images
            // TODO: unresolved image
            int images = strcmp(first->image, second->image);
            if (!images) {
                // same image: compare offsets
                return (first->offset - second->offset);
            } else {
                // they belong to different images
                return images;
            }            
        } else {
            // first symbol unresolved, second resolved
            return -256;
        }
    } else {
        if (second_unresolved) {
            // first symbol solved, second unresolved
            return 256;
        } else {
            int names = strcmp(first->name, second->name);
            if (!names) {
                return strcmp(first->file, second->file);
            } else {
                return names;
            }    
        }
    } */ 
}

static int compareSymbols2(hcct_sym_t* first, hcct_sym_t* second) {
    //return strcmp(first->name, second->name);
    return 0;
}

static int distinctRoutinesAndCallSitesCompare(const void* a, const void* b) {
    hcct_node_t* first = (hcct_node_t*) a;
    hcct_node_t* second = (hcct_node_t*) b;                
    
    //printf("%lu %lu\n", first->counter, second->counter);    
    
    //printf("%p %p\n", first->routine_sym->name, second->routine_sym->name); // SEGFAULT here           
    return first->counter - second->counter;
            
    //int ret = compareSymbols2(first->routine_sym, second->routine_sym);
    //if (!ret) {
        //return compareSymbols(first->call_site_sym, second->call_site_sym);
    //}    
    //return ret;
}

static void distinctRoutinesAndCallSitesAux(hcct_node_t* node, hcct_node_t** nodes_array, int* index) {   
    int i = *index;
    nodes_array[i] = node;
    ++(*index); // TODO
   
    hcct_node_t* child;
    for (child = node->first_child; child != NULL; child = child->next_sibling)
        distinctRoutinesAndCallSitesAux(child, nodes_array, index);
}

static void distinctRoutinesAndCallSites(hcct_tree_t* tree, UINT32* routines, UINT32* call_sites) {
    UINT32 nodes = tree->nodes;
    int index = 0;
    hcct_node_t** nodes_array = malloc(nodes * sizeof(hcct_node_t*));
    distinctRoutinesAndCallSitesAux(tree->root, nodes_array, &index);
    
    printf("%d\n", index);
        
    for (index = 0; index < nodes; ++index) {
        /*
        printf("%s ", nodes_array[index]->routine_sym->name);
        printf("%s ", nodes_array[index]->routine_sym->file);
        printf("%s ", nodes_array[index]->routine_sym->image);
        printf("%s ", nodes_array[index]->call_site_sym->name);
        printf("%s ", nodes_array[index]->call_site_sym->file);
        printf("%s\n", nodes_array[index]->call_site_sym->image);
        */
    }
           
    printf("DONE\n");
        
    // using nodes or index is the same here
    qsort(*nodes_array, index, sizeof(hcct_node_t*), distinctRoutinesAndCallSitesCompare);
    
    *routines = 0; // one is <dummy>
    /*
    printf("Routine -1: %s\n", nodes_array[0]->routine_sym->name);
    for (index = 1; index < nodes; ++index) 
        if (compareSymbols(nodes_array[index]->routine_sym, nodes_array[index-1]->routine_sym)) {
            printf("Routine %lu: %s\n", *routines, nodes_array[index]->routine_sym->name);
            ++(*routines);            
        }
    */
    *call_sites = 1;
}
