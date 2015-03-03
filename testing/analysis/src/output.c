#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h> // basename()

#include "common.h"

// auxiliary methods (for internal use only)
static char* printFileName(char* full_name);
static char* printNodeForScreen(hcct_node_t* node, hcct_tree_t* tree);
static char* printNodeForGraphviz(hcct_node_t* node, hcct_tree_t* tree);
static void printTreeAux(hcct_node_t* node, hcct_tree_t* tree, int level, char*(*print)(hcct_node_t*, hcct_tree_t*));
static void printGraphvizAux(hcct_node_t* node,  hcct_tree_t* tree, FILE* out, char*(*print)(hcct_node_t*, hcct_tree_t*));

/* Print a tree to screen (with symbols, if available) */
void printTree(hcct_tree_t* tree, char*(*print)(hcct_node_t*, hcct_tree_t*)) {
    if (tree == NULL) return;
    char*(*print_function)(hcct_node_t*, hcct_tree_t*) = print;
    if (print_function == NULL)
        print_function = printNodeForScreen;
    
    printTreeAux(tree->root, tree, 0, print_function);
}

/* Save a tree in Graphviz format */
void printGraphviz(hcct_tree_t* tree, FILE* out, char*(*print)(hcct_node_t*, hcct_tree_t*)) {    
    if (tree == NULL) return;
    char*(*print_function)(hcct_node_t*, hcct_tree_t*) = print;
    if (print_function == NULL)
        print_function = printNodeForGraphviz;
  
    fprintf(out, "digraph HCCT{\n");
    if (tree->root->routine_sym != NULL)
        fprintf(out, "\"%lx\" [label=\"%s\"];\n", (ADDRINT) tree->root, tree->root->routine_sym->name);
    else
        fprintf(out, "\"%lx\" [label=\"%lx\"];\n", (ADDRINT) tree->root, tree->root->routine_id); // TODO

    hcct_node_t* child;
    for (child = tree->root->first_child; child != NULL; child = child->next_sibling)
        printGraphvizAux(child, tree, out, print_function);

    fprintf(out, "}\n");
}

/* Save a tree in JSON format */
void printJSON(hcct_node_t* node, FILE* out) {
    if (node == NULL) return;

    if (node->routine_sym != NULL)
        fprintf(out, "{\"name\": \"%s\"\n", node->routine_sym->name);
    else
        fprintf(out, "{\"name\": \"%lx\"\n", node->routine_id); // TODO

    hcct_node_t* tmp;
    if (node->first_child != NULL) {
        fprintf(out, ", \"children\": [\n");
        printJSON(node->first_child, out);
        for (tmp = node->first_child->next_sibling; tmp != NULL; tmp = tmp->next_sibling) {
            fprintf(out, ", ");
            printJSON(tmp, out);
        }
        fprintf(out, "]");
    }

    fprintf(out, "}");
}

static char* printFileName(char* full_name) {
    char *a, *b;

    a = strtok(full_name, "/");
    if (a == NULL) return full_name;
    while (a != NULL) {
        b = a;
        a = strtok(NULL, "/");
    }
    return b;
}

static char* printNodeForScreen(hcct_node_t* node, hcct_tree_t* tree) {
    char buf[BUFLEN + 1];
    if (node->routine_sym != NULL && node->call_site_sym != NULL) {
        // symbols have been solved
        char *file_called, *image_called;
        char *file_caller, *image_caller;

        sprintf(buf, "(%lu)", node->counter); // counter

        file_called = printFileName(node->routine_sym->file);
        image_called = basename(node->routine_sym->image);
        if (!strcmp(image_called, tree->short_name)) { // do not show image name
            sprintf(buf + strlen(buf), " %s [%s]", node->routine_sym->name, file_called);
        } else {
            sprintf(buf + strlen(buf), " %s [%s (%s)]", node->routine_sym->name, file_called, image_called);
        }

        file_caller = printFileName(node->call_site_sym->file);
        image_caller = basename(node->call_site_sym->image);
        if (!strcmp(image_caller, tree->short_name)) { // do not show image name
            sprintf(buf + strlen(buf), " from %s [%s]", node->call_site_sym->name, file_caller);
        } else {
            sprintf(buf + strlen(buf), " from %s [%s (%s)]", node->call_site_sym->name, file_caller, image_caller);
        }
    } else {
        // no symbol resolution available
        sprintf(buf, "(%lu) %lx [%lx]\n", node->counter, node->routine_id, node->call_site);
    }

    return strdup(buf);
}

static char* printNodeForGraphviz(hcct_node_t* node, hcct_tree_t* tree) {
    char buf[2*BUFLEN + 1];    
    if (node != tree->root)
        sprintf(buf, "\"%lx\" -> \"%lx\" [label=\"calls: %lu\"];\n", (ADDRINT) node->parent, (ADDRINT) node, node->counter);    
    if (node->routine_sym != NULL) {
        sprintf(buf + strlen(buf), "\"%lx\" [label=\"%s\"];", (ADDRINT) node, node->routine_sym->name);
    } else {
        sprintf(buf + strlen(buf), "\"%lx\" [label=\"%lx\"];", (ADDRINT) node, node->routine_id);
    }    
    return strdup(buf);
}

static void printTreeAux(hcct_node_t* node, hcct_tree_t* tree, int level, char*(*print)(hcct_node_t*, hcct_tree_t*)) {
    if (node == NULL) return;

    hcct_node_t* tmp;
    int i;

    printf("|=");
    for (i = 0; i < level; ++i) printf("=");

    char* summary = print(node, tree);
    printf("> %s\n", summary);
    free(summary);

    for (tmp = node->first_child; tmp != NULL; tmp = tmp->next_sibling)
        printTreeAux(tmp, tree, level + 1, print);
}

static void printGraphvizAux(hcct_node_t* node,  hcct_tree_t* tree, FILE* out, char*(*print)(hcct_node_t*, hcct_tree_t*)) {
    if (node == NULL) return;

    hcct_node_t* tmp;

    char* representation = print(node, tree);
    fprintf(out, "%s\n", representation);
    free(representation);    

    for (tmp = node->first_child; tmp != NULL; tmp = tmp->next_sibling)
        printGraphvizAux(tmp, tree, out, print);
}