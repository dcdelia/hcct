load "palette.p"
set terminal x11
set title "Avg/max frequency of contexts not included in ({/Symbol f},{/Symbol e})-HCCT"
set key right top inside
set auto x
set logscale y
set xlabel "Benchmarks"
set ylabel "Uncovered frequency (% of the hottest context)"
set yrange [0.00001:*]
set style data histogram
set style histogram cluster gap 1
set xtic rotate by -45 scale 0
set style fill solid border -2
set grid y
set boxwidth 0.8
plot '../../logs/final-logs/10000-50000/lss-hcct-metrics-10000-50000.log' u ($12*100/$18):xtic(1) t 'LSS avg', \
     '../../logs/final-logs/10000-50000/lss-hcct-metrics-10000-50000.log' u ($14*100/$18) t 'LSS max'
set terminal postscript eps color enhanced dashed "Helvetica" 18
set output '../../docs/paper/charts/avg-max-uncovered.eps'
replot

# Results for lc are identical to lss and are not plotted.
# Add the following lines to check:
#    '../../logs/final-logs/lc-hcct-metrics-10000-50000.log' u ($10*100/$16) t 'average frequency lc' , \
#    '../../logs/final-logs/lc-hcct-metrics-10000-50000.log' u ($12*100/$16) t 'maximum frequency lc' , \

# Results of LC burst not yet available (nov 14, sunday)


#    '../../logs/final-logs/10000-50000/lss-hcct-burst-metrics-10000-50000.log' u ($10*100/$16) t 'LSS-burst avg' ,\
#     '../../logs/final-logs/10000-50000/lss-hcct-burst-metrics-10000-50000.log' u ($12*100/$16) t 'LSS-burst max'
