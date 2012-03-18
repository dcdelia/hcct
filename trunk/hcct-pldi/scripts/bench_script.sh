#!/bin/bash
SAMPLING_RATES=("10" "25" "50")
BENCHS_FULL=("cct" "lss-hcct-perf" "lc-hcct-perf" "empty")
BENCHS_CCTB=("empty-burst" "cct-burst" "cct-adaptive-5" "cct-adaptive-10"\
                "cct-adaptive-20" "cct-adaptive-no-RR")
BENCHS_LSSB=("lss-hcct-burst-perf" "lss-hcct-adaptive-5-perf" "lss-hcct-adaptive-10-perf" \
                 "lss-hcct-adaptive-20-perf" "lss-hcct-adaptive-no-RR-perf")
BENCHS_LCB=("lc-hcct-burst-perf" "lc-hcct-adaptive-5-perf" "lc-hcct-adaptive-10-perf" \
                 "lc-hcct-adaptive-20-perf" "lc-hcct-adaptive-no-RR-perf")

APP_NAME="vlc"
TRACE_NAME="vlc"

ITERATIONS=5
PAUSE=2

PHI=10000
EPSILON=50000
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
    for BENCH in ${BENCHS_FULL[@]}; do
        bin/$BENCH traces/$APP_NAME/rectrace-$TRACE_NAME-0.cs.trace $APP_NAME $APP_NAME-$BENCH.log \
            -phi $PHI -epsilon $EPSILON -tau $TAU
        sleep $PAUSE
    done

    # Benchmarks - SAMPLING
    for SAMPLING in ${SAMPLING_RATES[@]}; do
        # Benchmarks - CCT
        echo "------------------------------------------------"
        echo "------> Benchmarks - CCT with SI="$SAMPLING" tics <-----"
        echo "------------------------------------------------"
        for BENCH in ${BENCHS_CCTB[@]}; do
            bin/$BENCH traces/$APP_NAME/rectrace-$TRACE_NAME-0.cs.trace $APP_NAME $APP_NAME-$BENCH.log.$SAMPLING \
                -phi $PHI -epsilon $EPSILON -tau $TAU -sampling $SAMPLING -burst $BURST
            sleep $PAUSE
        done
        # Benchmarks - LSS
        echo "------------------------------------------------"
        echo "------> Benchmarks - LSS with SI="$SAMPLING" tics <-----"
        echo "------------------------------------------------"
        for BENCH in ${BENCHS_LSSB[@]}; do
            bin/$BENCH traces/$APP_NAME/rectrace-$TRACE_NAME-0.cs.trace $APP_NAME $APP_NAME-$BENCH.log.$SAMPLING \
                -phi $PHI -epsilon $EPSILON -tau $TAU -sampling $SAMPLING -burst $BURST
            sleep $PAUSE
        done
        # Benchmarks - LC
        echo "----------------------------------------------"
        echo "------> Benchmarks - LC with SI="$SAMPLING" tics <-----"
        echo "----------------------------------------------"
        for BENCH in ${BENCHS_LCB[@]}; do
            bin/$BENCH traces/$APP_NAME/rectrace-$TRACE_NAME-0.cs.trace $APP_NAME $APP_NAME-$BENCH.log.$SAMPLING \
                -phi $PHI -epsilon $EPSILON -tau $TAU -sampling $SAMPLING -burst $BURST
            sleep $PAUSE
        done
    done
    let COUNT=COUNT+1
done
