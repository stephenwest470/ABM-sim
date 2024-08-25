/*
 * link.h - Simulation of population moving
 *		$Id: link.h,v 1.2 2013/02/10 15:42:50 void Exp $
 * vi: ts=4 sw=4 sts=4
 */
typedef struct link {
	int	from;
	int	to;
	double prob;
} LINK;

typedef struct route ROUTE;
struct route {
	ROUTE	*prev;
	ROUTE	*next;
	bool	dummy;
	LINK	*link;
	double   prob;
	bool	relation;
	int	count_in_year;
};

extern int n_link;
