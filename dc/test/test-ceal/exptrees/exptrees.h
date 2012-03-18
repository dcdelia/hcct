#include "ceal.h"
//#include "box.h"

typedef enum {PLUS,MINUS,TIMES,DIV} op_t;
typedef enum {LEAF,NODE} kind_t;

typedef long T;

typedef struct {
  kind_t kind;       
  modref_t* left;
  modref_t* right;
  op_t op;      
} node_t ;

typedef struct {
  kind_t kind;       
  T num;
} leaf_t ;


modref_t* exptrees_random(int num_nodes);
leaf_t* exptrees_leaf (T num);
int exptrees_count_leaves (modref_t *tree);
void exptrees_fill_leaf_ptr_array_ext (modref_t *tree, modref_t **leaf_ptr_array);
afun exptrees_eval (modref_t* tree, modref_t* result);

