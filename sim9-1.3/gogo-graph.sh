#!/bin/sh

cat logN.txt | ./proc.rb > poplog.data
gnuplot pop.plot
mv poplog.png poplogN.png

cat logN.txt | ./gvalue.rb > gvalue.data
gnuplot gvalue.plot
mv gvalue.png gvalueN.png

cat logN.txt | ./skill.rb > skill.data
gnuplot hinaskill.plot
mv skill.png skillN.png

cat logN.txt | ./migration.rb > migration.data
gnuplot hinamigration.plot
mv migration.png migrationN.png