/*
 * history.h - Simulation of population moving
 *		$Id: history.h,v 1.3 2013/02/24 07:16:51 void Exp $
 * vi: ts=4 sw=4 sts=4
 */
typedef enum {
	none = 0,		// -
	create = 1,		// int month
	birth = 2,		// PERSON *mother
	marriage = 3,		// PERSON *mate
	pregnancy = 4,		// PERSON *preg_mate
	delivery = 6,		// PERSON *chile
	move = 8,		// LINK *route
	lose = 9,		// PERSON *mate
	death = 10		// -
} EVENT;

typedef struct history HISTORY;
struct history {
	HISTORY	*prev;
	HISTORY	*next;
	bool	dummy;
	EVENT	event;
	// int	current_area;	// 現在地
	int	month;
	union {
			int month;
			PERSON *mother;
			PERSON *mate;
			PERSON *preg_mate;
			PERSON *child;
			LINK *route;
			void *none;
	} para;
	long long int id;
};

void add_history(PERSON *, int, int, EVENT, void *, long long int);
void print_dhistory(PERSON *);
void print_ahistory(PERSON *);
void print_history(PERSON *);
void purge_history_list(PERSON *);
int first_marriage(PERSON *);
