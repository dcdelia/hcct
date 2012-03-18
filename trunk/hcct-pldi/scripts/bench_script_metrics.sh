#!/bin/bash
SAMPLING_RATES=("10" "25" "50")
BENCHS_BASE=("cct")
BENCHS_THRESHOLD=("lss-hcct-metrics" "lc-hcct-metrics")
BENCHS_CCTB=("lss-hcct-burst-metrics" "lss-hcct-adaptive-5-metrics" "lss-hcct-adaptive-10-metrics" \
                 "lss-hcct-adaptive-20-metrics" "lss-hcct-adaptive-no-RR-metrics")
BENCHS_LSSB=("lss-hcct-burst-metrics" "lss-hcct-adaptive-5-metrics" "lss-hcct-adaptive-10-metrics" \
                 "lss-hcct-adaptive-20-metrics" "lss-hcct-adaptive-no-RR-metrics")
BENCHS_LCB=("lc-hcct-burst-metrics" "lc-hcct-adaptive-5-metrics" "lc-hcct-adaptive-10-metrics" \
                 "lc-hcct-adaptive-20-metrics" "lc-hcct-adaptive-no-RR-metrics")

APP_NAME="vlc"
TRACE_NAME="vlc"
# LENGTH >= |CCT|
LENGTH=4000000

ITERATIONS=1
PAUSE=2

THRESHOLDS=(10000 15000 20000 25000)
EPS_PHI=2
TAU=10
BURST=1

COUNT=0

while [ $COUNT -lt $ITERATIONS ]; do
    echo "---------------------------"
    echo "---------------------------"
    echo "------> Iteration #"$[$COUNT+1]" <-----"
    echo "---------------------------"
    echo "---------------------------"

    # Benchmarks - FULL
    echo "--------------------------------"
    echo "------> Benchmarks - FULL <-----"
    echo "--------------------------------"
    
    # Benchmarks without threshold
    for BENCH in ${BENCHS_BASE[@]}; do
        bin/$BENCH traces/$APP_NAME/rectrace-$TRACE_NAME-0.cs.trace $APP_NAME $APP_NAME-$BENCH.log
        sleep $PAUSE
    done

    # Benchmarks with different thresholds
    for PHI in ${THRESHOLDS[@]}; do
        EPSILON=$[$PHI*$EPS_PHI]     
        for BENCH in ${BENCHS_THRESHOLD[@]}; do
            bin/$BENCH traces/$APP_NAME/rectrace-$TRACE_NAME-0.cs.trace $APP_NAME $APP_NAME-$BENCH-$PHI-$EPSILON.log \
                -phi $PHI -epsilon $EPSILON -tau $TAU
            sleep $PAUSE
        done
    done

    # Benchmarks - SAMPLING
    for SAMPLING in ${SAMPLING_RATES[@]}; do
        # Benchmarks - CCT (using lss-hcct without pruning)
        echo "------------------------------------------------"
        echo "------> Benchmarks - CCT with SI="$SAMPLING" tics <-----"
        echo "------------------------------------------------"
        for BENCH in ${BENCHS_CCTB[@]}; do
            bin/$BENCH traces/$APP_NAME/rectrace-$TRACE_NAME-0.cs.trace $APP_NAME $APP_NAME-CCT-$BENCH.log.$SAMPLING \
                -phi $LENGTH -epsilon $LENGTH -tau $TAU -sampling $SAMPLING -burst $BURST
            sleep $PAUSE
        done
        
        # Benchmarks with different thresholds
        for PHI in ${THRESHOLDS[@]}; do
            EPSILON=$[$PHI*$EPS_PHI]
            # Benchmarks - LSS
            echo "------------------------------------------------"
            echo "------> Benchmarks - LSS with SI="$SAMPLING" tics <-----"
            echo "------------------------------------------------"
                for BENCH in ${BENCHS_LSSB[@]}; do
                bin/$BENCH traces/$APP_NAME/rectrace-$TRACE_NAME-0.cs.trace $APP_NAME $APP_NAME-$BENCH-$PHI-$EPSILON.log.$SAMPLING \
                    -phi $PHI -epsilon $EPSILON -tau $TAU -sampling $SAMPLING -burst $BURST
                sleep $PAUSE
            done
            # Benchmarks - LC
            echo "----------------------------------------------"
            echo "------> Benchmarks - LC with SI="$SAMPLING" tics <-----"
            echo "----------------------------------------------"
            for BENCH in ${BENCHS_LCB[@]}; do
                bin/$BENCH traces/$APP_NAME/rectrace-$TRACE_NAME-0.cs.trace $APP_NAME $APP_NAME-$BENCH-$PHI-$EPSILON.log.$SAMPLING \
                    -phi $PHI -epsilon $EPSILON -tau $TAU -sampling $SAMPLING -burst $BURST
                sleep $PAUSE
            done
        done
    done
    let COUNT=COUNT+1
done
