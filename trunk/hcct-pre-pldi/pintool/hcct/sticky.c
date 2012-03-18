#include "streaming.h"

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <math.h>

#define MakeC(x) ((STICKY_counter*)(&((x)[1])))
#define MakeT(x) ((STICKY_type*)(&((x)[1])))

// Command line parameters
KNOB<string> KnobEpsilon(KNOB_MODE_WRITEONCE, "pintool", "e", "25000", "specify 1/epsilon");
KNOB<string> KnobThreshold(KNOB_MODE_WRITEONCE, "pintool", "t", "2500", "specify 1/threshold");
KNOB<string> KnobFailure(KNOB_MODE_WRITEONCE, "pintool", "f", "0.75", "specify failure probability");
int EPSILON, THRESHOLD;
double FAILURE;

typedef struct STICKY_stats {
    int                		entries;
	int						deleted;
    struct STICKY_stats*    next;
} STICKY_stats;

typedef struct STICKY_counter {
	unsigned int			count;
	CCTNode*				next;
} STICKY_counter;

typedef struct STICKY_type {
	CCTNode*				nodes;
	long					N;
	int						epoch;
	int						t;
	unsigned int			entries;
	unsigned int			maxEntries;
	int						index;
	STICKY_stats*			stats;
} STICKY_type;

short fatNodeBytes=sizeof(STICKY_counter);
short fatThreadBytes=sizeof(STICKY_type);

void ST_new(ThreadRec* tdata) {
	CCTNode* n=(CCTNode*)malloc(sizeof(CCTNode)+fatNodeBytes);
	MakeC(n)->next=NULL;
	MakeC(n)->count=0;

	EPSILON=(int)strtol(KnobEpsilon.Value().c_str(), NULL, 0);
	THRESHOLD=(int)strtol(KnobThreshold.Value().c_str(), NULL, 0);
	FAILURE=strtod(KnobFailure.Value().c_str(), NULL);

	STICKY_type* result=MakeT(tdata);
	result->N=0;
	result->index=0;
	result->epoch=1; // TODO
	result->t=(int)(EPSILON*(ceil(log(THRESHOLD/FAILURE))));
	result->entries=0;
	result->maxEntries=0;
	result->nodes=n; // dummy head node

	result->stats=(STICKY_stats*)malloc(sizeof(STICKY_stats));

	result->stats->next=NULL; // altrimenti avrei corruzione per stats (???)

	srand(time(NULL));
	
	ST_update(tdata, tdata->root); // root node
}

static void changeSamplingRate(STICKY_type* sticky) {
	STICKY_counter *c;
	CCTNode *prev, *next;

	STICKY_stats* oldStats=sticky->stats;
	STICKY_stats* newStats=(STICKY_stats*)malloc(sizeof(STICKY_stats));
	oldStats->entries=sticky->entries;
	newStats->next=oldStats;
	sticky->stats=newStats;
	
	if (sticky->maxEntries < sticky->entries)
		sticky->maxEntries=sticky->entries;
	sticky->index=0;
	
	srand(time(NULL)); // FORSE NON SERVE!

	for (prev=sticky->nodes, next=MakeC(prev)->next; next!=NULL; next=MakeC(prev)->next) {
		c=MakeC(next);
		while (c->count>0) {
			//if (rand()%2>0) { // toss an unbiased coin
			if (rand()&1) { // toss an unbiased coin
				c->count--;
			} else break;
		}
		if (c->count==0) {
			MakeC(prev)->next=MakeC(next)->next;
			MakeC(next)->next=NULL; // NECESSARIO???
			// REMOVE(next)
			sticky->entries--;
		} else prev=next;
	}

	oldStats->deleted=oldStats->entries - sticky->entries;
}

void ST_update(ThreadRec* tdata, CCTNode* theNodePtr) {
	STICKY_type* sticky=MakeT(tdata);
	
	sticky->N++; // N si può ricostruire tramite epoch e index :)
	sticky->index++;
	
	STICKY_counter *c=MakeC(theNodePtr);
	
	if (c->count==0) { // new entry
		if (rand()%sticky->epoch==0) { // sample with rate 1/2^i
			c->count=1;
			c->next=MakeC(sticky->nodes)->next;
			MakeC(sticky->nodes)->next=theNodePtr;
			sticky->entries++;
		}
	} else { // update existing entry
		c->count++;
	}

	if (sticky->epoch==1) {
		if (sticky->index == 2*sticky->t) {
			sticky->epoch++;
			changeSamplingRate(sticky);
		}
	} else if (sticky->index == sticky->epoch * sticky->t) {
		sticky->epoch*=2;
		changeSamplingRate(sticky);
	}
}

void ST_free(ThreadRec* tdata) {
	free(MakeT(tdata)->nodes);
	
	STICKY_stats *prev, *next;
	prev=MakeT(tdata)->stats;
	for (; prev!=NULL; prev=next) {
		next=prev->next;
		free(prev);
	}
}

static void elementsForThreshold(CCTNode* root, int frequency, int* counter) {
	if (root->count>=(unsigned)frequency)
		(*counter)++;
	CCTNode* ptr;
	for (ptr=root->firstChild; ptr!=NULL; ptr=ptr->nextSibling)
		elementsForThreshold(ptr, frequency, counter);
}

static void checkFalseNegatives(CCTNode* root, int frequency, unsigned cut, FILE* out) {
	if (root->count>=(unsigned)frequency && MakeC(root)->count<cut)
		fprintf(out, "%d %lu %d\n", root->count, (unsigned long)root, MakeC(root)->count);
	CCTNode* ptr;
	for (ptr=root->firstChild; ptr!=NULL; ptr=ptr->nextSibling)
		checkFalseNegatives(ptr, frequency, cut, out);
}

void ST_dumpStats(ThreadRec* tdata, FILE* out) {
	STICKY_type* sticky=MakeT(tdata);

	fprintf(out, "-----\nInformazioni globali\n-----\n");
	fprintf(out, "Lunghezza stream: %ld\n", sticky->N);
	fprintf(out, "Elementi attualmente nella synopsis: %u\n", sticky->entries);
	fprintf(out, "Massimo numero di elementi nella synopsis (pre-pruning): %u\n", sticky->maxEntries);
	fprintf(out, "Valore usato per EPSILON: %d\n", EPSILON);
	fprintf(out, "Valore usato per THRESHOLD: %d\n", THRESHOLD);
	fprintf(out, "Valore usato per FAILURE: %f\n", FAILURE);
	fprintf(out, "Valore calcolato per t: %d\n", sticky->t);
	fprintf(out, "---\n");

	fprintf(out, "Elementi per ogni riga di dump:\n---\n");
	//fprintf(out, "- Fattore soglia (*2)\n"); // In Sticky Sampling threshold è fixed
	fprintf(out, "- Soglia corrente (1/threshold)\n");
	fprintf(out, "- Frequenza di taglio\n");
	fprintf(out, "- Frequenza di soglia\n");
	fprintf(out, "- Elementi sopra la soglia\n");
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

	STICKY_counter* c;
	CCTNode* node;
	
	int errore_max, tmp, conteggi_sbagliati, errore_massimo_1, errore_massimo_2, errore_massimo_4, elements,
		falsi_positivi, errore_falsi_positivi, margine_25, margine_50, margine_75, margine;
	
	//~ for (i=2, threshold=(EPSILON*2)/i; i<=20; threshold=(EPSILON*2)/++i) {
		errore_max=0, conteggi_sbagliati=0, errore_massimo_1=0, errore_massimo_2=0, errore_massimo_4=0, elements=0,
			falsi_positivi=0, errore_falsi_positivi=0, margine_25=0, margine_50=0, margine_75=0;
		
		
		for (node=MakeC(sticky->nodes)->next; node!=NULL; node=MakeC(node)->next) {
			c=MakeC(node);
			if (c->count >= (unsigned)(sticky->N/THRESHOLD - sticky->N/EPSILON)) {
				elements++;
			
				tmp=(int)(node->count - c->count);
				if (tmp>0) {
					conteggi_sbagliati++;
					if (tmp<=(int)node->count/100) errore_massimo_1++;
					else if (tmp<=(int)node->count/50) errore_massimo_2++;
						else if(tmp<=(int)node->count/25) errore_massimo_4++;
				
					tmp=tmp*10000/node->count;
					if (tmp > errore_max)
						errore_max=tmp;
				}

				margine=(int)(sticky->N/THRESHOLD - node->count);
				if (margine>0) {
					falsi_positivi++;
					if (margine<(sticky->N/EPSILON)/4) margine_25++;
					else if (margine<(sticky->N/EPSILON)/2) margine_50++;
						else if(margine<3*(sticky->N/EPSILON)/4) margine_75++;
					margine=10000*margine/(sticky->N/EPSILON);
					if (margine > errore_falsi_positivi)
						errore_falsi_positivi=margine;
				}
			}
		}
	
		fprintf(out, "%d %ld %ld %d %d %d %d %d %d %d %d %d %d %d\n", THRESHOLD, sticky->N/THRESHOLD - sticky->N/EPSILON, sticky->N/THRESHOLD, elements,
			conteggi_sbagliati, errore_massimo_1, errore_massimo_2, errore_massimo_4, errore_max, falsi_positivi, margine_25, margine_50, margine_75, errore_falsi_positivi);
	//~ }
	
	int xxx=0;
	elementsForThreshold(tdata->root, sticky->N/THRESHOLD, &xxx);
	fprintf(out, "\nElementi realmente sopra la soglia: %d\n", xxx);
	fprintf(out, "Elementi conteggiati e sopra la soglia: %d\n", elements-falsi_positivi);

	if (xxx-elements+falsi_positivi!=0) {
		fprintf(out, "\nFalsi negativi trovati:\n");
		checkFalseNegatives(tdata->root, sticky->N/THRESHOLD, (unsigned)(sticky->N/THRESHOLD - sticky->N/EPSILON), out);
	}

    fprintf(out, "\nSpace usage statistics (epoch, counters, deleted):\n");
	int t=(int)(log(sticky->epoch)/log(2))+1;
    fprintf(out, "%d %d 0\n", t, sticky->entries);

	STICKY_stats* stats=sticky->stats->next;
    for (int i=t; i>1 && stats!=NULL; stats=stats->next)
        fprintf(out, "%d %d %d\n",--i, stats->entries, stats->deleted);
}

void ST_dumpNode(CCTNode* root, FILE* out) {
	#if CALL_SITE
    fprintf(out, "v %lu %s %s [%x->%x] %d\n", (unsigned long)root, 
            STool_RoutineNameByAddr(root->target), STool_StripPath(STool_LibraryNameByAddr(root->target)), 
            root->callSite, root->target, MakeC(root)->count);
    #else
    fprintf(out, "v %lu %s %s [%x] %d\n", (unsigned long)root, 
            STool_RoutineNameByAddr(root->target), STool_StripPath(STool_LibraryNameByAddr(root->target)), 
            root->target, MakeC(root)->count);
    #endif
}

#if STREAMING
long ST_length(ThreadRec* tdata) {
	return MakeT(tdata)->N;
}

// TODO: stats->numNodes
static int countNodes(CCTNode* root) {
	int n=1;
	CCTNode* child;
	for (child=root->firstChild; child!=NULL; child=child->nextSibling);
		n+=countNodes(child);
	return n;
}

int	ST_nodes(ThreadRec* tdata) {
	return countNodes(tdata->root);
}
#endif
