#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <libgen.h> // basename()
#include <locale.h> // setlocale()
#include <sys/types.h> // pid_t

#include "analysis.h"

void freeSym(hcct_sym_t* sym) {
	if (sym==NULL) return; // for safety
	free(sym->name);
	free(sym->file);
	free(sym->image);
	free(sym);
}

void freeTreeAux(hcct_node_t* node) {	
    //if (node==NULL) return; // not needed
    hcct_node_t *ptr, *tmp;
    for (ptr=node->first_child; ptr!=NULL;) {
		tmp=ptr->next_sibling;
        freeTreeAux(ptr);
        ptr=tmp;
    }
    
    freeSym(node->routine_sym);
    freeSym(node->call_site_sym);
    free(node);    
}

void freeTree(hcct_tree_t* tree) {    
    if (tree->root!=NULL) // for safety
		freeTreeAux(tree->root);	
	free(tree->short_name);
	free(tree->program_path);
    free(tree);
    printf("\nTree has been deleted from memory.\n");
}

// remove from tree every node such that node->counter <= min_counter (Space Saving algorithm)
void pruneTree(hcct_tree_t* tree, hcct_node_t* node, UINT32 min_counter) {
	if (node==NULL) return;
	
	hcct_node_t *ptr, *tmp;
		
	for (ptr=node->first_child; ptr!=NULL; ) {
		tmp=ptr->next_sibling;
		pruneTree(tree, ptr, min_counter);
		ptr=tmp;		
	}
			
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
		freeSym(node->routine_sym);
		freeSym(node->call_site_sym);
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
	if (node->routine_sym->image[0]=='/')	
		printf("> (%lu) %s in %s [%s]\n", node->counter, node->routine_sym->name, node->routine_sym->file, basename(node->routine_sym->image));
	else
		printf("> (%lu) %s in %s [%s]\n", node->counter, node->routine_sym->name, node->routine_sym->file, node->routine_sym->image);
	
	for (tmp=node->first_child; tmp!=NULL; tmp=tmp->next_sibling)
		printTree(tmp, level+1);	
}

void printGraphvizAux(hcct_node_t* node, FILE* out) {
	if (node==NULL) return;
	
	hcct_node_t* tmp;
	
	fprintf(out, "\"%lx\" -> \"%lx\" [label=\"calls: %lu\"];\n", (ADDRINT)node->parent, (ADDRINT)node, node->counter);	
	fprintf(out, "\"%lx\" [label=\"%s\"];\n", (ADDRINT)node, node->routine_sym->name);
		
	for (tmp=node->first_child; tmp!=NULL; tmp=tmp->next_sibling)
		printGraphvizAux(tmp, out);
	
}

void printGraphviz(hcct_tree_t* tree, FILE* out) {
	fprintf(out, "digraph HCCT{\n");
	fprintf(out, "\"%lx\" [label=\"%s\"];\n", (ADDRINT)tree->root, tree->root->routine_sym->name);
	
	hcct_node_t* child;
	for (child=tree->root->first_child; child!=NULL; child=child->next_sibling)
		 printGraphvizAux(child, out);		
	
	fprintf(out, "}\n");
}

void printD3json(hcct_node_t* node, FILE* out) {
	if (node==NULL) return;
	
	fprintf(out, "{\"name\": \"%s\"\n", node->routine_sym->name);
	
	hcct_node_t* tmp;
	if (node->first_child!=NULL) {
		fprintf(out, ", \"children\": [\n");
		printD3json(node->first_child, out);
		for (tmp=node->first_child->next_sibling; tmp!=NULL; tmp=tmp->next_sibling) {
			fprintf(out, ", ");
			printD3json(tmp, out);
		}
		fprintf(out, "]");
	}
	
	fprintf(out, "}");
}


// process line from /proc maps file
int parseMemoryMapLine(char* line, ADDRINT *start, ADDRINT *end, ADDRINT *offset, char** pathname) {

	ADDRINT tmp;
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
		*pathname=strdup("<unknown>");
	} else {
		s[strlen(s)-1]='\0';
		*pathname=strdup(s);
	}
	
	return 0;
}

hcct_sym_t* getFunctionFromAddress(ADDRINT addr, hcct_tree_t* tree, hcct_map_t* map) {	
	FILE* fp;
	char buf[BUFLEN+1], command[BUFLEN+1];
	hcct_sym_t* sym=malloc(sizeof(hcct_sym_t));
		
	sprintf(command, "addr2line --e %s/%s -f -s -C %lx", tree->program_path, tree->short_name, addr);
	fp=popen(command, "r");
	if (fp == NULL) {
		printf("Error: unable to execute addr2line!");
		exit(1);
	}
	
	if (fgets(buf, BUFLEN, fp) != NULL) {				
		if (buf[0]!='?' && buf[1]!='?') { // addr2line worked!						
			buf[strlen(buf)-1]='\0';
			sym->name=strdup(buf);						
			if (fgets(buf, BUFLEN, fp) != NULL) { // addr2line will produce two output lines								
				buf[strlen(buf)-1]='\0';
				sym->file=strdup(buf);								
				sym->image=strdup(tree->short_name);				
				pclose(fp);
				return sym;
			} else {
				printf("Error: addr2line does not work!");
				exit(1);
			}
		} // else: try to solve it later
	} else {
		printf("Error: addr2line does not work!");
		exit(1);
	}
		
	pclose(fp);			
		
	for ( ; map!=NULL; map=map->next)
		if (addr >= map->start && addr <= map->end) {			
			if (map->pathname[0]!='[' && strcmp(map->pathname, "<unknown>")) { // avoid [heap], [stack] ...				
				ADDRINT offset= addr - map->start;								
				
				sprintf(command, "addr2line -e %s -f -s -C %lx 2> /dev/null", map->pathname, offset);
				fp=popen(command, "r");			
				if (fp == NULL) {
					printf("Error: unable to execute addr2line!");
					exit(1);
				}
				
				// addr2line on a shared library
				if (fgets(buf, BUFLEN, fp) != NULL)
					if (buf[0]!='?' && buf[1]!='?') { // addr2line worked!
						buf[strlen(buf)-1]='\0';
						sym->name=strdup(buf);
						if (fgets(buf, BUFLEN, fp) != NULL) { // addr2line will produce two output lines
							buf[strlen(buf)-1]='\0';
							sym->file=strdup(buf);							
							sym->image=strdup(map->pathname);
							pclose(fp);
							return sym;						
						} else {
							printf("Error: addr2line does not work!");
							exit(1);	
						}
					}					
				
				// addr2line not executed, address not recognized or invalid format
				sprintf(buf, "%lx", addr);
				sym->name=strdup(buf);
				sprintf(buf, "%lx-%lx", map->start, map->end);
				sym->file=strdup(buf);
				sym->image=strdup(map->pathname);
								
				pclose(fp);					
				return sym;					
			} else {
				sprintf(buf, "%lx", addr);
				sym->name=strdup(buf);
				sprintf(buf, "%lx-%lx", map->start, map->end);
				sym->file=strdup(buf);
				sym->image=strdup(map->pathname);
				return sym;
			}
		}
			
	// address is not in the map!
	sprintf(buf, "%lx", addr);
	sym->name=strdup(buf);
	sym->file=strdup("<unknown>");
	sym->image=strdup("<unknown>");	
	return sym;
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
    char buf[BUFLEN+1];    

	hcct_map_t* first=(hcct_map_t*)malloc(sizeof(hcct_map_t));
	first->next=NULL;
	
	if (fgets(buf, BUFLEN, mapfile)==NULL) {
		printf("Warning: file containing memory mapped regions is broken!\n");
		free(filename);
		free(first);
		return NULL;
	}
	
	int ret;
	ret=parseMemoryMapLine(buf, &(first->start), &(first->end), &(first->offset), &(first->pathname));
	if (ret) {
			printf("Error: map file seems to be broken!\n");
			exit(1);
	}
	
	hcct_map_t *next=first, *tmp;
	
	while (fgets(buf, BUFLEN, mapfile)!=NULL) {
		tmp=(hcct_map_t*)malloc(sizeof(hcct_map_t));			
		ret=parseMemoryMapLine(buf, &(tmp->start), &(tmp->end), &(tmp->offset), &(tmp->pathname));
		if (ret) {
			printf("Error: map file seems to be broken!\n");
			exit(1);
		}
		next->next=tmp;
		next=tmp;		
	}
	
	next->next=NULL;
		       
    fclose(mapfile);
	
	return first;    
}

void freeMemoryMap(hcct_map_t* map) {
	hcct_map_t *m1, *m2;
	
	for (m1=map; m1!=NULL; ) {
		m2=m1->next;		
		free(m1->pathname);
		free(m1);
		m1=m2;
	}
}

// create tree in memory
hcct_tree_t* createTree(FILE* logfile) {    
    char buf[BUFLEN+1];
    char *s;
    
    hcct_tree_t *tree=malloc(sizeof(hcct_tree_t));
    tree->phi=0;
        
    // Syntax of the first two rows
    // c <tool> [<epsilon>] [<sampling_interval> <burst_length> <exhaustive_enter_events>]
    // c <command> <process/thread id> <working directory>
    
    // First row
    if (fgets(buf, BUFLEN, logfile)==NULL) {
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
        tree->epsilon=0;
        tree->exhaustive_enter_events=0;
        printf("Log generated by tool: %s\n", CCT_FULL_STRING); // CCT_FULL_STRING ends by \n
    } else if (strcmp(s, LSS_FULL_STRING)==0) {
        tree->tool=LSS_FULL;
        tree->exhaustive_enter_events=0;
        s=strtok(NULL, " ");        
        tree->epsilon=strtod(s, &s);
        if (tree->epsilon==0) {
			setlocale(LC_NUMERIC,"");
			tree->epsilon=strtod(s, &s);
		}        
        printf("Log generated by tool: %s\n", LSS_FULL_STRING);
        printf("Parameters: epsilon=%f\n", tree->epsilon);        
    } else if (strcmp(s, CCT_BURST_STRING)==0) {        
        tree->tool=CCT_BURST;
        tree->epsilon=0;
        s=strtok(NULL, " ");        
        tree->sampling_interval=strtoul(s, &s, 0);
        s++;
        tree->burst_length=strtoul(s, &s, 0);
        s++;
        tree->exhaustive_enter_events=strtoull(s, &s, 0);
        printf("Log generated by tool: %s\n", CCT_BURST_STRING);
        printf("Parameters: sampling_interval=%lu burst_length=%lu exhaustive_enter_events=%llu\n",
                tree->sampling_interval, tree->burst_length, tree->exhaustive_enter_events);
    } else if (strcmp(s, LSS_BURST_STRING)==0) {
        tree->tool=LSS_BURST;
        s=strtok(NULL, " ");        
        tree->epsilon=strtod(s, &s);
        if (tree->epsilon==0) {
			setlocale(LC_NUMERIC,"");
			tree->epsilon=strtod(s, &s);
		}
        s++;
        tree->sampling_interval=strtoul(s, &s, 0);
        s++;
        tree->burst_length=strtoul(s, &s, 0);
        s++;
        tree->exhaustive_enter_events=strtoull(s, &s, 0);
        printf("Log generated by tool: %s\n", LSS_BURST_STRING);
        printf("Parameters: epsilon=%f sampling_interval=%lu (ns) burst_length=%lu (ns) exhaustive_enter_events=%llu\n",
                tree->epsilon, tree->sampling_interval, tree->burst_length, tree->exhaustive_enter_events);
    }
    
    // Second row
    if (fgets(buf, BUFLEN, logfile)==NULL) {
		printf("Error: broken logfile\n");
		exit(1);
	}
    s=strtok(buf, " ");
    if (strcmp(s, "c")) {
        printf("Error: wrong format!\n");
        exit(1);
    }
    
    char* short_name, *tid, *program_path;
    short_name=strtok(NULL, " ");    
    tid=strtok(NULL, " ");
    program_path=strtok(NULL, "\n");
    
    // Remove ./ from program name
    if (short_name[0]=='.' && short_name[1]=='/')
		tree->short_name=strdup(short_name+2);
	else
		tree->short_name=strdup(short_name);
    
    tree->program_path=strdup(program_path);
    tree->tid=(pid_t)strtol(tid, NULL, 0);
    
    printf("Instrumented program:");
    printf(" %s %s (TID: %d)\n", tree->program_path, tree->short_name, tree->tid);
    
    // create map from .map file    
    hcct_map_t* map=createMemoryMap(tree->short_name);	

    // I need a stack to reconstruct the tree
    hcct_pair_t tree_stack[STACK_MAX_DEPTH];
    int stack_idx=0;
    UINT32 nodes=0;
    
    ADDRINT node_id, parent_id;    
        
    // Create root node
    hcct_node_t* root=malloc(sizeof(hcct_node_t));
    root->parent=NULL;
    root->first_child=NULL;
    root->next_sibling=NULL;  
    
    if (fgets(buf, BUFLEN, logfile)==NULL) {
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
        
    root->counter=strtoul(s, &s, 0);    
    s++;
    root->routine_id=strtoul(s, &s, 16);
    s++;
    root->call_site=strtoul(s, &s, 16);
    if (root->counter!=1 || root->routine_id!=0 || root->call_site!=0) {
            printf("Error: there's something strange in root node data (counter, routine_id or call_site)!\n");
            exit(1);
    }
    
    // remaining fields    
    root->routine_sym=malloc(sizeof(hcct_sym_t));
    root->routine_sym->name=strdup("<dummy>");
    root->routine_sym->file=strdup("<dummy>");    
    root->routine_sym->image=strdup(tree->short_name);
    
    root->call_site_sym=malloc(sizeof(hcct_sym_t));
    root->call_site_sym->name=strdup("<dummy>");
    root->call_site_sym->file=strdup("<dummy>");    
    root->call_site_sym->image=strdup(tree->short_name);
    
    nodes++;    
        
    // Syntax: v <node id> <parent id> <counter> <routine_id> <call_site>    
    hcct_node_t *node, *parent, *tmp;
    while(1) {
        if (fgets(buf, BUFLEN, logfile)==NULL) {
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
		node->first_child=NULL;
		node->next_sibling=NULL;
		
        node_id=strtoul(&buf[2], &s, 16);
        s++;    
        parent_id=strtoul(s, &s, 16);
        s++;
        node->counter=strtoul(s, &s, 0);
        s++;
        node->routine_id=strtoul(s, &s, 16);
        s++;
        node->call_site=strtoul(s, &s, 16);
        
        // solve addresses
        node->routine_sym=getFunctionFromAddress(node->routine_id, tree, map);
        node->call_site_sym=getFunctionFromAddress(node->call_site, tree, map);
                
        // Attach node to the tree
        while (tree_stack[stack_idx].id!=parent_id && stack_idx>=0)
            stack_idx--;
        if (tree_stack[stack_idx].id!=parent_id) {
            printf("Error: log file is broken - missing node(s)\n");
            exit(1);            
        }        
        parent=tree_stack[stack_idx].node;
        node->parent=parent;        
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
    
    tree->sampled_enter_events=strtoull(s, NULL, 0);
    if (tree->tool==CCT_FULL || tree->tool==LSS_FULL)
		tree->exhaustive_enter_events=tree->sampled_enter_events;
    
    printf("Total number of nodes: %lu\n", tree->nodes);
    printf("Total number of rtn_enter_events: %llu\n", tree->exhaustive_enter_events);
    
    if (tree->tool==CCT_BURST || tree->tool==LSS_BURST)
		printf("Total number of sampled rtn_enter_events: %llu\n", tree->sampled_enter_events);    
    
    tree->root=root;
        
    printf("\nTree has been built.\n");
    
    freeMemoryMap(map);
    
    return tree;
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
    
    double phi=0;
    
    if (argc<2 || argc >3) {
        printf("Syntax: %s <logfile> [<PHI>]\n", argv[0]);
        exit(1);
    }
    
    if (argc==3) {
		char* end;
		double d=strtod(argv[2], &end);		
		if (d<=0 || d >= 1.0) { // 0 is an error code            			
            printf("WARNING: invalid value specified for phi, using default (phi=10*epsilon) instead\n");            
        } else phi=d;		
	} else printf("No value specified for phi, using default (phi=10*epsilon) instead\n");
	
    FILE* logfile;    
    logfile=fopen(argv[1], "r");
    if (logfile==NULL) {
        printf("The specified file does not exist!\n");
        exit(1);
    }
    hcct_tree_t *tree;
    tree=createTree(logfile);    
    fclose(logfile);
    
    if (tree->tool==CCT_FULL || tree->tool==LSS_FULL) // FIX HERE
		printf("\nBefore pruning, tree has %lu nodes and stream length is %llu.\n", tree->nodes, tree->exhaustive_enter_events);
	else
		printf("\nBefore pruning, tree has %lu nodes and length of sampled stream is %llu.\n", tree->nodes, tree->sampled_enter_events);	
    
    if (tree->epsilon==0 && ( tree->tool==CCT_FULL || tree->tool == CCT_BURST ) ) {
		if (phi==0) tree->epsilon=1.0/EPSILON;
		else tree->epsilon=phi*10;
    }
    
    if (phi==0) phi=tree->epsilon*10;
	else if (phi <= tree->epsilon) {
		phi=tree->epsilon*10;
		printf("WARNING: PHI must be greater than EPSILON, using PHI=10*EPSILON instead");
	}
	
	printf("\nValue chosen for phi: %f\n", phi);
	tree->phi=phi;	
    UINT32 min_counter=(UINT32)(phi*tree->exhaustive_enter_events);    
    pruneTree(tree, tree->root, min_counter);
    
    //printf("Value chosen for phi: %f\n", phi);
    printf("Number of nodes (hot and cold): %lu\n", tree->nodes);
    UINT32 hot=hotNodes(tree->root, min_counter);
    printf("Number of hot nodes (counter>%lu): %lu\n", min_counter, hot);
    UINT64 sum=sumCounters(tree->root);
    printf("Sum of counters: %llu\n", sum);
    UINT32 hottest=hottestCounter(tree->root);
    printf("Hottest counter: %lu\n", hottest);
    UINT32 closest=largerThanHottest(tree->root, 10, hottest); // TAU=10
    printf("Hottest nodes for TAU=10: %lu\n", closest);
    
    // print tree to screen
    printf("\n");
    printTree(tree->root, 0);
    printf("\n");
    
    // to be reused from now on!
    char tmp_buf[BUFLEN+1];
    //char command[BUFLEN+1];
            
    // create graphviz output file
    FILE* outgraph;    
    sprintf(tmp_buf, "%s-%d.dot", tree->short_name, tree->tid);
    char* outgraph_name=strdup(tmp_buf); // will be reused later!
    outgraph=fopen(outgraph_name, "w+"); // same size
    printf("Saving HCCT in GraphViz file %s...", outgraph_name);
    printGraphviz(tree, outgraph);
    printf(" done!\n");    
    fclose(outgraph);
    
    /* Currently almost useless
    // create png graph        
    sprintf(tmp_buf, "%s-%d.png", tree->short_name, tree->tid);        
    sprintf(command, "dot -Tpng %s -o %s &> /dev/null", outgraph_name, tmp_buf);        
	if (system(command)!=0) printf("Please check that GraphViz is installed in order to generate PNG graph!\n");
	else printf("PNG graph %s generated successfully!\n", tmp_buf);
	
	// create svg graph (very useful for large graphs)
	sprintf(tmp_buf, "%s-%d.svg", tree->short_name, tree->tid);
	sprintf(command, "dot -Tsvg %s -o %s &> /dev/null", outgraph_name, tmp_buf);	
	if (system(command)!=0) printf("Please check that GraphViz is installed in order to generate SVG graph!\n");
	else printf("SVG graph %s generated successfully!\n", tmp_buf);	
	*/
	free(outgraph_name);
           
    /* Currently almost useless
    // create JSON file for D3	
    FILE* outjson;
    sprintf(tmp_buf, "%s-%d.json", tree->short_name, tree->tid);
    printf("Saving HCCT in JSON file %s for D3...", tmp_buf); 
    outjson=fopen(tmp_buf, "w+");
    printD3json(tree->root, outjson);
    printf(" done!\n\n");
    fclose(outjson);
    */
       
    freeTree(tree);
            
    return 0;
}
