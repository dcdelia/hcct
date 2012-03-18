# plot cumulative frequencies
set terminal x11
set title "cumulative frequency distributions"
set xlabel "% of hot contexts sorted by rank"
set ylabel "cumulative frequency relative to total number of calls"
set xtics 10
set grid
plot 'cct-audacity-0.out' using 2:3 with linespoints title 'audacity' \
     , 'cct-audacity-startup-0.out' using 2:3 with linespoints title 'audacity (startup only)' \
     , 'cct-bzip2-0.out' using 2:3 with linespoints title 'bzip2' \
     , 'cct-gimp-0.out' using 2:3 with linespoints title 'gimp' \
     , 'cct-gnome-dictionary-0.out' using 2:3 with linespoints title 'gnome-dictionary' \
     , 'cct-inkscape-startup-0.out' using 2:3 with linespoints title 'inkscape' \
#    , 'cct-ps-0.out' using 2:3 with linespoints title 'ps'
set terminal postscript eps enhanced mono dashed "Helvetica" 18
set output 'cumulative.eps'
replot
