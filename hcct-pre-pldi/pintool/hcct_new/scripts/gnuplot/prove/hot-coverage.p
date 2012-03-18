# plot degree of overlap histogram
set terminal x11
set title "hot edge coverage with respect to CCT"
set auto x
#set logscale y
set xlabel "benchmarks"
set ylabel "hot coverage (%)"
set yrange [0:*]
set style data histogram
set xtic rotate by -45 scale 0
set style histogram gap 1
set style fill solid border -2
set boxwidth 0.8
#set grid
plot '../../logs/final-logs/old-lss-hcct-metrics.log' using ($16):xticlabels(1) title 'lss-hcct'
set terminal postscript eps color enhanced dashed "Helvetica" 16
set output 'hot-coverage.eps'
replot
