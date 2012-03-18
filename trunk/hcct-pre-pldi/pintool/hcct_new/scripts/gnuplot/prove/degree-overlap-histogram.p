# plot degree of overlap histogram
set terminal x11
set title "degree of overlap with full CCT"
set auto x
set logscale y
set xlabel "benchmarks"
set ylabel "degree of overlap (%)"
set yrange [0.1:*]
set style data histogram
set xtic rotate by -45 scale 0
set style histogram cluster gap 1
set style fill solid border -2
set boxwidth 0.8
#set grid
plot  '../../logs/final-logs/lss-hcct-metrics-10000-50000.log' using ($9):xticlabels(1) title 'LSS ({/Symbol f},{/Symbol e})-HCCT' \
    , '../../logs/final-logs/lc-hcct-metrics-10000-50000.log' using ($9) t 'LC ({/Symbol f},{/Symbol e})-HCCT' \
    , '../../logs/final-logs/lss-hcct-metrics-10000-50000.log' using ($27) t 'HCCT'
set terminal postscript eps color enhanced dashed "Helvetica" 16
set output 'degree-overlap.eps'
replot
