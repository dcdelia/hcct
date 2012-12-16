#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "analysis.h"

// process line from /proc/<PID>/maps file
static int parseMemoryMapLine(char* line, ADDRINT *start, ADDRINT *end, ADDRINT *offset, char** pathname) {

	ADDRINT tmp;
	char *s;
	
	// syntax: <start_address-end_address> <perms> <offset> <dev> <inode> <pathname>
	
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

// free data structure for memory mapped regions
void freeMemoryMap(hcct_map_t* map) {
	hcct_map_t *m1, *m2;
	
	for (m1=map; m1!=NULL; ) {
		m2=m1->next;		
		free(m1->pathname);
		free(m1);
		m1=m2;
	}
}

// solve an address
hcct_sym_t* getFunctionFromAddress(ADDRINT addr, hcct_tree_t* tree, hcct_map_t* map) {
	FILE* fp;
	char buf[BUFLEN+1], command[BUFLEN+1];
	hcct_sym_t* sym=malloc(sizeof(hcct_sym_t));
		
	sprintf(command, "addr2line --e %s/%s -f -C %lx", tree->program_path, tree->short_name, addr);
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
				
				sprintf(command, "addr2line -e %s -f -C %lx 2> /dev/null", map->pathname, offset);
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

// free symbols
void freeSym(hcct_sym_t* sym) {
	if (sym==NULL) return;
	free(sym->name);
	free(sym->file);
	free(sym->image);
	free(sym);
}

