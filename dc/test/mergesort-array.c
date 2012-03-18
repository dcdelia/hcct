#include <stdlib.h>
#include <stdio.h>
#include "dc.h"

// macros
#define TRACE   1
#define N       37
#define SEED    971
#define MAX_VAL 100

// merger object
typedef struct {
	int na, nb;     // sizes of input arrays a and b
	int *a, *b, *c; // a and b are the sorted input arrays;
                    // c is the sorted array of size na+nb 
                    // obtained by merging a and b
    int *i, *j;     // i is an array of size na containing indices to a
                    // j is an array of size nb containing indices to b
} merger_t;

// local merger constraint data
typedef struct {
    merger_t* m;    // merger
    int k;          // cell index in m->c
} cons_data_t;

// merging constraint
void merge_cons(cons_data_t* d) {

    register int i, j, k = d->k;
    register merger_t* m = d->m;
    
    #if TRACE
    printf("[merger constraint %p] cell: %d\n", m, k);
    #endif

	if (k < 1) i = j = 0;
	else i = m->i[k-1], j = m->j[k-1];

	if (j == m->nb || (i < m->na && m->a[i] < m->b[j])) {
		m->c[k] = m->a[i];
		m->i[k] = i + 1;
		m->j[k] = j;
	}
	else {
		m->c[k] = m->b[j];
		m->i[k] = i;
		m->j[k] = j + 1;
	}
}

// merger
int* merge(int *a, int *b, int na, int nb) {
    
	merger_t* m = (merger_t*)malloc(sizeof(merger_t));
	m->a = a;
	m->b = b;
	m->c = (int*)rmalloc((na+nb)*sizeof(int));
	m->i = (int*)rmalloc((na+nb)*sizeof(int));
	m->j = (int*)rmalloc((na+nb)*sizeof(int));
	m->na = na;
	m->nb = nb;
    int k;

	for (k = 0; k < na + nb; k++) {
        cons_data_t* d = (cons_data_t*)malloc(sizeof(cons_data_t));
        d->m = m;
        d->k = k;
		newcons((cons_t)merge_cons, NULL, d);
    }

	return m->c;
}

// simple assignment constraint
typedef struct { int *src, *des; } pair_t;
void assign_cons(pair_t* d) {
    #if TRACE
    printf("[leaf constraint %p] %d -> %d\n", d->src, *(int*)rm_get_inactive_ptr(d->des), *d->src);
    #endif

    *d->des = *d->src;
}

// mergesort algorithm
int* mergesort(int* source, int from, int to) {

	if (to - from < 2) {
        pair_t* d = (pair_t*)malloc(sizeof(pair_t)); 
        d->src = &source[from];
		d->des = (int*)rmalloc(sizeof(int));
        newcons((cons_t)assign_cons, NULL, d);
		return d->des;
	}
	else {
		int m = (from + to)/2;
		int* left  = mergesort(source, from, m);
		int* right = mergesort(source, m, to);
		return merge(left, right, m-from, to-m);
	}
}


// print array of integers
void print_array(char* s, int* v, int n) {
    int i;
	printf("%s: ", s);
	for (i=0; i<n; ++i) printf("%d ", v[i]);
	printf("\n");
}

int main(){

	int i, *source = rmalloc(sizeof(int)*N);
    
    // init random array
	srand(SEED);
	for (i=0; i<N; ++i) source[i] = rand() % MAX_VAL;

    // produce sorted version of source array
	int* sorted = mergesort(source, 0, N);

    // dump source array
	print_array("Source", source, N);

    // dump sorted array
	print_array("Sorted", sorted, N);

    // update input array -> sorted array automagically updated
	source[N/2] = 5;

    // dump source array
	print_array("Source", source, N);

    // dump sorted array
	print_array("Sorted", sorted, N);

    // update input array -> sorted array automagically updated
	source[0] = 91;

    // dump source array
	print_array("Source", source, N);

    // dump sorted array
	print_array("Sorted", sorted, N);

    // update input array -> sorted array automagically updated
	source[0]++;

    // dump source array
	print_array("Source", source, N);

    // dump sorted array
	print_array("Sorted", sorted, N);
    
    return 0;
}

