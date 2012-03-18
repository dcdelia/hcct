set terminal x11
set title "avg/max frequency of contexts not included in HCCT (% of the hottest context)"
set auto x
set key under
set logscale y
#set xlabel "benchmarks"
set ylabel "avg/max frequency (%)"
set yrange [0.00001:*]
set style data histogram
set style histogram cluster gap 1
set xtic rotate by -45 scale 0
set style fill solid border -2
set grid y
set boxwidth 0.8
plot \
    newhistogram "average", \
        '../../logs/lss-hcct-metrics.log' u 11:xtic(1) t 'lss', \
        '../../logs/lc-hcct-metrics.log' u 11 t 'lc', \
    newhistogram "maximum", \
        '../../logs/lss-hcct-metrics.log' u 13:xtic(1) t 'lss', \
        '../../logs/lc-hcct-metrics.log' u 13 t 'lc'

set terminal postscript eps color enhanced dashed "Helvetica" 16
set output 'avg-max-uncovered.eps'
replot
