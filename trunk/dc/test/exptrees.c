#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/times.h>
#include <sys/param.h>

#include "exptrees.h"
#include "profile.h"
#include "dc.h"
#include "pool.h"

#define MAKE_DUMP 0
//#define CHECK_CORRECTNESS 1
#define MALLOC rmalloc

//num nodes...
//#define N 1000000


//global variables...
int g_index;
int g_num_att;

void op_h(node_t* p) {
  
	g_num_att++;
  
	T r_l = p->left->num;
	T r_r = p->right->num;

		 if (p->op == PLUS)  p->num = r_l + r_r; 
	else if (p->op == MINUS) p->num = r_l - r_r; 
	else if (p->op == TIMES) p->num = r_l * r_r; 
	else                     p->num = r_l + r_r; 
}

// leaf constructor
node_t* exptrees_leaf (T num) {
	node_t* p = MALLOC(sizeof(node_t));
	p->num = num;
	p->left = p->right = NULL;
	return p;
}

// node constructor
node_t* exptrees_node (op_t op) {
	node_t* p = MALLOC(sizeof(node_t));
	p->op = op;
	return p;
}

// build a random tree with num_nodes nodes, 
// the tree can have some extra nodes (probably just one).
int exptrees_random_rec(int num_nodes, node_t** dest) {  
	if (num_nodes > 2) {

		node_t* node = exptrees_node (rand() % 4);
		*dest = node;

		int n = num_nodes - 1;
    
		int n_l = exptrees_random_rec (n / 2, &node->left);
		int n_r = exptrees_random_rec (n - n_l, &node->right);

		newcons((cons_t) op_h, node);
		//op_h(node);

		return (n_l + n_r + 1);
	}
	else if (num_nodes == 1) {
		*dest = exptrees_leaf (rand() % 100);
		return 1;
	}
	else if (num_nodes == 2) {
		return (exptrees_random_rec (3, dest));
	}

	return 0;
}


node_t* exptrees_random(int num_nodes) {
	node_t* root;
	exptrees_random_rec (num_nodes, &root);
	return root;
}


void exptrees_print (node_t* root) {
	if (root!=NULL) {
		
		printf ("(");
		exptrees_print (root->left);
		if (root->left==NULL && root->right==NULL)
			printf (" %ld ", root->num);
		else
		{
				 if (root->op == PLUS)  printf ("+");
			else if (root->op == MINUS) printf ("-");
			else if (root->op == TIMES) printf ("*");
			else                        printf ("/");
		}
		exptrees_print (root->right);
		printf (")");
	}  
}

int exptrees_count_leaves (node_t *root)
{
	if (root==NULL)
		return 0;
  
	if (root->left==NULL && root->right==NULL)
		return 1;
	else
		return exptrees_count_leaves (root->left) + exptrees_count_leaves (root->right);
}

void exptrees_fill_leaf_ptr_array_ext (node_t *root, node_t **leaf_ptr_array)
{
    g_index=0;
    exptrees_fill_leaf_ptr_array (root, leaf_ptr_array);
}

// set g_index=0 before calling!!!
void exptrees_fill_leaf_ptr_array (node_t *root, node_t **leaf_ptr_array)
{
	if (root==NULL)
		return;
  
	if (root->left==NULL && root->right==NULL) {    
		leaf_ptr_array[g_index]=root;
		g_index++;
	}
	else {
		exptrees_fill_leaf_ptr_array (root->left, leaf_ptr_array);
		exptrees_fill_leaf_ptr_array (root->right, leaf_ptr_array);
	}
}

T exptrees_conv_eval (node_t *root) {

	if (root->left==NULL && root->right==NULL)
		return root->num;
  
	if (root->op==PLUS)
		return exptrees_conv_eval (root->left) + exptrees_conv_eval (root->right);
	if (root->op==MINUS)
		return exptrees_conv_eval (root->left) - exptrees_conv_eval (root->right);
	if (root->op==TIMES)
		return exptrees_conv_eval (root->left) * exptrees_conv_eval (root->right);
	if (root->op==DIV)
		return exptrees_conv_eval (root->left) + exptrees_conv_eval (root->right);

    return 0;
}

#if 0
int main () {

	node_t *theTreeRoot, **theLeafPtrArray;
	int theNumLeaves, i;
	T theBackup, theFirstResult, theFirstConvResult;

	// time structs
    time_rec_t start_time, end_time;

	printf ("Total number of cached instructions = %u\n", g_stats.cache_instr_number);

	#if MAKE_DUMP
    rm_make_dump_file("logs/exptrees-start.dump");
	#endif

	// initializes rand generator...
	srand (123);

	// fetches initial time...
    get_time(&start_time);

	// creates exptree...
	theTreeRoot = exptrees_random (N);

    // fetches final time
    get_time(&end_time);
    dump_elapsed_time(&start_time, &end_time, "Total time taken for tree and cons creation");
	printf ("Total number of cached instructions executed so far = %u\n", g_stats.exec_cache_instr_count);

	// prints input size...
	printf ("\nN = %d\n", N);

	// counts leaves...
	theNumLeaves=exptrees_count_leaves (theTreeRoot);
	printf ("Num leaves = %d\n\n", theNumLeaves);

	// prints tree structure (for debugging purposes)...
    #if 0
	printf ("Tree structure:\n");
	exptrees_print (theTreeRoot);
	printf ("\n\n");
    #endif

	// result of first eval...
	printf ("Result of tree evaluation = %ld\n", theTreeRoot->num);
	printf ("Total number of cached instructions executed so far = %u\n", g_stats.exec_cache_instr_count);
	theFirstResult=theTreeRoot->num;

    #if CHECK_CORRECTNESS
	theFirstConvResult=exptrees_conv_eval (theTreeRoot);
	printf ("Result of conventional eval = %ld\n", theFirstConvResult);
	printf ("Number of constraint evaluations = %d\n\n", g_num_att);
  	printf ("Total number of cached instructions executed so far = %u\n", g_stats.exec_cache_instr_count);
    #endif

	// allocates mem for leaf pointer array...
	theLeafPtrArray=(node_t **)malloc (theNumLeaves*sizeof (node_t *));
  
	// fills leaf pointer array...
	g_index=0;
	exptrees_fill_leaf_ptr_array(theTreeRoot, theLeafPtrArray);
  
	// fetches initial time...
    get_time(&start_time);

	// test loop...
    int j;
    for (j=0; j<2; j++) {
        printf ("Running test loop %d\n", j);
        for (i=0; i<theNumLeaves; i++) {
            int curr_att = g_num_att;

            theBackup = (theLeafPtrArray[i])->num;

            begin_at();
            (theLeafPtrArray[i])->num=100;
            //printf ("Result of tree evaluation = %d\n\n", theTreeRoot->num);   
            end_at();

            begin_at();
            (theLeafPtrArray[i])->num=theBackup;
            //printf ("Result of tree evaluation = %d\n\n", theTreeRoot->num);
            end_at();

            if (theFirstResult!=theTreeRoot->num) {
                printf ("New tree value at iteration %d: %ld\n", i, theTreeRoot->num);
                printf ("Conventional value: %ld\n", exptrees_conv_eval (theTreeRoot));
                printf ("# of activations: %d\n", g_num_att-curr_att);
            } 
    
            if (i==theNumLeaves-1 && theFirstResult!=theTreeRoot->num)
                printf ("New tree value in test loop = %ld\n\n", theTreeRoot->num);
        }
    }
  
    // fetches final time
    get_time(&end_time);
    dump_elapsed_time(&start_time, &end_time, "Total time taken for all updates");
	printf ("Total number of cached instructions executed so far = %u\n", g_stats.exec_cache_instr_count);
  
	// just checking...
	printf ("Final tree value = %ld\n", theTreeRoot->num);
	printf ("Total number of constraint evaluations = %d\n", g_num_att);

	#if MAKE_DUMP
    rm_make_dump_file("logs/exptrees-end.dump");
	#endif

    dump_proc_status("Process status before exiting:");

	//cleanup...
	free (theLeafPtrArray);
  
	return 0;
}
#endif

