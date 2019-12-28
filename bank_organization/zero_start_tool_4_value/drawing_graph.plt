#!/bin/gnuplot
set terminal png truecolor size 1248,200#1024
#set yrange [19200000000.0:19800000000.0]
#set yrange [:20050000000.0]
#set yrange [-1:2]
#set xrange [:500]
set grid
#set logscale y
#set xtic rotate by 90
set style data histograms
set style fill solid 1.00 border -1
#plot "cpu_read_cnt" using 2:xtic(1) title "read request count"
#set style histogram clustered
#set pointsize 1
set output "results/organization_results.png"
plot  "results/identify_bits" using 2:xtic(1)
#plot  "results/identify_bank_rank" using 1:2
