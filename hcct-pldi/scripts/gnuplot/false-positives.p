# plot false positives, cold and hot percentage
load "palette.p"
set terminal x11
set title "Classification of ({/Symbol f},{/Symbol e})-HCCT nodes"
#set auto x
set key under
set ylabel "Cold nodes / hot nodes / false positives (%)"
set yrange [0:100]
set style data histogram
set xtic rotate by -45 scale 0
set style histogram rowstacked title offset 0,-1
set style fill solid border -2
set boxwidth 0.7

plot \
     newhistogram,\
     '../../logs/final-logs/falseColdHot/tmp/hcct-metrics-10000-50000-merge.log.amarok' using (100*$8/$7):xtic(1) notitle fs solid lc rgb "blue",\
     '' using (100*$9/$7)  notitle fs lc rgb "green",\
     '' using (100*$10/$7) notitle fs lc rgb "red",\
     newhistogram,\
     '../../logs/final-logs/falseColdHot/tmp/hcct-metrics-10000-50000-merge.log.ark' using (100*$8/$7):xtic(1) notitle fs solid lc rgb "blue",\
     '' using (100*$9/$7)  notitle fs lc rgb "green",\
     '' using (100*$10/$7) notitle fs lc rgb "red",\
     newhistogram,\
     '../../logs/final-logs/falseColdHot/tmp/hcct-metrics-10000-50000-merge.log.audacity' using (100*$8/$7):xtic(1) notitle fs solid lc rgb "blue",\
     '' using (100*$9/$7)  notitle fs lc rgb "green",\
     '' using (100*$10/$7) notitle fs lc rgb "red",\
     newhistogram,\
     '../../logs/final-logs/falseColdHot/tmp/hcct-metrics-10000-50000-merge.log.bluefish' using (100*$8/$7):xtic(1) notitle fs solid lc rgb "blue",\
     '' using (100*$9/$7)  notitle fs lc rgb "green",\
     '' using (100*$10/$7) notitle fs lc rgb "red",\
     newhistogram,\
     '../../logs/final-logs/falseColdHot/tmp/hcct-metrics-10000-50000-merge.log.dolphin' using (100*$8/$7):xtic(1) notitle fs solid lc rgb "blue",\
     '' using (100*$9/$7)  notitle fs lc rgb "green",\
     '' using (100*$10/$7) notitle fs lc rgb "red",\
     newhistogram,\
     '../../logs/final-logs/falseColdHot/tmp/hcct-metrics-10000-50000-merge.log.firefox' using (100*$8/$7):xtic(1) notitle fs solid lc rgb "blue",\
     '' using (100*$9/$7)  notitle fs lc rgb "green",\
     '' using (100*$10/$7) notitle fs lc rgb "red",\
     newhistogram,\
     '../../logs/final-logs/falseColdHot/tmp/hcct-metrics-10000-50000-merge.log.gedit' using (100*$8/$7):xtic(1) notitle fs solid lc rgb "blue",\
     '' using (100*$9/$7)  notitle fs lc rgb "green",\
     '' using (100*$10/$7) notitle fs lc rgb "red",\
     newhistogram,\
     '../../logs/final-logs/falseColdHot/tmp/hcct-metrics-10000-50000-merge.log.ghex2' using (100*$8/$7):xtic(1) notitle fs solid lc rgb "blue",\
     '' using (100*$9/$7)  notitle fs lc rgb "green",\
     '' using (100*$10/$7) notitle fs lc rgb "red",\
     newhistogram,\
     '../../logs/final-logs/falseColdHot/tmp/hcct-metrics-10000-50000-merge.log.gimp' using (100*$8/$7):xtic(1) notitle fs solid lc rgb "blue",\
     '' using (100*$9/$7)  notitle fs lc rgb "green",\
     '' using (100*$10/$7) notitle fs lc rgb "red",\
     newhistogram,\
     '../../logs/final-logs/falseColdHot/tmp/hcct-metrics-10000-50000-merge.log.sudoku' using (100*$8/$7):xtic(1) notitle fs solid lc rgb "blue",\
     '' using (100*$9/$7)  notitle fs lc rgb "green",\
     '' using (100*$10/$7) notitle fs lc rgb "red",\
     newhistogram,\
     '../../logs/final-logs/falseColdHot/tmp/hcct-metrics-10000-50000-merge.log.gwenview' using (100*$8/$7):xtic(1) notitle fs solid lc rgb "blue",\
     '' using (100*$9/$7)  notitle fs lc rgb "green",\
     '' using (100*$10/$7) notitle fs lc rgb "red",\
     newhistogram,\
     '../../logs/final-logs/falseColdHot/tmp/hcct-metrics-10000-50000-merge.log.inkscape' using (100*$8/$7):xtic(1) notitle fs solid lc rgb "blue",\
     '' using (100*$9/$7)  notitle fs lc rgb "green",\
     '' using (100*$10/$7) notitle fs lc rgb "red",\
     newhistogram,\
     '../../logs/final-logs/falseColdHot/tmp/hcct-metrics-10000-50000-merge.log.oocalc' using (100*$8/$7):xtic(1) notitle fs solid lc rgb "blue",\
     '' using (100*$9/$7)  notitle fs lc rgb "green",\
     '' using (100*$10/$7) notitle fs lc rgb "red",\
     newhistogram,\
     '../../logs/final-logs/falseColdHot/tmp/hcct-metrics-10000-50000-merge.log.ooimpress' using (100*$8/$7):xtic(1) notitle fs solid lc rgb "blue",\
     '' using (100*$9/$7)  notitle fs lc rgb "green",\
     '' using (100*$10/$7) notitle fs lc rgb "red",\
     newhistogram,\
     '../../logs/final-logs/falseColdHot/tmp/hcct-metrics-10000-50000-merge.log.oowriter' using (100*$8/$7):xtic(1) notitle fs solid lc rgb "blue",\
     '' using (100*$9/$7)  notitle fs lc rgb "green",\
     '' using (100*$10/$7) notitle fs lc rgb "red",\
     newhistogram,\
     '../../logs/final-logs/falseColdHot/tmp/hcct-metrics-10000-50000-merge.log.pidgin' using (100*$8/$7):xtic(1) notitle fs solid lc rgb "blue",\
     '' using (100*$9/$7)  notitle fs lc rgb "green",\
     '' using (100*$10/$7) notitle fs lc rgb "red",\
     newhistogram,\
     '../../logs/final-logs/falseColdHot/tmp/hcct-metrics-10000-50000-merge.log.quanta' using (100*$8/$7):xtic(1) notitle fs solid lc rgb "blue",\
     '' using (100*$9/$7)  notitle fs lc rgb "green",\
     '' using (100*$10/$7) notitle fs lc rgb "red",\
     newhistogram,\
     '../../logs/final-logs/falseColdHot/tmp/hcct-metrics-10000-50000-merge.log.vlc' using (100*$8/$7):xtic(1) notitle fs solid lc rgb "blue",\
     '' using (100*$9/$7)  notitle fs lc rgb "green",\
     '' using (100*$10/$7) notitle fs lc rgb "red"
set terminal postscript eps color enhanced dashed "Helvetica" 18
set output '../../docs/paper/charts/false-positives.eps'
replot


#plot '../../logs/final-logs/falseColdHot/hcct-metrics-10000-50000-merge.log' using (100*$8/$7):xtic(1) notitle,\
#     '' using (100*$9/$7)  notitle,\
#     '' using (100*$10/$7) notitle



#     newhistogram "False positives",\
#     '../../logs/final-logs/10000-50000/lss-hcct-metrics-10000-50000.log' u (100*$8/$5):xtic(1) t 'LSS',\
#     '../../logs/final-logs/10000-50000/lc-hcct-metrics-10000-50000.log' u (100*$8/$5) t 'LC' ,\
#     newhistogram "True hot",\
#     '../../logs/final-logs/10000-50000/lss-hcct-metrics-10000-50000.log' u (100*$6/$5):xtic(1) t 'LSS', \
#     '../../logs/final-logs/10000-50000/lc-hcct-metrics-10000-50000.log' u (100*$6/$5) t 'LC', \
#     '../../logs/final-logs/10000-50000/lss-hcct-metrics-10000-50000.log' u (100*$25/$24) t 'Exact HCCT', \
#     newhistogram "Cold",\
#     '../../logs/final-logs/10000-50000/lss-hcct-metrics-10000-50000.log' u (100*$7/$5):xtic(1) t 'LSS', \
#     '../../logs/final-logs/10000-50000/lc-hcct-metrics-10000-50000.log' u (100*$7/$5) t 'LC', \
#     '../../logs/final-logs/10000-50000/lss-hcct-metrics-10000-50000.log' u (100*$26/$24) t 'Exact HCCT'


