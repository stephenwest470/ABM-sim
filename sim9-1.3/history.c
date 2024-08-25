/*
 * history.c - Simulation of population moving
 *		$Id: history.c,v 1.3 2013/02/24 07:16:51 void Exp $
 * vi: ts=4 sw=4 sts=4
 */
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "sim.h"
#include "main.h"
#include "person_pre.h"
#include "link.h"
#include "relpointer.h"
#include "history.h"
#include "person.h"
#include "resident.h"
#include "area.h"

extern FILE *fp;
extern bool SIM;
extern char file[50];
extern char file_info[50];
static HISTORY *purge_history(HISTORY *);

void purge_history_list(PERSON *p)
{
	for (HISTORY *h = p->history.next; !h->dummy; h = purge_history(h))
		;
}

static HISTORY *purge_history(HISTORY *h)
{
	HISTORY *n;

	n = h->next;
	h->next->prev = h->prev;
	h->prev->next = h->next;
	FREE(h);
	return n;
}

void add_history(PERSON *p, int y, int m, EVENT e, void *para, long long int id)
{
	HISTORY *h;

	if (SIM) {
		if ((h = MALLOC(sizeof(HISTORY))) == NULL)
			error("Not enough memory. (HISTORY)");

		h->dummy = false;
		h->event = e;
		h->month = y * TERM_IN_YEAR + m;

		switch (e) {
		case create:
			h->para.month = p->month;
			h->id = 0LL;
			break;
		case birth:
			h->para.mother = (PERSON *)para;
			h->id = h->para.mother == NULL ? 0LL : h->para.mother->id;
			break;
		case marriage:
			h->para.mate = (PERSON *)para;
			h->id = h->para.mate->id;
			break;
		case pregnancy:
			h->para.preg_mate = (PERSON *)para;
			h->id = h->para.preg_mate->id;
			break;
		case delivery:
			h->para.child = (PERSON *)para;
			h->id = h->para.child->id;
			break;
		case move:
			h->para.route = (LINK *)para;
			h->id = id;
			break;
		case lose:
			h->para.mate = (PERSON *)para;
			h->id = h->para.mate->id;
			break;
		case death:
			h->para.none = NULL;
			h->id = 0LL;
			break;
		default:
			error("Illegal Event.");
			break;
		}

		h->prev = p->history.prev;
		h->next = &p->history;
		p->history.prev->next = h;
		p->history.prev = h;
	}
}

void print_dhistory(PERSON *p)	// !!
{
	printf("\t<history>\n");
	for (HISTORY *h = p->history.next; !h->dummy; h = h->next) {

		printf("\t%d / %d: ", h->month / TERM_IN_YEAR, h->month % TERM_IN_YEAR);

		switch (h->event) {
		case create:
			printf("Initial (%d): ", h->para.month);
			break;
		case birth:
			printf("Birth (%lld): ", h->id);
			break;
		case marriage:
			printf("Marriage (%lld): ", h->id);
			break;
		case pregnancy:
			printf("Pregnancy: (%lld)", h->id);
			break;
		case delivery:
			printf("Delivery (%lld): ", h->id);
			break;
		case move:
			printf("Move: (%s-%s)", area_name(h->para.route->from), area_name(h->para.route->to));
			break;
		case lose:
			printf("Lose (%lld): ", h->id);
			break;
		case death:
			printf("Death: ");
			break;
		default:
			error("Illegal Event.");
			break;
		}
		printf("\n");
	}
}

void print_history(PERSON *p)
{
	printf("\t<history>\n");
	for (HISTORY *h = p->history.next; !h->dummy; h = h->next) {
		printf("\t%d / %d: ", h->month / TERM_IN_YEAR, h->month % TERM_IN_YEAR);

		switch (h->event) {
		case create:
			printf("Initial (%d): ", h->para.month);
			break;
		case birth:
			printf("Birth (%lld): ", h->id);
			break;
		case marriage:
			printf("Marriage (%lld): ", h->id);
			break;
		case pregnancy:
			printf("Pregnancy: (%lld)", h->id);
			break;
		case delivery:
			printf("Delivery (%lld): ", h->id);
			break;
		case move:
			printf("Move: (%s-%s)", area_name(h->para.route->from), area_name(h->para.route->to));
			break;
		case lose:
			printf("Lose (%lld): ", h->id);
			break;
		case death:
			printf("Death: ");
			break;
		default:
			error("Illegal Event.");
			break;
		}
		printf("\n");
	}
}


void print_ahistory(PERSON *p)	// !!
{
#if DNAversion
  fprintf(fp, "%s,%lld,%s,%d,%d,%d, %f", areax(p->area)->name,p->id,sex(p),p->month,p->move,p->moveWith, p->gvalue); // with gvalue
#else
	fprintf(fp, "%lld,%s,%d,%d,%d", p->id, sex(p), p->month, p->move, p->moveWith); // 
#endif
	for (HISTORY *h = p->history.next; !h->dummy; h = h->next) {
		switch (h->event) {
		case create:
			fprintf(fp, ",create");
			fprintf(fp, ",%d", h->month);
			fprintf(fp, ",%d", p->birth_area);
			fprintf(fp, ",%lld", h->id);
			break;
		case birth:
			// if (h->month != 0) {
			//	for (i = 0; i < h->month; i++)
			//		fprintf(fp, ",-1");
			// }
			fprintf(fp, ",birth");
			fprintf(fp, ",%d", h->month);
			fprintf(fp, ",%d", p->birth_area);
			fprintf(fp, ",%lld", h->id);
			break;
		case marriage:
			fprintf(fp, ",marriage");
			fprintf(fp, ",%d", h->month);
			fprintf(fp, ",%lld", p->id);
			fprintf(fp, ",%lld", h->id);
			break;
		case delivery:
			fprintf(fp, ",delivery");
			fprintf(fp, ",%d", h->month);
			fprintf(fp, ",%lld", h->id); // child id
			break;
		case move:
			fprintf(fp, ",move");
			fprintf(fp, ",%d", h->month);
			fprintf(fp, ",%d", h->para.route->to);
			fprintf(fp, ",%lld", h->id);
			break;
		case death:
			fprintf(fp, ",death");
			fprintf(fp, ",%d", h->month);
			fprintf(fp, ",%d", p->area);
			fprintf(fp, ",%lld", h->id);
			break;
		default:
			break;
		}
	}
	fprintf(fp, "\n");
}

int first_marriage(PERSON *p) // 初婚月齢 ここではbirth event のない人ははずす(-1を返す)
{
  int b_m = -1;

  for (HISTORY *h = p->history.next; !h->dummy; h = h->next) {
    if(h->event == birth){
      b_m = h->month;
    }
    if (h->event == marriage) { //最初にみつかった結婚イベント
      if(b_m >= 0){
	return h->month - b_m;
      }else{
	return -1; // 不明の意　crete event の人もこれになる
      }
    }
 }
  if(b_m == -1) return -1; //birth event のない人
  return 0; // 未婚の人
}
