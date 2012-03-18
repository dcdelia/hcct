#!/bin/bash
BENCHS_BASE=("cct")
BENCHS_THRESHOLD=("lss-hcct-metrics" "lc-hcct-metrics")
BENCHS_LSSB=("lss-hcct-burst-metrics" "lss-hcct-adaptive-5-metrics" "lss-hcct-adaptive-10-metrics" \
                 "lss-hcct-adaptive-20-metrics" "lss-hcct-adaptive-no-RR-metrics")
BENCHS_LCB=("lc-hcct-burst-metrics" "lc-hcct-adaptive-5-metrics" "lc-hcct-adaptive-10-metrics" \
                 "lc-hcct-adaptive-20-metrics" "lc-hcct-adaptive-no-RR-metrics")
BENCHS_CCTB=("CCT-lss-hcct-burst-metrics" "CCT-lss-hcct-adaptive-5-metrics" "CCT-lss-hcct-adaptive-10-metrics" \
                 "CCT-lss-hcct-adaptive-20-metrics" "CCT-lss-hcct-adaptive-no-RR-metrics")
SAMPLING_RATES=("10" "25" "50")
#THRESHOLDS=(10000 15000 20000 25000)
THRESHOLDS=(25000 50000 75000)
EPS_PHI=2
APP_NAME="vlc"
DEST="vlc-metrics"

# Creating folders
mkdir -p $DEST
for PHI in ${THRESHOLDS[@]}; do
    EPSILON=$[$PHI*$EPS_PHI]
    mkdir -p $DEST/$PHI-$EPSILON
done

# Benchmarks without thresholds
for BENCH in ${BENCHS_BASE[@]}; do
    cp $APP_NAME-$BENCH.log $DEST/$BENCH.log
done

# Benchmarks with different thresholds (without sampling)
for BENCH in ${BENCHS_THRESHOLD[@]}; do
    AUX=${THRESHOLDS[0]}
    head -n 1 $APP_NAME-$BENCH-$AUX-$[$AUX*$EPS_PHI].log > $DEST/$BENCH.log
    for PHI in ${THRESHOLDS[@]}; do
        EPSILON=$[$PHI*$EPS_PHI]
        tail -n 1 $APP_NAME-$BENCH-$PHI-$EPSILON.log >> $DEST/$BENCH.log
    done
done

# Benchmarks with different sampling rates
for SAMPLING in ${SAMPLING_RATES[@]}; do
        # Benchmarks - CCT (using lss-hcct without pruning)
        head -n 1 $APP_NAME-${BENCHS_CCTB[0]}.log.$SAMPLING > $DEST/cct.log.$SAMPLING
        for BENCH in ${BENCHS_CCTB[@]}; do
            tail -n 1 $APP_NAME-$BENCH.log.$SAMPLING >> $DEST/cct.log.$SAMPLING
        done
        
        # Benchmarks with different thresholds
        for PHI in ${THRESHOLDS[@]}; do
            EPSILON=$[$PHI*$EPS_PHI]
            # LSS
            head -n 1 $APP_NAME-${BENCHS_LSSB[0]}-$PHI-$EPSILON.log.$SAMPLING > \
                $DEST/$PHI-$EPSILON/lss-hcct-$PHI-$EPSILON.log.$SAMPLING
            for BENCH in ${BENCHS_LSSB[@]}; do
                tail -n 1 $APP_NAME-$BENCH-$PHI-$EPSILON.log.$SAMPLING >> \
                    $DEST/$PHI-$EPSILON/lss-hcct-$PHI-$EPSILON.log.$SAMPLING
            done
            # LC
            head -n 1 $APP_NAME-${BENCHS_LCB[0]}-$PHI-$EPSILON.log.$SAMPLING > \
                $DEST/$PHI-$EPSILON/lc-hcct-$PHI-$EPSILON.log.$SAMPLING
            for BENCH in ${BENCHS_LCB[@]}; do
                tail -n 1 $APP_NAME-$BENCH-$PHI-$EPSILON.log.$SAMPLING >> \
                    $DEST/$PHI-$EPSILON/lc-hcct-$PHI-$EPSILON.log.$SAMPLING
            done
        done
done
