#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "analysis.h"

// remove from tree every node such that node->counter <= min_counter
void pruneTree(hcct_tree_t* tree, hcct_node_t* node, UINT32 min_counter) {
	if (node==NULL) return;
	
	hcct_node_t* tmp;
	for (tmp=node->first_child; tmp!=NULL; tmp=tmp->next_sibling)
		pruneTree(tree, tmp, min_counter);
			
	if (node->counter <= min_counter && node->first_child == NULL && node->routine_id != 0) {
		// detach node from the tree
		if (node->parent->first_child==node) {
			node->parent->first_child=node->next_sibling;
		} else {
			for (tmp=node->parent->first_child; tmp!=NULL; tmp=tmp->next_sibling) {
				if (tmp->next_sibling==node) {
					tmp->next_sibling=node->next_sibling;
					break;					
				}
			}
		}
		tree->nodes--;
		free(node);
	}
}

// print tree to screen
void printTree(hcct_node_t* node, int level) {
	if (node==NULL) return;
	
	hcct_node_t* tmp;
	int i;
	
	printf("|=");	
	for (i=0; i<level; ++i) printf("=");
	printf("> (%lu) %s\n", node->counter, node->info);
	
	for (tmp=node->first_child; tmp!=NULL; tmp=tmp->next_sibling)
		printTree(tmp, level+1);	
}

// process line from /proc maps file
int parseMemoryMapLine(char* line, UINT32 *start, UINT32 *end, UINT32 *offset, char** pathname) {

	UINT32 tmp;
	char *s;	
	
	// start address
	s=strtok(line, "-");			
	tmp=strtoul(s, NULL, 16);
	if (tmp==0) return 1;	
	*start=tmp;
	
	// end address
	s=strtok(NULL, " ");
	tmp=strtoul(s, NULL, 16);
	if (tmp==0) return 1;	
	*end=tmp;
	
	s=strtok(NULL, " "); // skip permissions column
	
	// offset
	s=strtok(NULL, " ");	
	tmp=strtoul(s, NULL, 16);
	*offset=tmp;
	
	s=strtok(NULL, " "); // skip dev column
	s=strtok(NULL, " "); // skip inode column
			
	// pathname
	s=strtok(NULL, " ");
	if (strlen(s)==1) {
		*pathname="<unknown>";
	} else {
		s[strlen(s)-1]='\0';
		*pathname=strdup(s);
	}
}

// TODO: call sites, shared libraries
void getFunctionFromAddress(char** s, UINT32 addr, hcct_tree_t* tree, hcct_map_t* map) {	
	#define BUFLEN 512
	FILE* fp;
	char buf[BUFLEN+1];
		
	char* command=malloc(13+strlen(tree->program_path)+1+strlen(tree->short_name)+10+8+1); // 8 characters for 32 bit address	
	sprintf(command, "addr2line -e %s/%s -f -s -p %lx", tree->program_path, tree->short_name, addr);
		
	fp=popen(command, "r");
	if (fp == NULL) {
		printf("Error: unable to execute addr2line!");
		exit(1);
	}
	
	free(command);
    
	if (fgets(buf, BUFLEN, fp) != NULL) {
		if (buf[0]!='?' && buf[1]!='?') { // addr2line worked!
			buf[strlen(buf)-1]='\0';
			*s=strdup(buf);
			pclose(fp);
			return;
		} // else: try to solve it manually (see below)
	} else {
		printf("Error: addr2line does not work!");
		exit(1);
	}

	pclose(fp);		
	
	// Try to solve it manually...
	
	for ( ; map!=NULL; map=map->next)
		if (addr >= map->start && addr <= map->end) {
			/* // Old method
			*s=malloc(33+2*8+strlen(map->pathname)+1); // 8 characters for each address
			sprintf(*s, "??? in memory region %lx-%lx mapped to %s", map->start, map->end, map->pathname);
			return;
			*/
			if (map->pathname[0]='/') { // dynamic library
				UINT32 offset= addr - map->start;
				
				char* command=malloc(13+strlen(map->pathname)+10+8+1); // 8 characters for 32 bit address	
				sprintf(command, "addr2line -e %s -f -s -p %lx", map->pathname, offset);
				fp=popen(command, "r");
			
				if (fp == NULL) {
					printf("Error: unable to execute addr2line!");
					exit(1);
				}
				
				free(command);
				
				// addr2line on a shared library
				if (fgets(buf, BUFLEN, fp) != NULL) {
					if (buf[0]!='?' && buf[1]!='?') { // addr2line worked!
						buf[strlen(buf)-1]='\0';
						*s=malloc(1+strlen(map->pathname)+2+strlen(buf)+1);
						sprintf(*s, "[%s] %s", map->pathname, buf);					
					} else {
						*s=malloc(33+2*8+strlen(map->pathname)+1); // 8 characters for each address
						sprintf(*s, "??? in memory region %lx-%lx mapped to %s", map->start, map->end, map->pathname);
					}
					pclose(fp);
					return;
				} else {
					printf("Error: addr2line does not work!");
					exit(1);
				} // end of addr2line block			
			}			
		}
	
	*s="<unknown>";	// address not solved :(
}

// create data structure for memory mapped regions
hcct_map_t* createMemoryMap(char* program) {
	FILE* mapfile;
	char* filename;	
			
	filename=malloc(strlen(program)+5); // .map\0
	sprintf(filename, "%s.map", program);
    
    mapfile=fopen(filename, "r");
    if (mapfile==NULL) {
        printf("Warning: unable to load file containing memory mapped regions!\n");
        free(filename);
        return NULL;
    }
    
    free(filename);
    
    // parse file
    char buf[BUF+1];
    char *s;

	hcct_map_t* first=(hcct_map_t*)malloc(sizeof(hcct_map_t));
	first->next=NULL;
	
	if (fgets(buf, BUF, mapfile)==NULL) {
		printf("Warning: file containing memory mapped regions is broken!\n");
		free(filename);
		free(first);
		return NULL;
	}
	
	UINT32 start, end, offset;
	char* pathname;

	// TODO: valore di ritorno
	parseMemoryMapLine(buf, &(first->start), &(first->end), &(first->offset), &(first->pathname));
	hcct_map_t *next=first, *tmp;
	
	while (fgets(buf, BUF, mapfile)!=NULL) {
		tmp=(hcct_map_t*)malloc(sizeof(hcct_map_t));		
		// TODO: valore di ritorno
		parseMemoryMapLine(buf, &(tmp->start), &(tmp->end), &(tmp->offset), &(tmp->pathname));
		next->next=tmp;
		next=tmp;		
	}
	
	next->next=NULL;
		       
    fclose(mapfile);
	
	return first;    
}

// create tree in memory
hcct_tree_t* createTree(FILE* logfile) {    
    char buf[BUF+1];
    char *s;
    
    hcct_tree_t *tree=malloc(sizeof(hcct_tree_t));
        
    // Syntax of the first two rows
    // c <tool> [<epsilon> <phi>] [<sampling_interval> <burst_length> <burst_enter_events>]
    // c <command> <process/thread id> <working directory>
    
    // First row
    if (fgets(buf, BUF, logfile)==NULL) {
		printf("Error: broken logfile\n");
		exit(1);
	}
    s=strtok(buf, " ");
    if (strcmp(s, "c")) {
        printf("Error: wrong format!\n");
        exit(1);
    }
    s=strtok(NULL, " ");
        
    if (strcmp(s, CCT_FULL_STRING)==0) {	
        tree->tool=CCT_FULL;        
        tree->burst_enter_events=0;
        printf("Log generated by tool: %s\n", CCT_FULL_STRING); // CCT_FULL_STRING ends by \n
    } else if (strcmp(s, LSS_FULL_STRING)==0) {
        tree->tool=LSS_FULL;
        tree->burst_enter_events=0;
        s=strtok(NULL, " ");        
        tree->epsilon=strtoul(s, &s, 0);
        s++;
        tree->phi=strtoul(s, &s, 0);
        printf("Log generated by tool: %s\n", LSS_FULL_STRING);
        printf("Parameters: epsilon=%lu phi=%lu\n", tree->epsilon, tree->phi);        
    } else if (strcmp(s, CCT_BURST_STRING)==0) {
        tree->tool=CCT_BURST;
        s=strtok(NULL, " ");        
        tree->sampling_interval=strtoul(s, &s, 0);
        s++;
        tree->burst_length=strtoul(s, &s, 0);
        s++;
        tree->burst_enter_events=strtoull(s, &s, 0);
        printf("Log generated by tool: %s\n", CCT_BURST_STRING);
        printf("Parameters: sampling_interval=%lu burst_length=%lu burst_enter_events=%llu\n",
                tree->sampling_interval, tree->burst_length, tree->burst_enter_events);
    } else if (strcmp(s, LSS_BURST_STRING)==0) {
        tree->tool=LSS_BURST;
        s=strtok(NULL, " ");        
        tree->epsilon=strtoul(s, &s, 0);
        s++;
        tree->phi=strtoul(s, &s, 0);
        s++;
        tree->sampling_interval=strtoul(s, &s, 0);
        s++;
        tree->burst_length=strtoul(s, &s, 0);
        s++;
        tree->burst_enter_events=strtoull(s, &s, 0);
        printf("Log generated by tool: %s\n", LSS_BURST_STRING);
        printf("Parameters: epsilon=%lu phi=%lu sampling_interval=%lu burst_length=%lu burst_enter_events=%llu\n",
                tree->epsilon, tree->phi, tree->sampling_interval, tree->burst_length, tree->burst_enter_events);
    }
    
    // Second row
    if (fgets(buf, BUF, logfile)==NULL) {
		printf("Error: broken logfile\n");
		exit(1);
	}
    s=strtok(buf, " ");
    if (strcmp(s, "c")) {
        printf("Error: wrong format!\n");
        exit(1);
    }
    
    char* short_name, *pid, *program_path;
    short_name=strtok(NULL, " ");    
    pid=strtok(NULL, " "); // useless
    program_path=strtok(NULL, "\n");
    
    tree->program_path=strdup(program_path);
    tree->short_name=strdup(short_name);
    
    printf("Instrumented program:");
    printf(" %s %s (PID: %s)\n", tree->program_path, tree->short_name, pid);
    
    // CHECK LOG PATH
    hcct_map_t* map=createMemoryMap(tree->short_name);	

    // I need a stack to reconstruct the tree
    hcct_pair_t tree_stack[STACK_MAX_DEPTH];
    int stack_idx=0;
    UINT32 nodes=0;
    
    UINT32 node_id, parent_id, counter, routine_id;
    UINT16 call_site;
        
    // Create root node
    hcct_node_t* root=malloc(sizeof(hcct_node_t));    
    
    if (fgets(buf, BUF, logfile)==NULL) {
		printf("Error: broken logfile\n");
		exit(1);
	}
    s=strtok(buf, " ");        
    if (strcmp(s, "v")) {
            printf("Error: wrong format!\n");
            exit(1);
    }
    
    tree_stack[0].node=root;    
    tree_stack[0].id=strtoul(&buf[2], &s, 16); // node_id
    s++; // skip " " character
    if (strtoul(s, &s, 16)!=0) { // parent_id should be 0
            printf("Error: root node cannot have a parent node!\n");
            exit(1);
    }
    s++;
    root->info="<dummy root node>";
    root->parent=NULL;
    root->first_child=NULL;
    root->next_sibling=NULL;
    root->counter=strtoul(s, &s, 0);    
    s++;
    root->routine_id=strtoul(s, &s, 16);
    s++;
    root->call_site=(unsigned short)strtoul(s, &s, 16);
    if (root->counter!=1 || root->routine_id!=0 || root->call_site!=0) {
            printf("Error: there's something strange in root node data (counter, routine_id or call_site)!\n");
            exit(1);
    }
    nodes++;    
        
    // Syntax: v <node id> <parent id> <counter> <routine_id> <call_site>    
    hcct_node_t *node, *parent, *tmp;
    while(1) {
        if (fgets(buf, BUF, logfile)==NULL) {
			printf("Error: broken logfile\n");
			exit(1);
		}
        s=strtok(buf, " ");        
        if (strcmp(s, "p")==0) break;
        if (strcmp(s, "v")) {
            printf("Error: wrong format!\n");
            exit(1);
        }
        
        node=malloc(sizeof(hcct_node_t));
        node_id=strtoul(&buf[2], &s, 16);
        s++;    
        parent_id=strtoul(s, &s, 16);
        s++;
        node->counter=strtoul(s, &s, 0);
        s++;
        node->routine_id=strtoul(s, &s, 16);
        s++;
        node->call_site=(unsigned short)strtoul(s, &s, 16);
        
        getFunctionFromAddress(&(node->info), node->routine_id, tree, map);
                
        // Attach node to the tree
        while (tree_stack[stack_idx].id!=parent_id && stack_idx>=0)
            stack_idx--;
        if (tree_stack[stack_idx].id!=parent_id) {
            printf("Error: log file is broken - missing node(s)\n");
            exit(1);            
        }        
        parent=tree_stack[stack_idx].node;
        node->parent=parent;
        node->first_child=NULL;
        node->next_sibling=NULL;
        if (parent->first_child==NULL)
            parent->first_child=node;
        else {
            // Attach as rightmost child
            for (tmp=parent->first_child; tmp->next_sibling!=NULL; tmp=tmp->next_sibling);
            tmp->next_sibling=node;                        
        }
        stack_idx++;
        tree_stack[stack_idx].node=node;
        tree_stack[stack_idx].id=node_id;
        nodes++;
    }
    
    // Syntax: p <num_nodes>
    tree->nodes=strtoul(&buf[2], &s, 0);
    if (tree->nodes!=nodes) { // Sanity check
        printf("Error: log file is broken - wrong number of nodes\n");
        exit(1);
    }        
    s++;
    tree->enter_events=strtoull(s, NULL, 0);
    
    printf("Total number of nodes: %lu\n", tree->nodes);
    printf("Total number of rtn_enter_events: %llu\n", tree->enter_events);
    
    tree->root=root;
        
    printf("Tree has been built.\n");
    
    return tree;
}

void freeTreeAux(hcct_node_t* root) {
    hcct_node_t *ptr;
    for (ptr=root->first_child; ptr!=NULL; ptr=ptr->next_sibling)
        freeTreeAux(ptr);
    free(root);
}

void freeTree(hcct_tree_t* tree) {
    if (tree->root!=NULL)
        freeTreeAux(tree->root);
    free(tree);
    printf("Tree has been deleted from memory.\n");
}

// ---------------------------------------------------------------------
//  routines adapted from metrics.c (PLDI version)
// ---------------------------------------------------------------------
UINT32 hotNodes(hcct_node_t* root, UINT32 threshold) {
    UINT32 h=0;
    hcct_node_t* child;
    for (child=root->first_child; child!=NULL; child=child->next_sibling)
        h += hotNodes(child,threshold);
    if (root->counter >= threshold) {
            // printf per stampa a video??
            return h+1; 
    }
    else return h;
}

UINT64 sumCounters(hcct_node_t* root) {
    UINT64 theSum=(UINT64)root->counter;
    hcct_node_t* child;
    for (child=root->first_child; child!=NULL; child=child->next_sibling)
        theSum += sumCounters(child);
    return theSum;
}

UINT32 hottestCounter(hcct_node_t* root) {
    UINT32 theHottest=root->counter;
    hcct_node_t* child;
    for (child=root->first_child; child!=NULL; child=child->next_sibling)  {
        UINT32 childHottest = hottestCounter(child);
        if (theHottest<childHottest) theHottest=childHottest;
    }
    return theHottest;
}

UINT32 largerThanHottest(hcct_node_t* root, UINT32 TAU, UINT32 hottest) {
    UINT32 num=0;
    hcct_node_t* child;
    if (root->counter*TAU >= hottest) num++;
    for (child=root->first_child; child!=NULL; child=child->next_sibling) 
        num += largerThanHottest(child,TAU,hottest);
    return num;
}
// ---------------------------------------------------------------------
//  end of routines adapted from metrics.c (PLDI version)
// ---------------------------------------------------------------------


int main(int argc, char* argv[]) {
    if (argc!=2) {
        printf("Syntax: %s <logfile>\n", argv[0]);
        exit(1);
    }
    FILE* logfile;
    logfile=fopen(argv[1], "r");
    if (logfile==NULL) {
        printf("The specified file does not exist!\n");
        exit(1);
    }
    hcct_tree_t *tree;
    tree=createTree(logfile);
    
    UINT64 stream_length=tree->burst_enter_events;
    if (stream_length==0) stream_length=tree->enter_events;
    printf("\nBefore pruning, tree has %lu nodes and stream length is %llu\n", tree->nodes, stream_length);
    
    printf("Using demo phi=0.001 as hotness threshold...\n\n");    
    UINT32 min_counter=stream_length/1000;    
    pruneTree(tree, tree->root, min_counter);
    
    printf("Number of nodes (hot and cold): %lu\n", tree->nodes);
    UINT32 hot=hotNodes(tree->root, min_counter);
    printf("Number of hot nodes (counter>%lu): %lu\n", min_counter, hot);
    UINT64 sum=sumCounters(tree->root);
    printf("Sum of counters: %llu\n", sum);
    UINT32 hottest=hottestCounter(tree->root);
    printf("Hottest counter: %lu\n", hottest);
    UINT32 closest=largerThanHottest(tree->root, 10, hottest); // TAU=10
    printf("Hottest nodes for TAU=10: %lu\n", closest);
    
    printf("\n");
    printTree(tree->root, 0);
    printf("\n");
        
    fclose(logfile);
    freeTree(tree);
    return 0;
}