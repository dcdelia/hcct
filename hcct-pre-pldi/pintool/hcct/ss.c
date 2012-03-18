#include "streaming.h"

#include <stdlib.h>
#include <stdio.h>

#define MakeC(x) ((SS_counter*)(&((x)[1])))
#define MakeT(x) ((SS_type*)(&((x)[1])))

// Command line parameters
KNOB<string> KnobEpsilon(KNOB_MODE_WRITEONCE, "pintool", "e", "25000", "specify 1/epsilon");
KNOB<string> KnobThreshold(KNOB_MODE_WRITEONCE, "pintool", "t", "20", "specify threshold as 2*coefficient for epsilon");

int EPSILON, THRESHOLD;

struct SS_counter;
struct SS_bucket;

typedef struct SS_bucket {
	int					value;
	struct CCTNode*		ctr;
	struct SS_bucket	*next, *prev;
} SS_bucket;

typedef struct SS_counter {
	int					eps;
	struct CCTNode		*next, *prev;
	struct SS_bucket*	bucket;
} SS_counter;

typedef struct SS_type {
	SS_bucket *bfirst;
	long N;
	int numc, numb;
	int numNodes;
} SS_type;

short fatNodeBytes=sizeof(SS_counter);
short fatThreadBytes=sizeof(SS_type);

void ST_new(ThreadRec* tdata) {	
	SS_type *result = MakeT(tdata);

	EPSILON=(int)strtol(KnobEpsilon.Value().c_str(), NULL, 0);
	THRESHOLD=(int)strtol(KnobThreshold.Value().c_str(), NULL, 0);
	
	result->bfirst = (SS_bucket *)calloc(1, sizeof(SS_bucket));
	//if (result->bfirst == NULL) { printf("\ncalloc error");	exit(100); }
	result->bfirst->value = 0;
	result->bfirst->next = NULL;
	result->bfirst->prev = NULL;
	result->N = 0;
	result->numc = 1; // will be increased step by step
	result->numb = 1;

	result->numNodes = 1;

	// first counter
	SS_counter* ctr=MakeC(tdata->root);
	ctr->prev = NULL;
	ctr->next = NULL;
	ctr->bucket = result->bfirst;
	result->bfirst->ctr = tdata->root;

	ST_update(tdata, tdata->root);
}

static void increment_counter(SS_type* ss, CCTNode* node) {
	SS_bucket *b, *bplus, *newb;
	SS_counter* c = MakeC(node);
	b = c->bucket;
	long cnt = b->value;
	cnt++;
	
	// detach c
	if (c->prev != NULL) {	// c is not first counter for a bucket
		MakeC(c->prev)->next = c->next;	
		if (c->next != NULL) {
			MakeC(c->next)->prev = c->prev;
		}
	} else {		// c is first counter for a bucket
		c->bucket->ctr = c->next;
		if (c->next != NULL) {
			MakeC(c->next)->prev = NULL;
		}
	}
	
	c->next = NULL;
	c->prev = NULL;

	bplus = b->next;
	if (bplus == NULL || bplus->value != cnt) {	// bplus needs to be created
		ss->numb++;
		newb = (SS_bucket *)calloc(1, sizeof(SS_bucket));
		if (newb == NULL) { printf("\ncalloc error"); exit(100); }
		newb->value = cnt;
		newb->ctr = node;
		newb->prev = c->bucket;
		newb->next = c->bucket->next;
		newb->prev->next = newb;
		if (newb->next != NULL)
			newb->next->prev = newb;
		MakeC(newb->ctr)->bucket = newb;
	} else {	// bplus exists
		c->next = bplus->ctr;
		MakeC(bplus->ctr)->prev = node;
		c->prev = NULL;
		bplus->ctr = node; // insert at the beginning
		c->bucket = bplus;
	}

	if (b->ctr == NULL) {	// bucket b is empty
		ss->numb--;
		if (b->prev != NULL) {
			b->prev->next = b->next;
			if (b->next != NULL) {
				b->next->prev = b->prev;
			}
		} else {	// b is first bucket
			ss->bfirst = b->next;
			if (b->next != NULL) {
				b->next->prev = NULL;
			}
		}

		free(b);
	}
}

#if PRUNING
static void removeNode(CCTNode* node, int* numNodes) {
	CCTNode *p, *c;
	p=node->parent;
	if (p==NULL) return; // for very high frequencies (cannot remove root node)
	
	if (p->firstChild==node) {
		p->firstChild=node->nextSibling;
		if (p->firstChild==NULL) // internal node becomes a leaf
			if (MakeC(p)->bucket==NULL) // cold leaf: can be removed!
				removeNode(p, numNodes);
	} else
		for (c=p->firstChild; c!=NULL; c=c->nextSibling)
			if (c->nextSibling==node) {
				c->nextSibling=node->nextSibling;
				break;
			}
	free(node);
	(*numNodes)--;
}
#endif

void ST_update(ThreadRec* tdata, CCTNode* theNodePtr) {
	SS_type* ss=MakeT(tdata);
	SS_counter* c=MakeC(theNodePtr);

	ss->N++;
	
	if (c->bucket==NULL) { // set to 0 by calloc
		CCTNode* r = ss->bfirst->ctr; // remove first element in first bucket
		
		//#if PRUNING
		if (MakeC(theNodePtr)->eps==0)
		//#endif
		ss->numNodes++;

		c->bucket=ss->bfirst;
		c->eps = ss->bfirst->value;
		c->prev = NULL; // VEDI DOPO SE È VERAMENTE NECESSARIO

		if (ss->numc==EPSILON) { // replacement step
			c->next = MakeC(r)->next;
			if (c->next!=NULL)
				(MakeC(c->next))->prev=theNodePtr; // for doubly linked list coherence
			#if PRUNING
			if (r->firstChild!=NULL) { // cold node
				MakeC(r)->eps-=MakeC(r)->bucket->value;
				MakeC(r)->bucket=NULL;
			} else removeNode(r, &(ss->numNodes));
			#else
			MakeC(r)->eps-=MakeC(r)->bucket->value;
			MakeC(r)->bucket=NULL;
			#endif
		} else { // create new counter
			ss->numc++;
			c->next=r;
			MakeC(r)->prev=theNodePtr;
		}
		ss->bfirst->ctr=theNodePtr;
	}

	increment_counter (ss, theNodePtr);
}

#if PRUNING
static void pruneTree(SS_type* ss, int threshold) {
	SS_bucket* b;
	CCTNode *node, *prev, *next, *first;
	for (b=ss->bfirst; b != NULL; b = b->next) {
		if (b->value >= ss->N/threshold) break;
		first=b->ctr;
		for (node=b->ctr; node != NULL; ) {
			if (node->firstChild==NULL) { // leaf
				prev=MakeC(node)->prev;
				next=MakeC(node)->next;
				if (prev!=NULL)
					MakeC(prev)->next=next;
				else
					first=next;
				if (next!=NULL)
					MakeC(next)->prev=prev;
				removeNode(node, &(ss->numNodes));
				node=next;
			} else node=MakeC(node)->next;
		}
		b->ctr=first;
	}
}

static void callCoverage(CCTNode* root, int* count, int* eps) {
	CCTNode* child;
	SS_counter* c=MakeC(root);
	if (c->bucket!=NULL) {
		(*count)+=c->bucket->value;
		(*eps)+=c->eps;
	} else (*count)-=c->eps; // cold node
	for (child=root->firstChild; child!=NULL; child=child->nextSibling)		
		callCoverage(child, count, eps);
}
#endif

void ST_dumpStats(ThreadRec* tdata, FILE* out) {
	SS_type *ss=MakeT(tdata);
	SS_bucket *b;
	CCTNode* node;

	fprintf(out, "-----\nInformazioni globali\n-----\n");
	fprintf(out, "Lunghezza stream: %ld\n", ss->N);
	fprintf(out, "Nodi in uso (intero albero): %d\n", ss->numNodes);
	fprintf(out, "Numero contatori: %d\n", ss->numc);	
	fprintf(out, "Numero bucket: %d\n", ss->numb);
	fprintf(out, "---\n");

	fprintf(out, "Elementi per ogni riga di dump:\n---\n");
	fprintf(out, "- Fattore soglia (*2)\n");
	fprintf(out, "- Soglia corrente (1/threshold)\n");
	
	#if PRUNING
	fprintf(out, "- Elementi sopra la soglia\n");
	fprintf(out, "- Elementi rimossi\n");
	fprintf(out, "- Call coverage (without eps)\n");
	fprintf(out, "- Call coverage (with eps)\n");
	fprintf(out, "- Call coverage %%%%%% (without eps)\n");
	fprintf(out, "- Call coverage %%%%%% (with eps)\n");
	fprintf(out, "- Numero di nodi in uso\n\n");
	#else
	fprintf(out, "- Sovrastima totale\n");
	fprintf(out, "- Elementi sopra la soglia\n");
	fprintf(out, "- Conteggi sbagliati\n");
	fprintf(out, "- Elementi sovrastimati di al più l'1%%\n");
	fprintf(out, "- Elementi sovrastimati tra l'1%% e il 2%%\n");
	fprintf(out, "- Elementi sovrastimati tra il 2%% e il 4%%\n");
	fprintf(out, "- Errore massimo percentuale (da dividere per 100)\n");
	fprintf(out, "- Numero falsi positivi\n");
	fprintf(out, "- Falsi positivi con margine entro il 25%% rispetto N/EPSILON\n");
	fprintf(out, "- Falsi positivi con margine tra il 25 e il 50%% rispetto a N/EPSILON\n");
	fprintf(out, "- Falsi positivi con margine tra il 50 e il 75%% rispetto a N/EPSILON\n");
	fprintf(out, "- Margine massimo percentuale su falsi positivi (da dividere per 100)\n\n");
	#endif

	int i, threshold, elements;
	#if PRUNING
	int coverage_N, coverage_eps, removed, removedTot=0;
	#else
	int errore_max, tmp, conteggi_sbagliati, errore_massimo_1, errore_massimo_2, errore_massimo_4, falsi_positivi, errore_falsi_positivi, margine_25, margine_50, margine_75, margine;
	long sovrastima_totale;
	#endif

	for (i=2, threshold=(EPSILON*2)/i; i<=THRESHOLD; threshold=(EPSILON*2)/++i) {
		elements=0;
		#if PRUNING
		coverage_N=0, coverage_eps=0, removed=0;
		#else
		errore_max=0, conteggi_sbagliati=0, errore_massimo_1=0, errore_massimo_2=0, errore_massimo_4=0, sovrastima_totale=0,
			falsi_positivi=0, errore_falsi_positivi=0, margine_25=0, margine_50=0, margine_75=0;
		#endif
	
		#if PRUNING
		removed=ss->numNodes;
		pruneTree(ss, threshold);
		removed-=ss->numNodes;
		removedTot+=removed;
		callCoverage(tdata->root, &coverage_N, &coverage_eps);
		for (b=ss->bfirst; b != NULL; b = b->next) {
			if (b->value < ss->N/threshold) 
				continue;
			for (node=b->ctr; node != NULL; node = MakeC(node)->next)
				elements++;
		}
		fprintf(out, "%d %d %d %d %d %d %lld %lld %d\n", i, threshold, elements, removed, coverage_N-coverage_eps, coverage_N,
			(10000*((long long)(coverage_N-coverage_eps)))/ss->N, (10000*(long long)coverage_N)/ss->N, ss->numNodes);
		#else
		for (b=ss->bfirst; b != NULL; b = b->next) {
			if (b->value < ss->N/threshold) 
				continue;
				
			for (node=b->ctr; node != NULL; node = MakeC(node)->next) {
					elements++;
					tmp=b->value - node->count;
					if (tmp>0) {
						sovrastima_totale+=tmp;
						conteggi_sbagliati++;
						if (tmp<=(int)node->count/100) errore_massimo_1++;
						else if (tmp<=(int)node->count/50) errore_massimo_2++;
							else if(tmp<=(int)node->count/25) errore_massimo_4++;
						
						tmp=tmp*10000/node->count;
						
						if (tmp > errore_max)
							errore_max=tmp;

						margine=(int)(ss->N/threshold - node->count);
						if (margine>0) {
							falsi_positivi++;
							if (margine<(ss->N/EPSILON)/4) margine_25++;
							else if (margine<(ss->N/EPSILON)/2) margine_50++;
								else if(margine<3*(ss->N/EPSILON)/4) margine_75++;
							margine=10000*margine/(ss->N/EPSILON);
							if (margine > errore_falsi_positivi)
								errore_falsi_positivi=margine;
						}
					}
				}
		}
		fprintf(out, "%d %d %ld %d %d %d %d %d %d %d %d %d %d %d\n", i, threshold, sovrastima_totale, elements, conteggi_sbagliati,
			errore_massimo_1, errore_massimo_2, errore_massimo_4, errore_max, falsi_positivi, margine_25, margine_50, margine_75, errore_falsi_positivi);
	#endif
	}
	#if PRUNING
	fprintf(out, "Nodi rimossi: %d", removedTot);
	#endif
}

void ST_dumpNode(CCTNode* root, FILE* out) {
	if (MakeC(root)->bucket!=NULL)
		#if CALL_SITE
		fprintf(out, "v %lu %s %s [%x->%x] %d -%d\n", (unsigned long)root,  
            STool_RoutineNameByAddr(root->target), STool_StripPath(STool_LibraryNameByAddr(root->target)), 
            root->callSite, root->target, MakeC(root)->bucket->value, MakeC(root)->eps);
		#else
		fprintf(out, "v %lu %s %s [%x] %d -%d\n", (unsigned long)root, 
            STool_RoutineNameByAddr(root->target), STool_StripPath(STool_LibraryNameByAddr(root->target)), 
            root->target, MakeC(root)->bucket->value, MakeC(root)->eps);
		#endif
	else
		#if CALL_SITE
		fprintf(out, "v %lu %s %s [%x->%x] 0\n", (unsigned long)root,  
            STool_RoutineNameByAddr(root->target), STool_StripPath(STool_LibraryNameByAddr(root->target)), 
            root->callSite, root->target);
		#else
		fprintf(out, "v %lu %s %s [%x] 0\n", (unsigned long)root, 
            STool_RoutineNameByAddr(root->target), STool_StripPath(STool_LibraryNameByAddr(root->target)), 
            root->target);
		#endif
}

void ST_free(ThreadRec* tdata) {
	SS_bucket *b, *btemp;
	
	for (b=MakeT(tdata)->bfirst; b != NULL; ) {
		btemp = b;
		b = b->next;
		free(btemp);
	}
}

#if STREAMING
long ST_length(ThreadRec* tdata) {
	return MakeT(tdata)->N;
}

static int countNodes(CCTNode* root) {
	int n=1;
	CCTNode* child;
	for (child=root->firstChild; child!=NULL; child=child->nextSibling)
		n+=countNodes(child);
	return n;
}

int	ST_nodes(ThreadRec* tdata) {
	return countNodes(tdata->root);
}
#endif
