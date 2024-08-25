/*
 * resident.c - Simulation of population moving
 *		$Id: resident.c,v 1.3 2013/02/24 07:16:51 void Exp $
 * vi: ts=4 sw=4 sts=4
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "sim.h"
#include "main.h"
#include "person_pre.h"
#include "link.h"
#include "relpointer.h"
#include "history.h"
#include "person.h"
#include "resident.h"
#include "area.h"

RESIDENT *create_resident(int area_code, PERSON *p)
{
	RESIDENT *res;

	if ((res = MALLOC(sizeof(RESIDENT))) == NULL)
		error("Not enough memory. (RESIDENT)");

	res->dummy = false;
	res->person = p;

	res->prev = areax(area_code)->resident.prev;
	res->next = &areax(area_code)->resident;
	areax(area_code)->resident.prev->next = res;
	areax(area_code)->resident.prev = res;
	return res;
}

void purge_resident_from_person(PERSON *p)
{
	RESIDENT *res;

	for (res = areax(p->area)->resident.next; !res->dummy; res = res->next) {
		if (res->person == p)
			break;
	}
	if (res->dummy) {
		printf("%lld", p->id);
		if (p->dead)
			printf("(death)\n");
		fflush(stdout);
		error("Resident not found!");
	}
	(void)purge_resident(res);
}

RESIDENT *purge_resident(RESIDENT *res)
{
	RESIDENT *n;

	n = res->next;
	res->next->prev = res->prev;
	res->prev->next = res->next;
	FREE(res);
	return n;
}
