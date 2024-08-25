/*
 * main.h - Simulation of population moving
 *		$Id: main.h,v 1.2 2013/02/10 15:42:50 void Exp $
 * vi: ts=4 sw=4 sts=4
 */
void *MALLOC(size_t);
void FREE(void *);
unsigned int getSeed(void);
void cant(const char *);
void error(const char *);

// methods are in person.c
void people(int);
void people_marriage_childinfo();

