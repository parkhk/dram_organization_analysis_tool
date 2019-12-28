#!/bin/gnuplot
set terminal png truecolor size 1248,200#1024
#set yrange [1500:2000]
set xrange [0:512]
set grid
#set xtic rotate by 90
#set style data histograms
#set style fill solid 1.00 border -1
#plot "cpu_read_cnt" using 2:xtic(1) title "read request count"
set pointsize 0.3
set output "results/organization_results_64M.png"
plot  "results/bank_conflict_64M" using 1:2
