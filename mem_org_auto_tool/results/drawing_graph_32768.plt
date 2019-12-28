#!/bin/gnuplot
set terminal png truecolor size 1248,200#1024
#set yrange [1500:2000]
#set yrange [-1:2]
#set xrange [64*5:64*6]
set xrange [:200]
set grid
#set xtics 1

#set style data histograms
#set style fill solid 1.00 border -1
#plot "cpu_read_cnt" using 2:xtic(1) title "read request count"
set pointsize 0.2
set output "organization_results_cl32768.png"
#set style line 3 pt 12
plot  "bank_conflict_cl32768" pt 13 #using 1:2 pt 13
