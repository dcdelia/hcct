#!/bin/bash
benchmarks=( amarok ark audacity bluefish dolphin firefox gedit ghex2 gimp sudoku gwenview inkscape oocalc ooimpress oowriter pidgin quanta vlc )
for (( i=0 ; i < ${#benchmarks[@]} ; i++ )) {
	## Format: original_file_name.program_name
	# Header
	head -n 1 $1 | cat > $1.${benchmarks[$i]}
	head -n $((2*$i+3)) $1 | tail -n 2 | cat >> $1.${benchmarks[$i]}
}
