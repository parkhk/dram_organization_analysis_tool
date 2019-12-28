#!/bin/gnuplot
set terminal png truecolor size 1248,200#1024
#set yrange [1500:2000]
#set yrange [-1:2]
set xrange [1024*12:1024*16]
#set xrange [:4096]
set grid
set xtics 64 
#set style data histograms
#set style fill solid 1.00 border -1
#plot "cpu_read_cnt" using 2:xtic(1) title "read request count"
set pointsize 0.3
set output "organization_results_cl512.png"
plot  "bank_conflict_cl512" using 1:2
