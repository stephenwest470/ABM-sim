/*
 * area.c - Simulation of population moving
 *		$Id: area.c,v 1.4 2013/02/24 17:12:26 void Exp $
 * vi: ts=4 sw=4 sts=4
 * sim7-5
 */
#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include "sim.h"
#include "main.h"
#include "person_pre.h"
#include "link.h"
#include "relpointer.h"
#include "history.h"
#include "person.h"
#include "resident.h"
#include "area.h"
#include "init.h"

static void bar(int, int, int);
static void graph(AREA *);
static void print_skill(AREA *, long long int);
static void print_unmarried_no(AREA *); // ただし再婚不可には対応してない

extern int n_area;
extern AREA areas[MAX_AREA];

//sim7-6-debug
extern PERSON* master_best76[MAX_AREA];

char *area_name(int area_code)
{
	for (int i = 0; i < n_area; i++) {
		if (areas[i].code == area_code)
			return areas[i].name;
	}
	error("Illegal area code. (for name)");
	return NULL;
}

AREA *areax(int area_code)
{
	for (int i = 0; i < n_area; i++) {
		if (areas[i].code == area_code) {
			return &areas[i];
		}
	}
	error("!!Illegal area code. (for index)");
	return NULL;
}

void inc_pop(AREA *a, SEX sex, int mon)
{
	a->population[sex][AGELAYER(mon)]++;
	a->pop1[sex][AGE(mon)]++;
	a->area_population++;
}

void dec_pop(AREA *a, SEX sex, int mon)
{
	a->population[sex][AGELAYER(mon)]--;
	a->pop1[sex][AGE(mon)]--;
	a->area_population--;
}

void raise10(AREA *a, SEX sex, int mon)
{
	a->population[sex][AGELAYER(mon)]++;
	a->population[sex][AGELAYER(mon - 1)]--;
}

void raise(AREA *a, SEX sex, int mon)
{
	a->pop1[sex][AGE(mon)]++;
	a->pop1[sex][AGE(mon - 1)]--;
}

void clear_move_count()
{
	for (int i = 0; i < n_area; i++) {
		for (ROUTE *rt = areas[i].come_from_list.next; !rt->dummy; rt = rt->next)
			rt->count_in_year = 0;
		for (ROUTE *rt = areas[i].go_to_list.next; !rt->dummy; rt = rt->next)
			rt->count_in_year = 0;
		areas[i].birth_count_in_year = 0LL;
		areas[i].death_count_in_year = 0LL;
	}
}

void purge_all_resident()
{
	for (int i = 0; i < n_area; i++) {
		for (RESIDENT *res = areas[i].resident.next; !res->dummy; res = purge_resident(res))
			;
	}
}

long long int count_all_resident(void)
{
	long long int r = 0LL;

	for (int i = 0; i < n_area; i++)
		r += count_resident(&areas[i]);
	return r;
}

long long int count_resident(const AREA *area)
{
	long long int count;

	count = 0LL;
	for (const RESIDENT *res = area->resident.next; !res->dummy; res = res->next)
		++count;
	return count;
}

long long int count_residentByBirthplace(const AREA *area, int code)	// !!出生地別人口
{
	long long int count;

	count = 0LL;
	for (const RESIDENT *res = area->resident.next; !res->dummy; res = res->next) {
		PERSON *p = res->person;
		if (code == p->birth_area)
			++count;
	}

	return count;
}

double GetMortalityRate(const PERSON *p)
{	// updated 5-1 & 5-3
	// return areax(p->birth_area)->MortalityRate[p->sex][AGE(p->month)];
	return areax(p->area)->MortalityRate[p->sex][AGE(p->month)];
}

void print_population10(bool graph_flag) // by 10 years
{
	long long int f_total;
	long long int m_total;

	for (int i = 0; i < n_area; i++) {
		printf("%02d %s",
			areas[i].code,
			areas[i].name
		);
		f_total = 0LL;
		m_total = 0LL;
		for (int j = 0; j < MAX_AGELAYER; j++) {
			printf(" %lld/%lld",
				areas[i].population[female][j],
				areas[i].population[male][j]
			);
			f_total += areas[i].population[female][j];
			m_total += areas[i].population[male][j];
		}

		printf(" %lld %3.2lf (+%lld/-%lld)", count_resident(&areas[i]),
			(double)m_total / (double)f_total,
			areas[i].birth_count_in_year, areas[i].death_count_in_year);

		for (ROUTE *rt = areas[i].come_from_list.next; !rt->dummy; rt = rt->next) {
		  int out_count_in_year = 0; // sim7-6 変更

			ROUTE *q;
			for (q = areas[i].go_to_list.next; !q->dummy; q = q->next) {
				if (q->link->to == rt->link->from) {
					out_count_in_year = q->count_in_year;
					break;
				}
			}
			/* sim7-6 comment out
			if (q->dummy)
				error("Invalid link (in/out mismatch) (printing).");
			*/
			printf(" %s[%d/%d]",
				area_name(rt->link->from),
				rt->count_in_year,
				out_count_in_year);
		}

		printf("\n");
		if (graph_flag)
			graph(&areas[i]);

	}
}

void print_population(bool graph_flag) //by a year
{
	long long int f_total;
	long long int m_total;
	long long int g_total;

	for (int i = 0; i < n_area; i++) {
		// 地区名
		printf("%s", areas[i].name);
		// 年齢別人口(区分は年齢別)
		f_total = 0LL;
		m_total = 0LL;
		for (int j = 0; j <= MAX_AGE; j++) {
			printf(",%lld,%lld",
				areas[i].pop1[female][j],
				areas[i].pop1[male][j]
			);
			f_total += areas[i].pop1[female][j];
			m_total += areas[i].pop1[male][j];
		}

		// その地域の総人口
		// 男女比
		// 前年からの自然増人口(その地域で生まれた人の数)
		// 前年からの自然減人口(その地域で死亡した人の数)
		printf(",%lld,%3.2lf,%lld,-%lld", g_total = count_resident(&areas[i]),
			(f_total == 0LL) ? 0LL : (double)m_total / (double)f_total,
			areas[i].birth_count_in_year, areas[i].death_count_in_year);

		// 前年からの流入者数(流入元地域ごと)
		// 前年からの流出者数(入出先地域ごと)

		/*sim7-6 で双方向でなくてもいいようにしてみようとしてコメントアウト
		if (areas[i].in_routes != areas[i].out_routes)
			error("Invalid link (in/out mismatch)!!\n");
		*/

		for (ROUTE *rt = areas[i].come_from_list.next; !rt->dummy; rt = rt->next) {
		  int out_count_in_year = 0; //sim7-6 変更

			ROUTE *q;
			for (q = areas[i].go_to_list.next; !q->dummy; q = q->next) {
				if (q->link->to == rt->link->from) {
					out_count_in_year = q->count_in_year;
					break;
				}
			}
			/* sim7-6 comment out
			if (q->dummy)
				error("Invalid link (in/out mismatch) (printing).");
			*/

			printf(",%s,%d,%d",
				area_name(rt->link->from),
				rt->count_in_year,
				out_count_in_year);
		}

		/*// !!
		// 出身地別の人口表示
		for (int j = 0; j < n_area; j++) {
			printf(",%s,%lld",
				areas[j].name,
				count_residentByBirthplace(&areas[i], areas[j].code));
			// printf("area,code:%s,%d\n",areas[j].name,areas[j].code);
		}*/

		printf("\n");
		if (graph_flag)
			graph(&areas[i]);

		print_skill(&areas[i], g_total);

		// for sim9-1. Number of unmarried persons age >= 15 years old
		print_unmarried_no(&areas[i]);

	}
}

static void graph(AREA *a)
{
	for (int i = MAX_AGELAYER - 1; i >= 0; i--)
		bar(10, a->population[female][i], a->population[male][i]);
}

static void print_skill(AREA *area, long long int g_total)
{
	double total;
	double tmp;
	double total15; // sim7-6
	double tmp15; // sim7-6
	int AGE15; // sim7-6
	int n;
	int n15; // sim7-6
	double top15; // sim7-6
	double bottom15; //sim7-6

	AGE15 = 15;
	n = 0; n15 = 0;
	total = 0.0; total15 = 0.0;
	top15 = -1.0; bottom15 = -1.0;
	for (const RESIDENT *res = area->resident.next; !res->dummy; res = res->next) {
		total += res->person->skill[0];
		if(res->person->month >= AGE15 * TERM_IN_YEAR){
		  total15 += res->person->skill[0];
		  ++n15;
		  if(top15 < 0){
		    top15 = res->person->skill[0];
		    bottom15 = res->person->skill[0];
		  }else{
		    if(res->person->skill[0] > top15){
		      top15 = res->person->skill[0];
		    }
		    if(res->person->skill[0] < bottom15){
		      bottom15 = res->person->skill[0];
		    }
		  }
		}
		++n;
	}
	tmp = 0.0; tmp15 = 0.0;
	for (const RESIDENT *res = area->resident.next; !res->dummy; res = res->next) {
		tmp += pow(total / g_total - res->person->skill[0], 2.0);
		if(res->person->month >= AGE15 * TERM_IN_YEAR){
		  tmp15 += pow(total15 / n15 - res->person->skill[0], 2.0);
		}
	}
	printf("area %s skill = %4.3f (%3.3f)\n", area->name, total / g_total, sqrt(tmp / n));
	printf("Skill15 area %s = %4.3f (%3.3f)\n", area->name, total15 / n15, sqrt(tmp15 / n15));
	printf("Skilltop area %s = %4.3f\n", area->name, top15);
	printf("Skillbottom area %s = %4.3f\n", area->name, bottom15);
	//sim7-6-debug
	if(master_best76[area->code] != NULL)
	  printf("master_best76 area %s = %4.3f, id=%lld, id->area=%d\n", area->name, master_best76[area->code]->skill[0], master_best76[area->code]->id, master_best76[area->code]->area);


}

static void print_unmarried_no(AREA *area) // ただし再婚不可には対応してない
{
	int AGE15; 
	int n;

	AGE15 = 15;
	n = 0;
	for (const RESIDENT *res = area->resident.next; !res->dummy; res = res->next) {
	  if(res->person->month >= AGE15 * TERM_IN_YEAR){
	    if(res->person->spouse_curno == 0){
	      n++;
	    }
	  }
	}
	printf("NumofNoMarriage area %s = %d\n", area->name, n);
}

static void bar(int r, int f, int m)
{
	for (int i = 0; i < 50; i++)
		printf(f / r < 50 - i ? " " : "*");

	printf("%8d%8d ", f, m);

	for (int i = 0; i < 50; i++)
		printf(i < m / r ? "*" : " ");

	printf("\n");
}

#if DNAversion
double total_gvalue(const AREA *area)
{
	double total_gvalue;

	total_gvalue = 0.0;
	for (const RESIDENT *res = area->resident.next; !res->dummy; res = res->next)
		total_gvalue += res->person->gvalue;
	return total_gvalue;
}

double total_square_gvalue(const AREA *area)
{
	double total_square_gvalue;

	total_square_gvalue = 0.0;
	for (const RESIDENT *res = area->resident.next; !res->dummy; res = res->next)
		total_square_gvalue += res->person->gvalue * res->person->gvalue;
	return total_square_gvalue;
}


double set_last_gvalue_means(AREA *area, double means)
{
	area->last_gvalue_means = means;
	return area->last_gvalue_means;
}

double get_last_gvalue_means(const AREA *area)
{
	return area->last_gvalue_means;
}

gvalueP count_gvalue_pattern(const AREA *area)
{
  gvalueP gp;
  gp.zero = 0; gp.one = 0; gp.two = 0; gp.three = 0;

  for (const RESIDENT *res = area->resident.next; !res->dummy; res = res->next){
    if(res->person->gvalue == 0.0){
      gp.zero++;
    }else if(res->person->gvalue == 1.0){
      gp.one++;
    }else if(res->person->gvalue == 2.0){
      gp.two++;
    }else if(res->person->gvalue == 3.0){
      gp.three++;
    }
  }

  return gp;
}
#endif

PERSON *searchResident(AREA *a, long long int n){
  for (RESIDENT *res = a->resident.prev; !res->dummy; res = res->prev) {
    if(res->person->id == n){
      return res->person;
    }
  }

  return NULL;
}

double get_pop_rate(AREA *a){
  return (double)a->area_population / (double)a->capacity;
}

long long int get_pop_under_cap(AREA *a){
  return a->capacity - a->area_population;
}


ROUTE* get_Go_Route(AREA *a, int code){
  ROUTE *rt;

  for(rt = a->go_to_list.next; !rt->dummy; rt = rt->next) {
    if(rt->link->to == code)
      break;
  }

  return rt;
}

