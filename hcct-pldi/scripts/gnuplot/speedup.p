# plot speedup
load "palette.p"
set terminal x11
set title "Comparison of analysis running times using static and adaptive bursting"
set key left top inside
#set size ratio 0.2
#set style line 1 lt 1 pt 1
#set auto x
set xlabel ""
set ylabel "Speedup w.r.t. CCT construction"
#set yrange [0:45]
set yrange [0.25:64]
#set xrange [5:30]
set logscale y
set ytics 0.5
#set ytics 5
set style data histogram
set xtic rotate by -45 scale 0
set style histogram cluster gap 1
set style fill solid border -2
set boxwidth 0.8
set grid y
set grid noxtics
plot \
    newhistogram "    No sampling", \
    '../../logs/final-logs/speedup/speedup-full.log' using ($2):xticlabels(1) title 'Full CCT' lt 1, \
    '../../logs/final-logs/speedup/speedup-full.log' using ($3) title ''  lt 4, \
    '../../logs/final-logs/speedup/speedup-full.log' using ($4) title ''  lt 2, \
    newhistogram " Sampling interval = 2 ms", \
    '../../logs/final-logs/speedup/speedup-2ms.log' using ($2):xticlabels(1) title 'Bursting' lt 6, \
    '../../logs/final-logs/speedup/speedup-2ms.log' using ($3) title 'LSS'  lt 4, \
    '../../logs/final-logs/speedup/speedup-2ms.log' using ($4) title 'LC'  lt 2, \
    newhistogram "Sampling interval = 10 ms", \
    '../../logs/final-logs/speedup/speedup-10ms.log' using ($2):xticlabels(1) title '' lt 6, \
    '../../logs/final-logs/speedup/speedup-10ms.log' using ($3) title '' lt 4, \
    '../../logs/final-logs/speedup/speedup-10ms.log' using ($4) title '' lt 2
set terminal postscript eps color enhanced dashed "Helvetica" 18
set output '../../docs/paper/charts/speedup.eps'
replot

#plot \
#    newhistogram "No sampling", \
#    '../../logs/final-logs/speedup/speedup-full.log' using ($2) title '' lt 2, \
#    newhistogram "Sampling interval = 2 ms", \
#    '../../logs/final-logs/speedup/speedup-2ms.log' using ($2):xticlabels(1) title 'CCT', \
#    '../../logs/final-logs/speedup/speedup-2ms.log' using ($3) title 'LSS', \
#    '../../logs/final-logs/speedup/speedup-2ms.log' using ($4) title '  LC', \
#    newhistogram "Sampling interval = 5 ms", \
#    '../../logs/final-logs/speedup/speedup-5ms.log' using ($2):xticlabels(1) title '  CCT', \
#    '../../logs/final-logs/speedup/speedup-5ms.log' using ($3) title '  LSS', \
#    '../../logs/final-logs/speedup/speedup-5ms.log' using ($4) title '  LC'

