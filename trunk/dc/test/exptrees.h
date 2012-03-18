typedef enum {PLUS,MINUS,TIMES,DIV} op_t;

typedef long T;

typedef struct tagnode_t {
    T num;
    op_t op;
    struct tagnode_t* left;
    struct tagnode_t* right;
} node_t;
 
node_t* exptrees_random(int num_nodes);
int exptrees_count_leaves (node_t *root);
void exptrees_fill_leaf_ptr_array_ext (node_t *root, node_t **leaf_ptr_array);
void exptrees_fill_leaf_ptr_array (node_t *root, node_t **leaf_ptr_array);
T exptrees_conv_eval (node_t *root);

