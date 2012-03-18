/* =====================================================================
 *  metrics.c
 * =====================================================================

 *  Author:         (c) 2010 D.C. D'Elia, C. Demetrescu, I. Finocchi
 *  License:        See the end of this file for license information
 *  Created:        November 4, 2010
 *  Module:         hcct

 *  Last changed:   $Date: 2010/11/20 07:22:14 $
 *  Changed by:     $Author: pctips $
 *  Revision:       $Revision: 1.27 $
*/


#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "metrics.h"
#include "pool.h"

#if COMPUTE_HOT_PER_PHI==1
#include "cct.h"
#endif


#define DEBUG               0
#define BASE                1.25
#define DEGREE_OVERLAP(j)   (100.0-100.0/pow(BASE,(j)))



// ---------------------------------------------------------------------
//  is_always_monitored
// ---------------------------------------------------------------------
// Every cct node is considered monitored
int is_always_monitored(cct_node_t* node) {
    return 1;
}


// ---------------------------------------------------------------------
//  nodes
// ---------------------------------------------------------------------
UINT32 nodes(cct_node_t* root) {
    UINT32 n=1;
    cct_node_t* child;
    for (child=root->first_child; child!=NULL; child=child->next_sibling)
        n+=nodes(child);
    return n;
}



// ---------------------------------------------------------------------
//  hotNodes
// ---------------------------------------------------------------------
UINT32 hotNodes(cct_node_t* root, UINT32 threshold, 
                int (*is_monitored)(cct_node_t* node)) {
    UINT32 h=0;
    cct_node_t* child;
    for (child=root->first_child; child!=NULL; child=child->next_sibling)
        h += hotNodes(child,threshold,is_monitored);
    if (is_monitored(root)==1 && root->counter >= threshold) {
            //printf("hot = %lu  %p  %lu  %lu\n",h+1,root,root->counter,threshold);
            return h+1; 
    }
    else return h;
}


// ---------------------------------------------------------------------
//  sum
// ---------------------------------------------------------------------
UINT64 sum(cct_node_t* root) {
    UINT64 theSum=(UINT64)root->counter;
    cct_node_t* child;
    for (child=root->first_child; child!=NULL; child=child->next_sibling)
        theSum += sum(child);
    return theSum;
}


// ---------------------------------------------------------------------
//  hottest
// ---------------------------------------------------------------------
UINT32 hottest(cct_node_t* root) {
    UINT32 theHottest=root->counter;
    cct_node_t* child;
    for (child=root->first_child; child!=NULL; child=child->next_sibling)  {
        UINT32 childHottest = hottest(child);
        if (theHottest<childHottest) theHottest=childHottest;
    }
    return theHottest;
}


// ---------------------------------------------------------------------
//  larger_than_hottest
// ---------------------------------------------------------------------
UINT32 larger_than_hottest(cct_node_t* root, UINT32 TAU, UINT32 H2) {
    UINT32 num=0;
    cct_node_t* child;
    if (root->counter >= H2/TAU) num++;
    for (child=root->first_child; child!=NULL; child=child->next_sibling) 
        num += larger_than_hottest(child,TAU,H2);
    return num;
}


// ---------------------------------------------------------------------
//  uncoveredFrequency
// ---------------------------------------------------------------------
// max frequency and sum of frequencies of CCT nodes uncovered in HCCT
void uncoveredFrequency(cct_node_t* root, cct_node_t* cct_root,
                        UINT64* sum, UINT32* max, UINT32* uncovered) { 
    cct_node_t* child, *cct_child;
    UINT32 tmp_max;
    
    if (cct_root==NULL) return;
    
    if (root==NULL)  {
        *max=cct_root->counter;
        *sum+=(UINT64)*max;
        (*uncovered)+=1;
    }
    else *max=0;
    
    for (cct_child=cct_root->first_child; cct_child!=NULL; cct_child=cct_child->next_sibling) {
        if (root==NULL) child=NULL;
        else for (child=root->first_child; child!=NULL; child=child->next_sibling) 
                if (cct_child->routine_id == child->routine_id &&
                    cct_child->call_site == child->call_site) break;
        uncoveredFrequency(child,cct_child,sum,&tmp_max,uncovered);
        if (tmp_max>*max) *max=tmp_max;
    }
}


// ---------------------------------------------------------------------
//  dumpTreeArray
// ---------------------------------------------------------------------
void dumpTreeArray(cct_node_t* root, UINT32 *pos, cct_node_t **dump) {
    cct_node_t* child;
    if (root==NULL) return;
    dump[*pos]=root;
    *pos = *pos +1;
    for (child=root->first_child; child!=NULL; child=child->next_sibling)
        dumpTreeArray(child,pos,dump);
}

// Array of CCT nodes comparison function
int compare (const void * a, const void * b) {
  return (*(cct_node_t**)b)->counter - (*(cct_node_t**)a)->counter;
}


// ---------------------------------------------------------------------
//  dump_max_phi
// ---------------------------------------------------------------------
// compute largest value of phi that guarantees degree of overlap >= 1%, 
// 2%, 3%, ..., 99%, 100%
void dump_max_phi(cct_node_t* root, logfile_t* f, driver_t* driver) { 
    
    UINT32 i;
    UINT32 n = nodes(root);
    UINT64 N = driver->rtn_enter_events;
    
    #if DEBUG == 1
    printf("CCT nodes = %lu\n",n);
    #endif

    // Alloc array of CCT nodes
    cct_node_t **dump = calloc((size_t) n, (size_t) sizeof(cct_node_t *));
    if (dump==NULL) panic("Failed creation of CCT array dump\n");

    // Fill in array of CCT nodes
    UINT32 pos = 0;
    dumpTreeArray(root,&pos,dump);

    // Sort nodes by decreasing frequencies
    qsort(dump,n,sizeof(cct_node_t *),compare);

    #if 0
    printf("\nAfter sort: ");
    for (i=0; i<200; i++) printf("%lu  ",dump[i]->routine_id);
    #endif

    // Alloc max Phi array
    UINT32 MAX_PHI_SIZE = (UINT32)(log(1000.0)/log(BASE));

    UINT32 *maxPhiArray = calloc(MAX_PHI_SIZE, (size_t) sizeof(UINT32));
    if (maxPhiArray==NULL) panic("Failed creation of maxPhiArray\n");

    UINT32 j = 0;
    UINT64 prefixSum = 0;
    
    // Compute max Phi array
    for (i=0; i<n; i++) {
        prefixSum += (UINT64) dump[i]->counter;
        while (j<MAX_PHI_SIZE && prefixSum*100 >= N*DEGREE_OVERLAP(j)) {
            maxPhiArray[j] = i;
            j++;
        }
        if (j>=MAX_PHI_SIZE) break;
    }

    #if DEBUG == 1
    printf("\n\nMax phi values:\n");
    for (i=0; i<MAX_PHI_SIZE; i++) 
        printf("%f%%  phi=%.9f\n", DEGREE_OVERLAP(i), 
            (double) dump[maxPhiArray[i]]->counter / (double) N); 
    #endif
   
    // header starts with # so that it can be skipped by gnuplot
    logfile_add_to_header(f, "%-14s ", "#benchmark");
    logfile_add_to_header(f, "%-14s ", "deg overlap ");
    logfile_add_to_header(f, "%-14s ", "max phi ");
    logfile_add_to_header(f, "%-12s ", "CCT nodes ");
    logfile_add_to_header(f, "%-12s ", "hot nodes ");
    logfile_add_to_header(f, "%-14s ", "(%) ");

    for (i=0; i<MAX_PHI_SIZE; i++) {
        logfile_add_to_row   (f, "%-14s ", driver->benchmark);
        logfile_add_to_row   (f, "%-14.9f ", DEGREE_OVERLAP(i));    
        logfile_add_to_row   (f, "%-14.9f ", 
                (double) dump[maxPhiArray[i]]->counter / (double) N);    
        logfile_add_to_row   (f, "%-12lu ", n);    
        logfile_add_to_row   (f, "%-12lu ", maxPhiArray[i]);    
        logfile_add_to_row   (f, "%-14.9f ", maxPhiArray[i]*100.0 / n);    
        logfile_new_row(f);
    }

    logfile_add_to_row(f, "%-14s ", driver->benchmark);
    logfile_add_to_row(f, "%-14.9f ", 100.0);
    logfile_add_to_row(f, "%-14.9f ", (double)dump[n-1]->counter/(double)N);
    logfile_add_to_row(f, "%-12lu ", n);
    logfile_add_to_row(f, "%-12lu ", n);
    logfile_add_to_row(f, "%-14.9f ", 100.0);    
    logfile_new_row(f);

    free(maxPhiArray);
    free(dump);
}


// Array of CCT nodes comparison by ID function
int compareByID (const void * a, const void * b) {
    if ((*(cct_node_t**)a)->routine_id != (*(cct_node_t**)b)->routine_id)
        return (*(cct_node_t**)a)->routine_id - (*(cct_node_t**)b)->routine_id;
    else return (*(cct_node_t**)a)->call_site - (*(cct_node_t**)b)->call_site;
}

// ---------------------------------------------------------------------
//  dump_instance_data
// ---------------------------------------------------------------------
void dump_instance_data(cct_node_t* root, logfile_t* f, driver_t* driver) { 
    
    UINT32 i;
    UINT32 n = nodes(root);
    //UINT64 N = driver->rtn_enter_events;
    
    // Alloc array of CCT nodes
    cct_node_t **dump = calloc((size_t) n, (size_t) sizeof(cct_node_t *));
    if (dump==NULL) panic("Failed creation of CCT array dump\n");

    // Fill in array of CCT nodes
    UINT32 pos = 0;
    dumpTreeArray(root,&pos,dump);

    // Sort nodes by routine ID
    qsort(dump,n,sizeof(cct_node_t *),compareByID);

    // Compute number of distinct routines
    UINT32 distinct = 1;
    for (i=1; i<n; i++) 
        if (dump[i]->routine_id != dump[i-1]->routine_id) distinct++;

    // Compute number of distinct call sites
    UINT32 distinctSites = 1;
    for (i=1; i<n; i++) 
        if (dump[i]->routine_id != dump[i-1]->routine_id) distinctSites++;
        else if (dump[i]->call_site != dump[i-1]->call_site) distinctSites++;

    #if 0
    printf("\nAfter sort: ");
    for (i=0; i<200; i++) printf("%lu  ",dump[i]->routine_id);
    #endif
   
    logfile_add_to_header(f, "%-12s ", "|CCT| ");
    //logfile_add_to_header(f, "%-19s ", "N (call tree nodes)");
    logfile_add_to_header(f, "%-12s ", "|call graph|");
    logfile_add_to_header(f, "%-13s ", "|call sites|");

    logfile_add_to_row   (f, "%-12lu ", n);    
    //logfile_add_to_row   (f, "%-19llu ", N);    
    logfile_add_to_row   (f, "%-12lu ", distinct);    
    logfile_add_to_row   (f, "%-13lu ", distinctSites);    

    free(dump);
}


// ---------------------------------------------------------------------
//  exactCounters   
// ---------------------------------------------------------------------
// update counters in HCCT with exact frequencies
// during the visit, also computes max and avg counter error among hot nodes
void exactCounters(cct_node_t* root, cct_node_t* cct_root,
                   float *maxError, float *sumError,
                   UINT32 stream_threshold, int (*is_monitored)(cct_node_t* node),
                   UINT32 cct_threshold, UINT32 *falsePositives,
                   float adjustBursting) { 
    cct_node_t* child, *cct_child;
    float childError;
    
    *maxError = 0;
    
    if (root==NULL) return;
    if (cct_root==NULL) 
        panic("Failed CCT vs. HCCT comparison: cannot update counters");

    if (is_monitored(root)) {
        if (cct_root->counter >= cct_threshold ) {
            #if LSS_ADJUST_EPS==1
            // eps in bytes 24-27 of lss_hcct_node_t
            float adjustedCounter = (root->counter - *(((UINT32*)root)+6)) * adjustBursting;
            #else
            float adjustedCounter = root->counter * adjustBursting;
            #endif
            *maxError = (cct_root->counter > adjustedCounter) ? 
                     (cct_root->counter - adjustedCounter)*100.0/cct_root->counter : 
                     (adjustedCounter - cct_root->counter)*100.0/cct_root->counter ;
            *sumError += *maxError;
        }
        else if (root->counter >= stream_threshold) (*falsePositives)++;
    }
    
    root->counter = cct_root->counter;     
   
    for (child=root->first_child; child!=NULL; child=child->next_sibling) { 
        for (cct_child=cct_root->first_child; cct_child!=NULL; 
             cct_child=cct_child->next_sibling) 
            if (cct_child->routine_id == child->routine_id &&
                cct_child->call_site == child->call_site) break;
        exactCounters(child, cct_child, &childError, sumError, stream_threshold, 
                        is_monitored, cct_threshold, falsePositives,adjustBursting);
        if (childError > *maxError) *maxError = childError;
    }        
}


// ---------------------------------------------------------------------
//  metrics_write_output
// ---------------------------------------------------------------------
void metrics_write_output(logfile_t* f, driver_t* driver,
                          UINT32 cct_threshold, 
                          UINT32 stream_threshold,
                          cct_node_t *hcct_root,
                          UINT32 epsilon, UINT32 phi, UINT32 tau,
                          int (*is_monitored)(cct_node_t* node)) {

    #if COMPARE_TO_CCT == 1

    UINT64  N = driver->rtn_enter_events;

    #if BURSTING == 1
    UINT64  N_B = driver->rtn_enter_burst_events;
    float adjustBursting = (float) N / (float) N_B;
    #else
    UINT64  N_B = N;
    float adjustBursting = 1.0;
    #endif

    UINT32  stream_mcct_nodes,
            stream_hcct_nodes,
            stream_hcct_false_positives=0, 
            stream_hcct_hot_nodes,
            H1, H2, max, uncovered=0, // TODO warning for max here
            hcct_nodes, hcct_hot_nodes;
    UINT64  stream_hcct_weight_sum, theSum=0, hcct_sum;
    float   maxError,sumError=0;
    
    // log number of CCT nodes
    write_output_cct(f, driver);

    // log MCCT nodes 
    stream_mcct_nodes = nodes((cct_node_t *)hcct_root);

    logfile_add_to_header(f, "%-10s ", "|MCCT| ");
    logfile_add_to_row   (f, "%-10lu ", stream_mcct_nodes);    

    // build (f,e)-HCCT
    finalPrune(hcct_root, N_B);  

    // log total number of HCCT nodes
    stream_hcct_nodes = nodes((cct_node_t *)hcct_root);
    
    logfile_add_to_header(f, "%-10s ", "|HCCT| ");
    logfile_add_to_row   (f, "%-10lu ", stream_hcct_nodes);    

    // change counters in HCCT to their exact values before computing 
    // remaining metrics
    // compute number of false positives, max and avg error
    exactCounters((cct_node_t *)hcct_root, cct_get_root(),
                  &maxError, &sumError, stream_threshold, is_monitored,
                  cct_threshold, &stream_hcct_false_positives,
                  adjustBursting);

    //compute number of hot HCCT nodes (monitored = 1 and true counter >= cct_threshold)
    stream_hcct_hot_nodes = 
        hotNodes((cct_node_t *)hcct_root, cct_threshold, is_monitored);

    // log number of cold HCCT nodes
    logfile_add_to_header(f, "%-8s ", "(cold)");
    logfile_add_to_row   (f, "%-8lu ",  stream_hcct_nodes
                                        - stream_hcct_hot_nodes
                                        - stream_hcct_false_positives);    

    // log number of hot HCCT nodes
    logfile_add_to_header(f, "%-8s ", "(hot)");
    logfile_add_to_row   (f, "%-8lu ", stream_hcct_hot_nodes);    

    // log number of false positives
    logfile_add_to_header(f, "%-8s ", "(false)");
    logfile_add_to_row   (f, "%-8lu ", stream_hcct_false_positives);    

    // compute sum of (exact) counters in HCCT
    stream_hcct_weight_sum = sum((cct_node_t *)hcct_root);

    logfile_add_to_header(f, "%-12s ", "Deg overlap");
    logfile_add_to_row   (f, "%-12.2f ", ((double)stream_hcct_weight_sum / 
                                         (double)N) * 100);    

    uncoveredFrequency((cct_node_t *)hcct_root, cct_get_root(),
                        &theSum, &max, &uncovered);

    // compute hottest counter in HCCT
    H1 = hottest((cct_node_t *)hcct_root);

    // compute hottest counter in CCT
    H2 = hottest(cct_get_root());
 
    // log avg counter value of nodes not included in HCCT
    logfile_add_to_header(f, "%-13s ", "Avg uncovered");
    logfile_add_to_row   (f, "%-13lu ",
                            (UINT32)(theSum/(UINT64)uncovered));    

    logfile_add_to_header(f, "%-8s ", "(%)");
    logfile_add_to_row   (f, "%-8.5f ",
                            ((double)((double)theSum/
                            (double)uncovered))*100/(double)H1);    

    // log max counter value of nodes not included in HCCT
    logfile_add_to_header(f, "%-13s ", "Max uncovered");
    logfile_add_to_row   (f, "%-13lu ",max);    

    // percentage
    logfile_add_to_header(f, "%-8s ", "(%)");
    logfile_add_to_row   (f, "%-8.4f ",(double)max*100/(double)H1);    

    logfile_add_to_header(f, "%-9s ", "phi*N");
    logfile_add_to_row   (f, "%-9lu ", cct_threshold);    

    logfile_add_to_header(f, "%-6s ", "1/tau");
    logfile_add_to_row   (f, "%-6lu ", tau);    

    logfile_add_to_header(f, "%-10s ", "Max hcct");
    logfile_add_to_row   (f, "%-10lu ", H1);    

    logfile_add_to_header(f, "%-10s ", "Max cct");
    logfile_add_to_row   (f, "%-10lu ", H2);    

    logfile_add_to_header(f, "%-9s ", "Hot cov.");
    logfile_add_to_row   (f, "%-9.2f ", 
            (((double)larger_than_hottest((cct_node_t *)hcct_root,
            tau,H1)) /
            ((double)larger_than_hottest(cct_get_root(),tau,H2)))*100);

    logfile_add_to_header(f, "%-13s ", "min tau");
    logfile_add_to_row   (f, "%-13.9f ", (float)cct_threshold/H1);    

    logfile_add_to_header(f, "%-10s ", "max error");
    logfile_add_to_row   (f, "%-10.5f ", maxError);    

    logfile_add_to_header(f, "%-10s ", "avg error");
    logfile_add_to_row   (f, "%-10.5f ", sumError/stream_hcct_hot_nodes);    

    logfile_add_to_header(f, "%-9s ", "1/phi");
    logfile_add_to_row   (f, "%-9lu ", phi);    

    logfile_add_to_header(f, "%-8s ", "1/eps");
    logfile_add_to_row   (f, "%-8lu ", epsilon);

    // FROM THIS POINT ON, METRICS RELATED TO EXACT HCCT

    // build exact HCCT
    cctPrune(cct_get_root(), cct_threshold); 
    
    // log total number of exact HCCT nodes
    hcct_nodes = nodes((cct_node_t *)cct_get_root());
    
    logfile_add_to_header(f, "%-10s ", "|e-HCCT| ");
    logfile_add_to_row   (f, "%-10lu ", hcct_nodes);    

    // log number of hot exact-HCCT nodes
    hcct_hot_nodes = 
        hotNodes((cct_node_t *)cct_get_root(), cct_threshold, is_always_monitored);

    logfile_add_to_header(f, "%-8s ", "(hot)");
    logfile_add_to_row   (f, "%-8lu ", hcct_hot_nodes);    

    logfile_add_to_header(f, "%-8s ", "(cold)");
    logfile_add_to_row   (f, "%-8lu ", hcct_nodes-hcct_hot_nodes);    

    // compute sum of counters in exact HCCT and log degree of overlap
    hcct_sum = sum((cct_node_t *)cct_get_root());

    logfile_add_to_header(f, "%-16s ", "Overlap e-HCCT");
    logfile_add_to_row   (f, "%-16.4f ", ((double)hcct_sum / 
                                         (double)N) * 100);    

    #endif
}

#if COMPUTE_HOT_PER_PHI==1
// ---------------------------------------------------------------------
//  hotNodesCCT
// ---------------------------------------------------------------------
UINT32 hotNodesCCT(cct_node_t* root, UINT32 threshold) {
    UINT32 h=0;
    cct_node_t* child;
    for (child=root->first_child; child!=NULL; child=child->next_sibling)
        h += hotNodesCCT(child,threshold);
    if (root->counter >= threshold) {
            return h+1; 
    }
    else return h;
}

// ---------------------------------------------------------------------
//  dump_hot_per_phi
// ---------------------------------------------------------------------

void dump_hot_per_phi(cct_node_t* cct_root, logfile_t* f, driver_t* driver) {
    UINT64 N = driver->rtn_enter_events;
    UINT32 phi7, phi5, phi3;
    UINT32 nodes7, nodes5, nodes3;
    UINT32 hotNodes7, hotNodes5, hotNodes3;
    
    // phi = 10^-7
    phi7 = (UINT32)(N/(UINT64)(10000000));
    cctPrune(cct_root, phi7);
    nodes7=nodes(cct_root);
    hotNodes7=hotNodesCCT(cct_root, phi7);
    
    // phi = 10^-5
    phi5 = (UINT32)(N/(UINT64)(100000));
    cctPrune(cct_root, phi5);
    nodes5=nodes(cct_root);
    hotNodes5=hotNodesCCT(cct_root, phi5);
    
    // phi = 10^-3
    phi3 = (UINT32)(N/(UINT64)(1000));
    cctPrune(cct_root, phi3);
    nodes3=nodes(cct_root);
    hotNodes3=hotNodesCCT(cct_root, phi3);
    
    // print statistics
    logfile_add_to_header(f, "%-10s ", "|10^-7| ");
    logfile_add_to_row   (f, "%-10lu ", nodes7);    

    logfile_add_to_header(f, "%-8s ", "(hot)");
    logfile_add_to_row   (f, "%-8lu ", hotNodes7);    
    
    logfile_add_to_header(f, "%-10s ", "|10^-5| ");
    logfile_add_to_row   (f, "%-10lu ", nodes5);    

    logfile_add_to_header(f, "%-8s ", "(hot)");
    logfile_add_to_row   (f, "%-8lu ", hotNodes5);    
    
    logfile_add_to_header(f, "%-10s ", "|10^-3| ");
    logfile_add_to_row   (f, "%-10lu ", nodes3);    

    logfile_add_to_header(f, "%-8s ", "(hot)");
    logfile_add_to_row   (f, "%-8lu ", hotNodes3);    
    
}
#endif

/* Copyright (c) 2010 D.C. D'Elia, C. Demetrescu, I. Finocchi

 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.

 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/
