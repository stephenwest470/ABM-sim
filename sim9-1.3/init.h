/*
 * init.h - Simulation of population moving
 *		$Id: init.h,v 1.2 2013/02/24 17:12:26 void Exp $
 * vi: ts=4 sw=4 sts=4
 */
int read_area(const char *);
int read_link(const char *);
int read_init(const char *);
int read_MortalityRate(const char *);
int read_BirthRate(const char *);
int read_Skill(const char *);
void initial(int, FLAGS);
