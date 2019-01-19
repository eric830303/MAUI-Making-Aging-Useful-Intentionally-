set title "Benchmark s13207"
set xlabel "Improvement (%)" 	font "Times_New_Roman,20 "
set ylabel "Instances count" 	font "Times_New_Roman,20 "
set terminal png 		font "Times_New_Roman,20 "
set output "s13207_Imp_dist.png"
set xtics offset -1 		font "Times_New_Roman,16"
set ytics 			font "Times_New_Roman,16 "
set key left 


##------ PLOT --------------------------------------------------------------------
plot \
"Imp_dist.txt" using 1:2 with linespoints linewidth 2 pt 5 ps 1 lc 1 notitle, \
