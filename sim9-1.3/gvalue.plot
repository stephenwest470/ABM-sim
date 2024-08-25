set terminal png
set output "gvalue.png"

set title "sim test 2022/4/24"
set xlabel "year"
set ylabel "gvalue"
plot [][] 'gvalue.data' using 1:2 title '1' with line, \
     '' using 1:3 title '2' with line, \
     '' using 1:4 title '3' with line, \
     '' using 1:5 title '4' with line, \
     '' using 1:6 title '5' with line, \
     '' using 1:7 title '6' with line, \
     '' using 1:8 title '7' with line, \
     '' using 1:9 title '8' with line, \
     '' using 1:10 title '9' with line, \
     '' using 1:11 title '10' with line, \
     '' using 1:12 title '11' with line, \
     '' using 1:13 title '12' with line, \
     '' using 1:14 title '13' with line, \
     '' using 1:15 title '14' with line, \
     '' using 1:16 title '15' with line, \
     '' using 1:17 title '16' with line, \
     '' using 1:18 title '17' with line, \
     '' using 1:19 title '18' with line, \
     '' using 1:20 title '19' with line, \
     '' using 1:21 title '20' with line, \
     '' using 1:22 title '21' with line, \
     '' using 1:23 title '22' with line
