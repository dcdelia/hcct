# plot max phi
load "palette.p"
set terminal x11
set title "Largest value of {/Symbol f} that guarantees a given degree of overlap"
set style line 1 lt 1 pt 1
set key right top outside
set xlabel "Degree of overlap (%)"
set ylabel "Max {/Symbol f}"
set yrange [0.000000001:*]
set xrange [5:*]
set logscale y
#set logscale x
#set xtics 10
set grid
plot    '../../logs/final-logs/max-phi/cct-max-phi-amarok.log' using 2:3  with linespoints title 'amarok'  ,\
        '../../logs/final-logs/max-phi/cct-max-phi-audacity.log' using 2:3  with linespoints title 'audacity'  ,\
        '../../logs/final-logs/max-phi/cct-max-phi-bluefish.log' using 2:3  with linespoints title 'bluefish'  ,\
        '../../logs/final-logs/max-phi/cct-max-phi-firefox.log' using 2:3  with linespoints title 'firefox'  ,\
        '../../logs/final-logs/max-phi/cct-max-phi-gedit.log' using 2:3  with linespoints title 'gedit'  ,\
        '../../logs/final-logs/max-phi/cct-max-phi-gwenview.log' using 2:3  with linespoints title 'gwenview'  ,\
        '../../logs/final-logs/max-phi/cct-max-phi-oocalc.log' using 2:3  with linespoints title 'oocalc'  ,\
        '../../logs/final-logs/max-phi/cct-max-phi-pidgin.log' using 2:3  with linespoints title 'pidgin'  ,\
        '../../logs/final-logs/max-phi/cct-max-phi-quanta.log' using 2:3  with linespoints title 'quanta'  ,\
        '../../logs/final-logs/max-phi/cct-max-phi-vlc.log' using 2:3  with linespoints title 'vlc' 
set terminal postscript eps enhanced color dashed "Helvetica" 18
set output '../../docs/paper/charts/maxphi.eps'
replot


#plot    '../../logs/final-logs/max-phi/cct-max-phi-amarok.log' using 2:3  with linespoints title 'amarok'  ,\
#        '../../logs/final-logs/max-phi/cct-max-phi-ark.log' using 2:3  with linespoints title 'ark'  ,\
#        '../../logs/final-logs/max-phi/cct-max-phi-audacity.log' using 2:3  with linespoints title 'audacity'  ,\
#        '../../logs/final-logs/max-phi/cct-max-phi-bluefish.log' using 2:3  with linespoints title 'bluefish'  ,\
#        '../../logs/final-logs/max-phi/cct-max-phi-dolphin.log' using 2:3  with linespoints title 'dolphin'  ,\
#        '../../logs/final-logs/max-phi/cct-max-phi-firefox.log' using 2:3  with linespoints title 'firefox'  ,\
#        '../../logs/final-logs/max-phi/cct-max-phi-gedit.log' using 2:3  with linespoints title 'gedit'  ,\
#        '../../logs/final-logs/max-phi/cct-max-phi-ghex2.log' using 2:3  with linespoints title 'ghex'  ,\
#        '../../logs/final-logs/max-phi/cct-max-phi-gimp.log' using 2:3  with linespoints title 'gimp'  ,\
#        '../../logs/final-logs/max-phi/cct-max-phi-gwenview.log' using 2:3  with linespoints title 'gwenview'  ,\
#        '../../logs/final-logs/max-phi/cct-max-phi-inkscape.log' using 2:3  with linespoints title 'inkscape'  ,\
#        '../../logs/final-logs/max-phi/cct-max-phi-oocalc.log' using 2:3  with linespoints title 'oocalc'  ,\
#        '../../logs/final-logs/max-phi/cct-max-phi-ooimpress.log' using 2:3  with linespoints title ' ooimpress'  ,\
#        '../../logs/final-logs/max-phi/cct-max-phi-oowriter.log' using 2:3  with linespoints title 'oowriter'  ,\
#        '../../logs/final-logs/max-phi/cct-max-phi-pidgin.log' using 2:3  with linespoints title 'pidgin'  ,\
#        '../../logs/final-logs/max-phi/cct-max-phi-quanta.log' using 2:3  with linespoints title 'quanta'  ,\
#        '../../logs/final-logs/max-phi/cct-max-phi-sudoku.log' using 2:3  with linespoints title 'sudoku'  ,\
#        '../../logs/final-logs/max-phi/cct-max-phi-vlc.log' using 2:3  with linespoints title 'vlc' 
