#!/bin/bash

if [ "$#" -eq "1" ]
then
	echo "Usage: ./$0 f1 [f2 ..]"
	exit
fi

img=graphe.png

gnuplot << EOF
set grid
set xlabel "temps (ms)"
set ylabel ""
set title "Temps de latence entre deux zigduino"

plot \
"11" using 1 with linespoints lt 1 title "11", \
"15" using 1 with linespoints lt 3 title "15", \
"20" using 1 with linespoints lt 4 title "20", \
"25" using 1 with linespoints lt 6 title "25" 
set terminal png
set output "${img}"
replot
EOF
