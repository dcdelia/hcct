# plot false positives, cold and hot percentage
set terminal x11
set title "Cold nodes, true hots, and false positives (%)"
set auto x
set key under
set ylabel "Number of nodes (%)"
set yrange [0:*]
set style data histogram
set xtic rotate by -45 scale 0
set style histogram cluster gap 1
set style fill solid border -2
set boxwidth 0.8
plot \
     '../../logs/final-logs/10000-50000/lss-hcct-metrics-10000-50000.log' u (100*$6/($7+$8)):xtic(1) t 'LSS', \
     '../../logs/final-logs/10000-50000/lc-hcct-metrics-10000-50000.log' u (100*$6/($7+$8)) t 'LC'
set terminal postscript eps color enhanced dashed "Helvetica" 16
set output 'cold-histogram.eps'
replot


#     newhistogram "True hot",\
#     '../../logs/final-logs/10000-50000/lss-hcct-metrics-10000-50000.log' u (100*$6/$5):xtic(1) t 'LSS', \
#     '../../logs/final-logs/10000-50000/lc-hcct-metrics-10000-50000.log' u (100*$6/$5) t 'LC', \
#     '../../logs/final-logs/10000-50000/lss-hcct-metrics-10000-50000.log' u (100*$25/$24) t 'Exact HCCT', \


#     '../../logs/final-logs/10000-50000/lss-hcct-metrics-10000-50000.log' u (100*$26/$25) t '   HCCT'
