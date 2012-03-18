# plot speedup
load "palette.p"
set terminal x11
set title "Comparison of hot edge coverage results using static and adaptive bursting"
set key right top outside
set xlabel ""
set ylabel "Hot edge coverage (%)"
set yrange [0:100]
set ytics 10
set style data histogram
set xtic rotate by -45 scale 0
set style histogram cluster gap 1
set style fill solid border -2
set boxwidth 0.8
set grid y
set grid noxtics
plot \
    newhistogram "", \
    '../../logs/final-logs/sampling-accuracy/tau40.log' using ($3):xticlabels(1) title '  LSS' lt 4, \
    '../../logs/final-logs/sampling-accuracy/tau40.log' using ($2) title '  LC' lt 2
set terminal postscript eps color enhanced dashed "Helvetica" 18
set output '../../docs/paper/charts/sampling-accuracy.eps'
replot

#    newhistogram "{/Symbol t} = 0.05", \
#    '../../logs/final-logs/sampling-accuracy/tau20.log' using ($3):xticlabels(1) title '  LSS' lt 4, \
#    '../../logs/final-logs/sampling-accuracy/tau20.log' using ($2) title '  LC' lt 2, \
#    newhistogram "{/Symbol t} = 0.025", \
#    '../../logs/final-logs/sampling-accuracy/tau40.log' using ($3):xticlabels(1) title '' lt 4, \
#    '../../logs/final-logs/sampling-accuracy/tau40.log' using ($2) title '' lt 2, \
#    newhistogram "{/Symbol t} = 0.0125", \
#    '../../logs/final-logs/sampling-accuracy/tau80.log' using ($3):xticlabels(1) title '' lt 4, \
#    '../../logs/final-logs/sampling-accuracy/tau80.log' using ($2) title '' lt 2
