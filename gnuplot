# set term png
set datafile separator comma
set key autotitle columnhead
plot "bpsk.csv" using 0:($1-19) with lines, '' using 0:2 with lines, '' using 0:3 with lines, '' using 0:4 with lines, '' using 0:5 with lines, '' using 0:6 with lines, '' using 0:7 with lines, '' using 0:8 with lines, '' using 0:9 with lines, '' using 0:10 with lines, '' using 0:11 with lines
pause 100
