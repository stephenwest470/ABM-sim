/*
 * relpointer.h - Simulation of population moving
 *		$Id: relpointer.h,v 1.3 2013/02/24 07:16:51 void Exp $
 * vi: ts=4 sw=4 sts=4
 */
typedef enum {
	noneStat = 0,
	inheritance = 1, // 両親の親戚を引き継ぐ
	childAck = 2, // 親に子を追加
	materAck = 3, // 母親を追加
	paterAck = 4, // 父親を追加
	marryAck = 5, // 配偶者を追加
	affinity = 6, // 互いに相手の親戚をマージ
	marryNotice = 7, // 結婚通知
	birthNotice = 8, // 誕生通知
	deathNotice = 9, // 死亡通知
	purgeNotice = 10 // 破棄通知
} RELSTATUS;

typedef struct relpointer RELPOINTER;
struct relpointer {
	RELPOINTER *prev;
	RELPOINTER *next;
	bool dummy;
	PERSON *person;
	int	degree;
	// int long long	id;	// ##
	// int sex;	// ##
	bool dead;
	bool byAffinity; // relative by affinity 配偶者の血族
	bool byAffinity2; // relative by affinity 血族の配偶者
	RELSTATUS status;
};

void purge_relpointer_list(PERSON *);
void birth_relpointer(PERSON *, PERSON *, PERSON *, int);
void marriage_relpointer(PERSON *, PERSON *);
void death_relpointer(PERSON *);
void purge_relpointer(PERSON *);
bool isAlone(const PERSON *);
bool isRelative(const PERSON *, const PERSON *, int);
bool isRelative2(const PERSON *, const PERSON *, int);
bool hasRelative(const PERSON *, int, int);
bool hasPregnant(const PERSON *);
void print_relpointer(const PERSON *);

