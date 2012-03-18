#include "streaming.h"

#include <stdlib.h>
#include <stdio.h>

#define MakeC(x) ((LC_counter*)(&((x)[1])))
#define MakeT(x) ((LC_type*)(&((x)[1])))

// Command line parameters
KNOB<string> KnobEpsilon(KNOB_MODE_WRITEONCE, "pintool", "e", "25000", "specify 1/epsilon");
KNOB<string> KnobThreshold(KNOB_MODE_WRITEONCE, "pintool", "t", "20", "specify threshold as 2*coefficient for epsilon");

int EPSILON, THRESHOLD;

typedef struct LC_stats {
    int                 entries;
	int					deleted;
	long				numNodes;
    struct LC_stats*    next;
} LC_stats;

typedef struct LC_counter {
	struct CCTNode*	    next;
	int 				count;
	int 				delta;
} LC_counter;

typedef struct LC_type {
	LC_counter*			counters;
	long				N;
	int					buckets;
	int					index;
	int					window;
	unsigned int		entries;
	unsigned int		maxEntries;
    LC_stats*           stats;
	long				numNodes;
} LC_type;

short fatNodeBytes=sizeof(LC_counter);
short fatThreadBytes=sizeof(LC_type);

#if PRUNING
static void removeNodeAux(CCTNode* node, long* numNodes) {
	CCTNode *p, *c;
	p=node->parent;
	if (p==NULL) return; // for very high frequencies (cannot remove root node)
	if (p->firstChild==node) {
		p->firstChild=node->nextSibling;
		if (p->firstChild==NULL) // internal node becomes a leaf
			if (MakeC(p)->count<0) // cold leaf: can be removed!
				removeNodeAux(p, numNodes);
	} else
		for (c=p->firstChild; c!=NULL; c=c->nextSibling)
			if (c->nextSibling==node) {
				c->nextSibling=node->nextSibling;
				break;
			}
	free(node);
	(*numNodes)--;
}

static void removeNode(CCTNode* node, long* numNodes) {
	if (node->firstChild==NULL) 
		removeNodeAux(node, numNodes);
	else
		MakeC(node)->count*=-1; // cold node
}
#endif

void ST_new(ThreadRec* tdata) {
	LC_counter* c=(LC_counter*)malloc(sizeof(LC_counter)); // dummy head node
	c->next=NULL;

	EPSILON=(int)strtol(KnobEpsilon.Value().c_str(), NULL, 0);
	THRESHOLD=(int)strtol(KnobThreshold.Value().c_str(), NULL, 0);

    LC_type* result=MakeT(tdata);
	result->counters=c;
	result->N=0;
	result->index=0; // == N mod w
	result->buckets=1;
	result->window=EPSILON;
	result->entries=0;
	result->maxEntries=0;

    result->numNodes=0;

	result->stats=(LC_stats*)malloc(sizeof(LC_stats));
	result->stats->numNodes=0;
	result->stats->next=NULL;

	ST_update(tdata, tdata->root); // root node
}

void ST_update(ThreadRec* tdata, CCTNode* theNodePtr) {
	LC_type* lc=MakeT(tdata);
	LC_counter* c=MakeC(theNodePtr);

	lc->N++;
	lc->index++;
    
    if (c->count<=0) {
		#if PRUNING
        if (c->count==0) {
        	c->delta=lc->buckets-1;
			lc->numNodes++;
        } else {
            c->count*=-1; // cold node becomes hot
        }
		c->next=lc->counters->next; // insert at the beginning
		#else
		if (c->count==0)
			lc->numNodes++;
		c->delta=lc->buckets-1;
        c->count=0;
		#endif
		
		c->next=lc->counters->next; // insert at the beginning
		lc->counters->next=theNodePtr;
		lc->entries++;        
    }
    
    c->count++;

	if (lc->index == lc->window) { // pruning time
        // Space usage statistics
		LC_stats* oldStats=lc->stats;
        LC_stats* newStats=(LC_stats*)malloc(sizeof(LC_stats));
        oldStats->entries=lc->entries;
		oldStats->numNodes=lc->numNodes; // peak here (before pruning)
        newStats->next=oldStats;
        lc->stats=newStats;
 
		if (lc->entries > lc->maxEntries)
			lc->maxEntries=lc->entries;
		c=lc->counters;
		
		if (c->next==theNodePtr) // skip last inserted element (count=1)
			c=MakeC(c->next);

		while (c->next!=NULL) {
			if (MakeC(c->next)->count + MakeC(c->next)->delta <= lc->buckets) {
				#if PRUNING
				removeNode(c->next, &(lc->numNodes));
				#else
				MakeC(c->next)->count*=-1;
				#endif
				c->next=MakeC(c->next)->next;
				lc->entries--;
			} else c=MakeC(c->next);
        }
		lc->index=0;
		lc->buckets++;
	
		oldStats->deleted=oldStats->entries-lc->entries;
	}
}

void ST_free(ThreadRec* tdata) {
	free(MakeT(tdata)->counters);
	LC_stats *prev, *next;
	prev=MakeT(tdata)->stats;
	for (; prev!=NULL; prev=next) {
		next=prev->next;
		free(prev);
	}
}

#if PRUNING
static int finalPrune(LC_type* lc, int threshold) {
	CCTNode* tmp;
	int removed=0;
	LC_counter* c=lc->counters;
	while (c->next!=NULL) {
		if (MakeC(c->next)->count < lc->N/threshold - MakeC(c->next)->delta) { // fewer false positives
			tmp=c->next;
			c->next=MakeC(c->next)->next;
			removeNode(tmp, &(lc->numNodes));
			removed++; // può esserci disallineamento di 1 se root non viene eliminato :S
			lc->entries--;
		} else c=MakeC(c->next);
	}
	
	return removed;
}

static void callCoverage(CCTNode* root, int* count, int* delta) {
	CCTNode* child;
	LC_counter* c=MakeC(root);
	(*delta)+=c->delta;
	if (c->count>0)
		(*count)+=c->count;
	else
		(*count)-=c->count; // cold node (count<0)
	for (child=root->firstChild; child!=NULL; child=child->nextSibling)		
		callCoverage(child, count, delta);
}
#endif

void ST_dumpStats(ThreadRec* tdata, FILE* out) {
	LC_type* lc=MakeT(tdata);
	
	if (lc->entries > lc->maxEntries)
		lc->maxEntries=lc->entries;

	fprintf(out, "-----\nInformazioni globali\n-----\n");
	fprintf(out, "Lunghezza stream: %ld\n", lc->N);
	fprintf(out, "Bucket corrente: %d\n", lc->buckets);
	fprintf(out, "Dimensione finestra (EPSILON): %d\n", lc->window);
	fprintf(out, "Elementi attualmente nella synopsis: %u\n", lc->entries);
	fprintf(out, "Massimo numero di elementi nella synopsis: %u\n", lc->maxEntries);
	fprintf(out, "---\n");
	
	#if PRUNING
	fprintf(out, "Elementi per ogni riga di dump:\n---\n");
	fprintf(out, "- Fattore soglia (*2)\n");
	fprintf(out, "- Soglia corrente (1/threshold)\n");
	fprintf(out, "- Elementi sopra la soglia\n");
	fprintf(out, "- Elementi sopra la soglia (con correzioni di iterazioni precedenti)\n");
	fprintf(out, "- Elementi rimossi\n");
	fprintf(out, "- Call coverage (N)\n");
	fprintf(out, "- Call coverage (delta)\n");
	fprintf(out, "- Call coverage %%%%%% (without delta)\n");
	fprintf(out, "- Call coverage %%%%%% (with delta)\n");
	fprintf(out, "- Numero di nodi in uso\n\n");
	#else
	fprintf(out, "Elementi per ogni riga di dump:\n---\n");
	fprintf(out, "- Fattore soglia (*2)\n");
	fprintf(out, "- Soglia corrente (1/threshold)\n");
	fprintf(out, "- Elementi sopra la soglia (no correzione)\n");
	fprintf(out, "- Elementi sopra la soglia (con correzione \"locale\")\n");
	fprintf(out, "- Conteggi sbagliati\n");
	fprintf(out, "- Elementi sottostimati di al più l'1%%\n");
	fprintf(out, "- Elementi sottostimati tra l'1%% e il 2%%\n");
	fprintf(out, "- Elementi sottostimati tra il 2%% e il 4%%\n");
	fprintf(out, "- Errore massimo percentuale (da dividere per 100)\n");
	fprintf(out, "- Numero falsi positivi\n");
	fprintf(out, "- Falsi positivi con margine entro il 25%% rispetto N/EPSILON\n");
	fprintf(out, "- Falsi positivi con margine tra il 25 e il 50%% rispetto a N/EPSILON\n");
	fprintf(out, "- Falsi positivi con margine tra il 50 e il 75%% rispetto a N/EPSILON\n");
	fprintf(out, "- Margine massimo percentuale su falsi positivi (da dividere per 100)\n\n");
	#endif

    CCTNode* ptr;
	
	#if PRUNING
	int i, threshold, elements, elements_2, coverage_N, coverage_delta, removed, removedTot=0;
	#else
	int i, threshold, errore_max, tmp, conteggi_sbagliati, errore_massimo_1, errore_massimo_2, errore_massimo_4,
		elements, elements_2, falsi_positivi, errore_falsi_positivi, margine_25, margine_50, margine_75, margine;
	#endif
	
	for (i=2, threshold=(EPSILON*2)/i; i<=THRESHOLD; threshold=(EPSILON*2)/++i) {
	#if PRUNING
		elements=0, elements_2=0, coverage_N=0, coverage_delta=0;
		for (ptr=lc->counters->next; ptr!=NULL; ptr=MakeC(ptr)->next) {
			if (MakeC(ptr)->count >= lc->N/threshold - MakeC(ptr)->delta)
				elements++;
			if (MakeC(ptr)->count >= lc->N/threshold - lc->N/EPSILON)
				elements_2++;
		}
		removed=finalPrune(lc, threshold);
		removedTot+=removed;
		if (tdata->root!=NULL)
			callCoverage(tdata->root, &coverage_N, &coverage_delta);
	
		fprintf(out, "%d %d %d %d %d %d %d %lld %lld %ld\n", i, threshold, elements, elements_2, removed, coverage_N, coverage_delta,
			(10000*(long long)coverage_N)/lc->N, (10000*(((long long)coverage_N)+coverage_delta))/lc->N, lc->numNodes);
	#else
		errore_max=0, conteggi_sbagliati=0, errore_massimo_1=0, errore_massimo_2=0, errore_massimo_4=0, elements=0,
			falsi_positivi=0, errore_falsi_positivi=0, margine_25=0, margine_50=0, margine_75=0, elements_2=0;
		for (ptr=lc->counters->next; ptr!=NULL; ptr=MakeC(ptr)->next) {
			if (MakeC(ptr)->count >= lc->N/threshold - lc->N/EPSILON)
				elements_2++;
			if (MakeC(ptr)->count >= lc->N/threshold - MakeC(ptr)->delta) {
				elements++;

				tmp=(int)ptr->count - MakeC(ptr)->count;
				if (tmp>0) {
					conteggi_sbagliati++;
					if (tmp<=((int)ptr->count)/100) errore_massimo_1++;
					else if (tmp<=(int)ptr->count/50) errore_massimo_2++;
						else if(tmp<=(int)ptr->count/25) errore_massimo_4++;
					
					tmp=tmp*10000/ptr->count;
					if (tmp > errore_max)
						errore_max=tmp;
				}
				
				margine=(int)(lc->N/threshold - ptr->count);
				if (margine>0) {
					falsi_positivi++;
					if (margine<(lc->N/EPSILON)/4) margine_25++;
					else if (margine<(lc->N/EPSILON)/2) margine_50++;
						else if(margine<3*(lc->N/EPSILON)/4) margine_75++;
					margine=10000*margine/(lc->N/EPSILON);
					if (margine > errore_falsi_positivi)
						errore_falsi_positivi=margine;
				}
			}
		}
		fprintf(out, "%d %d %d %d %d %d %d %d %d %d %d %d %d %d\n", i, threshold, elements_2, elements, conteggi_sbagliati,
			errore_massimo_1, errore_massimo_2, errore_massimo_4, errore_max, falsi_positivi, margine_25, margine_50, margine_75, errore_falsi_positivi);
	#endif
	}

	#if PRUNING
	fprintf(out, "\nGlobal space statistics:\n");
	
	double spaceAvg_LC=0, spaceAvg=0;
	long spaceMax=lc->numNodes;
	LC_stats* stats;

	spaceAvg_LC+=(double)lc->entries/lc->buckets;
	spaceAvg=(double)lc->numNodes/lc->buckets;

	stats=lc->stats->next;
	for (int i=lc->buckets; i>1 && stats!=NULL; stats=stats->next) {
		spaceAvg_LC+=(double)stats->entries/lc->buckets;
		spaceAvg+=(double)stats->numNodes/lc->buckets;
		if (stats->numNodes>spaceMax)
			spaceMax=stats->numNodes;
	}

	fprintf(out, "%ld %ld %f %d %f %ld\n", lc->N, lc->numNodes, spaceAvg_LC, lc->maxEntries, spaceAvg, spaceMax);
	fprintf(out, "(numCalls, numNodes, spaceAvg_LC, spaceMax_LC, spaceAvg, spaceMax)\n");
    
	fprintf(out, "\nSpace usage statistics (bucket, counters, deleted, #nodes):\n");
	fprintf(out, "%d %d %d %ld\n", lc->buckets, lc->entries, removedTot, lc->numNodes);

    stats=lc->stats->next;
	for (int i=lc->buckets; i>1 && stats!=NULL; stats=stats->next)
		fprintf(out, "%d %d %d %ld\n", --i, stats->entries, stats->deleted, stats->numNodes);
	#else
	fprintf(out, "\nSpace usage statistics (bucket, counters, deleted):\n");
	fprintf(out, "%d %d %d\n", lc->buckets, lc->entries, 0);

    LC_stats* stats=lc->stats->next;
	for (int i=lc->buckets; i>1 && stats!=NULL; stats=stats->next)
		fprintf(out, "%d %d %d\n", --i, stats->entries, stats->deleted);
	#endif
}

void ST_dumpNode(CCTNode* root, FILE* out) {
	#if CALL_SITE
    fprintf(out, "v %lu %s %s [%x->%x] %d %d\n", (unsigned long)root, 
            STool_RoutineNameByAddr(root->target), STool_StripPath(STool_LibraryNameByAddr(root->target)), 
            root->callSite, root->target, MakeC(root)->count, MakeC(root)->delta);
    #else
    fprintf(out, "v %lu %s %s [%x] %d %d\n", (unsigned long)root,
            STool_RoutineNameByAddr(root->target), STool_StripPath(STool_LibraryNameByAddr(root->target)), 
            root->target, MakeC(root)->count, MakeC(root)->delta);
    #endif
}

#if STREAMING
long ST_length(ThreadRec* tdata) {
	return MakeT(tdata)->N;
}

int ST_nodes(ThreadRec* tdata) {
	return MakeT(tdata)->numNodes;
}
#endif
