#!/bin/sh

(time ./sim -Go -d1 -r3 -f2 -Si7 -Sx1 -Sa1.1 -SX1 -Se2 -Sm1.0 -Sg0.0 -SE$i -Sk2 -Su81 -Pm -Px4 -Pb0.25 -Ps1 -Be1.2 500) > logPY.txt 2>&1
(time ./sim -Go -d1 -r3 -f2 -Si7 -Sx1 -Sa1.1 -SX1 -Se2 -Sm1.0 -Sg0.0 -SE$i -Sk2 -Su81 -Pm -Px4 -Be1.2 500) > logP.txt 2>&1
(time ./sim -Go -d1 -r3 -f2 -Si7 -Sx1 -Sa1.1 -SX1 -Se2 -Sm1.0 -Sg0.0 -SE$i -Sk2 -Su81 -Be1.2 500) > logM.txt 2>&1