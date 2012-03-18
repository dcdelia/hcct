# plot cct/hcct performance histogram
set terminal x11
set title "tree construction time"
set auto x
#set logscale y
set xlabel "benchmarks"
set ylabel "average time per routine enter/exit (nsec)"
set yrange [0:16]
set style data histogram
set xtic rotate by -45 scale 0
set style histogram cluster gap 1
set style fill solid border -2
set boxwidth 0.8
#set grid
plot '< join ../../logs/final-logs/empty.log ../../logs/cct.log' using ($12-$5):xticlabels(1) title 'cct', \
     '< join ../../logs/final-logs/empty.log ../../logs/lss-hcct-perf.log' using ($12-$5) title 'lss-hcct'
set terminal postscript eps color enhanced dashed "Helvetica" 16
set output 'time.eps'
replot
