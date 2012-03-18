#include <stdlib.h>
#include "ceal.h"
#include "exptrees.h"
//#include "box.h"

//global variables...
int g_index;

// leaf initializer
ifun exptrees_leaf_init (leaf_t* p, T num) {
  p->kind = LEAF;
  p->num = num;
}

// leaf allocator
leaf_t* exptrees_leaf (T num) {
  return alloc (sizeof (leaf_t), &exptrees_leaf_init, num);
}

// node initializer
ifun exptrees_node_init (node_t *p, op_t op) {
  p->kind = NODE;
  p->op = op;
  
  p->left = modref ();
  p->right = modref ();
}

// node allocator
node_t* exptrees_node (op_t op) {
  return (alloc (sizeof (node_t), &exptrees_node_init, op));
}

// build a random tree with num_nodes nodes, 
// the tree can have some extra nodes (probably just one).
int exptrees_random_rec(int num_nodes, modref_t* dest) {  
  if(num_nodes > 2) {

    node_t* node = exptrees_node (rand() % 4);

    int n = num_nodes - 1;
    
    int n_l = exptrees_random_rec (n / 2, node->left);
    int n_r = exptrees_random_rec (n - n_l, node->right);

    write (dest,node);
    return (n_l + n_r + 1);
  }
  else if (num_nodes == 1) {
    leaf_t* leaf = exptrees_leaf (rand () % 100);
    write (dest, leaf);
    return 1;
  }
  else if (num_nodes == 2) {
    return (exptrees_random_rec (3, dest));
  }
  abort();
  return 0;
}

modref_t* exptrees_random(int num_nodes) {
  modref_t* root = modref();
  exptrees_random_rec (num_nodes, root);
  return root;
}

afun exptrees_eval_2 (node_t* t, T r_l, T r_r, modref_t* result) {
  //float r_l = a->u._float;
  //float r_r = b->u._float;

  if (t->op == PLUS) {
    write (result, r_l + r_r); 
  }
  else if (t->op == MINUS) {
    write (result, r_l - r_r); 
  }
  else if (t->op == TIMES) {
    write (result, r_l * r_r); 
  }
  else {
    write (result, r_l + r_r);
  }
}  

afun exptrees_eval (modref_t* tree, modref_t* result) {
  
  node_t* t = read (tree);
   
  if (t->kind == LEAF) {
    leaf_t* l = (leaf_t*) t;
    write (result, l->num/*Box_float(l->num)*/);
  }
  else {
    modref_t* r_left = modref ();
    modref_t* r_right = modref ();
    
    exptrees_eval(t->left, r_left);
    exptrees_eval(t->right, r_right);
    exptrees_eval_2(t, read(r_left), read(r_right), result);
  }
}

int exptrees_count_leaves (modref_t *tree)
{
  node_t *root, *left, *right;  
  
  root=modref_deref (tree);
  
  if (root==NULL)
    return 0;
  
  if (root->kind==LEAF)
    return 1;
 
  return exptrees_count_leaves (root->left) + exptrees_count_leaves (root->right);
}

void exptrees_fill_leaf_ptr_array_ext (modref_t *tree, modref_t **leaf_ptr_array)
{
    g_index=0;
    exptrees_fill_leaf_ptr_array (tree, leaf_ptr_array);
}

//set g_index=0 before calling!!!
void exptrees_fill_leaf_ptr_array (modref_t *tree, modref_t **leaf_ptr_array)
{
  node_t *root;
  
  root=modref_deref (tree);
  
  if (root==NULL)
    return;
  
  if (root->kind==LEAF)
  {    
    leaf_ptr_array[g_index]=tree;
    g_index++;
  }
  else
  {
    exptrees_fill_leaf_ptr_array (root->left, leaf_ptr_array);
    exptrees_fill_leaf_ptr_array (root->right, leaf_ptr_array);
  }
}

#if 0
void dump_proc_status() {
	pid_t thePID;
	char theBase[]="cat /proc/";
	char theComplete[1000];
	char theBuffer[1000];

	printf ("\nProcess status before exiting:\n\n");

	/*gets pid...*/
	thePID = getpid ();
	/*generates filename...*/
	strcpy (theComplete, theBase);
	sprintf (theBuffer, "%u", thePID);
	strcat (theComplete, theBuffer);
	strcat (theComplete, "/status");
	int res = system (theComplete);
	fflush (stdout);
}
#endif

//num nodes...
//#define N 1000000

#if 0
int main () {
    slime_t *comp=slime_open ();
  
    //this modref is created by exptrees_random()...
    modref_t *theTreeRoot;

    //THIS modref must be created by user...
    modref_t *theFirstResult=modref ();
    T theFirstResultBox, theFinalResultBox;
  
    leaf_t *theBackup, *theModLeaf=exptrees_leaf (100);
  
    modref_t **theLeafPtrArray;
  
    int theNumLeaves, i;
  
    struct timeval tvBegin, tvEnd;
    double theLoopTime;
  
    //initializes rand generator...
    //srand((unsigned)time (NULL));
    srand (123);
  
    /*fetches initial time...*/
    gettimeofday (&tvBegin, NULL);
    //creates exptree...
    theTreeRoot=exptrees_random (N);
  
    //evaluates it...
    exptrees_eval (theTreeRoot, theFirstResult);
  
    /*fetches final time...*/
    gettimeofday (&tvEnd, NULL);
    theLoopTime=tvEnd.tv_sec+(tvEnd.tv_usec*0.000001)
            -tvBegin.tv_sec-(tvBegin.tv_usec*0.000001);
    printf ("Total seconds taken for tree construction and from-scratch computation: %f\n", theLoopTime);
  
    //prints input size...
    printf ("\nN = %d\n", N);
  
    //counts leaves...
    theNumLeaves=exptrees_count_leaves (theTreeRoot);
    printf ("Num leaves = %d\n\n", theNumLeaves);
  
    //prints result of first eval...
    theFirstResultBox=(T)modref_deref (theFirstResult);
    printf ("Result of tree evaluation: %d\n", theFirstResultBox);
  
    //allocates mem for leaf pointer array...
    theLeafPtrArray=(node_t **)malloc (theNumLeaves*sizeof (modref_t *));
  
    //fills leaf pointer array...
    g_index=0;
    exptrees_fill_leaf_ptr_array (theTreeRoot, theLeafPtrArray);
  
    //test loop...
    /*fetches initial time...*/
    gettimeofday (&tvBegin, NULL);
    int j;
    for (j=0; j<2; j++) {
        printf ("Running test loop %d\n", j);
        for (i=0; i<theNumLeaves; i++) {
            slime_meta_start ();    
            theBackup=modref_deref (theLeafPtrArray[i]); 
            write (theLeafPtrArray[i], theModLeaf);
            slime_propagate ();
    
            slime_meta_start ();
            write (theLeafPtrArray[i], theBackup);
            slime_propagate ();
        }
    }

    /*fetches final time...*/
    gettimeofday (&tvEnd, NULL);
    theLoopTime=tvEnd.tv_sec+(tvEnd.tv_usec*0.000001)
                -tvBegin.tv_sec-(tvBegin.tv_usec*0.000001);
    printf ("Total seconds taken: %f\n", theLoopTime);
    //printf ("Avg secs over %d updates: %f\n\n", theNumLeaves, theLoopTime/theNumLeaves);
  
    //prints result of first eval...
    theFinalResultBox=(T)modref_deref (theFirstResult);
    printf ("Final tree value: %d\n", theFinalResultBox);
  
    dump_proc_status();

    //cleanup...
    free (theLeafPtrArray);
    slime_close (comp);

    return 0;
}
#endif
