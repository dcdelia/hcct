# plot cct/hcct performance ratio histogram
set terminal x11
set title "Space comparison with full CCT"
set key right top inside
set auto x
set xlabel "Benchmarks"
set ylabel "Virtual memory peak (% of full CCT)"
set yrange [*:8]
set style data histogram
#set ytics 0.02
set xtic rotate by -45 scale 0
set style histogram cluster gap 1
set style fill solid border -2
set boxwidth 0.8
set grid y
set grid noxtics
plot \
    '< join ../../logs/final-logs/perf/lss-hcct-burst-perf-10000-50000.log ../../logs/final-logs/perf/cct.log' using (100*$8/$21):xticlabels(1) title 'LSS burst',\
    '< join ../../logs/final-logs/perf/lss-hcct-perf-10000-50000.log ../../logs/final-logs/perf/cct.log' using (100*$8/$15):xticlabels(1) title 'LSS',\
    '< join ../../logs/final-logs/perf/bss-hcct-perf-10000-50000.log ../../logs/final-logs/perf/cct.log' using (100*$8/$15) title 'BSS',\
    '< join ../../logs/final-logs/perf/lc-hcct-burst-perf-10000-50000.log ../../logs/final-logs/perf/cct.log' using (100*$8/$21) title 'LC burst',\
    '< join ../../logs/final-logs/perf/lc-hcct-perf-10000-50000.log ../../logs/final-logs/perf/cct.log' using (100*$8/$15) title 'LC'
set terminal postscript eps color enhanced dashed "Helvetica" 16
set output 'space.eps'
replot


#    '../../logs/final-logs/perf/cct.log' using ($8):xticlabels(1) title 'CCT'
