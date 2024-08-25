/*
 * init.c - Simulation of population moving
 *		$Id: init.c,v 1.4 2013/02/24 17:12:26 void Exp $
 * vi: ts=4 sw=4 sts=4
 * sim7-5
 */
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "sim.h"
#include "main.h"
#include "rand.h"
#include "init.h"
#include "person_pre.h"
#include "link.h"
#include "relpointer.h"
#include "history.h"
#include "person.h"
#include "resident.h"
#include "area.h"

#define MAX_LINK		1000
#define LINE_LEN		2000
#define INIT_MAX_AGE    100

//sim7-6
double dn_mean = 10.0;
double dn_sigma = 1.0; // Ŭ���˷�᤿�����
//sim78-6 end

int n_area;
AREA areas[MAX_AREA];
LINK links[MAX_LINK];

int n_link; // 2022version

static ROUTE *add_route(ROUTE *, LINK *);

int read_area(const char *filename)
{
	FILE *fp;
	char buf[LINE_LEN + 1];
	int i;
	char searchM, randomM, rememberM;

	if ((fp = fopen(filename, "r")) == NULL)
		cant(filename);

	for (i = 0; fgets(buf, LINE_LEN, fp) != NULL; i++) {
		if (i >= MAX_AREA)
			error("Too many area.");
		if (sscanf(buf, "%d %s %lld %lg %lg %lg %lg %c %c %c",
			&areas[i].code,
			areas[i].name,
			&areas[i].capacity,
			&areas[i].MoveRate,
			&areas[i].GrowthMoveRate,	// ###
			&areas[i].GrowthMortalityRate,
			&areas[i].SafeLine,
			&searchM,
			&randomM,
			&rememberM
		) != 10)
			error("Area format error.");
		for (int j = 0; j < i - 1; j++) {
			if (areas[j].code == areas[i].code)
				error("Area code is duplicated");
			if (strcmp(areas[j].name, areas[i].name) == 0)
				error("Area name is duplicated");
		}

		if(searchM == 'p')
		  areas[i].masterSearch = 0;
		else
		  areas[i].masterSearch = 1;

		if(randomM == 'c')
		  areas[i].masterRandom = false;
		else
		  areas[i].masterRandom = true;

		if(rememberM == 'n')
		  areas[i].masterRemember = false;
		else
		  areas[i].masterRemember = true;

		areas[i].FoodCrisis = false;
		areas[i].initMoveRate = areas[i].MoveRate;

		areas[i].come_from_list.prev = &areas[i].come_from_list;
		areas[i].come_from_list.next = &areas[i].come_from_list;
		areas[i].come_from_list.dummy = true;
		areas[i].come_from_list.link = NULL;
		areas[i].come_from_list.count_in_year = 0;
		areas[i].come_from_list.relation = false;
		areas[i].go_to_list.prev = &areas[i].go_to_list;
		areas[i].go_to_list.next = &areas[i].go_to_list;
		areas[i].go_to_list.dummy = true;
		areas[i].go_to_list.link = NULL;
		areas[i].go_to_list.count_in_year = 0;
		areas[i].go_to_list.relation = false;
		areas[i].in_routes = 0;
		areas[i].out_routes = 0;
		areas[i].birth_count_in_year = 0LL;
		areas[i].death_count_in_year = 0LL;

		for (int j = 0; j < MAX_AGELAYER; j++) {
			areas[i].population[female][j] = 0LL;
			areas[i].population[male][j] = 0LL;
		}
		for (int j = 0; j < MAX_AGELAYER; j++) {
			areas[i].pop1[female][j] = 0LL;
			areas[i].pop1[male][j] = 0LL;
		}
		areas[i].area_population = 0LL;

		areas[i].dnorm_flag = false; // for sim7-6
	}

	fclose(fp);
	n_area = i;
	return i;
}

int read_link(const char *filename)
{

	// printf("@readLink!\n");
	FILE *fp;
	char buf[LINE_LEN + 1];
	int i;

	if ((fp = fopen(filename, "r")) == NULL)
		cant(filename);

	for (i = 0; fgets(buf, LINE_LEN, fp) != NULL; i++) {
		int m;

		if (i >= MAX_LINK)
			error("Too many link.");
		if ((m = sscanf(buf, "%d %d %lg", &links[i].from, &links[i].to, &links[i].prob)) != 3)
			error("Link format error.");
		if (links[i].from == links[i].to)
			error("Invalid link (same).");

		areax(links[i].from)->out_routes++;
		areax(links[i].to)->in_routes++;
		(void)add_route(&areax(links[i].from)->go_to_list, &links[i]);
		(void)add_route(&areax(links[i].to)->come_from_list, &links[i]);

	}

	/* ����Ĥ��ʤ��Ƥ⤤����Ȼפ���...����Ĥ����ɬ����������ư��ǽ	
	for (int j = 0; j < n_area; j++) {
		if (areas[j].in_routes != areas[j].out_routes)
			error("Invalid link (in/out mismatch).");
			// printf("%d,",areas[j].in_routes);
	}
	*/

	fclose(fp);
	n_link = i;
	return i;
}

int read_init0(const char *filename)
{
	FILE *fp;
	char buf[LINE_LEN + 1];
	int i;

	if ((fp = fopen(filename, "r")) == NULL)
		cant(filename);

	for (i = 0; fgets(buf, LINE_LEN, fp) != NULL; i++) {
		int code;
		AREA *a;

		if (i >= MAX_AREA)
			error("Too many init.");
		if (sscanf(buf, "%d", &code) != 1)
			error("Init format error1.");
		a = areax(code);
		if (sscanf(buf, "%d "
			"%lld %lld %lld %lld %lld %lld %lld %lld %lld %lld %lld %lld %lld "
			"%lld %lld %lld %lld %lld %lld %lld %lld %lld %lld %lld %lld %lld ",
			&code,
			&a->population[female][0],
			&a->population[female][1],
			&a->population[female][2],
			&a->population[female][3],
			&a->population[female][4],
			&a->population[female][5],
			&a->population[female][6],
			&a->population[female][7],
			&a->population[female][8],
			&a->population[female][9],
			&a->population[female][10],
			&a->population[female][11],
			&a->population[female][12],
			&a->population[male][0],
			&a->population[male][1],
			&a->population[male][2],
			&a->population[male][3],
			&a->population[male][4],
			&a->population[male][5],
			&a->population[male][6],
			&a->population[male][7],
			&a->population[male][8],
			&a->population[male][9],
			&a->population[male][10],
			&a->population[male][11],
			&a->population[male][12]
		) != 1 + MAX_AGELAYER * 2)
			error("Init format error2.");
	}
	fclose(fp);
	return i;
}

int read_init(const char *filename)
{
	FILE *fp;
	char buf[LINE_LEN + 1];
	int i;

	if ((fp = fopen(filename, "r")) == NULL)
		cant(filename);

	for (i = 0; fgets(buf, LINE_LEN, fp) != NULL; i++) {
		int code;
		AREA *a;

		if (i >= MAX_AREA)
			error("Too many init.");
		if (sscanf(buf, "%d", &code) != 1)
			error("Init format error3.");
		a = areax(code);
#if DNAversion
		if (sscanf(buf, "%d %d %lld %lg %lg",
			&code,
			&a->init_pop_type,
			&a->init_pop,
			&a->init_sex_ratio,
			&a->init_gvalue
		) != 5)
			error("Init format error4.");
		a->last_gvalue_means = a->init_gvalue;
#else
		if (sscanf(buf, "%d %d %lld %lg",
			&code,
			&a->init_pop_type,
			&a->init_pop,
			&a->init_sex_ratio
		) != 4)
			error("Init format error4.");
#endif
		if (a->init_pop_type < 1 || a->init_pop_type > 3 ||
			a->init_pop < 0 ||
			a->init_sex_ratio == 0.0)
			error("Init value error5.");
	}
	fclose(fp);
	return i;
}

int read_MortalityRate(const char *filename)
{
	FILE *fp;
	char buf[LINE_LEN + 1];
	int i;

	if ((fp = fopen(filename, "r")) == NULL)
		cant(filename);

	for (i = 0; fgets(buf, LINE_LEN, fp) != NULL; i++) {
		int code;
		AREA *a;


		if (i >= MAX_AREA)
			error("Too many init.");
		if (sscanf(buf, "%d", &code) != 1)
			error("Init format error6.");
		a = areax(code);
		if (sscanf(buf, "%d "
			"%lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg "
			"%lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg "
			"%lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg "
			"%lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg "
			"%lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg "
			"%lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg "
			"%lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg "
			"%lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg "
			"%lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg "
			"%lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg "
			"%lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg "
			"%lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg "
			"%lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg "
			"%lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg "
			"%lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg "
			"%lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg ",
			&code,
			&a->MortalityRate[female][0],
			&a->MortalityRate[female][1],
			&a->MortalityRate[female][2],
			&a->MortalityRate[female][3],
			&a->MortalityRate[female][4],
			&a->MortalityRate[female][5],
			&a->MortalityRate[female][6],
			&a->MortalityRate[female][7],
			&a->MortalityRate[female][8],
			&a->MortalityRate[female][9],
			&a->MortalityRate[female][10],
			&a->MortalityRate[female][11],
			&a->MortalityRate[female][12],
			&a->MortalityRate[female][13],
			&a->MortalityRate[female][14],
			&a->MortalityRate[female][15],
			&a->MortalityRate[female][16],
			&a->MortalityRate[female][17],
			&a->MortalityRate[female][18],
			&a->MortalityRate[female][19],
			&a->MortalityRate[female][20],
			&a->MortalityRate[female][21],
			&a->MortalityRate[female][22],
			&a->MortalityRate[female][23],
			&a->MortalityRate[female][24],
			&a->MortalityRate[female][25],
			&a->MortalityRate[female][26],
			&a->MortalityRate[female][27],
			&a->MortalityRate[female][28],
			&a->MortalityRate[female][29],
			&a->MortalityRate[female][30],
			&a->MortalityRate[female][31],
			&a->MortalityRate[female][32],
			&a->MortalityRate[female][33],
			&a->MortalityRate[female][34],
			&a->MortalityRate[female][35],
			&a->MortalityRate[female][36],
			&a->MortalityRate[female][37],
			&a->MortalityRate[female][38],
			&a->MortalityRate[female][39],
			&a->MortalityRate[female][40],
			&a->MortalityRate[female][41],
			&a->MortalityRate[female][42],
			&a->MortalityRate[female][43],
			&a->MortalityRate[female][44],
			&a->MortalityRate[female][45],
			&a->MortalityRate[female][46],
			&a->MortalityRate[female][47],
			&a->MortalityRate[female][48],
			&a->MortalityRate[female][49],
			&a->MortalityRate[female][50],
			&a->MortalityRate[female][51],
			&a->MortalityRate[female][52],
			&a->MortalityRate[female][53],
			&a->MortalityRate[female][54],
			&a->MortalityRate[female][55],
			&a->MortalityRate[female][56],
			&a->MortalityRate[female][57],
			&a->MortalityRate[female][58],
			&a->MortalityRate[female][59],
			&a->MortalityRate[female][60],
			&a->MortalityRate[female][61],
			&a->MortalityRate[female][62],
			&a->MortalityRate[female][63],
			&a->MortalityRate[female][64],
			&a->MortalityRate[female][65],
			&a->MortalityRate[female][66],
			&a->MortalityRate[female][67],
			&a->MortalityRate[female][68],
			&a->MortalityRate[female][69],
			&a->MortalityRate[female][70],
			&a->MortalityRate[female][71],
			&a->MortalityRate[female][72],
			&a->MortalityRate[female][73],
			&a->MortalityRate[female][74],
			&a->MortalityRate[female][75],
			&a->MortalityRate[female][76],
			&a->MortalityRate[female][77],
			&a->MortalityRate[female][78],
			&a->MortalityRate[female][79],
			&a->MortalityRate[female][80],
			&a->MortalityRate[female][81],
			&a->MortalityRate[female][82],
			&a->MortalityRate[female][83],
			&a->MortalityRate[female][84],
			&a->MortalityRate[female][85],
			&a->MortalityRate[female][86],
			&a->MortalityRate[female][87],
			&a->MortalityRate[female][88],
			&a->MortalityRate[female][89],
			&a->MortalityRate[female][90],
			&a->MortalityRate[female][91],
			&a->MortalityRate[female][92],
			&a->MortalityRate[female][93],
			&a->MortalityRate[female][94],
			&a->MortalityRate[female][95],
			&a->MortalityRate[female][96],
			&a->MortalityRate[female][97],
			&a->MortalityRate[female][98],
			&a->MortalityRate[female][99],
			&a->MortalityRate[female][100],
			&a->MortalityRate[female][101],
			&a->MortalityRate[female][102],
			&a->MortalityRate[female][103],
			&a->MortalityRate[female][104],
			&a->MortalityRate[female][105],
			&a->MortalityRate[female][106],
			&a->MortalityRate[female][107],
			&a->MortalityRate[female][108],
			&a->MortalityRate[female][109],
			&a->MortalityRate[female][110],
			&a->MortalityRate[female][111],
			&a->MortalityRate[female][112],
			&a->MortalityRate[female][113],
			&a->MortalityRate[female][114],
			&a->MortalityRate[female][115],
			&a->MortalityRate[female][116],
			&a->MortalityRate[female][117],
			&a->MortalityRate[female][118],
			&a->MortalityRate[female][119],
			&a->MortalityRate[female][120],
			&a->MortalityRate[female][121],
			&a->MortalityRate[female][122],
			&a->MortalityRate[female][123],
			&a->MortalityRate[female][124],
			&a->MortalityRate[female][125],
			&a->MortalityRate[female][126],
			&a->MortalityRate[female][127],
			&a->MortalityRate[male][0],
			&a->MortalityRate[male][1],
			&a->MortalityRate[male][2],
			&a->MortalityRate[male][3],
			&a->MortalityRate[male][4],
			&a->MortalityRate[male][5],
			&a->MortalityRate[male][6],
			&a->MortalityRate[male][7],
			&a->MortalityRate[male][8],
			&a->MortalityRate[male][9],
			&a->MortalityRate[male][10],
			&a->MortalityRate[male][11],
			&a->MortalityRate[male][12],
			&a->MortalityRate[male][13],
			&a->MortalityRate[male][14],
			&a->MortalityRate[male][15],
			&a->MortalityRate[male][16],
			&a->MortalityRate[male][17],
			&a->MortalityRate[male][18],
			&a->MortalityRate[male][19],
			&a->MortalityRate[male][20],
			&a->MortalityRate[male][21],
			&a->MortalityRate[male][22],
			&a->MortalityRate[male][23],
			&a->MortalityRate[male][24],
			&a->MortalityRate[male][25],
			&a->MortalityRate[male][26],
			&a->MortalityRate[male][27],
			&a->MortalityRate[male][28],
			&a->MortalityRate[male][29],
			&a->MortalityRate[male][30],
			&a->MortalityRate[male][31],
			&a->MortalityRate[male][32],
			&a->MortalityRate[male][33],
			&a->MortalityRate[male][34],
			&a->MortalityRate[male][35],
			&a->MortalityRate[male][36],
			&a->MortalityRate[male][37],
			&a->MortalityRate[male][38],
			&a->MortalityRate[male][39],
			&a->MortalityRate[male][40],
			&a->MortalityRate[male][41],
			&a->MortalityRate[male][42],
			&a->MortalityRate[male][43],
			&a->MortalityRate[male][44],
			&a->MortalityRate[male][45],
			&a->MortalityRate[male][46],
			&a->MortalityRate[male][47],
			&a->MortalityRate[male][48],
			&a->MortalityRate[male][49],
			&a->MortalityRate[male][50],
			&a->MortalityRate[male][51],
			&a->MortalityRate[male][52],
			&a->MortalityRate[male][53],
			&a->MortalityRate[male][54],
			&a->MortalityRate[male][55],
			&a->MortalityRate[male][56],
			&a->MortalityRate[male][57],
			&a->MortalityRate[male][58],
			&a->MortalityRate[male][59],
			&a->MortalityRate[male][60],
			&a->MortalityRate[male][61],
			&a->MortalityRate[male][62],
			&a->MortalityRate[male][63],
			&a->MortalityRate[male][64],
			&a->MortalityRate[male][65],
			&a->MortalityRate[male][66],
			&a->MortalityRate[male][67],
			&a->MortalityRate[male][68],
			&a->MortalityRate[male][69],
			&a->MortalityRate[male][70],
			&a->MortalityRate[male][71],
			&a->MortalityRate[male][72],
			&a->MortalityRate[male][73],
			&a->MortalityRate[male][74],
			&a->MortalityRate[male][75],
			&a->MortalityRate[male][76],
			&a->MortalityRate[male][77],
			&a->MortalityRate[male][78],
			&a->MortalityRate[male][79],
			&a->MortalityRate[male][80],
			&a->MortalityRate[male][81],
			&a->MortalityRate[male][82],
			&a->MortalityRate[male][83],
			&a->MortalityRate[male][84],
			&a->MortalityRate[male][85],
			&a->MortalityRate[male][86],
			&a->MortalityRate[male][87],
			&a->MortalityRate[male][88],
			&a->MortalityRate[male][89],
			&a->MortalityRate[male][90],
			&a->MortalityRate[male][91],
			&a->MortalityRate[male][92],
			&a->MortalityRate[male][93],
			&a->MortalityRate[male][94],
			&a->MortalityRate[male][95],
			&a->MortalityRate[male][96],
			&a->MortalityRate[male][97],
			&a->MortalityRate[male][98],
			&a->MortalityRate[male][99],
			&a->MortalityRate[male][100],
			&a->MortalityRate[male][101],
			&a->MortalityRate[male][102],
			&a->MortalityRate[male][103],
			&a->MortalityRate[male][104],
			&a->MortalityRate[male][105],
			&a->MortalityRate[male][106],
			&a->MortalityRate[male][107],
			&a->MortalityRate[male][108],
			&a->MortalityRate[male][109],
			&a->MortalityRate[male][110],
			&a->MortalityRate[male][111],
			&a->MortalityRate[male][112],
			&a->MortalityRate[male][113],
			&a->MortalityRate[male][114],
			&a->MortalityRate[male][115],
			&a->MortalityRate[male][116],
			&a->MortalityRate[male][117],
			&a->MortalityRate[male][118],
			&a->MortalityRate[male][119],
			&a->MortalityRate[male][120],
			&a->MortalityRate[male][121],
			&a->MortalityRate[male][122],
			&a->MortalityRate[male][123],
			&a->MortalityRate[male][124],
			&a->MortalityRate[male][125],
			&a->MortalityRate[male][126],
			&a->MortalityRate[male][127]
		) != 1 + (MAX_AGE + 1) * 2)
			error("Mortality format error.");
	}
	fclose(fp);

	for (int i = 0; i < n_area; i++)
		for (int j = 0; j < 2; j++) {
			memcpy( areas[i].initMortalityRate[j], areas[i].MortalityRate[j], sizeof(double) * (MAX_AGE + 1));
	}
	// printf("%lf\n", areas[0].initMortalityRate[0][0]);

	return i;
}

int read_BirthRate(const char *filename)
{
	FILE *fp;
	char buf[LINE_LEN + 1];
	int i;

	if ((fp = fopen(filename, "r")) == NULL)
		cant(filename);

	for (i = 0; fgets(buf, LINE_LEN, fp) != NULL; i++) {
		int code;
		AREA *a;

		if (i >= MAX_AREA)
			error("Too many init.");
		if (sscanf(buf, "%d", &code) != 1)
			error("Init format error7.");
		a = areax(code);
		if (sscanf(buf, "%d "
			"%lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg "
			"%lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg "
			"%lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg "
			"%lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg "
			"%lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg "
			"%lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg "
			"%lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg "
			"%lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg "
			"%lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg "
			"%lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg "
			"%lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg "
			"%lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg "
			"%lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg "
			"%lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg "
			"%lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg "
			"%lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg ",
			&code,
			&a->BirthRate[female][0],
			&a->BirthRate[female][1],
			&a->BirthRate[female][2],
			&a->BirthRate[female][3],
			&a->BirthRate[female][4],
			&a->BirthRate[female][5],
			&a->BirthRate[female][6],
			&a->BirthRate[female][7],
			&a->BirthRate[female][8],
			&a->BirthRate[female][9],
			&a->BirthRate[female][10],
			&a->BirthRate[female][11],
			&a->BirthRate[female][12],
			&a->BirthRate[female][13],
			&a->BirthRate[female][14],
			&a->BirthRate[female][15],
			&a->BirthRate[female][16],
			&a->BirthRate[female][17],
			&a->BirthRate[female][18],
			&a->BirthRate[female][19],
			&a->BirthRate[female][20],
			&a->BirthRate[female][21],
			&a->BirthRate[female][22],
			&a->BirthRate[female][23],
			&a->BirthRate[female][24],
			&a->BirthRate[female][25],
			&a->BirthRate[female][26],
			&a->BirthRate[female][27],
			&a->BirthRate[female][28],
			&a->BirthRate[female][29],
			&a->BirthRate[female][30],
			&a->BirthRate[female][31],
			&a->BirthRate[female][32],
			&a->BirthRate[female][33],
			&a->BirthRate[female][34],
			&a->BirthRate[female][35],
			&a->BirthRate[female][36],
			&a->BirthRate[female][37],
			&a->BirthRate[female][38],
			&a->BirthRate[female][39],
			&a->BirthRate[female][40],
			&a->BirthRate[female][41],
			&a->BirthRate[female][42],
			&a->BirthRate[female][43],
			&a->BirthRate[female][44],
			&a->BirthRate[female][45],
			&a->BirthRate[female][46],
			&a->BirthRate[female][47],
			&a->BirthRate[female][48],
			&a->BirthRate[female][49],
			&a->BirthRate[female][50],
			&a->BirthRate[female][51],
			&a->BirthRate[female][52],
			&a->BirthRate[female][53],
			&a->BirthRate[female][54],
			&a->BirthRate[female][55],
			&a->BirthRate[female][56],
			&a->BirthRate[female][57],
			&a->BirthRate[female][58],
			&a->BirthRate[female][59],
			&a->BirthRate[female][60],
			&a->BirthRate[female][61],
			&a->BirthRate[female][62],
			&a->BirthRate[female][63],
			&a->BirthRate[female][64],
			&a->BirthRate[female][65],
			&a->BirthRate[female][66],
			&a->BirthRate[female][67],
			&a->BirthRate[female][68],
			&a->BirthRate[female][69],
			&a->BirthRate[female][70],
			&a->BirthRate[female][71],
			&a->BirthRate[female][72],
			&a->BirthRate[female][73],
			&a->BirthRate[female][74],
			&a->BirthRate[female][75],
			&a->BirthRate[female][76],
			&a->BirthRate[female][77],
			&a->BirthRate[female][78],
			&a->BirthRate[female][79],
			&a->BirthRate[female][80],
			&a->BirthRate[female][81],
			&a->BirthRate[female][82],
			&a->BirthRate[female][83],
			&a->BirthRate[female][84],
			&a->BirthRate[female][85],
			&a->BirthRate[female][86],
			&a->BirthRate[female][87],
			&a->BirthRate[female][88],
			&a->BirthRate[female][89],
			&a->BirthRate[female][90],
			&a->BirthRate[female][91],
			&a->BirthRate[female][92],
			&a->BirthRate[female][93],
			&a->BirthRate[female][94],
			&a->BirthRate[female][95],
			&a->BirthRate[female][96],
			&a->BirthRate[female][97],
			&a->BirthRate[female][98],
			&a->BirthRate[female][99],
			&a->BirthRate[female][100],
			&a->BirthRate[female][101],
			&a->BirthRate[female][102],
			&a->BirthRate[female][103],
			&a->BirthRate[female][104],
			&a->BirthRate[female][105],
			&a->BirthRate[female][106],
			&a->BirthRate[female][107],
			&a->BirthRate[female][108],
			&a->BirthRate[female][109],
			&a->BirthRate[female][110],
			&a->BirthRate[female][111],
			&a->BirthRate[female][112],
			&a->BirthRate[female][113],
			&a->BirthRate[female][114],
			&a->BirthRate[female][115],
			&a->BirthRate[female][116],
			&a->BirthRate[female][117],
			&a->BirthRate[female][118],
			&a->BirthRate[female][119],
			&a->BirthRate[female][120],
			&a->BirthRate[female][121],
			&a->BirthRate[female][122],
			&a->BirthRate[female][123],
			&a->BirthRate[female][124],
			&a->BirthRate[female][125],
			&a->BirthRate[female][126],
			&a->BirthRate[female][127],
			&a->BirthRate[male][0],
			&a->BirthRate[male][1],
			&a->BirthRate[male][2],
			&a->BirthRate[male][3],
			&a->BirthRate[male][4],
			&a->BirthRate[male][5],
			&a->BirthRate[male][6],
			&a->BirthRate[male][7],
			&a->BirthRate[male][8],
			&a->BirthRate[male][9],
			&a->BirthRate[male][10],
			&a->BirthRate[male][11],
			&a->BirthRate[male][12],
			&a->BirthRate[male][13],
			&a->BirthRate[male][14],
			&a->BirthRate[male][15],
			&a->BirthRate[male][16],
			&a->BirthRate[male][17],
			&a->BirthRate[male][18],
			&a->BirthRate[male][19],
			&a->BirthRate[male][20],
			&a->BirthRate[male][21],
			&a->BirthRate[male][22],
			&a->BirthRate[male][23],
			&a->BirthRate[male][24],
			&a->BirthRate[male][25],
			&a->BirthRate[male][26],
			&a->BirthRate[male][27],
			&a->BirthRate[male][28],
			&a->BirthRate[male][29],
			&a->BirthRate[male][30],
			&a->BirthRate[male][31],
			&a->BirthRate[male][32],
			&a->BirthRate[male][33],
			&a->BirthRate[male][34],
			&a->BirthRate[male][35],
			&a->BirthRate[male][36],
			&a->BirthRate[male][37],
			&a->BirthRate[male][38],
			&a->BirthRate[male][39],
			&a->BirthRate[male][40],
			&a->BirthRate[male][41],
			&a->BirthRate[male][42],
			&a->BirthRate[male][43],
			&a->BirthRate[male][44],
			&a->BirthRate[male][45],
			&a->BirthRate[male][46],
			&a->BirthRate[male][47],
			&a->BirthRate[male][48],
			&a->BirthRate[male][49],
			&a->BirthRate[male][50],
			&a->BirthRate[male][51],
			&a->BirthRate[male][52],
			&a->BirthRate[male][53],
			&a->BirthRate[male][54],
			&a->BirthRate[male][55],
			&a->BirthRate[male][56],
			&a->BirthRate[male][57],
			&a->BirthRate[male][58],
			&a->BirthRate[male][59],
			&a->BirthRate[male][60],
			&a->BirthRate[male][61],
			&a->BirthRate[male][62],
			&a->BirthRate[male][63],
			&a->BirthRate[male][64],
			&a->BirthRate[male][65],
			&a->BirthRate[male][66],
			&a->BirthRate[male][67],
			&a->BirthRate[male][68],
			&a->BirthRate[male][69],
			&a->BirthRate[male][70],
			&a->BirthRate[male][71],
			&a->BirthRate[male][72],
			&a->BirthRate[male][73],
			&a->BirthRate[male][74],
			&a->BirthRate[male][75],
			&a->BirthRate[male][76],
			&a->BirthRate[male][77],
			&a->BirthRate[male][78],
			&a->BirthRate[male][79],
			&a->BirthRate[male][80],
			&a->BirthRate[male][81],
			&a->BirthRate[male][82],
			&a->BirthRate[male][83],
			&a->BirthRate[male][84],
			&a->BirthRate[male][85],
			&a->BirthRate[male][86],
			&a->BirthRate[male][87],
			&a->BirthRate[male][88],
			&a->BirthRate[male][89],
			&a->BirthRate[male][90],
			&a->BirthRate[male][91],
			&a->BirthRate[male][92],
			&a->BirthRate[male][93],
			&a->BirthRate[male][94],
			&a->BirthRate[male][95],
			&a->BirthRate[male][96],
			&a->BirthRate[male][97],
			&a->BirthRate[male][98],
			&a->BirthRate[male][99],
			&a->BirthRate[male][100],
			&a->BirthRate[male][101],
			&a->BirthRate[male][102],
			&a->BirthRate[male][103],
			&a->BirthRate[male][104],
			&a->BirthRate[male][105],
			&a->BirthRate[male][106],
			&a->BirthRate[male][107],
			&a->BirthRate[male][108],
			&a->BirthRate[male][109],
			&a->BirthRate[male][110],
			&a->BirthRate[male][111],
			&a->BirthRate[male][112],
			&a->BirthRate[male][113],
			&a->BirthRate[male][114],
			&a->BirthRate[male][115],
			&a->BirthRate[male][116],
			&a->BirthRate[male][117],
			&a->BirthRate[male][118],
			&a->BirthRate[male][119],
			&a->BirthRate[male][120],
			&a->BirthRate[male][121],
			&a->BirthRate[male][122],
			&a->BirthRate[male][123],
			&a->BirthRate[male][124],
			&a->BirthRate[male][125],
			&a->BirthRate[male][126],
			&a->BirthRate[male][127]
		) != 1 + (MAX_AGE + 1) * 2)
			error("BirthRate format error.");
	}
	fclose(fp);
	return i;
}

int read_Skill(const char *filename)
{
	FILE *fp;
	char buf[LINE_LEN + 1];
	int i;

	if ((fp = fopen(filename, "r")) == NULL)
		cant(filename);

	for (i = 0; fgets(buf, LINE_LEN, fp) != NULL; i++) {
		int code;
		AREA *a;
		char c;

		if (i >= MAX_AREA)
			error("Too many init.");
		if (sscanf(buf, "%d", &code) != 1)
			error("Init format error7.");
		a = areax(code);

		if(sscanf(buf, "%d %c", &code, &c) == 2){ // ����ʬ�ۻ���
		  if(c == 'n'){
		    a->dnorm_flag = true;
		    for(int ii=0; ii<127; ii++){
		      a->Skill[female][ii] = 0.0;
		      a->Skill[male][ii] = 0.0;
		    }
		  }
		
		  if (sscanf(buf, "%d "
			"%lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg "
			"%lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg "
			"%lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg "
			"%lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg "
			"%lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg "
			"%lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg "
			"%lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg "
			"%lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg "
			"%lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg "
			"%lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg "
			"%lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg "
			"%lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg "
			"%lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg "
			"%lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg "
			"%lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg "
			"%lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg ",
			&code,
			&a->Skill[female][0],
			&a->Skill[female][1],
			&a->Skill[female][2],
			&a->Skill[female][3],
			&a->Skill[female][4],
			&a->Skill[female][5],
			&a->Skill[female][6],
			&a->Skill[female][7],
			&a->Skill[female][8],
			&a->Skill[female][9],
			&a->Skill[female][10],
			&a->Skill[female][11],
			&a->Skill[female][12],
			&a->Skill[female][13],
			&a->Skill[female][14],
			&a->Skill[female][15],
			&a->Skill[female][16],
			&a->Skill[female][17],
			&a->Skill[female][18],
			&a->Skill[female][19],
			&a->Skill[female][20],
			&a->Skill[female][21],
			&a->Skill[female][22],
			&a->Skill[female][23],
			&a->Skill[female][24],
			&a->Skill[female][25],
			&a->Skill[female][26],
			&a->Skill[female][27],
			&a->Skill[female][28],
			&a->Skill[female][29],
			&a->Skill[female][30],
			&a->Skill[female][31],
			&a->Skill[female][32],
			&a->Skill[female][33],
			&a->Skill[female][34],
			&a->Skill[female][35],
			&a->Skill[female][36],
			&a->Skill[female][37],
			&a->Skill[female][38],
			&a->Skill[female][39],
			&a->Skill[female][40],
			&a->Skill[female][41],
			&a->Skill[female][42],
			&a->Skill[female][43],
			&a->Skill[female][44],
			&a->Skill[female][45],
			&a->Skill[female][46],
			&a->Skill[female][47],
			&a->Skill[female][48],
			&a->Skill[female][49],
			&a->Skill[female][50],
			&a->Skill[female][51],
			&a->Skill[female][52],
			&a->Skill[female][53],
			&a->Skill[female][54],
			&a->Skill[female][55],
			&a->Skill[female][56],
			&a->Skill[female][57],
			&a->Skill[female][58],
			&a->Skill[female][59],
			&a->Skill[female][60],
			&a->Skill[female][61],
			&a->Skill[female][62],
			&a->Skill[female][63],
			&a->Skill[female][64],
			&a->Skill[female][65],
			&a->Skill[female][66],
			&a->Skill[female][67],
			&a->Skill[female][68],
			&a->Skill[female][69],
			&a->Skill[female][70],
			&a->Skill[female][71],
			&a->Skill[female][72],
			&a->Skill[female][73],
			&a->Skill[female][74],
			&a->Skill[female][75],
			&a->Skill[female][76],
			&a->Skill[female][77],
			&a->Skill[female][78],
			&a->Skill[female][79],
			&a->Skill[female][80],
			&a->Skill[female][81],
			&a->Skill[female][82],
			&a->Skill[female][83],
			&a->Skill[female][84],
			&a->Skill[female][85],
			&a->Skill[female][86],
			&a->Skill[female][87],
			&a->Skill[female][88],
			&a->Skill[female][89],
			&a->Skill[female][90],
			&a->Skill[female][91],
			&a->Skill[female][92],
			&a->Skill[female][93],
			&a->Skill[female][94],
			&a->Skill[female][95],
			&a->Skill[female][96],
			&a->Skill[female][97],
			&a->Skill[female][98],
			&a->Skill[female][99],
			&a->Skill[female][100],
			&a->Skill[female][101],
			&a->Skill[female][102],
			&a->Skill[female][103],
			&a->Skill[female][104],
			&a->Skill[female][105],
			&a->Skill[female][106],
			&a->Skill[female][107],
			&a->Skill[female][108],
			&a->Skill[female][109],
			&a->Skill[female][110],
			&a->Skill[female][111],
			&a->Skill[female][112],
			&a->Skill[female][113],
			&a->Skill[female][114],
			&a->Skill[female][115],
			&a->Skill[female][116],
			&a->Skill[female][117],
			&a->Skill[female][118],
			&a->Skill[female][119],
			&a->Skill[female][120],
			&a->Skill[female][121],
			&a->Skill[female][122],
			&a->Skill[female][123],
			&a->Skill[female][124],
			&a->Skill[female][125],
			&a->Skill[female][126],
			&a->Skill[female][127],
			&a->Skill[male][0],
			&a->Skill[male][1],
			&a->Skill[male][2],
			&a->Skill[male][3],
			&a->Skill[male][4],
			&a->Skill[male][5],
			&a->Skill[male][6],
			&a->Skill[male][7],
			&a->Skill[male][8],
			&a->Skill[male][9],
			&a->Skill[male][10],
			&a->Skill[male][11],
			&a->Skill[male][12],
			&a->Skill[male][13],
			&a->Skill[male][14],
			&a->Skill[male][15],
			&a->Skill[male][16],
			&a->Skill[male][17],
			&a->Skill[male][18],
			&a->Skill[male][19],
			&a->Skill[male][20],
			&a->Skill[male][21],
			&a->Skill[male][22],
			&a->Skill[male][23],
			&a->Skill[male][24],
			&a->Skill[male][25],
			&a->Skill[male][26],
			&a->Skill[male][27],
			&a->Skill[male][28],
			&a->Skill[male][29],
			&a->Skill[male][30],
			&a->Skill[male][31],
			&a->Skill[male][32],
			&a->Skill[male][33],
			&a->Skill[male][34],
			&a->Skill[male][35],
			&a->Skill[male][36],
			&a->Skill[male][37],
			&a->Skill[male][38],
			&a->Skill[male][39],
			&a->Skill[male][40],
			&a->Skill[male][41],
			&a->Skill[male][42],
			&a->Skill[male][43],
			&a->Skill[male][44],
			&a->Skill[male][45],
			&a->Skill[male][46],
			&a->Skill[male][47],
			&a->Skill[male][48],
			&a->Skill[male][49],
			&a->Skill[male][50],
			&a->Skill[male][51],
			&a->Skill[male][52],
			&a->Skill[male][53],
			&a->Skill[male][54],
			&a->Skill[male][55],
			&a->Skill[male][56],
			&a->Skill[male][57],
			&a->Skill[male][58],
			&a->Skill[male][59],
			&a->Skill[male][60],
			&a->Skill[male][61],
			&a->Skill[male][62],
			&a->Skill[male][63],
			&a->Skill[male][64],
			&a->Skill[male][65],
			&a->Skill[male][66],
			&a->Skill[male][67],
			&a->Skill[male][68],
			&a->Skill[male][69],
			&a->Skill[male][70],
			&a->Skill[male][71],
			&a->Skill[male][72],
			&a->Skill[male][73],
			&a->Skill[male][74],
			&a->Skill[male][75],
			&a->Skill[male][76],
			&a->Skill[male][77],
			&a->Skill[male][78],
			&a->Skill[male][79],
			&a->Skill[male][80],
			&a->Skill[male][81],
			&a->Skill[male][82],
			&a->Skill[male][83],
			&a->Skill[male][84],
			&a->Skill[male][85],
			&a->Skill[male][86],
			&a->Skill[male][87],
			&a->Skill[male][88],
			&a->Skill[male][89],
			&a->Skill[male][90],
			&a->Skill[male][91],
			&a->Skill[male][92],
			&a->Skill[male][93],
			&a->Skill[male][94],
			&a->Skill[male][95],
			&a->Skill[male][96],
			&a->Skill[male][97],
			&a->Skill[male][98],
			&a->Skill[male][99],
			&a->Skill[male][100],
			&a->Skill[male][101],
			&a->Skill[male][102],
			&a->Skill[male][103],
			&a->Skill[male][104],
			&a->Skill[male][105],
			&a->Skill[male][106],
			&a->Skill[male][107],
			&a->Skill[male][108],
			&a->Skill[male][109],
			&a->Skill[male][110],
			&a->Skill[male][111],
			&a->Skill[male][112],
			&a->Skill[male][113],
			&a->Skill[male][114],
			&a->Skill[male][115],
			&a->Skill[male][116],
			&a->Skill[male][117],
			&a->Skill[male][118],
			&a->Skill[male][119],
			&a->Skill[male][120],
			&a->Skill[male][121],
			&a->Skill[male][122],
			&a->Skill[male][123],
			&a->Skill[male][124],
			&a->Skill[male][125],
			&a->Skill[male][126],
			&a->Skill[male][127]
		) != 1 + (MAX_AGE + 1) * 2)
		    if(!a->dnorm_flag)
			error("Skill format error.");
		}
	}
	fclose(fp);
	return i;
}


void initial0(int n, AREA *a)
{
	for (int i = 0; i < n; i++) {
		a[i].resident.prev = &a[i].resident;
		a[i].resident.next = &a[i].resident;
		a[i].resident.dummy = true;
		a[i].resident.person = NULL;
		for (int j = 0; j < MAX_AGELAYER; j++) {
			PERSON *p;

			for (int s = 0; s < 2; s++) {
				for (int k = 0; k < a[i].population[s][j]; k++) {
				  p = create_person(a[i].code, s, j * TERM_IN_YEAR * YEAR_IN_LAYER + roulette(TERM_IN_YEAR * YEAR_IN_LAYER),0);
					(void)create_resident(a[i].code, p);
					add_history(p, 0, 0, create, NULL, 0LL);
				}
			}
		}
	}
}

void initial(int n, FLAGS opts)
{
  int type;
  // sim7-6
  double top15, bottom15;
  // sim7-6

  type = opts.dist_type;

	for (int i = 0; i < n; i++) {
		areas[i].resident.prev = &areas[i].resident;
		areas[i].resident.next = &areas[i].resident;
		areas[i].resident.dummy = true;
		areas[i].resident.person = NULL;
		top15 = -1.0; bottom15 = -1.0;
		for (int j = 0; j < areas[i].init_pop; j++) {
			PERSON *p;
			int month;

			switch (type) {
			case 1:
				month = roulette1(INIT_MAX_AGE * TERM_IN_YEAR);
				break;
			case 2:
				month = roulette2(INIT_MAX_AGE * TERM_IN_YEAR);
				break;
			default:
				month = roulette1(INIT_MAX_AGE * TERM_IN_YEAR);
				break;
			}

			p = create_person(areas[i].code, (probability(1 / (areas[i].init_sex_ratio + 1.0)) ? female : male), month,0);
			p->mater = NULL;
			p->pater = NULL;
#if DNAversion
			p->gvalue = areas[i].init_gvalue;
#endif
			/*
			// sim7-6
			if((opts.skill_e) && (p->month >= TERM_IN_YEAR * 15)){ //15�аʾ���ä���
			  if(areas[i].dnorm_flag){
			    p->skill[0] = rand_normal(dn_mean, dn_sigma);
			  }
			*/

			// sim8-2 ���ѹ���create�οͤϺǽ餫������ skill����
			if(opts.skill_e){ //0�аʾ��
			  if(areas[i].dnorm_flag){
			    p->skill[0] = rand_normal(dn_mean, dn_sigma);
			  }


			  // �ǹ⡤���� skill
			  if(top15 < 0){
			    top15 = p->skill[0];
			    bottom15 = p->skill[0];
			  }else{
			    if(p->skill[0] > top15){
			      top15 = p->skill[0];
			    }
			    if(p->skill[0] < bottom15){
			      bottom15 = p->skill[0];
			    }
			  }

			}
			// sim7-6 end

			(void)create_resident(areas[i].code, p);
			inc_pop(&areas[i], p->sex, p->month);
			// areas[i].birth_count_in_year++;
			add_history(p, 0, 0, create, NULL, 0LL);
		}

		// sim7-6
		printf("Initial Skilltop area %s = %4.3f\n", areas[i].name, top15);
		printf("Initial Skillbottom area %s = %4.3f\n", areas[i].name, bottom15);
		// sim7-6 end
	}
}


static ROUTE *add_route(ROUTE *rt, LINK *l)
{
	ROUTE *res;

	if ((res = MALLOC(sizeof(ROUTE))) == NULL)
		error("Not enough memory. (ROUTE)");

	res->dummy = false;
	res->link = l;
	res->count_in_year = 0;
	res->relation = false;

	res->prev = rt->prev;
	res->next = rt;
	rt->prev->next = res;
	rt->prev = res;

	return res;
}

