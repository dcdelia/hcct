# plot speedup
set terminal x11
set title "Comparison of analysis running times using static and adaptive bursting"

set style line 1 lt 1 lw 2 pt 8 ps 1
set style line 2 lt 1 lw 2 pt 2 ps 1
set style line 3 lt 1 lw 2 pt 5 ps 1
set style line 4 lt 2 lw 2 pt 2 ps 1
set style line 5 lt 2 lw 2 pt 5 ps 1

set key left top inside
set style line 1 lt 1 pt 1
set xlabel "algorithm setting"
set ylabel "speedup w.r.t. CCT construction"
#set yrange [0:0.1]
set xrange [5:30]
set grid
plot \
    '../../logs/final-logs/adaptive/running-times-lss-ark-10.log'  using 2:(1/(($3 - $4)/($5 - $6))):xticlabels(1) with linespoints ls 2 title 'ark, LSS, SR 10', \
    '../../logs/final-logs/adaptive/running-times-lc-ark-10.log'   using 2:(1/(($3 - $4)/($5 - $6))):xticlabels(1) with linespoints ls 3 title 'ark, LC, SR 10', \
    '../../logs/final-logs/adaptive/running-times-lss-ark-25.log'  using 2:(1/(($3 - $4)/($5 - $6))):xticlabels(1) with linespoints ls 4 title 'ark, LSS, SR 25', \
    '../../logs/final-logs/adaptive/running-times-lc-ark-25.log'   using 2:(1/(($3 - $4)/($5 - $6))):xticlabels(1) with linespoints ls 5 title 'ark, LC, SR 25'
set terminal postscript eps color enhanced dashed "Helvetica" 16
set output 'speedup.eps'
replot 

#    '../../logs/final-logs/adaptive/running-times-lss-ark-10.log'  using 2:(1/(($3 - $4)/($5 - $6))):xticlabels(1)   with linespoints ls 1 title 'ark, LSS, SR 10', \
#    '../../logs/final-logs/adaptive/running-times-lc-ark-10.log'   using 2:(1/(($3 - $4)/($5 - $6))):xticlabels(1)   with linespoints ls 2 title 'ark, LC, SR 10', \
#    '../../logs/final-logs/adaptive/running-times-lss-gimp-10.log' using 2:(1/(($3 - $4)/($5 - $6))):xticlabels(1) with linespoints ls 5 title 'gimp, LSS, SR 10', \
#    '../../logs/final-logs/adaptive/running-times-lc-gimp-10.log'  using 2:(1/(($3 - $4)/($5 - $6))):xticlabels(1) with linespoints ls 6 title 'gimp, LC, SR 10'

#    '../../logs/final-logs/adaptive/running-times-lss-ark-25.log'  using 2:(1/(($3 - $4)/($5 - $6))):xticlabels(1) with linespoints ls 3 title 'ark, LSS, SR 25', \
#    '../../logs/final-logs/adaptive/running-times-lc-ark-25.log'   using 2:(1/(($3 - $4)/($5 - $6))):xticlabels(1) with linespoints ls 4 title 'ark, LC, SR 25', \
#    '../../logs/final-logs/adaptive/running-times-lss-gimp-25.log' using 2:(1/(($3 - $4)/($5 - $6))):xticlabels(1) with linespoints ls 7 title 'gimp, LSS, SR 25', \
#    '../../logs/final-logs/adaptive/running-times-lc-gimp-25.log'  using 2:(1/(($3 - $4)/($5 - $6))):xticlabels(1) with linespoints ls 8 title 'gimp, LC, SR 25'

