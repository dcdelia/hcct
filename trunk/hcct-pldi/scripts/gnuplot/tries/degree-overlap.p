# plot degree of overlap as a function of phi
set terminal x11
set title "degree of overlap of HCCT with full CCT"
set style line 1 lt 1 pt 1
#set auto x
set xlabel "number of hottest nodes"
set ylabel "degree of overlap (%)"
set xrange [0.000001:*]
set yrange [0:100]
#set logscale y
#set logscale x
plot   '../../logs/final-logs/phi/lss-hcct-metrics-inkscape-phi.log' using (100*$25/$3):27  with linespoints title 'inkscape' ,\
        '../../logs/final-logs/phi/lss-hcct-metrics-ark-phi.log' using (100*$25/$3):27  with linespoints title 'ark' ,\
        '../../logs/final-logs/phi/lss-hcct-metrics-firefox-phi.log' using (100*$25/$3):27  with linespoints title 'firefox' ,\
        '../../logs/final-logs/phi/lss-hcct-metrics-oocalc-phi.log' using (100*$25/$3):27  with linespoints title 'oocalc'
set terminal postscript eps color enhanced dashed "Helvetica" 16
set output 'degree-overlap.eps'
replot
