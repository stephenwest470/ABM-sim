/*
 * resident.h - Simulation of population moving
 *		$Id: resident.h,v 1.1 2010/11/25 03:57:55 void Exp $
 * vi: ts=4 sw=4 sts=4
 */
typedef struct resident RESIDENT;
struct resident {
	RESIDENT	*prev;
	RESIDENT	*next;
	bool	dummy;
	PERSON	*person;
};

RESIDENT *create_resident(int, PERSON *);
RESIDENT *purge_resident(RESIDENT *);
void purge_resident_from_person(PERSON *);
