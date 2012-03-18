# plot counter error
set terminal x11
set title "Avg/max counter error among hot elements (% of the true frequency)"
set key left inside
set auto x
#set logscale y
set xlabel "Benchmarks"
set ylabel "Avg/max error (%)"
set yrange [0:*]
set style data histogram
set ytics 1
set xtic rotate by -45 scale 0
set style histogram cluster gap 1
set style fill solid border -2
set boxwidth 0.8
set grid noxtics
plot \
    newhistogram "Avg", \
    '../../logs/final-logs/10000-50000/lss-hcct-metrics-10000-50000.log' using ($23):xticlabels(1) title '  LSS avg error', \
    '../../logs/final-logs/10000-50000/lc-hcct-metrics-10000-50000.log' using ($23):xticlabels(1) title '  LC avg error', \
    newhistogram "Max", \
    '../../logs/final-logs/10000-50000/lss-hcct-metrics-10000-50000.log' using ($22):xticlabels(1) title '  LSS max error', \
    '../../logs/final-logs/10000-50000/lc-hcct-metrics-10000-50000.log' using ($22):xticlabels(1) title '  LC max error'
set terminal postscript eps color enhanced dashed "Helvetica" 16
set output 'counter-error.eps'
replot

#    newhistogram "Avg", \
#    '../../logs/final-logs/10000-50000/lss-hcct-metrics-10000-50000.log' using ($23):xticlabels(1) title '  LSS avg error', \
#    '../../logs/final-logs/10000-50000/lss-hcct-eps-metrics-10000-50000.log' using ($23):xticlabels(1) title '  LSS (eps) avg error', \
#    '../../logs/final-logs/10000-50000/lc-hcct-metrics-10000-50000.log' using ($23):xticlabels(1) title '  LC avg error', \
#    newhistogram "Max", \
#    '../../logs/final-logs/10000-50000/lss-hcct-metrics-10000-50000.log' using ($22):xticlabels(1) title '  LSS max error', \
#    '../../logs/final-logs/10000-50000/lss-hcct-eps-metrics-10000-50000.log' using ($22):xticlabels(1) title '  LSS (eps) max error', \
#    '../../logs/final-logs/10000-50000/lc-hcct-metrics-10000-50000.log' using ($22):xticlabels(1) title '  LC max error'


#     '../../logs/final-logs/10000-50000/lss-hcct-burst-metrics-10000-50000.log' using ($20):xticlabels(1) title 'LSS-burst max error' ,\
#     '../../logs/final-logs/10000-50000/lss-hcct-burst-metrics-10000-50000.log' using ($21):xticlabels(1) title 'LSS-burst avg error' 
