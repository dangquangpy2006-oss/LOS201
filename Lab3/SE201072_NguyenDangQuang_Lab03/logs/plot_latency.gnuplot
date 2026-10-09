set terminal pngcairo size 1400,800 enhanced font "DejaVu Sans,12"

set output "/home/nguyendangquang/embedded_lab3/logs/latency_histogram.png"

set title "LAB03 - Real-Time Latency Comparison" font ",18"

set xlabel "Latency (microseconds)"
set ylabel "Number of occurrences"

set xrange [0:1000]
set yrange [0.8:*]

set logscale y 10
set grid
set key top right

set label 1 "Kernel: 7.0.0-34-generic (PREEMPT_DYNAMIC)" at graph 0.02,0.95
set label 2 "Max latency: Standard = 554 us | Stress = 926 us" at graph 0.02,0.90

plot \
"/home/nguyendangquang/embedded_lab3/logs/hist_standard.txt" \
using 1:($2+$3+$4+$5) \
with steps linewidth 2 linecolor rgb "#2E75B6" \
title "Standard - No Active Stress", \
"/home/nguyendangquang/embedded_lab3/logs/hist_stress.txt" \
using 1:($2+$3+$4+$5) \
with steps linewidth 2 linecolor rgb "#C55A11" \
title "Under Stress - CPU + IO + RAM"
