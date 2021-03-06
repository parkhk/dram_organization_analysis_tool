#!/bin/gnuplot
set terminal png truecolor size 1248,200#1024
#set yrange [14200000:14800000]
#set yrange [-1:2]
set xrange [8192*3:8192*4]
set grid
#set logscale y
#set xtic rotate by 90
#set style data histograms
#set style fill solid 1.00 border -1
#plot "cpu_read_cnt" using 2:xtic(1) title "read request count"
#set style histogram clustered
set pointsize 0.3
set output "results/organization_results.png"
#plot  "results/identify_bank_rank" using 2:xtic(1)
plot  "results/identify_page" using 1:2
