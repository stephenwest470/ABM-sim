/*
 * sim.h - Simulation of population moving
 *		$Id: sim.h,v 1.4 2013/02/24 17:12:26 void Exp $
 * vi: ts=4 sw=4 sts=4
 */
#define MAX_AGELAYER	13
#define TERM_IN_YEAR	12

// area.h
#define NAME_LEN    32
#define MAX_AGE     127
// area.c
#define MAX_AREA    100

#define MAX_SKILL	4

// person.c, sim.c
int AGE(int m);

// area.c sim.c
int AGELAYER(int m);
#define YEAR_IN_LAYER	10

typedef struct {
	double at_birth;	// 出産による母親の死亡率
	int dist_type;		// 初期人口年代分布多産多死型(1:ピラミッド型)縄文型 or 2:安定型(ベル型)(弥生型)
	int family;		// 一緒に移動する家族の範囲指定 (2 or 3親等)
	int incest;		// 結婚相手タブー親等指定 (デフォルト3親等)
	bool marrige_limit;	// 女性の結婚可能年齢 true:45歳まで(年齢差を考慮しない) false:最大年齢なし(年齢差のみ考慮)
	int relation;		// 移動先決定の範囲に関する家族の範囲指定(X親等)
	double with_rate;	// 家族での移動発生率
	bool age_layer;		// 人口の出力を年齢層(10年区切り)ごと
	bool grph_flag;		// 人口分布のグラフを表示
	bool keep_flag;		// 疑似乱数の種まきをしません。つねに同じ結果になります
	bool last_flag;		// 途中経過を表示せず、最初と最後の結果のみを表示
	int purge_interval;	// 近い親戚のない死亡者のデータを整理するタイミング(default 10年ごと、0を指定すると行なわない)制御
	bool tomb_flag;		// 死亡ごとにその人の情報を表示
	bool exam_flag;		// デバッグ情報出力
	bool exam2_flag;	// 女性の生むこどもの数などの情報を含むデバッグ情報出力
	bool history_flag;      // 各個人の移動履歴をファイルに書き込む
	int family_degree;	// !!ファイルに書き込む家族情報の範囲(3親等以内)
	int sex;		// 移動時の性別の指定
	bool unmarried_flag;	// 移動時の未婚の指定
	bool remarriage_flag;	// 再婚の指定(defualt ON(true)  -q optionでfalseに
#if DNAversion
	double B_father;  // for Birth_rate in the case of gvalue of father >= 0.5
	double B_mother;  // for Birth_rate in the case of gvalue of mather >= 0.5
	double B_both;    // for Birth_rate in the case of gvalue of both >= 0.5
	double B_either;  // for Birth_rate in the case of gvalue of either >= 0.5
        int Genes_model; // gvalue の遺伝のモデル[0]
	bool move_by_gvalue_flag; // gvalue による移動をするかどうか
	double M_differ;  // gvalue 平均の違い．これ以上のとき次のフラグ倍に．[0.1]
	double M_scale; // M_differ以上の違いの時移動率この値倍[0.5]
	double W_differ; // 結婚相手の許容範囲(gvalue差) default 1.0
	double W_probability;  // 許容範囲外の場合の結婚確率 default 1.0

        int A; // sim9-1以降．
  // 0: 結婚は自分の地域だけ．1: 隣接地域の人とも結婚しキャパシティ空き率の高い方に移動
  // 2: 隣接地域の人とも結婚しキャパシティ空き人数の多い方に移動
  // 3: 隣接地域の人とも結婚し男性の方へ女性が移動
  // 4: 隣接地域の人とも結婚し女性の方へ男性が移動
#endif
        int skill_i; // 師匠を最初に捜す年齢[15]
	int skill_x;  // x歳ごとに技量パラメタをa倍する。y歳以上ではz歳ごとにb倍する
	double skill_a;  // .
	int skill_y;  // .
	int skill_z;  // .
	double skill_b;  // .
	int skill_x1;  // x1 歳ごとに「師匠」を一人さがす
	double skill_c;	// i)「師匠」の技量パラメタの c 倍の値を自分の技量パラメタに上乗せする
	double skill_d;	// ii) 自分の技量パラメタを「師匠」の技量パラメタの d 倍にする
	bool skill_s;	// iの場合はfalse、iiの場合はtrue
	bool skill_e;	// false で sim7-5, true でsim7-6
	int skill_E;  // sim7-6 で 1:ランダム，2:3親等，3:地域で一番
        bool skill_p; // sim7-6 ランダムの場合のprevたどり．trueでprev, falseでnext
        int skill_u; // sim7-6 スキル上昇年齢．デフォルト40
        int skill_r; // ランダムの場合の最大値 デフォルト地域人口そのもの
        int skill_k; // skill_E = 2 の場合の親等数の指定．デフォルト3.

        bool plural_marriage; //trueで一夫多妻．falseで一夫一婦
        int  plural_max; // 一夫多妻の場合の男性の最大配偶者数(固定値の場合）デフォルト1
        double plural_unit;  // 一夫多妻の場合で最大配偶者線形変化の場合，文化スキルがこれ毎に一人増える デフォルト 0.0
  int plural_ma; //配偶者選択アルゴリズム 0:従来通り最初に見つかった人 1:文化スキルの一番高い人 デフォルト 0

} FLAGS;

long long int sim(int, bool, FLAGS, int);
void ante(FLAGS, int, char *);	// !!
void createPerson();
void post(FLAGS);
void file_init(FLAGS, int);	// !!
void prePop(FLAGS, int);
void Pop(FLAGS, int);	// !!

#if DNAversion
void print_gvalue_means(); // -x option 時
#endif

// sim7-6
void set_initial_master_best();
// sim7-6 end
