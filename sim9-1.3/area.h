/*
 * area.h - Simulation of population moving
 *		$Id: area.h,v 1.4 2013/02/24 17:12:26 void Exp $
 * vi: ts=4 sw=4 sts=4
 * sim7-5
 */
#include "person.h"

typedef struct {
	int	code;
	char	name[NAME_LEN + 1];
	int	in_routes;
	int	out_routes;
	long long int	population[2][MAX_AGELAYER];
	long long int	pop1[2][MAX_AGE + 1];
	long long int	capacity;
	long long int	area_population;
	long long int	max_pop;	// ###
	long long int	*pop;	// !!
	int	init_pop_type;
	long long int	init_pop;
	double	init_sex_ratio;
#if DNAversion
	double  init_gvalue;
	double  last_gvalue_means;
#endif
	long long int	birth_count_in_year;
	long long int	death_count_in_year;
	// double initBabyMortalityRate[2];	// updated 5-1
	// double	BabyMortalityRate[2];	// updated 5-1
	double	MortalityRate[2][MAX_AGE + 1];	// updated 5-1
	double	initMortalityRate[2][MAX_AGE + 1];	// ## updated 5-1
	double	BirthRate[2][MAX_AGE + 1];
	double	DecreaseBirthRate;
	double	MoveRate;
	double	initMoveRate;	// ##
	double	LimitLine;		// ##
	double	GrowthMoveRate;	// ##
	double	GrowthMortalityRate;	// ##
	double	SafeLine;	// ##
        int     masterSearch; // 0: prev から 1: next から
        bool    masterRandom; // true: 1 -10 のどれか false: 1人目
        bool masterRemember; // true: 前のmasterを覚えてる false: 覚えてない
	bool	FoodCrisis;	// ##食糧危機
	RESIDENT resident;
	ROUTE	come_from_list;
	ROUTE	go_to_list;
	double	Skill[2][MAX_AGE + 1];
        bool dnorm_flag; // 正規分布かどうかのフラグ sim7-6
} AREA;

char *area_name(int);
AREA *areax(int);
void inc_pop(AREA *, SEX, int);
void dec_pop(AREA *, SEX, int);
void raise10(AREA *, SEX, int);
void raise(AREA *, SEX, int);
void clear_move_count(void);
long long int count_all_resident(void);
long long int count_resident(const AREA *);
long long int count_residentByBirthplace(const AREA *area, int code);
double GetMortalityRate(const PERSON *);
void print_population(bool);
void print_population10(bool);
void purge_all_resident(void);
double get_pop_rate(AREA *);
long long int get_pop_under_cap(AREA *);

#if DNAversion
typedef struct { // sim8-2 one-locus two-allele 用
  int zero;
  int one;
  int two;
  int three;
}gvalueP;
double total_gvalue(const AREA *);
double total_square_gvalue(const AREA *);
double set_last_gvalue_means(AREA *, double);
double get_last_gvalue_means(const AREA *area);
gvalueP count_gvalue_pattern(const AREA *);
#endif

PERSON *searchResident(AREA *, long long int);
ROUTE *get_Go_Route(AREA*, int);
