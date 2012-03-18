# plot cct/hcct performance ratio histogram
set terminal x11
set title "HCCT vs. CCT construction time"
#set auto x
set xlabel "Benchmarks"
set ylabel "Avg time per routine enter/exit (nsec)"
set yrange [0:*]
set style data histogram
set xtic rotate by -45 scale 0
set style histogram gap 1
set style fill solid border -2
set boxwidth 0.8
plot '< join ../../logs/final-logs/empty.log ../logs/lss-hcct-perf.log | join - ../../logs/final-logs/cct.log' using (($12)/($19)):xticlabels(1) title 'lss-hcct/cct'
set terminal postscript eps color enhanced dashed "Helvetica" 16
set output 'time-ratio.eps'
replot
