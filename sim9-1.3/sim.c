/*
 * sim.c - Simulation of population moving
 *		$Id: sim.c,v 1.5 2013/02/24 17:12:26 void Exp $SEX
 * vi: ts=4 sw=4 sts=4
 * sim 7-5 用
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
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

#define MARRIGEABLE_AGE_MIN	15
#define MARRIGEABLE_AGE_MAX	45
#define MOVABLE_AGE	15
#define BIRTH_MONTH		10
#define PREGNANT_AGE_MIN	15
#define PREGNANT_AGE_MAX	45
#define AGE_DIFF_MAX	10
#define PREGNANT_RACIO	0.8
#define ENCOUNTER_RACIO	1.0

#define AREA_FILENAME	"area.txt"
#define LINK_FILENAME	"link.txt"
#define INIT_FILENAME	"init.txt"
#define MORT_FILENAME	"MortalityRate.txt"
#define BRTH_FILENAME	"BirthRate.txt"
#define SKIL_FILENAME	"Skill.txt"

#define MAX_LINK 1000
#define LINK_FILENAME "link.txt"

extern int n_area;
extern AREA areas[MAX_AREA];

#if DNAversion
PERSON *encounter(int, SEX, FLAGS, PERSON *);
PERSON* encoutner_from_area(int, SEX, FLAGS, PERSON*, PERSON*); // sim9-1
#else
PERSON *encounter(int, SEX, PERSON *);
#endif
void moving(ROUTE *, PERSON *);
ROUTE *direction(const PERSON *, int, int);

static void clearRelationFlag(AREA *);
static PERSON *master_encounter(int, PERSON *);

//sim7-6 & sim8-2
static PERSON *master_random(FLAGS, int, PERSON *);
static PERSON *master_relative(int, PERSON *, int);
static PERSON *master_best(int, int);
static PERSON *set_master_best(int, PERSON *);
static PERSON *set_new_master_best(int, PERSON *);
static PERSON *master_parent_random(PERSON *);
static PERSON *master_relative_random(PERSON *, int);
PERSON* master_best76[MAX_AREA];  // only for sim7-6
//sim7-6 end

//sim8-1
static LIBERI marriageReq_list = {&marriageReq_list, &marriageReq_list, true, NULL}; // 初期化これであってる？
static PERSON *add_marriageReq(PERSON *, int, int, FLAGS);
static void doMarriage(FLAGS, int, int);
//sim8-1 end

extern PERSON live_list;

extern FILE *fp;
extern bool SIM;
extern char file[50];
extern char file_info[50];

LINK links[MAX_LINK];

void
print_master_best76(int n){
  //  printf("master_best change at %d, id = %lld, skill = %4.3f\n", n, master_best76[n]->id, master_best76[n]->skill[0]);
}

void ante(FLAGS opts, int loop_y, char *mort_file)
{
	int m_area;
	int m_link;
	int m_init;
	int m_mort;
	int m_brth;
	int m_skil;

	// if (!opts.keep_flag)
	//	seeding();

	m_area = read_area(AREA_FILENAME);	// init.c
	m_link = read_link(LINK_FILENAME);
	if ((m_init = read_init(INIT_FILENAME)) != m_area)
			error("Invalid init count");

	// if ((m_mort = read_MortalityRate(MORT_FILENAME)) != m_area)
	if (strlen(mort_file) == 0)
		strcpy(mort_file, MORT_FILENAME);
	if ((m_mort = read_MortalityRate(mort_file)) != m_area)
		error("Invalid Mortality Rate count");
	printf("%s\n", mort_file);
	if ((m_brth = read_BirthRate(BRTH_FILENAME)) != m_area)
		error("Invalid Birth Rate count");
	if ((m_skil = read_Skill(SKIL_FILENAME)) != m_area)
		error("Invalid Skill Value count");
	initial(m_area, opts);

	if (opts.exam_flag) {
		printf("area = %d\n", m_area);
			printf("link = %d\n", m_link);
			printf("init = %d\n", m_init);
			printf("mort = %d\n", m_mort);
			printf("brth = %d\n", m_brth);
			printf("skil = %d\n", m_skil);
	}

	opts.age_layer ? print_population10(opts.grph_flag) : print_population(opts.grph_flag);
	SIM = false;	// ##
}

// 人口格納用の領域確保
void prePop(FLAGS opts, int loop_y)
{
	for (int i = 0; i < n_area; i++)
		areas[i].pop = (long long int *)malloc(sizeof(long long int) * (loop_y + 1));
}


void Pop(FLAGS opts, int loop_y)
{
	// 地域パラメータ
	for (int i = 0; i < n_area; i++) {
		fprintf(fp, "%d,%s,c%lld,mr%lg,", areas[i].code, areas[i].name, areas[i].capacity, areas[i].initMoveRate);
		fprintf(fp, "pt%d,ip%lld,mp%lld,sr%lg\n", areas[i].init_pop_type, areas[i].init_pop, areas[i].max_pop, areas[i].init_sex_ratio);
		for (int j = 0; j < 2; j++) {
			for (int k = 0; k < MAX_AGE + 1; k++)
				fprintf(fp, "%lg,", areas[i].BirthRate[j][k]);
		}
		fprintf(fp, "\n");

		for (int j = 0; j < 2; j++) {	// updateed 5-1
			for (int k = 0; k < MAX_AGE + 1; k++)
				fprintf(fp, "%lg,", areas[i].MortalityRate[j][k]);
		}
		fprintf(fp, "\n");

		for (int j = 0; j < loop_y + 1; j++)
			fprintf(fp, "%lld,", areas[i].pop[j]);
		free( areas[i].pop );
		fprintf(fp, "\n");
	}

}


void post(FLAGS opts)
{
	if (opts.last_flag)	// 途中経過を表示せず、最初と最後の結果のみを表示
		opts.age_layer ? print_population10(opts.grph_flag) : print_population(opts.grph_flag);

	purge_all_dead_person();
	purge_all_live_person();
	purge_all_resident();
}

// !!ファイル書き込み
void file_init(FLAGS opts, int loop_y)
{
	FILE *fp = fopen(file_info, "w");

	fprintf(fp, "y%d,a%d,f%d,r%d,w%lg,ab%lg,dt%d,i%d,ml%d,h%d,o%d,s%d,u%d\n",
		loop_y, n_area, opts.family, opts.relation, opts.with_rate,
		opts.at_birth, opts.dist_type, opts.incest, opts.marrige_limit ? 1 : 0,
		opts.history_flag ? 1 : 0, opts.family_degree,
		opts.sex, opts.unmarried_flag ? 1 : 0);

	for (int i = 0; i < n_area; i++) {
		fprintf(fp, "%d,%s,", areas[i].code, areas[i].name);
	}
	fprintf(fp, "\n");

	for (int i = 0; i < n_link; i++) {
		fprintf(fp, "%d,%d,%lg,", links[i].from, links[i].to, links[i].prob);
	}
	fprintf(fp, "\n");

	fclose(fp);
}

void createPerson()
{
	PERSON *nextp;
	for (PERSON *p = live_list.next; !p->dummy; p = nextp) {
		nextp = p->next;
		if (p->mater != NULL && p->mater->id > personID())
			printf("OVER");
		if (p->pater != NULL && p->mater->id > personID())
			printf("OVER");
		fflush(stdout);
		add_history(p, 0, 0, create, NULL, 0LL);
	}
}

long long int sim(int year, bool slow, FLAGS opts, int loop_y)
{
	int max_degree;
	PERSON *nextp;

	// if ((-f n) > (-i m))
	//	return n;
	// else if ((-i m) > (-r m2))
	//	return m;
	// else
	//	m2;

	max_degree = (opts.family > opts.incest) ? opts.family : ((opts.incest > opts.relation) ? opts.incest : opts.relation);

	clear_move_count();

	if (year == 0 && opts.history_flag && SIM) {
		for (int i = 0; i < n_area; i++)
			areas[i].pop[0] = areas[i].area_population;
	}

	for (int m = 0; m < TERM_IN_YEAR; m++) { // 月ごとの処理


		for (PERSON *p = live_list.prev; !p->dummy; p = nextp) {

			nextp = p->prev;	// ループの途中で死亡するかもしれないので退避
			p->month++;	// 加齢(1か月分)

			// 出産
			SEX s = roulette(2);
			if (p->pregnant_month > 0 && p->month - p->pregnant_month >= BIRTH_MONTH) {
				double brate;
#if DNAversion
				if ((opts.B_both >= 0.0) & (p->gvalue >= 0.5) & (p->pregnant_spouse->gvalue >= 0.5)) {
					brate = areax(p->birth_area)->BirthRate[s][AGE(p->month)] * opts.B_both;
				} else if ((opts.B_either >= 0.0) & ((p->gvalue >= 0.5) || (p->pregnant_spouse->gvalue >= 0.5))) {
					brate = areax(p->birth_area)->BirthRate[s][AGE(p->month)] * opts.B_either;
				} else {
					brate = areax(p->birth_area)->BirthRate[s][AGE(p->month)];
					if ((opts.B_father >= 0.0) & (p->pregnant_spouse->gvalue >= 0.5)) {
						brate = brate * opts.B_father;
					}
					if ((opts.B_mother >= 0.0) & (p->gvalue >= 0.5)) {
						brate = brate * opts.B_mother;
					}
				}
#else
				brate = areax(p->birth_area)->BirthRate[s][AGE(p->month)];
#endif
				// debug start
				printf("Birth rate for id:%lld, area:%d, age:%d, rate:%f\n", p->id, p->area, p->month/12, brate);
				// debug end

				if (probability(brate)) {
					PERSON *baby;

					// debug start
					printf("Birth! for id:%lld, area:%d, age:%d\n", p->id, p->area, p->month/12);
					// debug end


					// 子の処理
					baby = create_person(p->area, s, 0, year);
					baby->mater = p;
					baby->pater = p->pregnant_spouse;
					birth_relpointer(baby, p, p->pregnant_spouse, max_degree); // 引数(子ども, 母親, 父親, 3)

					add_history(baby, year, m, birth, p, 0LL);

#if DNAversion
					inherit_gvalue(baby, opts.Genes_model); // gvalue の継承(DNAversion)
#endif
					// 母親の処理
					add_child(p, baby);
					add_history(p, year, m, delivery, baby, 0LL);

					// 父親の処理
					if (!p->pregnant_spouse->dead) {
						add_child(p->pregnant_spouse, baby);
						add_history(p->pregnant_spouse, year, m, delivery, baby, 0LL);
					}

					// 地域の処理
					(void)create_resident(baby->area, baby);
					inc_pop(areax(baby->area), baby->sex, baby->month);
					areax(baby->area)->birth_count_in_year++;


				}
				p->pregnant_month = 0;
				p->pregnant_spouse = NULL;

			}

			// 1年ごとの処理(誕生月)
			if (p->month % TERM_IN_YEAR == 0) {

				// 死亡
				if (probability(GetMortalityRate(p)) || AGE(p->month) >= MAX_AGE) {
					// if (p->id == ID) {
						// printf("(death:%lld:", p->id);
						//fflush(stdout);
					// }

					// 地域の処理
					purge_resident_from_person(p);
					dec_pop(areax(p->area), p->sex, p->month - 1);
					areax(p->area)->death_count_in_year++;

					// 本人の処理
					p->dead = true;
					bury_person(p);
					death_relpointer(p);
					add_history(p, year, m, death, NULL, 0LL);

					// 配偶者の処理 
					bereavement(p, opts);

					// master_best の処理  for sim7-6
					set_new_master_best(p->area, p);

					if(SIM & opts.exam2_flag) { // -x2 オプション
					  //   if(p->sex == female){ // sim8.1-3 男女とも表示する
					    print_marriage_childinfo(p);
					    //	  }
					}
					
					if (SIM && opts.history_flag) {	// !!-hオプション
						print_ahistory(p);	// 死んだ人の情報を表示
						if (opts.family_degree)
							person(p, opts.family_degree); // 家族情報を表示
						}
					if (opts.tomb_flag) {	// -tオプション
						  print_deadperson(p);
					}
					/* パージはまとめてやることに．
					if (opts.purge_interval > 0) {
					  // n親等以内の親戚が生存していなければ
					  if (isAlone(p)) {
					    // printf("!");
					    purge_relpointer(p);
					    (void)purge_person(p);
					  }
					}
					*/
						// printf("end:next->%lld)\n", nextp->id);
				} else {
						// 妊娠
						if (p->sex == female && p->married &&
							AGE(p->month) >= PREGNANT_AGE_MIN && AGE(p->month) <= PREGNANT_AGE_MAX &&
							p->spouse != NULL && probability(PREGNANT_RACIO)) {
							p->pregnant_month = p->month;
							p->pregnant_spouse = p->spouse;
							add_history(p, year, m, pregnancy, p->pregnant_spouse, 0LL);
						}

						// 年齢層またぎ処理
						if (p->month % (TERM_IN_YEAR * YEAR_IN_LAYER) == 0)
							raise10(areax(p->area), p->sex, p->month);
						if (p->month % TERM_IN_YEAR == 0)
							raise(areax(p->area), p->sex, p->month);

						// 移動
#if DNAversion
						if (opts.move_by_gvalue_flag) {
							if (SIM && AGE(p->month) >= MOVABLE_AGE &&
									areax(p->area)->out_routes > 0 &&
									(opts.unmarried_flag ? (!p->married && AGE(p->month) >= 15 && AGE(p->month) <= 40 ? true : false) : true) &&
									(opts.sex != 2 ? (opts.sex == p->sex ? true : false) : true)) {
								ROUTE *rt;
								int member;
								double rate;

								member = 1;
								if (probability(opts.with_rate)) {
									// 同じ地域に住む親戚の数を加算(一緒に移動してもキャパシティを越えないかを検査するため)
									for (RELPOINTER *res = p->relpointer.next; !res->dummy; res = res->next) {
										if (!res->dead &&
												res->person->area == p->area && res->degree <= opts.family) {
											++member;
										}
									}
								}

								rt = direction(p, opts.relation, member);	// 行き先の決定
								rate = areax(p->area)->MoveRate;	// 設定されている移動率

						if (rt != NULL) {	// 行き先があるなら
							double gvalue_here;
							double gvalue_there;

							gvalue_here = get_last_gvalue_means(areax(p->area));
							gvalue_there = get_last_gvalue_means(areax(rt->link->to));	// これあってる?
							if (fabs(gvalue_here - gvalue_there) >= opts.M_differ) {	// もし，gvalueの差が指定以上であったら
								rate = rate * opts.M_scale;
							}

							if (probability(rate)) {	// 確率で移動することになったら
								p->move++;
								moving(rt, p);
								add_history(p, year, m, move, rt->link, p->id);

							        // master_best の処理  for sim7-6
							        set_new_master_best(rt->link->from, p);
							        set_master_best(rt->link->to, p);

								if (member != 1) {
									p->moveWith++;

									// 同じ地域に住む親戚もいっしょに移動 これって必ずいっしょに移動になってない?
									long long int pid = 0LL;
									for (RELPOINTER *r = p->relpointer.next; !r->dummy; r = r->next) {
										if (r->person->id != pid) {
											if (!r->dead &&
												r->person->area == rt->link->from && r->degree <= opts.family) {
												r->person->move++;
												r->person->moveWith++;

												moving(rt, r->person);
												add_history(r->person, year, m, move, rt->link, p->id);

												// master_best の処理  for sim7-6
												set_new_master_best(rt->link->from, r->person);
												set_master_best(rt->link->to, r->person);
											}
										}
										pid = r->person->id;
									}
								}
							}	// 実際の移動処理終わり
						}
					    }
				} else // 従来通り
#endif
					if (SIM && AGE(p->month) >= MOVABLE_AGE &&
						probability(areax(p->area)->MoveRate) &&	// 成人移動発生率
						areax(p->area)->out_routes > 0 &&
						(opts.unmarried_flag ? (!p->married && AGE(p->month) >= 15 && AGE(p->month) <= 40 ? true : false) : true)
						&& (opts.sex != 2 ? (opts.sex == p->sex ? true:false) : true)) {

						ROUTE *rt;
						int member;

						//printf("(move:%lld", p->id);
						//fflush(stdout);
						// printf("%lld(%d)[m(%d),AGE(%d),sex(%d)]\n", p->id, opts.unmarried_flag, p->married, AGE(p->month), p->sex);
						// if (opts.sex != 2 && p->sex != opts.sex)
						// 	printf("%lld[sex(%d)]\n", p->id, p->sex);

						member = 1;
						if (probability(opts.with_rate)) {
							// 同じ地域に住む親戚の数を加算
							// for (RESIDENT *res = areax(p->area)->resident.next; !res->dummy; res = res->next) [
							for (RELPOINTER *res = p->relpointer.next; !res->dummy; res = res->next) {
								if (!res->dead 
										&& res->person->area == p->area && res->degree <= opts.family) {
								// if (isRelative(p, res->person, opts.family)) [
									++member;
								}
							}
						}

						if ((rt = direction(p, opts.relation, member)) != NULL) {
							p->move++;
							moving(rt, p);
							add_history(p, year, m, move, rt->link, p->id);

							printf("move members = %d\n", member); // debug
							if (member != 1) {
								p->moveWith++;

								// 同じ地域に住む親戚もいっしょに移動
								long long int pid = 0LL;
								for (RELPOINTER *r = p->relpointer.next; !r->dummy; r = r->next) {
									if (r->person->id != pid) {
										if (!r->dead && r->person->area == rt->link->from && r->degree <= opts.family) {
											r->person->move++;
											r->person->moveWith++;

											moving(rt, r->person);
											add_history(r->person, year, m, move, rt->link, p->id);
										}
									}
									pid = r->person->id;
								}
							}
						}
						// printf(":end:next->%lld)\n", nextp->id);
					}	// 移動の処理ここまで
#if DNAversion
					// DNAversionで-Mが指定されていたらelseからここまでは実行されないはず
#endif
					// 技量
				        if(opts.skill_e){ // sim7-6 original
					  int ageinit = TERM_IN_YEAR * opts.skill_i;
					  int ageup = TERM_IN_YEAR * opts.skill_u;
					  if(p->month == ageinit){ 
					    PERSON *master;
					    switch(opts.skill_E){
					    case 2: // 地域で3親等以内
					      //					      printf("case2\n");
					      master = master_relative(p->area, p, opts.skill_k);
					      break;
					    case 3: // 地域で一番
					      //					      printf("case3\n");
					      master = master_best(p->area, opts.skill_i);
					      break;
					    case 4: // 両親の内からランダム　sim8-2
					      master = master_parent_random(p);
					      break;
					    case 5: // 地域で3親等以内でランダム
					      master = master_relative_random(p, opts.skill_k);
					      break;
					      //					      printf("case1\n");
					    default:
					      master = master_random(opts, p->area, p);
					      break;
					    }
					    if (master != NULL){
					      p->skill[0] = master->skill[0];
					    }else{ // 念のため．ここいはこないはず．
					      printf("no master! p-skill = %f\n", p->skill[0]);
					      p->skill[0] = 0.0; 
					    }
					  }
					  if(p->month == ageup){ // スキルアップ処理．一回きり．デフォルトは40歳の時
					    p->skill[0] *= 1.1;
					    set_master_best(p->area, p);
					  }

					}else{ // sim7-5
					  if (p->month >= (TERM_IN_YEAR * opts.skill_y)) {
					    if (p->month % (TERM_IN_YEAR * opts.skill_z) == 0) {
					      p->skill[0] *= opts.skill_b;
					    }
					  } else {
					    if (p->month % (TERM_IN_YEAR * opts.skill_x) == 0) {
					      p->skill[0] *= opts.skill_a;
					    }
					  }
					  if (p->month % (TERM_IN_YEAR * opts.skill_x1) == 0) {
					    PERSON *master;

					    if ((master = master_encounter(p->area, p)) != NULL) {
					      if (opts.skill_s) {
						if (p->skill[0] < master->skill[0] * opts.skill_d)
						  p->skill[0] = master->skill[0] * opts.skill_d;
					      }
					      else
						p->skill[0] += master->skill[0] * opts.skill_c;
					    }
					  }
					}   // 技量終わり

				// 結婚
				if (marriageable(p, opts) && AGE(p->month) >= MARRIGEABLE_AGE_MIN){

				  add_marriageReq(p, year, m, opts);

				} // 結婚処理終わり

			      }	// 死亡以外の処理ここまで

			}	// 1年ごと(誕生月)の処理ここまで
			// printf("end\n");

		}	// 人ごとの処理ここまで

		if (SIM && slow && opts.exam_flag)
			printf("%d month\n", m);

		//一月分一気にまとめて結婚
		doMarriage(opts, year, m);
		
	}	// 月ごとの処理ここまで

	if (opts.purge_interval > 0 && year % opts.purge_interval == 0)
		purge_alone_dead_person(opts.exam_flag);

	//	if (!opts.last_flag && !opts.history_flag && SIM)	// !!
	if (!opts.last_flag && SIM)	// !! 技量の出力のために変更
		opts.age_layer ? print_population10(opts.grph_flag) : print_population(opts.grph_flag);

	// 人口格納
	if (SIM && opts.history_flag) {
		for (int j = 0; j < n_area; j++) {
			areas[j].pop[year + 1] = areas[j].area_population;	// !!
			if (areas[j].max_pop < areas[j].area_population)
				areas[j].max_pop = areas[j].area_population;	// ###
		}
	}

	// #### キャパ超え時、移動・死亡率引き上げ###
	for (int i = 0; i < n_area; i++) {
		if (areas[i].area_population >= areas[i].capacity && !areas[i].FoodCrisis) {	// 人口がキャパを超えたら
			areas[i].FoodCrisis = true;	// 食糧危機
			areas[i].MoveRate = areas[i].MoveRate * areas[i].GrowthMoveRate;

			for (int j = 0; j < 2; j++) {	// updated 5-1
				for (int k = 0; k < MAX_AGE + 1; k++) {
					areas[i].MortalityRate[j][k] = areas[i].MortalityRate[j][k] * areas[i].GrowthMortalityRate;
				if (areas[i].MortalityRate[j][k] >= 1.0)
					areas[i].MortalityRate[j][k] = 1.0;
				}
			}

		} else if (areas[i].FoodCrisis && areas[i].area_population <= areas[i].capacity * areas[i].SafeLine) {	// 7割まで落ちたら元に戻す
			for (int j = 0; j < 2; j++) {	// updated 5-1
				memcpy(areas[i].MortalityRate[j], areas[i].initMortalityRate[j], sizeof(double) * (MAX_AGE + 1));
			}
			areas[i].MoveRate = areas[i].MoveRate / areas[i].GrowthMoveRate;
			areas[i].FoodCrisis = false;
		}
	}

	// #### 安定期に入るまで移動制限 ###
	/*if (!SIM) {
		for (int i = 0; i < n_area; i++) {
			if (areas[i].area_population <= 300) {
				SIM = true;
			} else {
				SIM = false;
				break;
			}
		}
	}*/
	return count_all_resident();
}

// 2022version. 新しいdirectionのための構造体. route のwrapper.
typedef struct w_route W_ROUTE;
struct w_route {
	W_ROUTE *prev;
	W_ROUTE *next;
	bool dummy;
	ROUTE *rt;
	double prob;
};

// 2022version. 新しいdirectionのための関数. route のwrapperを作成する関数(単に作成するだけ）.
W_ROUTE* create_w_route (ROUTE *rt, double p){
  W_ROUTE *wr;

  	if ((wr = MALLOC(sizeof(W_ROUTE))) == NULL)
		error("Not enough memory. (W_ROUTE)");

	wr->rt = rt;
	wr->prob = p;
	wr->prev = wr;
	wr->next = wr;
	if (rt == NULL){
	  wr->dummy = true;
	}else{
	  wr->dummy = false;
	}
	
	return wr;
}

// 2022version. 新しいdirectionのための関数. route のwrapperlistにW_ROUTEを追加する関数．
// st で始まるリストにcurを追加する．追加する時にprobの値が大きい順になるようにする．
W_ROUTE* add_w_route (W_ROUTE *cur, W_ROUTE *st){
  W_ROUTE *wp;
  for (wp = st->next; !wp->dummy; wp = wp->next){
      if (cur->prob > wp->prob){ //wpの直前にcurを入れる
	cur->prev = wp->prev;
	cur->next = wp;
	wp->prev = cur;
	return st;
      }
    }
 // ここにきたということは，リストにまだ何も入っていないかあるいはcurのprobが一番小さい
 // どちらにしても curはstの直前に入れる
	cur->prev = st->prev;
	cur->next = st;
	st->prev = cur;
	if (st->next == st){//リストにまだ何も入っていない時
	  st->next = cur;
	}
	return st;
}
// 2022version. 新しいdirectionのための関数. 役目を終えたwrapperlistのメモリの解放．
void free_w_route (W_ROUTE *st){
  W_ROUTE *wp, *freep;
  wp = st->next;
  while(!wp->dummy){
    freep = wp;
    wp = wp->next;
    free(freep);
  }
  free(st);
  return;
}

// 移動先の決定
ROUTE *direction(const PERSON *p, int degree, int member)
{
	ROUTE *rt;
	AREA *a;
	int i, t, f;
	W_ROUTE *relative, *nonrelative, *wp;

	a = areax(p->area);
	i = 0;	
	t = 0;
	f = 0;

	//W_ROUTEの初期化
	relative = create_w_route(NULL,0);
	nonrelative = create_w_route(NULL,0);
	
	for (rt = a->go_to_list.next; !rt->dummy; rt = rt->next) {
		AREA *to_area = areax(rt->link->to);

		if (!to_area->FoodCrisis && (to_area->capacity >= to_area->area_population + member)) {	// memberは一緒に移動する家族の人数（本人含む）update in sim5-4

		  if(rt->link->prob > 0){// 確率が0の地域はそもそもリストに載せない
		    wp = create_w_route(rt, rt->link->prob); //まずwrapperを作成
		    if (hasRelative(p, rt->link->to, degree)) {
			  //rt->relation = true; 
			  //++t; //true listにareaを入力
		      //親戚がいるリストに追加　ここでコスト順になるよう追加
		      relative = add_w_route(wp, relative);
		    } else {
			  //rt->relation = false;
			  //++f; //false listにareaを入力
			  //こちらは親戚いないリスト　ここでコスト順になるよう追加
		      nonrelative = add_w_route(wp, nonrelative);
		    }
		  } else {//確率が0のとき
		  }
		}
	} // end of for
	
	//	if (t + f == 0) {
	//	clearRelationFlag(a);
	//	return NULL;
	//	}
		//まず、true listに地域があれば、その中から地域を決定する（prob値　高い順から）
		//true listに地域がなければ、false listの中から地域を決定する（prob値　高い順から）
	if(relative->next != relative){//親戚いるリストが空ではないとき
	  wp = relative;
	}else if (nonrelative->next != nonrelative){//親戚いるリストが空で親戚いないリストは空でないとき
	  wp = nonrelative;
	}else{//どちらも空だった！つまり行ける候補地がない．
	  free_w_route(relative);
	  free_w_route(nonrelative);
	  return NULL;
	}
	
	while (i == 0) {
	  for (wp = wp->next; !wp->dummy; wp = wp->next) {
	    if (probability(wp->prob)) { // 確率のチェックに合格するまで、永遠にループする
	      ++i;
	      break;
	    }
	    else {
	    }
	  }
	}
	    

	//	  for (rt = a->go_to_list.next; !rt->dummy; rt = rt->next) {
	//			//replace rt = a->go_to_list.next　with true list / false list
	//			AREA *to_area = areax(rt->link->to);
	//						
	//			if (!to_area->FoodCrisis && (to_area->capacity >= to_area->area_population + member && (t == 0 || rt->relation))) {	// update in sim5-4
	//			
	//			if (probability(rt->link->prob)) { // 確率のチェックに合格するまで、永遠にループする
	//				++i;
	//					break;
	//				}
	//				else {
	//				}
	//		}
	//}
	//}
	//clearRelationFlag(a);

	//	if (rt->dummy)
	//	error("Invalid route (internal error).");

	// メモリ解放処理
	rt = wp->rt;
	free_w_route(relative);
	free_w_route(nonrelative);
	return rt;
}

// 移動の処理
void moving(ROUTE *rt, PERSON *p)
{
	// printf("@moving");
	// fflush(stdout);	// debug
	ROUTE *q;

	rt->count_in_year++;

	for (q = areax(rt->link->to)->come_from_list.next; !q->dummy; q = q->next) {
		if (q->link->from == p->area)
			break;
	}
	if (q->dummy)
		error("Invalid link (in/out mismatch) (moving).");
	q->count_in_year++;

	purge_resident_from_person(p);

	dec_pop(areax(p->area), p->sex, p->month);

	p->area = rt->link->to;

	(void)create_resident(p->area, p);
	inc_pop(areax(p->area), p->sex, p->month);

	// printf("@\n");
	// fflush(stdout);
}

#if DNAversion
// 文化スキル考慮はDNAversionでのみ有効
static PERSON *add_marriageReq(PERSON *p, int year, int m, FLAGS opts){
   // 文化スキルの高い順になるようにリストを生成

   LIBERI *l, *lp;

   if ((l = MALLOC(sizeof(LIBERI))) == NULL)
     error("Not enough memory. (LIBERI)");

   l->dummy = false;
   l->p = p;


   for(lp = marriageReq_list.next; lp->dummy == false; lp = lp->next){
     if(lp->p->skill[0] < p->skill[0]) break; 
   }
   // ここでlpのprevにpを入れればいいはず．

   l->prev = lp->prev;
   l->next = lp;
   lp->prev->next = l;
   lp->prev = l;

   return p;
 }

static void doMarriage(FLAGS opts, int year, int m) {
   // 一挙に結婚処理をする．
   LIBERI *lp;
   PERSON *p;
   PERSON *mate;
   int unmarriedMale[MAX_AREA+1];
   int unmarriedFemale[MAX_AREA+1];
   char unmarriedMen[MAX_AREA+1][1024]; // 乱数で結局結婚しなかった人の表示用
   char unmarriedWomen[MAX_AREA+1][1024]; // 乱数で結局結婚しなかった人の表示用

   //debug
   int count;

   // 初期化
   for(int i=0; i< MAX_AREA+1; i++){
     unmarriedMale[i] = 0;
     unmarriedFemale[i] = 0;
   }


   count = 0;
   for(lp = marriageReq_list.next; lp->dummy != true; lp = lp->next) { // 結婚候補者リストを表示(これは地域関係なし）
     p = lp->p;
     if(p->sex == male)
       printf("doMarriage_list: No. %d, %lld, male, area:%d, age:%d, skill:%f \n", count, p->id, p->area, p->month/12, p->skill[0]); //info
     else
       printf("doMarriage_list: No. %d, %lld, female, area:%d, age:%d, skill:%f \n", count, p->id, p->area, p->month/12, p->skill[0]); //info
     count++;
   }

   for(int i=0; i< MAX_AREA+1; i++){
     unmarriedMen[i][0] = '\0';
     unmarriedWomen[i][0] = '\0';
   }
   // debug end
   
   for(lp = marriageReq_list.next; lp->dummy != true; lp = lp->next) { //結婚候補者リストを最初から見ていく(地域関係なし）
     p = lp->p;
     if(marriageable(p, opts)){ // このループの中で他の人の相手としてすでに結婚した可能性があるので
	 if (((mate = encounter(p->area, p->sex, opts, p)) != NULL) && probability(ENCOUNTER_RACIO)) {

	   if((opts.A >=1) && (opts.A <= 4)){ // opts.Aが1から4の時(条件を増やす時はここの値が増える)
	     if(p->area != mate->area){//住んでいる地域が違ったら
	       PERSON *mp, *mpmate;
	       ROUTE *rt;
	       switch(opts.A){
	       case 1: // 他地域に人と結婚する場合，キャパシティ率に余裕のある方に移動
		 if(get_pop_rate(areax(p->area)) > get_pop_rate(areax(mate->area))){//キャパシティ率の高い方が移動(mp)
		   mp = p; mpmate = mate;
		 }else{
		   mp = mate; mpmate = p;
		 }
		 break;
	       case 2: // 他地域に人と結婚する場合，キャパシティ人数に余裕のある方に移動
		 if(get_pop_under_cap(areax(p->area)) < get_pop_under_cap(areax(mate->area))){//キャパシティ余裕人数の少ない方が移動(mp)
		   mp = p; mpmate = mate;
		 }else{
		   mp = mate; mpmate = p;
		 }
		 break;
	       case 3: // 他地域に人と結婚する場合，女性が移動
		 if(p->sex == female){//女性が移動(mp)
		   mp = p; mpmate = mate;
		 }else{
		   mp = mate; mpmate = p;
		 }
		 break;
	       case 4: // 他地域に人と結婚する場合，男性が移動
		 if(p->sex == male){//男性が移動(mp)
		   mp = p; mpmate = mate;
		 }else{
		   mp = mate; mpmate = p;
		 }
		 break;
	       default: // ここには来るはずないのだけど...
		 mp = p; mpmate = mate;
		 printf("In encounter_from_area: something wrong\n");
		 break;
	       }

	       //  移動
	       // debug start
	       if(mp->sex == male)
		 printf("Move by Marriage: male ");
	       else
		 printf("Move by Marriage: female ");
	       printf("id:%lld, mpmaid:%lld, from:%d, to:%d\n", mp->id, mpmate->id, mp->area, mpmate->area);
	       // debug end
	       rt = get_Go_Route(areax(mp->area), mpmate->area);
	       mp->move++;
	       moving(rt, mp);
	       add_history(mp, year, m, move, rt->link, mp->id);
	     }
	   }
	   espousal(p, mate, year * TERM_IN_YEAR + m, 0);
	   marriage_relpointer(p, mate);
	   add_history(p, year, m, marriage, mate, 0LL);
	   add_history(mate, year, m, marriage, p, 0LL);
	 }else{ // debug　相手がいない，もしくは確率で結婚しなかった時の処理
	   char buf[32];

	   sprintf(buf, "%lld ", p->id);
	   if(p->sex == male) {
	     unmarriedMale[p->area]++;
	     strcat(unmarriedMen[p->area], buf);
	   }else{
	     unmarriedFemale[p->area]++;
	     strcat(unmarriedWomen[p->area], buf);
	   }
	 }
       } // if marriageable

   }

   // clean marriageReq
   for(lp = marriageReq_list.next; lp->dummy != true; ){
     LIBERI *ld;

     ld = lp;
     lp = lp->next;
     free(ld);
   }

   marriageReq_list.next = &marriageReq_list;
   marriageReq_list.prev = &marriageReq_list;
   marriageReq_list.dummy = true;
   marriageReq_list.p = NULL;

   // print debug
   //   printf("MarriageReq: 0 unmarriaed male %d, unmarried female %d\n", unmarriedMale[0], unmarriedFemale[0]); // エリア番号は1から
   /* 結婚しなかった人のリストを出したい時はここをコメントから外すこと*/
   /*   for(int i=1; i< n_area+1; i++){
     printf("MarriageReq: area %d unmarriaed male %d, unmarried female %d\n", i, unmarriedMale[i], unmarriedFemale[i]);
   }
   */
   for(int i=1; i< n_area+1; i++){
     printf("MarriageReq: no marriagePerson Male at area %d, %s\n", i, unmarriedMen[i]);
     printf("MarriageReq: no marriagePerson Female at area %d, %s\n", i, unmarriedWomen[i]);
   }

}



 
PERSON *encounter(int code, SEX s, FLAGS opts, PERSON *me)
{
	PERSON *p;
	PERSON *candidate, *new_candidate;

	// debug
	if(me->sex == male) printf("Encounter: male ");
	else printf("Encounter: female ");
	printf("id = %lld, area:%d, age:%d\n", me->id, me->area, me->month/12); //info

	if(marriageable(me, opts) == false) printf("id = %lld is not marriageable\n", me->id);
	// debug end

	//	printf("me->gvalue=%f\n", me->gvalue);
	candidate = NULL;
	for (RESIDENT *res = areax(code)->resident.prev; !res->dummy; res = res->prev) {// 自分の地域での候補者
	  p = res->person;
	  if (p->sex != s && marriageable(p, opts) && AGE(p->month) >= MARRIGEABLE_AGE_MIN) {
	    if ((opts.marrige_limit ? AGE(p->sex == female ? p->month : me->month) <= MARRIGEABLE_AGE_MAX : abs(p->month - me->month) <= AGE_DIFF_MAX * TERM_IN_YEAR) && !isRelative2(me, p, opts.incest)) 	{// 血族以外と結婚
	      if (candidate == NULL){
		if (fabs(me->gvalue - p->gvalue) <= opts.W_differ) {
		  //	printf("\npartner->gvalue=%f!!!\n", p->gvalue);
		  candidate = p;
		}
	      }else{ 
		if (fabs(me->gvalue - p->gvalue) <= opts.W_differ) {
		  //	printf("\npartner->gvalue=%f!!!\n", p->gvalue);
		  new_candidate = p;
		  if((opts.plural_ma == 1) && (new_candidate->skill[0] > candidate->skill[0])){
		    //printf("Marriage candidate changed from %f to %f\n", candidate->skill[0], new_candidate->skill[0]);
		    candidate = new_candidate;
		  }
		}
	      }
	    }
	  }
	} //for 文の最後

	if(opts.A != 0){ //sim9-1
	  candidate = encoutner_from_area(code, s, opts, me, candidate);
	}

	// debug
	if (candidate != NULL) {
	  //printf("\npartner->gvalue=%f!!!, skill =%f!!!", candidate->gvalue, candidate->skill[0]);
	  if(candidate->sex == male)
	    printf("Partner: id = %lld, male, area:%d, age:%d\n", candidate->id, candidate->area, candidate->month/12); //info
	  else
	    printf("Partner: id = %lld, female, area:%d, age:%d\n", candidate->id, candidate->area, candidate->month/12); //info

		if (probability(opts.W_probability))
			return candidate;
		//	else
		//printf("but not married.\n");
	} // debug end
	return NULL;
}

PERSON* encoutner_from_area(int code, SEX s, FLAGS opts, PERSON* me, PERSON* org_can){ // sim9-1
  double pop_rate; // 今の人口/キャパシティ
  AREA *a;
  ROUTE* rt;
  PERSON *candidate, *p, *new_candidate;

  candidate = org_can;
  //      1) まず自分の地域のキャパシティ余裕度をだしてそれを最高キャパシティ余裕とする
  a = areax(code);
  pop_rate = get_pop_rate(a); // 余裕度というか今の人口がキャパシティに対してどれくらいかの率
  
  for (rt = a->go_to_list.next; !rt->dummy; rt = rt->next) {
  //      2) 近隣エリア一つ一つに付いてキャパシティ余裕度を出して今の最高キャパシティ余裕と比較
    AREA *rt_a;
    double rt_pop_rate;

    rt_a = areax(rt->link->to);
    rt_pop_rate = get_pop_rate(rt_a);

  //      3) もし近隣エリアのキャパシティ余裕度の方が高いかもしくはcandidateがNULLならこのエリアで候補者を探す．見つかればそれをcandidateとし，この地域のキャパシティ余裕度を最高キャパシティ余裕度とする．
    if((rt_pop_rate < pop_rate) || (candidate == NULL)){

      pop_rate = rt_pop_rate;

      // 近隣から探す
      for (RESIDENT *res = rt_a->resident.prev; !res->dummy; res = res->prev) {// encounter よりコピーして編集した
	p = res->person;
	if (p->sex != s && marriageable(p, opts) && AGE(p->month) >= MARRIGEABLE_AGE_MIN) {
	  if ((opts.marrige_limit ? AGE(p->sex == female ? p->month : me->month) <= MARRIGEABLE_AGE_MAX : abs(p->month - me->month) <= AGE_DIFF_MAX * TERM_IN_YEAR) && !isRelative2(me, p, opts.incest)) 	{// 血族以外と結婚
	    if (candidate == NULL){
	      if (fabs(me->gvalue - p->gvalue) <= opts.W_differ) {
		//	printf("\npartner->gvalue=%f!!!\n", p->gvalue);
		candidate = p;
	      }
	    }else{ 
	      if (fabs(me->gvalue - p->gvalue) <= opts.W_differ) {
		//	printf("\npartner->gvalue=%f!!!\n", p->gvalue);
		new_candidate = p;
		if((opts.plural_ma == 1) && (new_candidate->skill[0] > candidate->skill[0])){
		  //printf("Marriage candidate changed from %f to %f\n", candidate->skill[0], new_candidate->skill[0]);
		  candidate = new_candidate;
		  }
		}
	      }
	    }
	  }
	} //for 文の最後
    } // if文の最後
  }//      4) これを全ての近隣に付いて行う

  //　　  5) 最後のcandidate を返す．

  return candidate;
}

#else
 static PERSON *add_marriageReq(PERSON *p, int year, int m, FLAGS opts){
   // もともとのリストと同じ順序になるように．ここではリストは作らずそのまま結婚のプロセスをここで呼ぼう.
   PERSON *mate;

   if(((mate = encounter(p->area, p->sex, p)) != NULL && probability(ENCOUNTER_RACIO)) {
     espousal(p, mate, year * TERM_IN_YEAR + m, 0);
     marriage_relpointer(p, mate);
     add_history(p, year, m, marriage, mate, 0LL);
     add_history(mate, year, m, marriage, p, 0LL);
   }
 
   return p;
 }

static void doMarriage(FLAGS opts, int year, int m){
   // 内容は add_marriageReq()でやってしまったのでここではなにもしない
 }

 PERSON *encounter(int code, SEX s, PERSON *me)
{
	PERSON *p;

	// debug
	if(me->sex == male) printf("Encounter: male ");
	else printf("Encounter: female ");
	printf("max %d cur %d\n", me->spouse_max, me->spouse_curno);
	// debug end

	for (RESIDENT *res = areax(code)->resident.prev; !res->dummy; res = res->prev) {
		p = res->person;
		if (p->sex != s && marriageable(p, opts) && AGE(p->month) >= MARRIGEABLE_AGE_MIN){
		  if ((opts.marrige_limit ? AGE(p->sex == female ? p->month : me->month) <= MARRIGEABLE_AGE_MAX : abs(p->month - me->month) <= AGE_DIFF_MAX * TERM_IN_YEAR) && !isRelative2(p, me, opts.incest)) 	// 血族以外と結婚
			return p;
		}
	}
	return NULL;
}
#endif


 static PERSON *master_encounter(int code, PERSON *own)
{
	PERSON *p;

	if(own->masterRemember && own->prevMaster > 0){//前の師匠を探す
	  p = searchResident(areax(code), own->prevMaster);
	  if((p != NULL) && (p->skill[0] > own->skill[0])) return p;
	  own->prevMaster = -1;
	}

	if(own->masterSearch == 0){ //prev
	  for (RESIDENT *res = areax(code)->resident.prev; !res->dummy; res = res->prev) {
	    p = res->person;
	    if (AGE(p->month) > AGE(own->month) && p->skill[0] > own->skill[0]){
	      own->prevMaster = p->id;
	      return p;
	    }
	  }
	}else{ //next
	  for (RESIDENT *res = areax(code)->resident.next; !res->dummy; res = res->next) {
	    p = res->person;
	    if (AGE(p->month) > AGE(own->month) && p->skill[0] > own->skill[0]){
	      own->prevMaster = p->id;
	      return p;
	    }
	  }
	}


	return NULL;
}

 // sim7-6 & sim8-2 only
 void set_initial_master_best(){ // set initial master_best

   for(int i = 0; i < MAX_AREA; i++){
     master_best76[i] = NULL;
   }

   for (PERSON *p = live_list.prev; !p->dummy; p = p->prev) {
     if(master_best76[p->area] == NULL){
       master_best76[p->area] = p;
       print_master_best76(p->area); //debug
     }else{
       if(master_best76[p->area]->skill < p->skill){
	 master_best76[p->area] = p;
       print_master_best76(p->area); //debug
       }
     }
   }

 }

static PERSON *master_random(FLAGS opts, int n, PERSON *p){ 
  // 本当のランダムじゃないけど，ランダム的に
  int maxnum; // 乱数の上限 最初3000にしてたけど，そのエリアの前年の人口に連動するように
  int maxrepeat = 10; // これ，ほんと適当に決めた．いいのに当たらなかった時のリトライ回数．
  //  int maxrepeat = 1; // いいのに当たらなかった時のリトライ回数．try7用
  int i, k;
  RESIDENT *res;
  PERSON *master = NULL;
  PERSON *q;


  //  maxnum = areax(n)->area_population / 2;
  maxnum = areax(n)->area_population;
  k = roulette(maxnum);
  i = 0;

  if(opts.skill_p){//新参者順
     res = areax(n)->resident.prev; //若い順

     while((master==NULL) && (i < maxrepeat)){
       for(int j=0; j<k; ){ // k まで飛ばせ!
	 res = res->prev;  // 若い順
	 if((!res->dummy) && (res->person->month > p->month)){ 
	   j++;
	 }
       }
       if(!res->dummy){
	 q = res->person;
	 if((q->month > p->month) ){ // q->skill[0] > p->skill[0] の条件は外すことになった
	   master = q; // master 見つかった．
	 }
       }
       i++;
     }

  }else{//年寄り順
    res = areax(n)->resident.next; //年より順

    while((master==NULL) && (i < maxrepeat)){
      for(int j=0; j<k; ){ // k まで飛ばせ!
	res = res->next;  // 年寄り順
	if((!res->dummy) && (res->person->month > p->month)){ 
	  j++;
	}
      }
      if(!res->dummy){
	q = res->person;
	if((q->month > p->month) ){ // q->skill[0] > p->skill[0] の条件は外すことになった
	  master = q; // master 見つかった．
	}
      }
      i++;
    }
  }

  /*
  if(master == NULL){ // debug
    printf("skill master is NULL, k = %d, i = %d, maxnum = %d\n", k, i, maxnum);
  }
  */
  return master;
}


 static PERSON *master_relative(int n, PERSON *p, int d){
  int degree = 3; // default 3 親等以内
  PERSON *master=NULL;

  degree = d;

  // とりあえず両親を初期値にしよう
  if((p->mater != NULL) && !p->mater->dead) master = p->mater;
  if((p->pater != NULL) && !p->pater->dead){
     if (master == NULL) master = p->pater;
     else if(p->pater->skill[0] > master->skill[0]) master = p->pater;
  }

  // 初期用．もし自分のskill値が一番高かったら自分をmaster初期値に
  if(master == NULL){
    if(p->skill[0] > 0.0) master = p;
  }else if(p->skill[0] > master->skill[0]) master = p;

  // debug
  if(master->skill[0] > 0.0)
    printf("Master_Relative_C:%d year %d myid:%lld parent-id:%lld skill=%f\n", p->area, p->birthday_year + 7, p->id, master->id, master->skill[0]);

  //親戚でこれより大きな人がいたら．
  for (RELPOINTER *r = p->relpointer.next; !r->dummy; r = r->next) {
    if (!r->dead &&  r->degree <= degree && r->person->area == p->area){
      if(master == NULL){
	master = r->person;
      }else{
	if(r->person->skill[0] > master->skill[0]){
	  master = r->person;
	  // debug
	  printf("Master_Relative_C:%d year %d myid:%lld r-id:%lld skill=%f degree=%d\n", p->area, p->birthday_year + 7, p->id, r->person->id, r->person->skill[0], r->degree);

	}
      }
    }
  }

  if(master == NULL)
    printf("Master_Relative_C:%d year %d myid:%lld nomaster-id:%d skill=%f\n", p->area, p->birthday_year + 7, p->id, -1, 0.0);  else if(master->skill[0] == 0.0)
    printf("Master_Relative_C:%d year %d myid:%lld zeromaster-id:%lld skill=%f\n", p->area, p->birthday_year + 7, p->id, master->id, master->skill[0]);

  return master;
}

static RESIDENT *add_rellist(RESIDENT *rl, PERSON *pp){ // master_relative_random 用
  RESIDENT *rp;

  if ((rp = MALLOC(sizeof(RESIDENT))) == NULL)
    error("Not enough memory. (rellist-RESIDENT)");
  rp->dummy = false;
  rp->person = pp;

  rp->prev = rl->prev;
  rl->prev->next = rp;
  rp->next = rl;
  rl->prev = rp;

  return rl;
}

 static PERSON *master_relative_random(PERSON *p, int d){
  int degree = 3; // default 3 親等以内
  PERSON *master=NULL;
  RESIDENT *rellist = NULL; // 候補者リストをここに作る．
  RESIDENT *rp;
  int maxnum = 100; //2親等以内の親戚の数の上限．乱数の上限．
  int k; 

  degree = d;

  // degree 以内で同じ地域に住んでいて自分より年上の生きている親戚の中からランダムにmasterを選ぶ

  // 候補者リストの作成
  // 初期化
  if ((rellist = MALLOC(sizeof(RESIDENT))) == NULL)
    error("Not enough memory. (rellist-RESIDENT)");

  rellist->dummy = true;
  rellist->person = NULL;

  rellist->prev = rellist;
  rellist->next = rellist;

  // まず両親を入れる．
  if((p->mater != NULL) && (!p->mater->dead)){ // 母親追加
    rellist = add_rellist(rellist, p->mater);
  }
  if((p->pater != NULL) && (!p->pater->dead)){ // 父親追加
    rellist = add_rellist(rellist, p->pater);
  }

  // 条件に合う親戚追加
  for (RELPOINTER *r = p->relpointer.next; !r->dummy; r = r->next) {
    if (!r->dead &&  r->degree <= degree && r->person->area == p->area && r->person->month > p->month){
      rellist = add_rellist(rellist, r->person);
    }
  }

  // ランダム値get
  k = roulette(maxnum);

  // master設定
  rp = rellist->next;
  for(int i=0; i<k; i++){
    rp = rp->next;
    if(rp->dummy) rp = rp->next;
  }
  
  if(!rp->dummy) master = rp->person;
  else master = p; // 自分自身

  // rellist 解放
  rp = rellist->next;
  while(!rp->dummy){
    RESIDENT *op;
    op = rp;
    rp = rp->next;
    free(op);
  }
  free(rp);
    
  return master;
}


static PERSON *master_best(int n, int ageinit){//毎回計算バージョン
   //  return master_best76[n];
   int AGEinit; // sim7-6
   PERSON *p;

   AGEinit = ageinit;
   p = NULL;
   for (const RESIDENT *res = areax(n)->resident.next; !res->dummy; res = res->next) {
     if(res->person->month >= AGEinit * TERM_IN_YEAR){
       if(p == NULL){
	 p = res->person;
       }else{
	 if(res->person->skill[0] > p->skill[0]){
	   p = res->person;
	 }
       }
     }
   }
   return p;
 }

 static PERSON *set_master_best(int n, PERSON *p){ 
   if(master_best76[n]->skill[0] < p->skill[0]){
     printf("old master_best76[%d] = %4.3f\n",n, master_best76[n]->skill[0]);
     master_best76[n] = p;
     printf("new master_best76[%d] = %4.3f\n",n, master_best76[n]->skill[0]);
     print_master_best76(n); //debug
   }
   return master_best76[n];
}

 static PERSON *set_new_master_best(int n, PERSON *q){ // master_best の人がいなくなったので新たなmaster_bestを設定
   if(master_best76[n]->id == q->id){
     master_best76[n] = NULL;

     for (RESIDENT *res = areax(n)->resident.prev; !res->dummy; res = res->prev) {
       PERSON *p = res->person;

       if(master_best76[n] == NULL){
	   master_best76[n] = p;
       }else{
	 if(master_best76[n]->skill[0] < p->skill[0]){
	   master_best76[n] = p;
	 }
       }
     }
     //     printf("master_best change by %lld death\n", q->id);
   }

   //   printf("set_new_master_best ny death of %lld\n", q->id);
   print_master_best76(n); //debug
   return master_best76[n];
 }

static PERSON* master_parent_random(PERSON *p){// 両親ともいないときは，masterは自分自身．ということは0.0　としたらすキルが下がりすぎたのでそうしてない
   PERSON *master = NULL;
   double rate;

  // 初期用．自分を初期値に
   master = p;

   rate = 0.5;

   printf("mp_start\n");// debug

   //   if(((p->mater != NULL) && (!p->mater->dead)) && ((p->pater != NULL) && (!p->pater->dead))){ // 両方生きてる
   if((p->mater != NULL) && (p->pater != NULL) ){ // 両方いる
     //          if(((double)rand() < (double)RAND_MAX * rate)) { // probability(rate) を使ったらなんか0が多かったので試しに. でこの方がいいみたい．うーむ
     if(probability(rate)) { //なぜか0が多くなる．materとpaterを交換しても同じ．なんか変だよなあ
       master = p->mater;
     }else{ 
       master =  p->pater;
     }
   }else if((p->mater == NULL) || (p->mater->dead)){ 
     //     if((p->pater != NULL) && (!p->pater->dead)){
     if(p->pater != NULL){
	 master = p->pater;
     }
   }else if((p->pater == NULL) || (p->pater->dead)){
     //     if((p->mater != NULL) && (!p->mater->dead)){
     if(p->mater != NULL){
	 master =  p->mater;
     }
   }

   if(master == p) if (master->skill[0] == 0.0) printf("master==p\n");

   if((p->mater != NULL) && (p->pater != NULL)){
     if(!p->mater->dead && !p->pater->dead){
       if(p->mater->skill[0] + p->pater->skill[0] == 1.0){
	 printf("master_parent master's skillm = %f", master->skill[0]);
	 printf(" mater skill = %f", p->mater->skill[0]);
	 printf(" pater skill = %f", p->pater->skill[0]);
	 printf(" p skill = %f", p->skill[0]);
	 printf("\n");
       }
     }

     if((p->mater->dead) && (p->pater->dead)){
       printf("mp: both dead\n");
     }

   }

   return master;
   
 }

 // sim7-6 onle end


int AGE(int m)
{
	int r;

	if ((r = (m) / TERM_IN_YEAR) > MAX_AGE)
			error("MAX_AGE overflow");
	return r;
}

int AGELAYER(int m)
{
	int r;

	if ((r = (m) / (TERM_IN_YEAR * YEAR_IN_LAYER)) >= MAX_AGELAYER)
			error("MAX_AGELAYER overflow");
	return r;
}

// 親戚がいたかどうかのフラグをクリア
static void clearRelationFlag(AREA *a)
{
	for (ROUTE *rt = a->go_to_list.next; !rt->dummy; rt = rt->next)
		rt->relation = false;
}

#if DNAversion
// -x option時の value の平均と分散を全体と地域毎に標準出力に出す関数
void print_gvalue_means()
{
	double t_gvalue[MAX_AREA];
	double t_s_gvalue[MAX_AREA];
	long long int population[MAX_AREA];
	gvalueP gp[MAX_AREA];

	// counting
	for (int i = 0; i < n_area; i++) {
		t_gvalue[i] = total_gvalue(&areas[i]); // area.c
		t_s_gvalue[i] = total_square_gvalue(&areas[i]); // area.c
		population[i] = count_resident(&areas[i]); // area.c
		gp[i] = count_gvalue_pattern(&areas[i]); // area.c
	}

	// caliculationg, printing and set areas[i].last_gvalue_means
	for (int i = 0; i < n_area; i++) {
		double means = t_gvalue[i] / population[i];
		printf("area_code: %d, gvalue means,%f, gvalue varience,%f\n", areas[i].code, means,
			t_s_gvalue[i] / population[i] - means * means);
		set_last_gvalue_means(&areas[i], means);

		printf("gvalueP area_code, %d, gp.zero, %d, gp.one, %d, gp.two, %d, gp.three, %d\n",
		       areas[i].code, gp[i].zero, gp[i].one, gp[i].two, gp[i].three);
	}
}
#endif


