# plot cct/hcct performance ratio histogram
load "palette.p"
set terminal x11
set title "Space comparison with full CCT"
set key right top outside
set size ratio 0.2
set auto x
set xlabel "Benchmarks"
set ylabel "Virtual memory peak\n(% of full CCT)"
#set yrange [0.1:8]
set logscale y
set style data histogram
#set ytics 0.02
set xtic rotate by -45 scale 0
set style histogram cluster gap 2
set style fill solid noborder
set grid y
set grid noxtics
plot \
    '< join ../../logs/final-logs/adaptive/space_2ms/lss-hcct-burst-perf.log.2ms ../../logs/final-logs/perf/cct.log' using (100*$8/$22):xticlabels(1) title 'LSS + static burst',\
    '< join ../../logs/final-logs/perf/lss-hcct-perf-10000-50000.log ../../logs/final-logs/perf/cct.log' using (100*$8/$15) title 'LSS',\
    '< join ../../logs/final-logs/perf/bss-hcct-perf-10000-50000.log ../../logs/final-logs/perf/cct.log' using (100*$8/$15) title 'BSS',\
    '< join ../../logs/final-logs/adaptive/space_2ms/lc-hcct-burst-perf.log.2ms ../../logs/final-logs/perf/cct.log' using (100*$8/$22) title 'LC + static burst',\
    '< join ../../logs/final-logs/perf/lc-hcct-perf-10000-50000.log ../../logs/final-logs/perf/cct.log' using (100*$8/$15) title 'LC',\
    '< join ../../logs/final-logs/adaptive/space_2ms/cct-burst.log.2ms ../../logs/final-logs/perf/cct.log' using (100*$8/$24) title 'static burst'
set terminal postscript eps color enhanced dashed "Helvetica" 8
set output '../../docs/paper/charts/space.eps'
replot
system "eps2eps ../../docs/paper/charts/space.eps ../../docs/paper/charts/space-bbok.eps"


#    '< join ../../logs/final-logs/adaptive/space_2ms/lss-hcct-adaptive-20-perf.log.2ms ../../logs/final-logs/perf/cct.log' using (100*$8/$22):xticlabels(1) title 'LSS RR 5%',\
#    '< join ../../logs/final-logs/adaptive/space_2ms/cct-adaptive-20.log.2ms ../../logs/final-logs/perf/cct.log' using (100*$8/$24) title 'CCT RR 5%',\
#    '< join ../../logs/final-logs/adaptive/space_2ms/cct-adaptive-no-RR.log.2ms ../../logs/final-logs/perf/cct.log' using (100*$8/$24) title 'CCT adaptive no RR'
