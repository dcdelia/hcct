# plot false positives as a function of epsilon
load "palette.p"
set terminal x11
set title "Number of false positives as a function of {/Symbol e}"
set style line 1 lt 1 lw 2 pt 7 ps 1.5
set style line 2 lt 1 lw 2 pt 2 ps 1.5
set style line 3 lt 1 lw 2 pt 9 ps 1.5
set style line 4 lt 1 lw 2 pt 5 ps 1.5
set style line 5 lt 2 lw 2 pt 7 ps 1.5
set style line 6 lt 2 lw 2 pt 2 ps 1.5
set style line 7 lt 2 lw 2 pt 9 ps 1.5
set style line 8 lt 2 lw 2 pt 5 ps 1.5
set key right top inside
set xlabel "{/Symbol f}/{/Symbol e}"
set ylabel "False positives (% of ({/Symbol f},{/Symbol e})-HCCT nodes)"
set yrange [-1:*]
#set logscale x
#set logscale y
set grid
plot \
    '../../logs/final-logs/epsilon/lss-hcct-metrics-firefox-eps.log' using ($26/$25):(100*$11/$8) with linespoints ls 1 title 'LSS - firefox'  ,\
    '../../logs/final-logs/epsilon/lss-hcct-metrics-oocalc-eps.log' using ($26/$25):(100*$11/$8) with linespoints ls 2 title 'LSS - oocalc'    ,\
    '../../logs/final-logs/epsilon/lss-hcct-metrics-inkscape-eps.log' using ($26/$25):(100*$11/$8) with linespoints ls 3 title 'LSS - inkscape',\
    '../../logs/final-logs/epsilon/lss-hcct-metrics-ark-eps.log' using ($26/$25):(100*$11/$8) with linespoints ls 4 title 'LSS - ark'          ,\
    '../../logs/final-logs/epsilon/lc-hcct-metrics-firefox-eps.log' using ($26/$25):(100*$11/$8) with linespoints ls 5 title 'LC - firefox'  ,\
    '../../logs/final-logs/epsilon/lc-hcct-metrics-oocalc-eps.log' using ($26/$25):(100*$11/$8) with linespoints ls 6 title 'LC - oocalc'    ,\
    '../../logs/final-logs/epsilon/lc-hcct-metrics-inkscape-eps.log' using ($26/$25):(100*$11/$8) with linespoints ls 7 title 'LC - inkscape' ,\
    '../../logs/final-logs/epsilon/lc-hcct-metrics-ark-eps.log' using ($26/$25):(100*$11/$8) with linespoints ls 8 title 'LC - ark'          
set terminal postscript eps color enhanced dashed "Helvetica" 18
set output '../../docs/paper/charts/false-positives-epsilon.eps'
replot

#    '../../logs/final-logs/epsilon/lss-hcct-metrics-amarok-eps.log' using ($26/$25):(100*$11/$8)  with linespoints title 'LSS - amarok'   ,\
#    '../../logs/final-logs/epsilon/lc-hcct-metrics-amarok-eps.log' using ($26/$25):(100*$11/$8)  with linespoints title 'LC - amarok'  
