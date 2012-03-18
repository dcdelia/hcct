# plot speedup
set terminal x11
set title "Non ho la minima idea di cosa stia succedendo qui sotto :)"

set style line 1 lt 1 lw 2 pt 8 ps 1
set style line 2 lt 1 lw 2 pt 1 ps 1
set style line 3 lt 1 lw 2 pt 6 ps 1
set style line 4 lt 1 lw 2 pt 3 ps 1
set style line 5 lt 1 lw 2 pt 4 ps 1
set style line 6 lt 1 lw 2 pt 8 ps 1
set style line 7 lt 2 lw 2 pt 1 ps 1
set style line 8 lt 2 lw 2 pt 6 ps 1
set style line 9 lt 2 lw 2 pt 3 ps 1
set style line 10 lt 2 lw 2 pt 4 ps 1
set style line 11 lt 2 lw 2 pt 8 ps 1
set style line 12 lt 3 lw 2 pt 1 ps 1


#set xtics (10, 25, 50, 75, 100, 250, 500, 750, 1000, 2500)
set key left top inside
set style line 1 lt 1 pt 1
set ylabel "MCCT construction time (s)"
set xlabel "1/eps * 10^-3"
#set logscale x

#set yrange [0:0.1]
#set xrange [5:30]
set grid
plot \
    '../../logs/final-logs/epsilon-time/lc-hcct-perf-epsilon-ark.log'  using ($9/1000):5 with linespoints ls 7 title 'ark, LC', \
    '../../logs/final-logs/epsilon-time/lss-hcct-perf-epsilon-ark.log'  using ($9/1000):5 with linespoints ls 2 title 'ark, LSS', \
    '../../logs/final-logs/epsilon-time/bss-hcct-perf-epsilon-ark.log'  using ($9/1000):5 with linespoints ls 12 title 'ark, BSS', \
    '../../logs/final-logs/epsilon-time/lc-hcct-perf-epsilon-ghex2.log'  using ($9/1000):5 with linespoints ls 8 title 'ghex2, LC', \
    '../../logs/final-logs/epsilon-time/lss-hcct-perf-epsilon-ghex2.log'  using ($9/1000):5 with linespoints ls 3 title 'ghex2, LSS', \
    '../../logs/final-logs/epsilon-time/lc-hcct-perf-epsilon-inkscape.log'  using ($9/1000):5 with linespoints ls 11 title 'inkscape, LC', \
    '../../logs/final-logs/epsilon-time/lss-hcct-perf-epsilon-inkscape.log'  using ($9/1000):5 with linespoints ls 6 title 'inkscape, LSS'
set terminal postscript eps color enhanced dashed "Helvetica" 16
set output 'speed-for-epsilon.eps'
replot 

# Full
#    '../../logs/final-logs/epsilon-time/lss-hcct-perf-epsilon-ark.log'  using ($9/1000):5 with linespoints ls 2 title 'ark, LSS', \
#    '../../logs/final-logs/epsilon-time/bss-hcct-perf-epsilon-ark.log'  using ($9/1000):5 with linespoints ls 12 title 'ark, BSS', \
#    '../../logs/final-logs/epsilon-time/lss-hcct-perf-epsilon-ghex2.log'  using ($9/1000):5 with linespoints ls 3 title 'ghex2, LSS', \
#    '../../logs/final-logs/epsilon-time/lss-hcct-perf-epsilon-gimp.log'  using ($9/1000):5 with linespoints ls 4 title 'gimp, LSS', \
#    '../../logs/final-logs/epsilon-time/lss-hcct-perf-epsilon-inkscape.log'  using ($9/1000):5 with linespoints ls 5 title 'inkscape, LSS', \
#    '../../logs/final-logs/epsilon-time/lss-hcct-perf-epsilon-oocalc.log'  using ($9/1000):5 with linespoints ls 6 title 'oocalc, LSS', \
#    '../../logs/final-logs/epsilon-time/lc-hcct-perf-epsilon-ark.log'  using ($9/1000):5 with linespoints ls 7 title 'ark, LC', \
#    '../../logs/final-logs/epsilon-time/lc-hcct-perf-epsilon-ghex2.log'  using ($9/1000):5 with linespoints ls 8 title 'ghex2, LC', \
#    '../../logs/final-logs/epsilon-time/lc-hcct-perf-epsilon-gimp.log'  using ($9/1000):5 with linespoints ls 9 title 'gimp, LC', \
#    '../../logs/final-logs/epsilon-time/lc-hcct-perf-epsilon-inkscape.log'  using ($9/1000):5 with linespoints ls 10 title 'inkscape, LC', \
#    '../../logs/final-logs/epsilon-time/lc-hcct-perf-epsilon-oocalc.log'  using ($9/1000):5 with linespoints ls 11 title 'oocalc, LC'


#    '../../logs/final-logs/epsilon-time/running-times-lss-ark-25.log'  using ($9/10000):5 with linespoints ls 3 title 'ark, LSS, SR 25', \
#    '../../logs/final-logs/epsilon-time/running-times-lc-ark-25.log'   using ($9/10000):5 with linespoints ls 4 title 'ark, LC, SR 25', \
#    '../../logs/final-logs/epsilon-time/running-times-lss-gimp-25.log' using ($9/10000):5 with linespoints ls 7 title 'gimp, LSS, SR 25', \
#    '../../logs/final-logs/epsilon-time/running-times-lc-gimp-25.log'  using ($9/10000):5 with linespoints ls 8 title 'gimp, LC, SR 25'

