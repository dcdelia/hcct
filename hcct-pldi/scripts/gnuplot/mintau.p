# plot min tau
load "palette.p"
set terminal x11
set title "Minimum {/Symbol t} for which hot edge coverage is 100%"
set style line 1 lt 1 pt 1
set key left top inside
set xlabel "{/Symbol f}"
set ylabel "Min {/Symbol t}"
set logscale y
set logscale x
#set xtics 10
set grid
plot   '../../logs/final-logs/phi/lss-hcct-metrics-firefox-phi.log' using (1/$22):($14>$16 ? (1) : ($14/$16)) with linespoints title '  firefox'  ,\
       '../../logs/final-logs/phi/lss-hcct-metrics-oocalc-phi.log' using (1/$22):($14>$16 ? (1) : ($14/$16)) with linespoints title '  oocalc'    ,\
       '../../logs/final-logs/phi/lss-hcct-metrics-amarok-phi.log' using (1/$22):($14>$16 ? (1) : ($14/$16))  with linespoints title '  amarok'   ,\
       '../../logs/final-logs/phi/lss-hcct-metrics-ark-phi.log' using (1/$22):($14>$16 ? (1) : ($14/$16)) with linespoints title '  ark'          ,\
       '../../logs/final-logs/phi/lss-hcct-metrics-inkscape-phi.log' using (1/$22):($14>$16 ? (1) : ($14/$16)) with linespoints title '  inkscape'
set terminal postscript eps enhanced color dashed "Helvetica" 18
set output '../../docs/paper/charts/mintau.eps'
replot


#plot   '../../logs/final-logs/phi/lss-hcct-metrics-amarok-phi.log' using (1/$22):((1/$22)/($14>$16 ? (1) : ($14/$16)))  with linespoints title 'amarok'   ,\
#       '../../logs/final-logs/phi/lss-hcct-metrics-ark-phi.log' using (1/$22):((1/$22)/($14>$16 ? (1) : ($14/$16))) with linespoints title 'ark'          ,\
#       '../../logs/final-logs/phi/lss-hcct-metrics-firefox-phi.log' using (1/$22):((1/$22)/($14>$16 ? (1) : ($14/$16))) with linespoints title 'firefox'  ,\
#       '../../logs/final-logs/phi/lss-hcct-metrics-inkscape-phi.log' using (1/$22):((1/$22)/($14>$16 ? (1) : ($14/$16))) with linespoints title 'inkscape',\
#       '../../logs/final-logs/phi/lss-hcct-metrics-oocalc-phi.log' using (1/$22):((1/$22)/($14>$16 ? (1) : ($14/$16))) with linespoints title 'oocalc' 
