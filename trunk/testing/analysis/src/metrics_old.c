static void printTreeWithScores(hcct_node_t* node, int level) {
    if (node == NULL) return;

    hcct_node_t* tmp;
    compare_info_t* info = (compare_info_t*) node->extra_info;
    int i;

    printf("|=%d", level + 1);
    //for (i = 0; i < level; ++i) printf("=");
    printf("> %s - S %.2f, L %.2f, C %.2f, W %.2f\n", node->routine_sym->name, info->score, info->local_score, info->children_score, info->weighted_score);

    for (tmp = node->first_child; tmp != NULL; tmp = tmp->next_sibling)
        printTreeWithScores(tmp, level + 1);
}

static void compareNodeWithChildrenFromOtherTree(hcct_node_t* node, hcct_node_t* second_parent) {
    if (node == NULL || second_parent == NULL) {
        // I should never reach this case in a proper usage scenario
        return;
    }

    hcct_node_t* best_match = NULL; // among second_parent's children
    hcct_node_t* child;
    double best_score = 0.0;
    double localSimilarity = 0.0;
    double childrenSimilarity = 0.0;
    for (child = second_parent->first_child; child != NULL; child = child->next_sibling) {
        localSimilarity = compareNodesLocally(node, child);
        childrenSimilarity = computeChildrenSimilarity(node, child);
        //double rawScore = localSimilarity + childrenSimilarity * COMPARE_CHILDREN_SIMILARITY_WEIGHT;
        double rawScore = localSimilarity + childrenSimilarity;
        // strictly > is required: you might pick a node with 0-similarity!
        if (rawScore > best_score && ((compare_info_t*) child->extra_info)->external_status != COMPARE_STATUS_FP) {
            best_score = rawScore;
            best_match = child;
        }
    }

    // store the information
    if (best_match == NULL) {
        setSubtreeAsNotMatched(node);
    } else {
        // set the node in the second tree as already picked
        ((compare_info_t*) best_match->extra_info)->external_status = COMPARE_STATUS_FP;

        // now save match info in the node from first tree
        double nodeScore = localSimilarity + childrenSimilarity * COMPARE_CHILDREN_SIMILARITY_WEIGHT;
        if (nodeScore > 1.0)
            nodeScore = 1.0;
        compare_info_t* info = calloc(sizeof (compare_info_t), 1);
        info->local_score = localSimilarity;
        info->children_score = childrenSimilarity;
        info->score = nodeScore;
        if (best_match->parent == NULL) {
            info->weighted_score = nodeScore;
        } else {
            if (best_match->parent->extra_info == NULL) {
                printf("[ERROR] Tree is corrupted!!\n");
            } else {
                info->weighted_score += COMPARE_PARENT_WEIGHT * ((compare_info_t*) best_match->parent->extra_info)->weighted_score;
                info->weighted_score += (1.0 - COMPARE_PARENT_WEIGHT) * nodeScore;
            }
        }
        info->internal_status = COMPARE_STATUS_OK;
        info->matched_node = best_match;
        node->extra_info = info;

        // now best_match is the root of the subtree to analize
        hcct_node_t* child;
        for (child = node->first_child; child != NULL; child = child->next_sibling) {
            compareNodeWithChildrenFromOtherTree(child, best_match);
        }
    }

}

static void compareTrees(hcct_tree_t* first, hcct_tree_t* second) {
    if (first == NULL || first->root == NULL || second == NULL || second->root == NULL) {
        printf("[WARNING] At least one of the two trees is empty!\n");
        return;
    }

    // create required extra fields for nodes in both trees
    initializeExtraInfo(first->root, COMPARE_STATUS_NE, COMPARE_STATUS_NE);
    initializeExtraInfo(second->root, COMPARE_STATUS_NE, COMPARE_STATUS_NE);

    // create a dummy root for the second tree
    compare_info_t* info = calloc(sizeof (compare_info_t), 1);
    info->internal_status = COMPARE_STATUS_OK;
    info->local_score = 1.0;
    info->weighted_score = 1.0;    
    hcct_node_t* dummy = calloc(sizeof (hcct_node_t), 1);
    dummy->counter = 1;
    dummy->first_child = first->root;
    dummy->extra_info = info;
    second->root->parent = dummy;

    compareNodeWithChildrenFromOtherTree(first->root, dummy);

    second->root->parent = NULL;
    free(dummy->extra_info);
    free(dummy);
}
