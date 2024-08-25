/*
 * relpointer.c - Simulation of population moving
 *		$Id: relpointer.c,v 1.3 2013/02/24 07:16:51 void Exp $
 * vi: ts=4 sw=4 sts=4
 */
#include <stdio.h>	// !!
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

static RELPOINTER *removeRelpointer(RELPOINTER *);
static RELPOINTER *insertRelpointer(PERSON *, PERSON *, int, bool, bool, bool, RELSTATUS);
static void notice(PERSON *, PERSON *, int, RELSTATUS);
static void noticeRel(PERSON *, RELSTATUS);
static void inherit(PERSON *, PERSON *, int);
static void merge(PERSON *, PERSON *);
static void remove_duplicate(PERSON *);
static void remove_from_list(PERSON *, PERSON *);

// RELPOINNTERのリストをすべて破棄
void purge_relpointer_list(PERSON *p)
{
	for (RELPOINTER *r = p->relpointer.next; !r->dummy; r = removeRelpointer(r))
		;
}

// 誕生
void birth_relpointer(PERSON *p, PERSON *mater, PERSON *pater, int maxDegree)
{
	inherit(p, mater, maxDegree);	// 母親の親戚を引き継ぐ
	inherit(p, pater, maxDegree);	// 父親の親戚を引き継ぐ
	remove_duplicate(p);		// 重複を取り除く
	noticeRel(p, birthNotice);	// 親戚に通知

	(void)insertRelpointer(p, mater, 1, mater->dead, false, false, materAck);	// 母親の分を追加
	(void)insertRelpointer(p, pater, 1, pater->dead, false, false, paterAck);	// 父親の分を追加

	(void)insertRelpointer(mater, p, 1, p->dead, false, false, childAck);	// 母親に追加
	(void)insertRelpointer(pater, p, 1, p->dead, false, false, childAck);	// 父親に追加

}

// 結婚
void marriage_relpointer(PERSON *f, PERSON *m)
{
	// 2人とも自分の親戚に通知
	noticeRel(f, marryNotice);
	noticeRel(m, marryNotice);

	// 互いに相手の親戚をマージ
	merge(f, m);
	merge(m, f);

	// 互いに相手を追加
	(void)insertRelpointer(f, m, 0, false, true, false, marryAck);
	(void)insertRelpointer(m, f, 0, false, true, false, marryAck);

}

// 死亡
void death_relpointer(PERSON *p)
{
	noticeRel(p, deathNotice);	// 親戚に通知
}

// 破棄
void purge_relpointer(PERSON *p)
{
	noticeRel(p, purgeNotice);	// 親戚に通知
}

// qはdegree親等以内の親戚か?
bool isRelative(const PERSON *p, const PERSON *q, int degree)
{
	for (RELPOINTER *r = p->relpointer.next; !r->dummy; r = r->next) {
		if (!r->dead && r->person == q && r->degree <= degree)
			return true;
	}
	return false;
}

// qはdegree親等以内の血族か?
bool isRelative2(const PERSON *p, const PERSON *q, int degree)
{
	for (RELPOINTER *r = p->relpointer.next; !r->dummy; r = r->next) {
		if (!r->dead && r->person == q) {
			if (r->byAffinity)
				return false;
			else if (r->degree <= degree)
				return true;
			// else printf("%lld-%lld(Aff:%d,deg:%d)\n", p->id, r->person->id, r->byAffinity, r->degree);
			break;
		}
	}
	return false;
}

// 生存している親戚がいない?
bool isAlone(const PERSON *p)
{
	for (RELPOINTER *r = p->relpointer.next; !r->dummy; r = r->next) {
		if (!r->dead)
			return false;
	}
	if (hasPregnant(p))
		return false;
	return true;
}

// 妊娠中の配偶者がいる?
bool hasPregnant(const PERSON *p)
{
  if (p->sex == male && p->spouse_curno > 0){
    for(SPONSAE *s = p->sponsae.next; !s->dummy; s = s->next){
      if(s->p->pregnant_month > 0)
	return true;
    }
  }
    return false;
}

// エリアiにn親等以内の親戚がいるかどうか?
bool hasRelative(const PERSON *p, int i, int degree)
{
	for (RELPOINTER *r = p->relpointer.next; !r->dummy; r = r->next) {
		if (!r->person->dead && r->person->area == i && r->degree <= degree)
			return true;
	}
	return false;
}

// 親戚のリストを表示
void print_relpointer(const PERSON *p)
{
	printf("\t<relpointer>\n");
	for (RELPOINTER *r = p->relpointer.next; !r->dummy; r = r->next) {
		printf("%p(%lld) [%d] %s",
			r->person,
			r->person->id,
			r->degree,
			(r->dead ? "dead" : "live"));

		if (!r->byAffinity && !r->byAffinity2)
			printf(", blood");
		else if	(r->byAffinity && !r->byAffinity2)
			printf(", Affi1");
		else if	(r->byAffinity2)
			printf(", Affi2");
		else
			printf(", ?????");

		printf(", ");
		switch (r->status) {
		case noneStat:
			printf("none\n");
			break;
		case inheritance:
			printf("両親の親戚を引き継ぐ\n");
			break;
		case childAck:
			printf("親に子を追加\n");
			break;
		case materAck:
			printf("母親を追加\n");
			break;
		case paterAck:
			printf("父親を追加\n");
			break;
		case marryAck:
			printf("配偶者を追加\n");
			break;
		case affinity:
			printf("相手の親戚をマージ\n");
			break;
		case marryNotice:
			printf("結婚通知\n");
			break;
		case birthNotice:
			printf("誕生通知\n");
			break;
		case deathNotice:
			printf("死亡通知\n");
			break;
		case purgeNotice:
			printf("破棄通知\n");
			break;
		default:
			printf("????\n");
			break;
		}
	}
}

// 親の親戚を引き継ぐ
static void inherit(PERSON *p, PERSON *parent, int maxDegree) // cf:) if (f_n > i_m) then maxDegree = n;
{
	for (RELPOINTER *r = parent->relpointer.next; !r->dummy; r = r->next) {
		// 姻族は含まず && (n-1)親等以内
	  if (!r->byAffinity && !r->dead && r->degree < maxDegree) //生きてる人のみひきつぐ
	    {
			(void)insertRelpointer(p, r->person, r->degree + 1, r->dead, r->byAffinity, r->byAffinity2, inheritance);
				// r->dead, false, false, inheritance);
	    }
	}	// !!
}

// 配偶者の親戚をマージ
static void merge(PERSON *own, PERSON *mate) // own->本人, mate->配偶者
{
	for (RELPOINTER *aff = mate->relpointer.next; !aff->dummy; aff = aff->next) {
	  if (!aff->dead && !aff->byAffinity /*!aff->byAffinity2*/) { // 生きている人のみ．また，姻族を除く. よって移動のとき，配偶者の血族の配偶者は親戚に含まれない．ただし，結婚の後結婚した配偶者の血族の配偶者は含まれることになる．ま，それでいいかな?  
			RELPOINTER *blood;

			for (blood = own->relpointer.next; !blood->dummy; blood = blood->next) {
				// 共通の親戚は新たに追加しない
				if (blood->person == aff->person || own == aff->person)
					break;
			}

			if (blood->dummy)
				(void)insertRelpointer(own, aff->person, aff->degree, aff->dead, true, aff->byAffinity2, affinity);	// aff->byAffinity2 に変更
		}
	}
}

// 重複を取り除く
static void remove_duplicate(PERSON *p)
{
	for (RELPOINTER *r = p->relpointer.next; !r->dummy; r = r->next) {
		RELPOINTER *next;

		for (RELPOINTER *s = r->next; !s->dummy; s = next) {
			next = s->next;
			if (r->person == s->person) // ここではdeadの人もアクセスしちゃうね．
				next = removeRelpointer(s);
		}
	}
}

// 死亡フラグをセット
static void dead_mark(PERSON *receive, PERSON *dead)
{
	for (RELPOINTER *r = receive->relpointer.next; !r->dummy; r = r->next) {
		if (!r->dead && r->person->id > personID()) {
			printf("Over ID.(dead) (%lld->%lld:%lld > %lld)\n", dead->id, receive->id, r->person->id, personID());
		}
		if (r->person == dead)
			r->dead = true;
	}
}


// リストから削除
static void remove_from_list(PERSON *receive, PERSON *dead)
{
	RELPOINTER *next;

	for (RELPOINTER *r = receive->relpointer.next; !r->dummy; r = next) {
		next = r->next;
		if (r->person == dead)
			next = removeRelpointer(r);
	}
}

// 親戚に通知
static void noticeRel(PERSON *from, RELSTATUS status) // from->本人
{
	for (RELPOINTER *r = from->relpointer.next; !r->dummy; r = r->next) {

		if (!r->dead && r->person->id > personID()) {
			printf("Over ID.(noticeRel) (%lld->%lld > %lld)\n", from->id, r->person->id, personID());
		}

		if (status == marryNotice) {
		  if (!r->byAffinity && !r->dead)
				notice(from, r->person, r->degree, status);
		} else // これは dead の人にも通知する
			notice(from, r->person, r->degree, status);
	}
}

// 通知を受け取る
static void notice(PERSON *from, PERSON *to, int degree, RELSTATUS status) // from->本人, to->親戚
{
	switch (status) {
	case marryNotice:	// 結婚通知
		// from本人の配偶者のみ追加
		(void)insertRelpointer(to, from->spouse, degree, false, true, true, status); // 血族の配偶者
		break;	// !!
	case birthNotice:	// 誕生通知 生きている血族にだけ
  	        if(!to->dead)
		(void)insertRelpointer(to, from, degree, false, false, false, status);
		break;
	case deathNotice: // 死亡通知
		dead_mark(to, from);
		break;
	case purgeNotice: // 破棄通知
		remove_from_list(to, from);
		break;
	default:
		break;
	}
}

// オブジェクトを生成して親族に追加
static RELPOINTER *insertRelpointer(PERSON *t, PERSON *p, int d, bool dead, bool aff, bool aff2, RELSTATUS status)
{
	RELPOINTER *relp;
	RELPOINTER *r;  
	bool insertflag = true;

	if ((relp = MALLOC(sizeof(RELPOINTER))) == NULL)
		error("Not enough memory. (RELPOINTER)");

	relp->dummy = false;
	relp->person = p;
	relp->degree = d;
	relp->dead = dead;
	relp->byAffinity = aff;
	relp->byAffinity2 = aff2;
	relp->status = status;

	// 昇順に(person->id、byAffinity、degreeについて)なるような位置に挿入
	// 同じpersonに対してはrelpointerは一つだけになるように変更
	for (r = t->relpointer.next; !r->dummy; r = r->next) {
	      if (r->person->id > relp->person->id){

		//		  新しいのを入れる
		relp->prev = r->prev;
		relp->next = r;
		r->prev->next = relp;
		r->prev = relp;
		insertflag = false;

		break;
	      }
	      else if (r->person->id == relp->person->id && r->byAffinity && !relp->byAffinity){
		//  これ，ありえへんよな．新しく来たのだけ!byAffiniyなんて．でももしこういうのが来たら前のを消して新しいのを入れる
		relp->prev = r->prev;
		relp->next = r->next;
		r->prev->next = relp;
		r->next->prev = relp;
		FREE(r);
		insertflag = false;

		break;
	      }
	      else if (r->person->id == relp->person->id && r->byAffinity == relp->byAffinity && r->degree > relp->degree){
		//		  これは前のを消して新しいのを入れる
		relp->prev = r->prev;
		relp->next = r->next;
		r->prev->next = relp;
		r->next->prev = relp;
		FREE(r);
		insertflag = false;

		break;
	      }
		else if (r->person->id == relp->person->id){
		  //  それ以外の条件で同じperson->idだったら新しいのを入れない
		  FREE(relp);
		  relp = NULL;
		  insertflag = false;

		  break;
		}
	}


	if(insertflag){//relpのperson->idが一番大きいとここに来る
	  //t のprev に挿入
		relp->prev = r->prev;
		relp->next = r;
		r->prev->next = relp;
		r->prev = relp;
		//		relp->prev = t->relpointer.prev;
		//		relp->next = &(t->relpointer);
		//		t->relpointer.prev->next = relp;
		//		t->relpointer.prev = relp;
	}

	return relp;
}

// オブジェクトを削除して破棄
RELPOINTER *removeRelpointer(RELPOINTER *relp)
{
	RELPOINTER *n;

	n = relp->next;
	relp->next->prev = relp->prev;
	relp->prev->next = relp->next;
	FREE(relp);
	return n;
}
