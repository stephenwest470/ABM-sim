/*
 * rand.c - Simulation of population moving
 *		$Id: rand.c,v 1.2 2013/02/10 15:42:50 void Exp $
 * vi: ts=4 sw=4 sts=4
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include "main.h"
#include "SFMT.h"
#include "rand.h"
#define M_PI 3.14159265358979323846264338327950288

static sfmt_t sfmt;

bool probability(double rate)
{
  //	bool a = ((double)rand() < (double)RAND_MAX * rate);
  bool a = (mydoublerand() < rate);
	return a;
}

int roulette(int n)
{
  //	return rand() % n;
  return myrand() % n;
}

int roulette1(int n)
{
	int r1;
	int r2;
	int r3;

	//	r1 = rand();
	//	r2 = rand();
	//	r3 = rand();
	r1 = myrand();
	r2 = myrand();
	r3 = myrand();

	return n - cbrt(n * n * (double)(r1 % n));
}

int roulette2(int n)
{
	int r1;
	int r2;
	int r3;

	//	r1 = rand();
	//	r2 = rand();
	//	r3 = rand();
	r1 = myrand();
	r2 = myrand();
	r3 = myrand();
	return abs(cbrt((double)(r1 % n) * (double)(r2 % n) * (double)(r3 % n)) - (n / 2)) * 2;
}

int roulette3(int n)
{
	int r1;
	int r2;

	//	r1 = rand();
	//	r2 = rand();
	r1 = myrand();
	r2 = myrand();
	return sqrt((double)(r1 % n) * (double)(r2 % n));
}

void seeding_default(void)
{
  sfmt_init_gen_rand(&sfmt, 0);
}

void seeding(void)
{
  //	srand(getSeed());
  sfmt_init_gen_rand(&sfmt, getSeed());
}

int myrand()
{ // sfmt_genrand_uint64 �� int �Ǽ�����ȥޥ��ʥ��ˤʤ��ͤ���������Τǡ�
  int x;

  x = sfmt_genrand_uint64(&sfmt);
  if (x < 0) x = x * (-1);
  return x;
}

double mydoublerand()
{
  return sfmt_genrand_res53(&sfmt);
}

double rand_normal( double mu, double sigma ){
  double z = sqrt(-2.0*log(mydoublerand())) * sin( 2.0*M_PI*mydoublerand());
  return mu + sigma*z;
}
