/*
 * person.c - Simulation of population moving
 *		$Id: person.c,v 1.4 2013/02/24 17:12:26 void Exp $
 * vi: ts=4 sw=4 sts=4
 * sim7-5
 */
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
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
#include "rand.h"

extern FILE *fp;
extern bool SIM;
extern char file[50];
extern char file_info[50];
PERSON live_list = {&live_list, &live_list, true};
static PERSON dead_list = {&dead_list, &dead_list, true};
static PERSON Dead_list = {&Dead_list, &Dead_list, true};

static LIBERI *purge_child(LIBERI *);
static void purge_child_list(PERSON *);

static long long int person_id = 0LL;

static int first_degree = 1;
static int second_degree = 2;
static int third_degree = 3;

PERSON *create_person(int area_code, SEX sex, int month, int birthday_year)
{
	PERSON *p;

	if ((p = MALLOC(sizeof(PERSON))) == NULL)
		error("Not enough memory. (PERSON)");

	p->dummy = false;
	p->dead = false;
	p->no_relatives = false;
	p->id = ++person_id;
	p->sex = sex;
	p->month = month;	// 年齢 * 12
	p->birth_month = month;
	p->birthday_year = birthday_year;
	p->birth_area = area_code;
	p->area = area_code;
	p->move = 0;
	p->moveWith = 0;
	p->married = false;
	p->mater = NULL;
	p->pater = NULL;
	p->spouse = NULL;
	p->liberi.prev = &p->liberi;
	p->liberi.next = &p->liberi;
	p->liberi.dummy = true;
	p->liberi.p = NULL;
	p->sponsae.prev = &p->sponsae;
	p->sponsae.next = &p->sponsae;
	p->sponsae.dummy = true;
	p->sponsae.p = NULL;
	p->sponsae.month = 0;
	p->pregnant_month = 0;
	p->pregnant_spouse = NULL;
	p->relpointer.prev = &p->relpointer;
	p->relpointer.next = &p->relpointer;
	p->relpointer.dummy = true;
	p->relpointer.person = NULL;
	p->relpointer.degree = -1;
	p->relpointer.dead = true;
	p->relpointer.byAffinity = false;
	p->relpointer.byAffinity2 = false;
	p->relpointer.status = noneStat;
	p->history.prev = &p->history;
	p->history.next = &p->history;
	p->history.dummy = true;
	p->history.event = none;
	p->history.month = 0;
	p->history.para.none = NULL;
	p->history.id = 0LL;

#if DNAversion
	p->gvalue = 0.0; // default 0
#endif
	p->skill[0] = areax(area_code)->Skill[p->sex][month / 12];
	p->masterSearch = areax(area_code)->masterSearch;
	p->masterRandom = areax(area_code)->masterRandom;
	p->masterRemember = areax(area_code)->masterRemember;
	p->prevMaster = -1;

	p->spouse_max = 1; //最大配偶者数の初期値
	p->spouse_curno = 0; //今の配偶者の数
        p->spouse_maxreal = 0; //実際生涯で一番配偶者が多かったときの人数

	p->prev = live_list.prev;
	p->next = &live_list;
	live_list.prev->next = p;
	live_list.prev = p;

	return p;
}

long long int personID()
{
	return person_id;
}

void bury_person(PERSON *p)
{
	p->next->prev = p->prev;
	p->prev->next = p->next;

	p->prev = dead_list.prev;
	p->next = &dead_list;
	dead_list.prev->next = p;
	dead_list.prev = p;

}

PERSON *purge_person(PERSON *p)
{
	PERSON *n;
	n = p->next;

	// if (p->spouse != NULL)
	//	p->spouse->spouse = NULL;

	purge_child_list(p);
	purge_relpointer_list(p);
	purge_history_list(p);

	p->next->prev = p->prev;
	p->prev->next = p->next;
	FREE(p);
	return n;
}

void purge_all_dead_person(void)
{
	for (PERSON *p = dead_list.next; !p->dummy; p = purge_person(p))
		;
}

void purge_all_live_person(void)
{
	for (PERSON *p = live_list.next; !p->dummy; p = purge_person(p))
		;
}

void purge_alone_dead_person(bool flag)
{
	PERSON *np;
	long long int dcount;
	long long int pcount;

	dcount = 0LL;
	pcount = 0LL;


	for (PERSON *p = dead_list.next; !p->dummy; p = np) {
		np = p->next;
		if (isAlone(p)) {
			// if (p->id == 358)
			//	printf("!!");
			purge_relpointer(p);
			np = purge_person(p);
			++pcount;
		}
		++dcount;
	}
	if (flag)
		printf("(purge %lld / %lld)\n", pcount, dcount);

}

void print_marriage_childinfo(PERSON *p)	// 結婚情報・こども情報を表示 (-x2)
{
	printf("[Marriage_Childinfo] %lld ", p->id);
	printf("%s %s %d %d %s ",
		area_name(p->birth_area),	// 出身地
	       area_name(p->area),             // 死亡地域
	       p->birthday_year,                    // 生まれた年
	       p->month,                      // 死亡月齢
		sex(p)
	);
	if(p->married){ //未婚か否か
	  printf("hooked ");
	}else{
	  printf("unmarried ");
	}

	printf("%d ", first_marriage(p)); // 初婚月齢
	printf("%d ", count_child(p)); // 生んだこどもの数
	printf("%d\n", p->spouse_maxreal); // 生涯で一番配偶者が多かったときの人数

}

void print_deadperson(PERSON *p)	// 死亡した人の情報を表示 (-t)
{
	printf("[Person] %lld %p/%p ", p->id, p, p->next);
	printf("%s %s %d %s\n",
		area_name(p->birth_area),	// 出身地
		area_name(p->area),
		p->month,
		sex(p)
	);

	print_child(p);
	print_spouse(p);
	print_relpointer(p);
	print_dhistory(p);
}

void print_child(PERSON *p)
{
	if (!p->liberi.next->dummy)
		printf("\t\t<children>\n");
	for (LIBERI *l = p->liberi.next; !l->dummy; l = l->next) {
		printf("\t\t%lld %s (%d)\n", l->p->id, sex(l->p), AGE(l->p->month));
	}
}

int count_child(PERSON *p)
{	
  int childno = 0;
  for (LIBERI *l = p->liberi.next; !l->dummy; l = l->next) {
    childno++;
  }
  return childno;
}

LIBERI *add_child(PERSON *p, PERSON *c)
{
	LIBERI *l;

	if ((l = MALLOC(sizeof(LIBERI))) == NULL)
		error("Not enough memory. (LIBERI)");

	l->dummy = false;
	l->p = c;

	l->prev = p->liberi.prev;
	l->next = &p->liberi;
	p->liberi.prev->next = l;
	p->liberi.prev = l;

	return l;
}

void print_spouse(PERSON *p)
{
	if (!p->sponsae.next->dummy)
		printf("\t\t<spouse>\n");
	for (SPONSAE *s = p->sponsae.next; !s->dummy; s = s->next) {
		printf("\t\t%lld %s (%d)\n", s->p->id, sex(s->p), AGE(s->p->month));
	}
}

SPONSAE *add_spouse(PERSON *p, PERSON *q, int month)
{
	SPONSAE *s;

	if ((s = MALLOC(sizeof(SPONSAE))) == NULL)
		error("Not enough memory. (SPONSAE)");

	s->dummy = false;
	s->p = q;
	s->month = month;

	s->prev = p->sponsae.prev;
	s->next = &p->sponsae;
	p->sponsae.prev->next = s;
	p->sponsae.prev = s;

	return s;
}

const char *sex(const PERSON *p)
{
	return p->sex == 0 ? "female" : "male";
}

static void purge_child_list(PERSON *p)
{
	for (LIBERI *l = p->liberi.next; !l->dummy; l = purge_child(l))
		;
}

static LIBERI *purge_child(LIBERI *l)
{
	LIBERI *n;

	n = l->next;
	l->next->prev = l->prev;
	l->prev->next = l->next;
	FREE(l);
	return n;
}

void people(int DEGREE)
{
	for (PERSON *p = live_list.next; !p->dummy; p =p->next) {
		print_ahistory(p);
		if (DEGREE != 0)
			person(p, DEGREE);
		fflush(stdout);
	}
}

void people_marriage_childinfo() // 生きている人用の -x2オプション．でもいまは使ってない．
{
  printf("Marriga_childinfo of alive persons\n");
  for (PERSON *p = live_list.next; !p->dummy; p =p->next) {
    if(p->sex == female){ 
      print_marriage_childinfo(p);
    }
  }
}

void person(PERSON *p,int DEGREE)
{
	int top_p = 1,top_s = 2;
	int down_p = 3,down_s = 4,down_c = 5;

	int deg_td;
	int relatives;
	int marriage;

	// id,name,degree,degree_td(1~6),relatives(0/spouse.id),marrage;

	fprintf(fp,"%lld,%s,%d,%d,%d,%d,",p->id,"person",0,0,0,0);
	// printf("%lld(%d):",p->id,p->month);
	//	fflush(stdout);

	RELPOINTER *prev = NULL;
	for (RELPOINTER *r = p->relpointer.next; !r->dummy; r = r->next) {
		char name[30] = "unkown";
		deg_td = -1;
		relatives = -1;
		marriage = -1;
		PERSON *spouse = NULL;
		bool Fp = false, Fs = false;

		if (isRelative2(p, r->person, DEGREE))
			Fp = true;
		for (SPONSAE *s = p->sponsae.next; !s->dummy && !Fp; s = s->next) {
			if (isRelative2(s->p, r->person, DEGREE)) {
				spouse = s->p;
				Fs = true;
				break;
			}
		}

		if (r->degree <= DEGREE) {
			if (r->degree == 0) {
				for (SPONSAE *s = p->sponsae.next; !s->dummy; s = s->next) {
					if (s->p == r->person)
						fprintf(fp, "%lld,%s,%d,%d,%lld,%d,", r->person->id, "spouse", r->degree, 0, r->person->id, 0);
				}
			}

			else if (r->degree == first_degree) {
				bool parents = false;
				bool childlen = false;

				// 父母
				if (Fp && p->mater != NULL) {
					if (p->mater == r->person || p->pater == r->person) {
						strcpy(name, (r->person->sex == 1) ? "pater" : "mater");	// @@@1
						deg_td = top_p;
						relatives = 0;
						marriage = 0;
						parents = true;
					}
					// 母の再婚相手
					for (SPONSAE *s = p->mater->sponsae.next; !s->dummy && !parents; s = s->next) {
						if (s->p == r->person) {
							strcpy(name, "pater");
							deg_td = top_p;
							relatives = 0;
							marriage = s->month;
							parents = true;
						}
					}
					// 父の再婚相手
					for (SPONSAE *s = p->pater->sponsae.next; !s->dummy && !parents; s = s->next) {
						if (s->p == r->person) {
							strcpy(name, "mater");
							deg_td = top_p;
							relatives = 0;
							marriage = s->month;
							parents = true;
						}
					}
				}

				// 配偶者の父母
				if (Fs && spouse->mater != NULL) {
					if (spouse->mater == r->person || spouse->pater == r->person) {
						strcpy(name, (r->person->sex == 1) ? "spouse_pater" : "spouse_mater");	// @@@2
						deg_td = top_s;
						relatives = spouse->id;
						marriage = 0;
						parents = true;
					}
					// 母の再婚相手
					for (SPONSAE *s2 = spouse->mater->sponsae.next; !s2->dummy && !parents; s2 = s2->next) {
						if (s2->p == r->person) {
							strcpy(name, "spouse_pater");
							deg_td = top_s;
							relatives = spouse->id;
							marriage = s2->month;
							parents = true;
						}
					}
					// 父の再婚相手
					for (SPONSAE *s2 = spouse->pater->sponsae.next; !s2->dummy && !parents; s2 = s2->next) {
						if (s2->p == r->person) {
							strcpy(name, "spouse_mater");
							deg_td = top_s;
							relatives = spouse->id;
							marriage = s2->month;
							parents = true;
						}
					}
				}

				// 子ども
				for (LIBERI *l = p->liberi.next; !l->dummy && !parents && !childlen; l = l->next) {
					if (l->p == r->person) {
						strcpy(name, (r->person->sex == 1) ? "son" : "daughter");	// @@@3
						deg_td = down_c;
						relatives = 0;
						marriage = 0;
						childlen = true;
					}
					for (SPONSAE *s = l->p->sponsae.next; !s->dummy && !parents && !childlen; s = s->next) {
						if (s->p == r->person) {
						strcpy(name, (r->person->sex == 0) ? "son_spouse" : "daughter_spouse");	// @@@4
							deg_td = down_c;
							relatives = 0;
							marriage = s->month;
							childlen = true;
						}
					}
				}

				// 配偶者の連れ子
				for (SPONSAE *s = p->sponsae.next; !s->dummy && !parents && !childlen; s = s->next) {
					for (LIBERI *l = s->p->liberi.next; !l->dummy && !childlen; l = l->next) {
						if (l->p == r->person) {
							strcpy(name, (r->person->sex == 1) ? "son" : "daughter");	// @@@5
							deg_td = down_c;
							relatives = 0;
							marriage = 0;
							childlen = true;
						}
						for (SPONSAE *s2 = l->p->sponsae.next; !s2->dummy && !childlen; s2 = s2->next) {
							if (s2->p == r->person) {
								strcpy(name, (r->person->sex == 0) ? "son_spouse" : "daughter_spouse");	// @@@6
								deg_td = down_c;
								relatives = 0;
								marriage = s2->month;
								childlen = true;
							}
						}
					}
				}

				if (!parents && !childlen) {
					// printf("%lld(deg:%d,Aff:%d,Aff2:%d)\n", r->person->id, r->degree, r->byAffinity, r->byAffinity2);
				} else
					fprintf(fp, "%lld,%s,%d,%d,%d,%d,", r->person->id, name, r->degree, deg_td, relatives, marriage);
			}

			else if (r->degree == second_degree) {
				bool grandparents = false;
				bool brothers = false;
				bool grandchild = false;

				// 母方祖父母
				if (Fp && p->mater != NULL && p->mater->mater != NULL) {
					if (p->mater->mater == r->person || p->mater->pater == r->person) {
						strcpy(name, (r->person->sex == 1) ? "grandpater" : "grandmater");	// @@@7
						deg_td = top_p;
						relatives = 0;
						marriage = 0;
						grandparents = true;
					}
					// 祖母の再婚相手
					for (SPONSAE *s = p->mater->mater->sponsae.next; !s->dummy && !grandparents; s = s->next) {
						if (s->p == r->person) {
							strcpy(name, "grandpater");
							deg_td = top_p;
							relatives = 0;
							marriage = s->month;
							grandparents = true;
						}
					}
					// 祖父の再婚相手
					for (SPONSAE *s = p->mater->pater->sponsae.next; !s->dummy && !grandparents; s = s->next) {
						if (s->p == r->person) {
							strcpy(name, "grandmater");
							deg_td = top_p;
							relatives = 0;
							marriage = s->month;
							grandparents = true;
						}
					}
				}

				// 父方祖父母
				if (Fp && !grandparents && p->pater != NULL && p->pater->mater != NULL) {
					if (p->pater->mater == r->person || p->pater->pater == r->person) {
						strcpy(name, (r->person->sex == 1) ? "grandpater" : "grandmater");	// @@@8
						deg_td = top_p;
						relatives = 0;
						marriage = 0;
						grandparents = true;
					}
					// 祖母の再婚相手
					for (SPONSAE *s = p->pater->mater->sponsae.next; !s->dummy && !grandparents; s = s->next) {
						if (s->p == r->person) {
							strcpy(name, "grandpater");
							deg_td = top_p;
							relatives = 0;
							marriage = s->month;
							grandparents = true;
						}
					}
					// 祖父の再婚相手
					for (SPONSAE *s = p->pater->pater->sponsae.next; !s->dummy && !grandparents; s = s->next) {
						if (s->p == r->person) {
							strcpy(name, "grandmater");
							deg_td = top_p;
							relatives = 0;
							marriage = s->month;
							grandparents = true;
						}
					}
				}

				// 配偶者の母方祖父母
				if (Fs && spouse->mater != NULL && spouse->mater->mater != NULL) {
					if (spouse->mater->mater == r->person || spouse->mater->pater == r->person) {
						strcpy(name, (r->person->sex == 1) ? "spouse_grandpater" : "spouse_grandmater");	// @@@9
						deg_td = top_s;
						relatives = spouse->id;
						marriage = 0;
						grandparents = true;
					}
					// 祖母の再婚相手
					for (SPONSAE *s2 = spouse->mater->mater->sponsae.next; !s2->dummy && !grandparents; s2 = s2->next) {
						if (s2->p == r->person) {
							strcpy(name, "spouse_grandpater");
							deg_td = top_s;
							relatives = spouse->id;
							marriage = s2->month;
							grandparents = true;
						}
					}
					// 祖父の再婚相手
					for (SPONSAE *s2 = spouse->mater->pater->sponsae.next; !s2->dummy && !grandparents; s2 = s2->next) {
						if (s2->p == r->person) {
							strcpy(name, "spouse_grandmater");
							deg_td = top_s;
							relatives = spouse->id;
							marriage = s2->month;
							grandparents = true;
						}
					}
				}

				// 配偶者の父方祖父母
				if (Fs && !grandparents && spouse->pater != NULL && spouse->pater->mater != NULL) {
					if (spouse->pater->mater == r->person || spouse->pater->pater == r->person) {
						strcpy(name, (r->person->sex == 1) ? "spouse_grandpater" : "spouse_grandmater");	// @@@10
						deg_td = top_s;
						relatives = spouse->id;
						marriage = 0;
						grandparents = true;
					}
					// 祖母の再婚相手
					for (SPONSAE *s2 = spouse->pater->mater->sponsae.next; !s2->dummy && !grandparents; s2 = s2->next) {
						if (s2->p == r->person) {
							strcpy(name, "spouse_grandpater");
							deg_td = top_s;
							relatives = spouse->id;
							marriage = s2->month;
							grandparents = true;
						}
					}
					// 祖父の再婚相手
					for (SPONSAE *s2 = spouse->pater->pater->sponsae.next; !s2->dummy && !grandparents; s2 = s2->next) {
						if (s2->p == r->person) {
							strcpy(name, "spouse_grandmater");
							deg_td = top_s;
							relatives = spouse->id;
							marriage = s2->month;
							grandparents = true;
						}
					}
				}

				// 兄弟
				if (Fp && !grandparents && p->mater != NULL) {
					for (LIBERI *l = p->mater->liberi.next; !l->dummy && !brothers; l = l->next) {
						if (p != l->p) {
							if (l->p == r->person) {
								strcpy(name, (r->person->sex == 1) ? "brother" : "sister");	// @@@11
								deg_td = top_p;
								relatives = 0;
								marriage = 0;
								brothers = true;
							}
							for (SPONSAE *s = l->p->sponsae.next; !s->dummy && !brothers; s = s->next) {
								if (s->p == r->person) {
									strcpy(name, (r->person->sex == 0) ? "brother_spouse" : "sister_spouse");	// @@@12
									deg_td = top_p;
									relatives = 0;
									marriage = s->month;
									brothers = true;
								}
							}
						}
					}
					for (LIBERI *l = p->pater->liberi.next; !l->dummy && !brothers; l = l->next) {
						if (p != l->p) {
							if (l->p == r->person) {
								strcpy(name, (r->person->sex == 1) ? "brother" : "sister");	// @@@13
								deg_td = top_p;
								relatives = 0;
								marriage = 0;
								brothers = true;
							}
							for (SPONSAE *s = l->p->sponsae.next; !s->dummy && !brothers; s = s->next) {
								if (s->p == r->person) {
									strcpy(name, (r->person->sex == 0) ? "brother_spouse" : "sister_spouse");	// @@@14
									deg_td = top_p;
									relatives = 0;
									marriage = s->month;
									brothers = true;
								}
							}
						}
					}
				}

				// 配偶者の兄弟
				if (Fs && !grandparents && spouse->mater != NULL) {
					for (LIBERI *l = spouse->mater->liberi.next; !l->dummy && !brothers; l = l->next) {
						if (spouse != l->p) {
							if (l->p == r->person) {
								strcpy(name, (r->person->sex == 1) ? "spouse_brother" : "spouse_sister");	// @@@15
								deg_td = top_s;
								relatives = spouse->id;
								marriage = 0;
								brothers = true;
							}
							for (SPONSAE *s2 = l->p->sponsae.next; !s2->dummy && !brothers; s2 = s2->next) {
								if (s2->p == r->person) {
									strcpy(name, (r->person->sex == 0) ? "spouse_brother_spouse" : "spouse_sister_spouse");	// @@@16
									deg_td = top_s;
									relatives = spouse->id;
									marriage = s2->month;
									brothers = true;
								}
							}
						}
					}
					for (LIBERI *l = spouse->pater->liberi.next; !l->dummy && !brothers; l = l->next) {
						if (spouse != l->p) {
							if (l->p == r->person) {
								strcpy(name, (r->person->sex == 1) ? "spouse_brother" : "spouse_sister");	// @@@17
								deg_td = top_s;
								relatives = spouse->id;
								marriage = 0;
								brothers = true;
							}
							for (SPONSAE *s2 = l->p->sponsae.next; !s2->dummy && !brothers; s2 = s2->next) {
								if (s2->p == r->person) {
									strcpy(name, (r->person->sex == 0) ? "spouse_brother_spouse" : "spouse_sister_spouse");	// @@@18
									deg_td = top_s;
									relatives = spouse->id;
									marriage = s2->month;
									brothers = true;
								}
							}
						}
					}
				}

				// 孫
				for (LIBERI *l = p->liberi.next; !l->dummy && !grandparents && !brothers && !grandchild; l = l->next) {
					for (LIBERI *l2 = l->p->liberi.next; !l2->dummy && !grandchild; l2 = l2->next) {
						if (l2->p == r->person) {
							strcpy(name, (r->person->sex == 1) ? "grandson" : "granddaughter");	// @@@19
							deg_td = down_c;
							relatives = 0;
							marriage = 0;
							grandchild = true;
						}
						for (SPONSAE *s = l2->p->sponsae.next; !s->dummy && !grandchild; s = s->next) {
							if (s->p == r->person) {
								strcpy(name, (r->person->sex == 0) ? "grandson_spouse" : "granddaughter_spouse");	// @@@20
								deg_td = down_c;
								relatives = 0;
								marriage = s->month;
								grandchild = true;
							}
						}
					}
				}
				if (Fs) {
					for (LIBERI *l = spouse->liberi.next; !l->dummy && !grandparents && !brothers && !grandchild; l = l->next) {
						for (LIBERI *l2 = l->p->liberi.next; !l2->dummy && !grandchild; l2 = l2->next) {
							if (l2->p == r->person) {
								strcpy(name, (r->person->sex == 1) ? "grandson" : "granddaughter");	// @@@21
								deg_td = down_c;
								relatives = 0;
								marriage = 0;
								grandchild = true;
							}
							for (SPONSAE *s2 = l->p->sponsae.next; !s2->dummy && !grandchild; s2 = s2->next) {
								if (s2->p == r->person) {
									strcpy(name, (r->person->sex == 0) ? "grandson_spouse" : "granddaughter_spouse");	// @@@22
									deg_td = down_c;
									relatives = 0;
									marriage = s2->month;
									grandchild = true;
								}
							}
						}
					}
				}

				if (!grandparents && !brothers && !grandchild) {
					// printf("%lld(deg:%d,Aff:%d,Aff2:%d)\n", r->person->id, r->degree, r->byAffinity, r->byAffinity2);
					// BREAK = true;
				} else
					fprintf(fp, "%lld,%s,%d,%d,%d,%d,", r->person->id, name, r->degree, deg_td, relatives, marriage);
			}

			else if (r->degree == third_degree) {
				// printf("[3"); fflush(stdout);
				bool great_gp = false;
				bool uncles = false;
				bool nephew = false;
				bool great_gc = false;

				// 母方曾祖父母
				if (Fp && p->mater != NULL) {
					if (p->mater->mater != NULL && p->mater->mater->mater != NULL) {
						if (p->mater->mater->mater == r->person || p->mater->mater->pater == r->person) {
							strcpy(name, (r->person->sex == 1) ? "greatgrandpater" : "greatgrandmater");	// @@@23
							deg_td = top_p;
							relatives = 0;
							marriage = 0;
							great_gp = true;
						}
						// 曾祖母の再婚相手
						for (SPONSAE *s = p->mater->mater->mater->sponsae.next; !s->dummy && !great_gp; s = s->next) {
							if (s->p == r->person) {
								strcpy(name, "greatgrandpater");
								deg_td = top_p;
								relatives = 0;
								marriage = s->month;
								great_gp = true;
							}
						}
						// 曾祖父の再婚相手
						for (SPONSAE *s = p->mater->mater->pater->sponsae.next; !s->dummy && !great_gp; s = s->next) {
							if (s->p == r->person) {
								strcpy(name, "greatgrandmater");
								deg_td = top_p;
								relatives = 0;
								marriage = s->month;
								great_gp = true;
							}
						}
					}
					if (p->mater->pater != NULL && p->mater->pater->mater != NULL) {
						if (p->mater->pater->mater == r->person || p->mater->pater->pater == r->person) {
							strcpy(name, (r->person->sex == 1) ? "greatgrandpater" : "greatgrandmater");	// @@@24
							deg_td = top_p;
							relatives = 0;
							marriage = 0;
							great_gp = true;
						}
						// 曾祖母の再婚相手
						for (SPONSAE *s = p->mater->pater->mater->sponsae.next; !s->dummy && !great_gp; s = s->next) {
							if (s->p == r->person) {
								strcpy(name, "greatgrandpater");
								deg_td = top_p;
								relatives = 0;
								marriage = s->month;
								great_gp = true;
							}
						}
						// 曾祖父の再婚相手
						for (SPONSAE *s = p->mater->pater->pater->sponsae.next; !s->dummy && !great_gp; s = s->next) {
							if (s->p == r->person) {
								strcpy(name, "greatgrandmater");
								deg_td = top_p;
								relatives = 0;
								marriage = s->month;
								great_gp = true;
							}
						}
					}
				}

				// 父方曾祖父母
				if (Fp && !great_gp && p->pater != NULL) {
					if (p->pater->mater != NULL && p->pater->mater->mater != NULL) {
						if (p->pater->mater->mater == r->person || p->pater->mater->pater == r->person) {
							strcpy(name, (r->person->sex == 1) ? "greatgrandpater" : "greatgrandmater");	// @@@25
							deg_td = top_p;
							relatives = 0;
							marriage = 0;
							great_gp = true;
						}
						// 曾祖母の再婚相手
						for (SPONSAE *s = p->pater->mater->mater->sponsae.next; !s->dummy && !great_gp; s = s->next) {
							if (s->p == r->person) {
								strcpy(name, "greatgrandpater");
								deg_td = top_p;
								relatives = 0;
								marriage = s->month;
								great_gp = true;
							}
						}
						// 曾祖父の再婚相手
						for (SPONSAE *s = p->pater->mater->pater->sponsae.next; !s->dummy && !great_gp; s = s->next) {
							if (s->p == r->person) {
								strcpy(name, "greatgrandmater");
								deg_td = top_p;
								relatives = 0;
								marriage = s->month;
								great_gp = true;
							}
						}
					}
					if (p->pater->pater != NULL && p->pater->pater->mater != NULL) {
						if (p->pater->pater->mater == r->person || p->pater->pater->pater == r->person) {
							strcpy(name, (r->person->sex == 1) ? "greatgrandpater" : "greatgrandmater");	// @@@26
							deg_td = top_p;
							relatives = 0;
							marriage = 0;
							great_gp = true;
						}
						// 曾祖母の再婚相手
						for (SPONSAE *s = p->pater->pater->mater->sponsae.next; !s->dummy && !great_gp; s = s->next) {
							if (s->p == r->person) {
								strcpy(name, "greatgrandpater");
								deg_td = top_p;
								relatives = 0;
								marriage = s->month;
								great_gp = true;
							}
						}
						// 曾祖父の再婚相手
						for (SPONSAE *s = p->pater->pater->pater->sponsae.next; !s->dummy && !great_gp; s = s->next) {
							if (s->p == r->person) {
								strcpy(name, "greatgrandmater");
								deg_td = top_p;
								relatives = 0;
								marriage = s->month;
								great_gp = true;
							}
						}
					}
				}

				// 配偶者の母方の曾祖父母
				if (Fs && spouse->mater != NULL) {
					// 母方の母方
					if (spouse->mater->mater != NULL && spouse->mater->mater->mater != NULL) {
						if (spouse->mater->mater->mater == r->person || spouse->mater->mater->pater == r->person) {
							strcpy(name, (r->person->sex == 1) ? "spouse_greatgrandpater" : "spouse_greatgrandmater");	// @@@27
							deg_td = top_s;
							relatives = spouse->id;
							marriage = 0;
							great_gp = true;
						}
						// 曾祖母の再婚相手
						for (SPONSAE *s2 = spouse->mater->mater->mater->sponsae.next; !s2->dummy && !great_gp; s2 = s2->next) {
							if (s2->p == r->person) {
								strcpy(name, "spouse_greatgrandpater");
								deg_td = top_s;
								relatives = spouse->id;
								marriage = s2->month;
								great_gp = true;
							}
						}
						// 曾祖父の再婚相手
						for (SPONSAE *s2 = spouse->mater->mater->pater->sponsae.next; !s2->dummy && !great_gp; s2 = s2->next) {
							if (s2->p == r->person) {
								strcpy(name, "spouse_greatgrandmater");
								deg_td = top_s;
								relatives = spouse->id;
								marriage = s2->month;
								great_gp = true;
							}
						}
					}

					// 母方の父方
					if (spouse->mater->pater != NULL && spouse->mater->pater->mater != NULL) {
						if (spouse->mater->pater->mater == r->person || spouse->mater->pater->pater == r->person) {
							strcpy(name, (r->person->sex == 1) ? "spouse_greatgrandpater" : "spouse_greatgrandmater");	// @@@28
							deg_td = top_s;
							relatives = spouse->id;
							marriage = 0;
							great_gp = true;
						}
						// 曾祖母の再婚相手
						for (SPONSAE *s2 = spouse->mater->pater->mater->sponsae.next; !s2->dummy && !great_gp; s2 = s2->next) {
							if (s2->p == r->person) {
								strcpy(name, "spouse_greatgrandpater");
								deg_td = top_s;
								relatives = spouse->id;
								marriage = s2->month;
								great_gp = true;
							}
						}
						// 曾祖父の再婚相手
						for (SPONSAE *s2 = spouse->mater->pater->pater->sponsae.next; !s2->dummy && !great_gp; s2 = s2->next) {
							if (s2->p == r->person) {
								strcpy(name, "spouse_grandmater");
								deg_td = top_s;
								relatives = spouse->id;
								marriage = s2->month;
								great_gp = true;
							}
						}
					}
				}

				// 配偶者の父方の曾祖父母
				if (Fs && !great_gp && spouse->pater != NULL) {
					// 父方の母方
					if (spouse->pater->mater != NULL && spouse->pater->mater->mater != NULL) {
						if (spouse->pater->mater->mater == r->person || spouse->pater->mater->pater == r->person) {
							strcpy(name, (r->person->sex == 1) ? "spouse_greatgrandpater" : "spouse_greatgrandmater");	// @@@29
							deg_td = top_s;
							relatives = spouse->id;
							marriage = 0;
							great_gp = true;
						}
						// 曾祖母の再婚相手
						for (SPONSAE *s2 = spouse->pater->mater->mater->sponsae.next; !s2->dummy && !great_gp; s2 = s2->next) {
							if (s2->p == r->person) {
								strcpy(name, "spouse_greatgrandpater");
								deg_td = top_s;
								relatives = spouse->id;
								marriage = s2->month;
								great_gp = true;
							}
						}
						// 曾祖父の再婚相手
						for (SPONSAE *s2 = spouse->pater->mater->pater->sponsae.next; !s2->dummy && !great_gp; s2 = s2->next) {
							if (s2->p == r->person) {
								strcpy(name, "spouse_greatgrandmater");
								deg_td = top_s;
								relatives = spouse->id;
								marriage = s2->month;
								great_gp = true;
							}
						}
					}

					// 父方の父方
					if (spouse->pater->pater != NULL && spouse->pater->pater->mater != NULL) {
						if (spouse->pater->pater->mater == r->person || spouse->pater->pater->pater == r->person) {
							strcpy(name, (r->person->sex == 1) ? "spouse_greatgrandpater" : "spouse_greatgrandmater");	// @@@30
							deg_td = top_s;
							relatives = spouse->id;
							marriage = 0;
							great_gp = true;
						}
						// 曾祖母の再婚相手
						for (SPONSAE *s2 = spouse->pater->pater->mater->sponsae.next; !s2->dummy && !great_gp; s2 = s2->next) {
							if (s2->p == r->person) {
								strcpy(name, "spouse_greatgrandpater");
								deg_td = top_s;
								relatives = spouse->id;
								marriage = s2->month;
								great_gp = true;
							}
						}
						// 曾祖父の再婚相手
						for (SPONSAE *s2 = spouse->pater->pater->pater->sponsae.next; !s2->dummy && !great_gp; s2 = s2->next) {
							if (s2->p == r->person) {
								strcpy(name, "spouse_grandmater");
								deg_td = top_s;
								relatives = spouse->id;
								marriage = s2->month;
								great_gp = true;
							}
						}
					}
				}

				// 母方叔父母
				if (Fp && !great_gp && p->mater != NULL && p->mater->mater != NULL) {
					for (LIBERI *l = p->mater->mater->liberi.next; !l->dummy && !uncles; l = l->next) {
						if (p->mater != l->p) {
							if (l->p == r->person) {
								strcpy(name, (r->person->sex == 1) ? "uncle" : "aunt");	// @@@31
								deg_td = top_p;
								relatives = 0;
								marriage = 0;
								uncles = true;
							}
							for (SPONSAE *s = l->p->sponsae.next; !s->dummy && !uncles; s = s->next) {
								if (s->p == r->person) {
									strcpy(name, (r->person->sex == 0) ? "uncle_spouse" : "aunt_spouse");	// @@@32
									deg_td = top_p;
									relatives = 0;
									marriage = s->month;
									uncles = true;
								}
							}
						}
					}
				}

				// 父方祖父母
				if (Fp && !great_gp && !uncles && p->pater != NULL && p->pater->mater != NULL) {
					for (LIBERI *l = p->pater->mater->liberi.next; !l->dummy && !uncles; l = l->next) {
						if (p->pater != l->p) {
							if (l->p == r->person) {
								strcpy(name, (r->person->sex == 1) ? "uncle" : "aunt");	// @@@33
								deg_td = top_p;
								relatives = 0;
								marriage = 0;
								uncles = true;
							}
							for (SPONSAE *s = l->p->sponsae.next; !s->dummy && !uncles; s = s->next) {
								if (s->p == r->person) {
									strcpy(name, (r->person->sex == 0) ? "uncle_spouse" : "aunt_spouse");	// @@@34
									deg_td = top_p;
									relatives = 0;
									marriage = s->month;
									uncles = true;
								}
							}
						}
					}
				}

				// 配偶者の母方叔父母
				if (Fs && !great_gp && spouse->mater != NULL && spouse->mater->mater != NULL) {
					for (LIBERI *l = spouse->mater->mater->liberi.next; !l->dummy && !uncles; l = l->next) {
						if (spouse->mater != l->p) {
							if (l->p == r->person) {
								strcpy(name, (r->person->sex == 1) ? "spouse_uncle" : "spouse_aunt");	// @@@35
								deg_td = top_s;
								relatives = spouse->id;
								marriage = 0;
								uncles = true;
							}
							for (SPONSAE *s2 = l->p->sponsae.next; !s2->dummy && !uncles; s2 = s2->next) {
								if (s2->p == r->person) {
									strcpy(name, (r->person->sex == 0) ? "spouse_uncle_spouse" : "spouse_aunt_spouse");	// @@@36
									deg_td = top_s;
									relatives = spouse->id;
									marriage = s2->month;
									uncles = true;
								}
							}
						}
					}
				}

				// 配偶者の父方叔父母
				if (Fs && !great_gp && !uncles && spouse->pater != NULL && spouse->pater->mater != NULL && !uncles) {
					for (LIBERI *l = spouse->pater->mater->liberi.next; !l->dummy && !uncles; l = l->next) {
						if (spouse->pater != l->p) {
							if (l->p == r->person) {
								strcpy(name, (r->person->sex == 1) ? "spouse_uncle" : "spouse_aunt");	// @@@37
								deg_td = top_s;
								relatives = spouse->id;
								marriage = 0;
								uncles = true;
							}

							for (SPONSAE *s2 = l->p->sponsae.next; !s2->dummy && !uncles; s2 = s2->next) {
								if (s2->p == r->person) {
									strcpy(name, (r->person->sex == 0) ? "spouse_uncle_spouse" : "spouse_aunt_spouse");	// @@@38
									deg_td = top_s;
									relatives = spouse->id;
									marriage = s2->month;
									uncles = true;
								}
							}
						}
					}
				}

				// 甥姪
				if (Fp && !great_gp && !uncles && p->mater != NULL) {
					for (LIBERI *l = p->mater->liberi.next; !l->dummy && !nephew; l = l->next) {
						if (p != l->p) {
							for (LIBERI *l2 = l->p->liberi.next; !l2->dummy && !nephew; l2 = l2->next) {
								if (l2->p == r->person) {
									strcpy(name, (r->person->sex == 1) ? "nephew" : "niece");	// @@@39
									deg_td = down_p;
									relatives = 0;
									marriage = 0;
									nephew = true;
								}
								for (SPONSAE *s = l->p->sponsae.next; !s->dummy && !nephew; s = s->next) {
									if (s->p == r->person) {
										strcpy(name, (r->person->sex == 0) ? "nephew_spouse" : "niece_spouse");	// @@@40
										deg_td = down_p;
										relatives = 0;
										marriage = s->month;
										nephew = true;
									}
								}
							}
						}
					}
					for (LIBERI *l = p->pater->liberi.next; !l->dummy && !nephew; l = l->next) {
						if (p != l->p) {
							for (LIBERI *l2 = l->p->liberi.next; !l2->dummy && !nephew; l2 = l2->next) {
								if (l2->p == r->person) {
									strcpy(name, (r->person->sex == 1) ? "nephew" : "niece");	// @@@41
									deg_td = down_p;
									relatives = 0;
									marriage = 0;
									nephew = true;
								}
								for (SPONSAE *s = l->p->sponsae.next; !s->dummy && !nephew; s = s->next) {
									if (s->p == r->person) {
										strcpy(name, (r->person->sex == 0) ? "nephew_spouse" : "niece_spouse");	// @@@42
										deg_td = down_p;
										relatives = 0;
										marriage = s->month;
										nephew = true;
									}
								}
							}
						}
					}
				}

				// 配偶者の甥姪
				if (Fs && !great_gp && !uncles && spouse->mater != NULL) {
					for (LIBERI *l = spouse->mater->liberi.next; !l->dummy && !nephew; l = l->next) {
						if (spouse != l->p) {
							for (LIBERI *l2 = l->p->liberi.next; !l2->dummy && !nephew; l2 = l2->next) {
								if (l2->p == r->person) {
									strcpy(name, (r->person->sex == 1) ? "spouse_nephew" : "spouse_niece");	// @@@43
									deg_td = down_s;
									relatives = spouse->id;
									marriage = 0;
									nephew = true;
								}
								for (SPONSAE *s2 = l->p->sponsae.next; !s2->dummy && !nephew; s2 = s2->next) {
									if (s2->p == r->person) {
										strcpy(name, (r->person->sex == 0) ? "spouse_nephew_spouse" : "spouse_niece_spouse");	// @@@44
										deg_td = down_s;
										relatives = spouse->id;
										marriage = s2->month;
										nephew = true;
									}
								}
							}
						}
					}
					for (LIBERI *l = spouse->pater->liberi.next; !l->dummy && !nephew; l = l->next) {
						if (spouse != l->p) {
							for (LIBERI *l2 = l->p->liberi.next; !l2->dummy && !nephew; l2 = l2->next) {
								if (l->p == r->person) {
									strcpy(name, (r->person->sex == 1) ? "spouse_nephew" : "spouse_niece");	// @@@45
									deg_td = down_s;
									relatives = spouse->id;
									marriage = 0;
									nephew = true;
								}
								for (SPONSAE *s2 = l->p->sponsae.next; !s2->dummy && !nephew; s2 = s2->next) {
									if (s2->p == r->person) {
										strcpy(name, (r->person->sex == 0) ? "spouse_nephew_spouse" : "spouse_niece_spouse");	// @@@46
										deg_td = down_s;
										relatives = spouse->id;
										marriage = s2->month;
										nephew = true;
									}
								}
							}
						}
					}
				}

				// ひ孫
				for (LIBERI *l = p->liberi.next; !l->dummy && !great_gp && !uncles && !nephew && !great_gc; l = l->next) {
					for (LIBERI *l2 = l->p->liberi.next; !l2->dummy && !great_gc; l2 = l2->next) {
						for (LIBERI *l3 = l2->p->liberi.next; !l3->dummy && !great_gc; l3 = l3->next) {
							if (l3->p == r->person) {
								strcpy(name, (r->person->sex == 1) ? "greatgrandson" : "greatgranddaughter");	// @@@47
								deg_td = down_c;
								relatives = 0;
								marriage = 0;
								great_gc = true;
							}
							for (SPONSAE *s = l3->p->sponsae.next; !s->dummy && !great_gc; s = s->next) {
								if (s->p == r->person) {
									strcpy(name, (r->person->sex == 0) ? "greatgrandson_spouse" : "greatgranddaughter_spouse");	// @@@48
									deg_td = down_c;
									relatives = 0;
									marriage = 0;
									great_gc = true;
								}
							}
						}
					}
				}
				if (Fs) {
					for (LIBERI *l = spouse->liberi.next; !l->dummy && !great_gc; l = l->next) {
						for (LIBERI *l2 = l->p->liberi.next; !l2->dummy && !great_gc; l2 = l2->next) {
							for (LIBERI *l3 = l2->p->liberi.next; !l3->dummy && !great_gc; l3 = l3->next) {
								if (l3->p == r->person) {
									strcpy(name, (r->person->sex == 1) ? "greatgrandson" : "greatgranddaughter");	// @@@49
									deg_td = down_c;
									relatives = 0;
									marriage = 0;
									great_gc = true;
								}
								for (SPONSAE *s = l3->p->sponsae.next; !s->dummy && !great_gc; s = s->next) {
									if (s->p == r->person) {
										strcpy(name, (r->person->sex == 0) ? "greatgrandson_spouse" : "greatgranddaughter_spouse");	// @@@50
										deg_td = down_c;
										relatives = 0;
										marriage = s->month;
										great_gc = true;
									}
								}
							}
						}
					}
				}

				if (!great_gp && !uncles && !nephew && !great_gc) {
					// printf("%lld(deg:%d,Aff:%d,Aff2:%d)\n", r->person->id, r->degree, r->byAffinity, r->byAffinity2);
				} else
					fprintf(fp, "%lld,%s,%d,%d,%d,%d,", r->person->id, name, r->degree, deg_td, relatives, marriage);
			}
			else {
				if (prev !=NULL && prev->person != r->person && prev->degree != r->degree) {
					if (Fp)
						fprintf(fp, "%lld,%s,%d,%d,%d,%d,", r->person->id, "relatives", r->degree, 6, 0, 0);
					else if (Fs)
						fprintf(fp, "%lld,%s,%d,%d,%lld,%d,", r->person->id, "spouse_relatives", r->degree, 6, spouse->id, 0);
					// else printf("Who >> %lld\n", r->person->id);
				}
				prev = r;
			}

		}
	}
	// printf("\n");
	fprintf(fp, "\n");
}

#if DNAversion
void inherit_gvalue(PERSON *baby, int mode)
{
  if(mode == 1){ // a one-locus two-allele model
    int mu, ml, pu, pl;
    int bu, bl;
    double rate;

    if(baby->mater->gvalue == 0){ mu = 0; ml = 0;}
    else if (baby->mater->gvalue == 1){ mu = 0; ml = 1;}
    else if (baby->mater->gvalue == 2){ mu = 1; ml = 0;}
    else if (baby->mater->gvalue == 3){ mu = 1; ml = 1;}
    else {mu = 0; ml = 0;}

    if(baby->pater->gvalue == 0){ pu = 0; pl = 0;}
    else if (baby->pater->gvalue == 1){ pu = 0; pl = 1;}
    else if (baby->pater->gvalue == 2){ pu = 1; pl = 0;}
    else if (baby->pater->gvalue == 3){ pu = 1; pl = 1;}
    else {pu = 0; pl = 0;}

    rate = 0.5;
    if(probability(rate)) {bu = mu; bl = pl;}
    else                  {bu = pu; bl = ml;}


    baby->gvalue = bu * 2 + bl;

    //debug
    printf("mater g=%f, pater g=%f, baby g=%f\n", baby->mater->gvalue, baby->pater->gvalue, baby->gvalue);

  }else{
    baby->gvalue = (baby->mater->gvalue + baby->pater->gvalue) / 2;
  }

}
#endif

// for plural_marriage
bool marriageable(PERSON *p, FLAGS opts){ //結婚可能かどうか
  /*   まず，spouse_curno == 0 ならば p->married で判断 */
  //   再婚不可の場合，一度 trueになったmarriedはfalseに戻らないようにしている．
  if (p->spouse_curno == 0) {
    if(opts.remarriage_flag) return true;
    else return !p->married;
  }

  // 最大配偶者数の計算
  if (p->sex == female){ //女性の場合の最大配偶者数は1
    p->spouse_max = 1;
  }else{ //男性の場合
    /*   まず，今の実行が一夫一婦制か一夫多妻制かの確認 */
    if(opts.plural_marriage){ //一夫多妻制
      p->spouse_max = opts.plural_max; //とりあえず固定値を入れる
      if(opts.plural_unit != 0.0){ //これがSkill変動制の場合
	/*Skill変動制ならばfloor(Skill[0]/opts.plural_unit)+1を最大配偶者数に */
	p->spouse_max = floor(p->skill[0]/opts.plural_unit) + 1;
      }
    }else{ //一夫一婦制
      p->spouse_max = 1;
    }
  }

  //配偶者を選ぶことができるか?
  /* spouse_max > spouse_curno の時 true */
  /* そうでなければ false */
  if (p->spouse_max > p->spouse_curno) return true;
  else return false;

}

SPONSAE *espousal(PERSON *p, PERSON *q, int month, int flag){ //結婚の処理
  // p が qと結婚したときのpの処理 flag==0で一回目，flag==1でその相手
  // まず p->married = true にする  
  p->married = true;
  // p の配偶者リストに q を加える(add_spouse)
  add_spouse(p, q, month);
  // p->spouse_curno++をする
  p->spouse_curno++;
  if(p->spouse_maxreal < p->spouse_curno) p->spouse_maxreal = p->spouse_curno;
  // もしflag==0ならば人を交換してespousalを呼ぶ．
  if (flag==0) espousal(q, p, month, 1);

  // p->spouse に q を入れる(一番最近に結婚した相手が入ることになる)
  p->spouse = q;

  return &(p->sponsae);
}

int cutspouse(PERSON *q, PERSON *p){
  // q の配偶者リストから p を削除し，q のspouse_curno--とする．そのspouse_curnoを返す
  // q が女性の場合，q->spouse もNULLとする

  for (SPONSAE *s = q->sponsae.next; !s->dummy; s = s->next){
    if (s->p == p){
      s->prev->next = s->next;
      s->next->prev = s->prev;
      FREE(s);
      q->spouse_curno--;
      if(q->sex == female){
	q->spouse = NULL;
      }
      return q->spouse_curno;
    }
  }

  return q->spouse_curno;
}

SPONSAE *bereavement(PERSON *p, FLAGS opts){ //死別の処理
  PERSON *q; // 配偶者

    if (p->spouse_curno > 0) { //配偶者がいる場合のみ処理

	for (SPONSAE *s = p->sponsae.next; !s->dummy; s = s->next){
	  q = s->p; //配偶者
	  cutspouse(q, p); // q の配偶者リストからpを削除

	}
    }

  return &(p->sponsae);
}
