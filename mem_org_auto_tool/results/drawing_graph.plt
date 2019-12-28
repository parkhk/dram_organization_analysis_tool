#!/bin/gnuplot
set terminal png truecolor size 1248,200#1024
#set yrange [1500:2000]
#set yrange [-1:2]
#set xrange [4096:4096*4]
set xrange [9216:9728]
set grid
set xtics 512
#set style data histograms
#set style fill solid 1.00 border -1
#plot "cpu_read_cnt" using 2:xtic(1) title "read request count"
set pointsize 0.3
set output "organization_results.png"
plot  "bank_conflict" using 1:2
