/*
 * rand.h - Simulation of population moving
 *		$Id: rand.h,v 1.1 2010/11/25 03:57:55 void Exp $
 * vi: ts=4 sw=4 sts=4
 */
bool probability(double);
int roulette(int);
int roulette1(int);
int roulette2(int);
int roulette3(int);
void seeding_default(void);
void seeding(void);
int myrand();
double mydoublerand();
double rand_normal(double, double);
