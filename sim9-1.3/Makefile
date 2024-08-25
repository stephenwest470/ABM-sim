#
# Makefile - Simulation of population moving
#		$Id: Makefile,v 1.4 2013/02/24 17:12:26 void Exp $
# vi: ts=4 sw=4 sts=4
#
CC = gcc
#COPTS=-pg -g -Wall -std=c99 -DDNAversion=1
#COPTS=-O3 -g -Wall -std=c99 -DDNAversion=0 -DSFMT_MEXP=19937
COPTS=-O3 -g -Wall -std=c99 -DDNAversion=1 -DSFMT_MEXP=19937 
#COPTS= -g -Wall -std=c99 -DDNAversion=0


EXEC=sim

$(EXEC): sim.o main.o rand.o history.o resident.o area.o person.o relpointer.o init.o SFMT.o
	$(CC) $(COPTS) main.o sim.o rand.o history.o resident.o area.o person.o relpointer.o init.o SFMT.o -lm -o $(EXEC)

area.o: area.c \
		sim.h main.h person_pre.h link.h relpointer.h history.h person.h resident.h area.h
	$(CC) $(COPTS) -c area.c

history.o: history.c \
		sim.h main.h person_pre.h link.h relpointer.h history.h person.h resident.h area.h
	$(CC) $(COPTS) -c history.c

init.o: init.c \
		sim.h main.h rand.h init.h person_pre.h link.h relpointer.h history.h person.h resident.h area.h
	$(CC) $(COPTS) -c init.c

main.o: main.c sim.c area.c history.c init.c rand.c relpointer.c person.c resident.c\
		sim.h main.h rand.h init.h person_pre.h link.h relpointer.h history.h person.h resident.h area.h
	$(CC) $(COPTS) -c main.c

person.o: person.c \
		sim.h main.h person_pre.h link.h relpointer.h history.h person.h resident.h area.h
	$(CC) $(COPTS) -c person.c

rand.o: rand.c \
		main.h rand.h
	$(CC) $(COPTS) -c rand.c

SFMT.o: SFMT.c \
		SFMT.h
	$(CC) $(COPTS) -c SFMT.c

relpointer.o: relpointer.c \
		sim.h main.h person_pre.h link.h relpointer.h history.h person.h resident.h area.h relpointer.h
	$(CC) $(COPTS) -c relpointer.c

resident.o: resident.c \
		sim.h main.h person_pre.h link.h relpointer.h history.h person.h resident.h area.h
	$(CC) $(COPTS) -c resident.c

sim.o: sim.c \
		sim.h main.h rand.h init.h person_pre.h link.h relpointer.h history.h person.h resident.h area.h
	$(CC) $(COPTS) -c sim.c

clean::
	rm -f *.o sim gmon.out callf.order callo.order gmon.order time.order

tags::
	ctags -t *.c *.h

prof::
	gprof -S sim | less
