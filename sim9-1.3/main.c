/*
 * main.c - Simulation of population mov
 *		$Id: main.c,v 1.4 2013/02/24 17:12:26 void Exp $
 * vi: ts=4 sw=4 sts=4
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include "sim.h"
#include "main.h"
#include "rand.h"
#define DEFAULT_LOOP_YEARS	700

char SIM_VERSION[] = "sim9-1.3";
// 一夫多妻制実装版 a one-locau two-allele model
// sim8-2.8以降，スキル獲得もオプション化
// sim9-1 は結婚相手を近隣からも探すバージョン

long long int *pop;	// !!
static void usage(void);

static double diff1;
static double diff2;
static FLAGS	opts = {
	.at_birth = 0.01,
	.dist_type = 1,
	.family = 3,
	.incest = 3,
	.marrige_limit = false,
	.relation = 0,
	.with_rate = 0.5,
	.age_layer = false,
	.grph_flag = false,
	.keep_flag = false,
	.last_flag = false,
	.purge_interval = 10,
	.tomb_flag = false,
	.exam_flag = false,
	.exam2_flag = false,
	.history_flag = false,
	.family_degree = 0,
	.sex = 2,
	.unmarried_flag = false,
	.remarriage_flag = true,
#if DNAversion
	.B_father = -1.0,
	.B_mother = -1.0,
	.B_both = -1.0,
	.B_either = -1.0,
	.Genes_model = 0, // 0:両親の半分, 1:a one-locus two-allele model
	.move_by_gvalue_flag = false,
	.M_differ = 0.1,
	.M_scale = 0.5,
	.W_differ = 1.0,
	.W_probability = 1.0,

	.A = 0, // デフォルトは自分の地域からだけ結婚相手を探す
#endif
	.skill_i = 15,  // sim8-2.7以降bb．最初に師匠を捜す年齢[15]
    .skill_x = 1,	// 1歳ごとに
    .skill_a = 1.15,	// 技量パラメーターを1.3倍する
    .skill_y = 35,	// 35歳以上では
    .skill_z = 1,	// 1歳ごとに
    .skill_b = 1.01,	// 1.1倍する
    .skill_x1 = 1,	// 1歳ごとに「師匠」を一人さがす
    .skill_c = 0.003,	// i)「師匠」の技量パラメタの 0.003 倍の値を自分の技量パラメタに上乗せする
    .skill_d = 0.68,	// ii) 自分の技量パラメタを「師匠」の技量パラメタの 0.68 倍にする
    .skill_s = false,	// cを使う場合はfalse、dを使う場合はtrue
    .skill_e = false, // -Se があったら true に sim7-6
    .skill_E = 0, // 1:ランダム，2:3親等以内，3:地域で一番．4:両親のうちからランダム -Se がなければ意味がない
                  // 2,3,4 以外ではランダムになる
    .skill_p = false, // sim7-6 SE1の場合 false でnext, trueでprevたどり
    .skill_u = 40, // sim7-6 スキル上昇年齢(一度きり)
    .skill_r = 1, // sim7-6 ランダムの場合の最大値．
    .skill_k = 3, // sim7-6 skill_E = 2 の時の親等数
    .plural_marriage = false, //一夫一婦．trueで一夫多妻．
    .plural_max = 1, //男性の配偶者の最大値
    .plural_unit = 0.0, //男性配偶者数可変の場合の文化スキル単位
    .plural_ma = 0 //配偶者選択アルゴリズム0:見つかった順 1:文化スキルが高い方 DNAversionでのみ有効(sim:encounter()参照
};
    int skill_x1;  
    double skill_c; 
    double skill_d; 

//sim7-6
extern double dn_mean;
extern double dn_sigma;
//sim7-6 end

//sim7-6.12
unsigned int seed = 0;
//end sim7-6.12


static int loop_years = DEFAULT_LOOP_YEARS;

FILE *fp;
bool SIM = false;
char file[50];
char file_info[50];
char mortality[50];

int main(int argc, char **argv)
{
	clock_t t1;
	clock_t t2;
	clock_t t3;
	clock_t t4;
	double diff = 0;
	char *s;

	while (--argc > 0 && **++argv == '-') {
		for (s = argv[0] + 1; *s != '\0'; s++) {
		  printf("lopptop: *s = %c\n", *s); 
			switch (*s) {
			case 'b':
				opts.at_birth = atof(++s);
				for (; *s != '\0'; s++)
					;
				--s;
				if (opts.at_birth == 0.0)
					fprintf(stderr, "-b donot takes 0\n");
				break;
			case 'd':
				opts.dist_type = atoi(++s);
				for (; *s != '\0'; s++)
					;
				--s;
				break;
			case 'f':
				opts.family = atoi(++s);
				for (; *s != '\0'; s++)
					;
				--s;
				break;
			case 'i':
				opts.incest = atoi(++s);
				for (; *s != '\0'; s++)
					;
				--s;
				break;
			case 'm':
				opts.marrige_limit = true;
				break;
			case 'r':
				opts.relation = atoi(++s);
				for (; *s != '\0'; s++)
					;
				--s;
				break;
			case 'w':
				opts.with_rate = atof(++s);
				for (; *s != '\0'; s++)
					;
				--s;
				break;
			case 'a':
				opts.age_layer = true;
				break;
			case 'g':
				opts.grph_flag = true;
				break;
			case 'k':
				opts.keep_flag = true;
				break;
			case 'K':
			        seed = atoi(++s);
				for (; *s != '\0'; s++)
					;
				--s;
  			        break;
			case 'l':
				opts.last_flag = true;
				break;
			case 'p':
				opts.purge_interval = atoi(++s);
				for (; *s != '\0'; s++)
					;
				--s;
				break;
			case 't':
				opts.tomb_flag = true;
				break;
			case 'x':
				opts.exam_flag = true;
				++s;
				if (*s == '2') {
				  opts.exam2_flag = true;
				}else{
				  --s;
				}
				break;
			case 'h':
				opts.history_flag = true;
				++s;
				if (*s == '\0') {
					strcpy(file, "sample.csv");	// デフォルト
					strcpy(file_info, "sample(info).csv");
					// strcpy(file_info, "sample.csv");
				} else {
					sprintf(file, "%s.csv", s);
					sprintf(file_info, "%s(info).csv", s);
					// sprintf(file_info, "%s.csv", s);
				}
				for (; *s != '\0'; s++)
					;
				--s;
				// printf("file = %s\n", file);
				break;
			case 'q':
				opts.remarriage_flag = false;
				for (; *s != '\0'; s++)
					;
				--s;
				break;
			case 'o':
				opts.family_degree = atof(++s);
				for (; *s != '\0'; s++)
					;
				--s;
				break;
			case 's':
				opts.sex = atof(++s);
				for (; *s != '\0'; s++)
					;
				--s;
				break;
			case 'u':
				opts.unmarried_flag = true;
				break;
			case 'z':
				++s;
				sprintf(mortality, "%s.txt", s);
				for (; *s != '\0'; s++)
					;
				--s;
				// printf("file = %s\n", mortality);
				break;
#if DNAversion
			case 'A':
			  ++s;
			  switch(*s) {
			  case 'R':
			    opts.A = 1;
			    break;
			  case 'N':
			    opts.A = 2;
			    break;
			  case 'M':
			    opts.A = 3;
			    break;
			  case 'F':
			    opts.A = 4;
			    break;
			  }
			  for (; *s != '\0'; s++)
			    ;
			  --s;
			  break;
			case 'B':
				++s;
				switch (*s) {
				case 'f':
					opts.B_father = atof(++s);
					break;
				case 'm':
					opts.B_mother = atof(++s);
					break;
				case 'b':
					opts.B_both = atof(++s);
					break;
				case 'e':
					opts.B_either = atof(++s);
					break;
				default:
					break;
				}
				for (; *s != '\0'; s++)
					;
				--s;
				printf("f = %f, m = %f, b = %f, e = %f\n",
					opts.B_father, opts.B_mother,
					opts.B_both, opts.B_either);
				break;
			case 'G':
				opts.exam_flag = true;	// これがないとgvalue平均が計算されない
				++s;
				switch (*s) {
				case 'o':
				  opts.Genes_model = 1;
					break;
				default:
					break;
				}
				for (; *s != '\0'; s++)
					;
				--s;
				break;
			case 'M':
				opts.move_by_gvalue_flag = true;
				opts.exam_flag = true;	// これがないとgvalue平均が計算されない
				++s;
				switch (*s) {
				case 'd':
					opts.M_differ = atof(++s);
					break;
				case 's':
					opts.M_scale = atof(++s);
					break;
				default:
					break;
				}
				for (; *s != '\0'; s++)
					;
				--s;
				printf("d = %f, s = %f\n",
					opts.M_differ, opts.M_scale);
				break;
			case 'W':
				++s;
				switch (*s) {
				case 'd':
					opts.W_differ = atof(++s);
					break;
				case 'p':
					opts.W_probability = atof(++s);
					break;
				default:
					break;
				}
				for (; *s != '\0'; s++)
					;
				--s;
				break;
#endif
			case 'S':
				switch (*++s) {
				case 'i':
					opts.skill_i = atoi(++s);
					break;
				case 'x':
					opts.skill_x = atoi(++s);
					break;
				case 'a':
					opts.skill_a = atof(++s);
					break;
				case 'y':
					opts.skill_y = atoi(++s);
					break;
				case 'z':
					opts.skill_z = atoi(++s);
					break;
				case 'b':
					opts.skill_b = atof(++s);
					break;
				case 'X':
					opts.skill_x1 = atoi(++s);
					break;
				case 'c':
					opts.skill_c = atof(++s);
					break;
				case 'd':
					opts.skill_d = atof(++s);
					break;
				case 's':
					opts.skill_s = true;
					break;
				case 'e':
					opts.skill_e = true;
					break;
				case 'E':
					opts.skill_E = atoi(++s);
					break;
				case 'g':
					dn_sigma = atof(++s);
					break;
				case 'k':
					opts.skill_k = atoi(++s);
					break;
				case 'm':
					dn_mean = atof(++s);
					break;
				case 'p':
					opts.skill_p = true;
					break;
				case 'r':
					opts.skill_r = atoi(++s);
					break;
				case 'u':
					opts.skill_u = atoi(++s);
					break;
				default:
					break;
				}
				for (; *s != '\0'; s++)
					;
				--s;
				break;
			case 'P':
				++s;
				switch (*s) {
				case 'b':
					opts.plural_unit = atof(++s);
					break;
				case 'm':
					opts.plural_marriage = true;
					break;
				case 's':
					opts.plural_ma = 1;
					break;
				case 'x':
					opts.plural_max = atoi(++s);
					break;
				default:
					break;
				}
				for (; *s != '\0'; s++)
					;
				--s;
				break;
			default:
				usage();
				break;
			}
		}
	}

	if (argc == 0)
		loop_years = DEFAULT_LOOP_YEARS;
	else
		loop_years = atoi(*argv);

	if (opts.exam_flag) {
	  time_t time_v;
	  time(&time_v);
	    printf("This simulation version is %s %s", SIM_VERSION, ctime(&time_v));
		printf("simulation_period = %d\n", loop_years);
		printf("at_birth = %lg\n", opts.at_birth);
		printf("dist_type = %d\n", opts.dist_type);
		printf("family = %d\n", opts.family);
		printf("incest = %d\n", opts.incest);
		printf("marrige_limit = %s\n", opts.marrige_limit ? "true" : "false");
		printf("relation = %d\n", opts.relation);
		printf("with_rate = %lg\n", opts.with_rate);
		printf("family_degree = %d\n", opts.family_degree);	// !!
		printf("age_layer = %s\n", opts.age_layer ? "true" : "false");
		printf("grph_flag = %s\n", opts.grph_flag ? "true" : "false");
		printf("keep_flag = %s\n", opts.keep_flag ? "true" : "false");
		printf("last_flag = %s\n", opts.last_flag ? "true" : "false");
		printf("tomb_flag = %s\n", opts.tomb_flag ? "true" : "false");
		printf("exam_flag = %s\n", opts.exam_flag ? "true" : "false");
		printf("exam2_flag = %s\n", opts.exam2_flag ? "true" : "false");
		printf("history_flag = %s\n", opts.history_flag ? "true" : "false");	// !!
		printf("sex = %dn", opts.sex);	// !!
		printf("unmarrid = %s\n", opts.unmarried_flag ? "true" : "false");	// !!
		printf("genes_model = %d\n", opts.Genes_model); // sim8-2
		printf("W_differ = %f\n", opts.W_differ); 
		
		printf("skill i + %d\n", opts.skill_i);
		printf("skill x = %d\n", opts.skill_z);
		printf("skill a = %g\n", opts.skill_a);
		printf("skill y = %d\n", opts.skill_y);
		printf("skill z = %d\n", opts.skill_z);
		printf("skill b = %g\n", opts.skill_b);
		printf("skill x1 = %d\n", opts.skill_x1);
		printf("skill c = %g\n", opts.skill_c);
		printf("skill d = %g\n", opts.skill_d);
		printf("skill e = %s\n", opts.skill_e ? "true" : "false");
		printf("skill E = %d\n", opts.skill_E);
		printf("dn_mean = %f\n", dn_mean);
		printf("dn_sigma = %f\n", dn_sigma);
		printf("skill p = %s\n", opts.skill_p ? "true" : "false");
		printf("skill u = %d\n", opts.skill_u);
		printf("skill r = %d\n", opts.skill_r);
		printf("skill k = %d\n", opts.skill_k);

		printf("plural_marriage = %s\n", opts.plural_marriage ? "true" : "false");
		printf("plural_max (for male) = %d\n", opts.plural_max);
		printf("plural_unit (for male) = %f\n", opts.plural_unit);
		printf("plural_ma = %d\n", opts.plural_ma);
	}

	t1 = clock();

	seeding_default();
	ante(opts, loop_years, mortality);	// !!
	if (!opts.keep_flag)
		seeding();

	// 人口格納用の領域確保
	prePop(opts, loop_years);
	pop = (long long int *)malloc(sizeof(long long int) * (loop_years + 1));

	// for sim7-6 master_bset の初期化
	set_initial_master_best();

	int pre = 0;
	while (pre < 30) {
		// printf("*");
		//	fflush(stdout);
		pop[0] = sim(0, diff > 10.0, opts, loop_years);	// !!preLoop	// ##########
		pre++;
	}

	SIM = true;
	createPerson();

	if (opts.history_flag) {
		file_init(opts, loop_years);	// シミュレーション情報の書き込み
		fp = fopen(file, "w");		// 個人の書き込みファイルオープン
	}

	for (int y = 0; y < loop_years; y++) {
		if (SIM) {
			printf("y:%d\n", y);
			fflush(stdout);
		}
		t3 = clock();
		pop[y + 1] = sim(y, diff > 10.0, opts, loop_years);	// !!
		t4 = clock();
		diff = (double)(t4 - t3) / (double)CLOCKS_PER_SEC;
		if (opts.exam_flag) {
#if DNAversion
			printf("---%dy--- %lld (%.1lf)\n", y, pop[y + 1], diff);
			print_gvalue_means(); // sim.c 内にあり．
#else
			printf("---%dy--- %lld (%.1lf)\n", y, pop[y + 1], diff);
#endif
		}
	}

	if (opts.history_flag) {
		people(opts.family_degree);	// 生きている人の情報書き込み
		fclose(fp);

		// 総人口
		fp = fopen(file_info, "a");
		for (int y = 0; y < loop_years + 1; y++)
			fprintf(fp, "%lld,", pop[y]);
		free(pop);
		fprintf(fp, "\n");
		Pop(opts, loop_years);	// !!人口
		fclose(fp);
	}

	post(opts);
	t2 = clock();
	if (opts.exam_flag) {
		printf("time = %lg\n", ((double)(t2 - t1) / (double)CLOCKS_PER_SEC));
		printf("malloc (max) = %lf(s)\n", diff1 / (double)CLOCKS_PER_SEC);
		printf("free   (max) = %lf(s)\n", diff2 / (double)CLOCKS_PER_SEC);
	}

	exit(EXIT_SUCCESS);
}

static void usage(void)
{
	fprintf(stderr, "Usage: sim [-k] [-t] [-l] [-m] [-g] [-x] [-d n] [-i n] [-w d.d] [-r n] [-f n] [-b d.d] [-h file] [-o n] [-s n] [-u]\n version9-1");
	fprintf(stderr, "       -k: Not seeding for randomizing. (Keep)\n");
	fprintf(stderr, "       -t: print person information each after a person died. (TombStone)\n");
	fprintf(stderr, "       -l: output only after done. (LastOnly)\n");
	fprintf(stderr, "       -h: output the movement history of each individual.\n");
	fprintf(stderr, "	-m: marrige_limit = true;\n");
	fprintf(stderr, "	-g: graph\n");
	fprintf(stderr, "	-x or -x2: exam\n");
	fprintf(stderr, "	-d n: distribution type n=1:pyramid n=2:bell\n");
	fprintf(stderr, "	-i n: immarriable degree of kindship\n");
	fprintf(stderr, "	-w d.d: with_rate\n");
	fprintf(stderr, "	-r n: degree of kindship counting on moving\n");
	fprintf(stderr, "	-f n: degree of kindship at moving with\n");
	fprintf(stderr, "	-b d.d: mother's mortality ratio at birth\n");
	fprintf(stderr, "	-h file: write infomation of personal trace\n");
	fprintf(stderr, "	-o n: write infomation of family\n");
	fprintf(stderr, "	-s n: sex at moving\n");
	fprintf(stderr, "	-u: unmarried at moving\n");
	fprintf(stderr, "	-z file: MortalityRate.txt\n");
	exit(EXIT_SUCCESS);
}

void *MALLOC(size_t n)
{
	void *p;
	clock_t t1;
	clock_t t2;

	t1 = clock();
	p = malloc(n);
	t2 = clock();
	if (diff1 < (double)(t2 - t1))
		diff1 = (double)(t2 - t1);
	return p;
}

void FREE(void *p)
{
	clock_t t1;
	clock_t t2;

	t1 = clock();
	free(p);
	t2 = clock();

	if (diff2 < (double)(t2 - t1))
		diff2 = (double)(t2 - t1);
}

unsigned int getSeed(void)
{
  unsigned int t;
  t = time(NULL);
  if(seed == 0){
    printf("Seed: %u\n", t);
    return t;
  }else{
    printf("Seed: %u\n", seed);
    return seed;
  }

}

void cant(const char *filename)
{
	fprintf(stderr, "Can't open `%s'.\n", filename);
	exit(EXIT_FAILURE);
}

void error(const char *message)
{
	fprintf(stderr, "%s\n", message);
	exit(EXIT_FAILURE);
}
