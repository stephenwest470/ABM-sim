/*
 * person.h - Simulation of population moving
 *		$Id: person.h,v 1.4 2013/02/24 17:12:26 void Exp $
 * vi: ts=4 sw=4 sts=4
 * sim7-5
 */
#ifndef PERSONH
#define PERSONH

typedef enum {female = 0, male = 1} SEX;

typedef struct liberi LIBERI;
typedef struct sponsae SPONSAE;

struct liberi {
	LIBERI	*prev;
	LIBERI	*next;
	bool	dummy;
	PERSON	*p;
} liberi;

struct sponsae {
	SPONSAE	*prev;
	SPONSAE	*next;
	bool	dummy;
	PERSON	*p;
	int month;
} sponsae;

struct person {
	PERSON	*prev;
	PERSON	*next;
	bool	dummy;
	bool	dead;
	bool	no_relatives;
	long long int id;
	SEX	sex;
        int	month; // 生きた月齢
        int	birth_month;  // 生成されたときの月齢(createの時正規分布，bitrhの時は0)
        int birthday_year; // 生まれた時．createで0それ以外で生まれたときの年
	int	birth_area;
	int	area;
	int	move;		// 移動回数
	int	moveWith;	// 家族と一緒に移動した回数
        bool married;       // 結婚経験フラグにする(sim8-1)
	PERSON	*mater;
	PERSON	*pater;
  PERSON	*spouse; // 男女とももっとも最近結婚した相手
	LIBERI	liberi;		// 子どもリスト
	SPONSAE	sponsae;	// 配偶者リスト
	int pregnant_month;
        PERSON *pregnant_spouse;//本人が女性で妊娠した場合，相手の男性．本人が男性の時は意味のない変数
	RELPOINTER relpointer;
	HISTORY history;
#if DNAversion
	double gvalue; // DNAversion DNA 割合
#endif
	double skill[MAX_SKILL];	// 技量
        int     masterSearch; // 0: prev から 1: next から
        bool    masterRandom; // true: 1 -10 のどれか false: 1人目
        bool masterRemember; // true: 前のmasterを覚えてる false: 覚えてない
        long long int prevMaster; // 前のmasterのID

        int spouse_max;   // 最大配偶者数．
        int spouse_curno; // 現在の配偶者数．
        int spouse_maxreal; //実際生涯で一番配偶者が多かったときの人数
};

PERSON *create_person(int, SEX, int, int);
PERSON *purge_person(PERSON *);
void bury_person(PERSON *);
void print_marriage_childinfo(PERSON *);
void print_deadperson(PERSON *);
void print_child(PERSON *);
int count_child(PERSON *);
LIBERI *add_child(PERSON *, PERSON *);
void purge_all_dead_person(void);
void purge_all_live_person(void);
void purge_alone_dead_person(bool);
bool alone(PERSON *, int);
const char *sex(const PERSON *);

// !!
long long int personID();
void print_spouse(PERSON *);
SPONSAE *add_spouse(PERSON *, PERSON *, int); // いらなくなるはず
bool marriageable(PERSON *, FLAGS); //結婚可能かどうか
SPONSAE *espousal(PERSON *, PERSON *, int, int); //結婚の処理
SPONSAE *bereavement(PERSON *, FLAGS); //死別の処理
void person(PERSON *, int);
bool check_down(PERSON *, PERSON *, int);
bool check_uncles(PERSON *, PERSON *);
bool check_brotherNephew(PERSON *, PERSON *, int);
bool check_top(PERSON *, PERSON *, int);

// DNAversion
#if DNAversion
void inherit_gvalue(PERSON *, int);
#endif

#endif
