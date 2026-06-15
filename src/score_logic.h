/*
 * score_logic.h — Scoring / GPA / graduation calculation
 *
 * Pure computation layer — no DB calls, no globals.
 * Ported and upgraded from the old Score_LOGIC.c.
 *
 * Included from main.c AFTER db.h so that struct_table.h is already pulled in.
 *
 * Public API
 * ──────────
 *   score_to_gpa(letter, plus)   — 4-scale GPA point for a stored grade
 *   calc_cpa(player, pass_only)  — overall CPA (0=all studied, 1=pass only)
 *   calc_cpa_type(player,t,p)    — CPA for one subject type
 *   calc_status_alert(npass)     — academic alert level (0-3)
 *   calc_effective_credits(p)    — credits toward graduation (module choice)
 *   calc_required_credits(p)     — total credits required to graduate
 *   update_player_status(p)      — recompute status_can_grauate + status_alert
 *
 *  Graduation rules come from gGradRules[] (populated by DB_LoadGradRules()
 *  seeded from assets/grad_config.cfg into the user's per-major DB).
 */

#pragma once

#include "struct_table.h"

/* ── Graduate thresholds per subject type ────────────────────────────────
 *  Index matches the index_subject_type enum in struct_table.h:
 *  [0]=co_so_nganh  [1]=dai_cuong   [2]=the_thao(by subject count)
 *  [3]=ly_luat_chinh_tri  [4]=tu_chon  [5]=thuc_tap
 *  [6]=modunI  [7]=modunII  [8]=modunIII   (pick best of I/II/III)
 *  [9]=modunIV  [10]=modunV               (pick best of IV/V)
 *  [11]=do_an_tot_nghiep
 *
 *  Two entries are fixed constants:
 *    the_thao = 4  — minimum sports *subjects* to pass (all have 0 credits)
 *    tu_chon  = 9  — minimum elective *credits* (pool is larger than required)
 *  All other entries are set at runtime by init_grad_limits() to the sum of
 *  all credits for that type from subjects.dat (via Subject_Type.Total_Credit).
 * ──────────────────────────────────────────────────────────────────────── */

/* Resolve effective limit for type i:
 *   GRAD_TOTAL_CREDIT        -> Total_Credit of that type (all credits in subjects.dat)
 *   GRAD_FIXED / GRAD_SUBJECT_COUNT -> gGradRules[i].limit_val
 */
static int _sl_resolve_limit(Player *p, int i)
{
    if (gGradRules[i].mode == GRAD_TOTAL_CREDIT)
        return (int)p->numofSubjectType[i].Total_Credit;
    return gGradRules[i].limit_val;
}
/* Resolve effective pass for type i:
 *   GRAD_SUBJECT_COUNT        -> count_passSubject it only count pass subject
 *   GRAD_FIXED / GRAD_TOTAL_CREDIT -> count_passCredit get pass credit
 */
static int _sl_resolve_pass(Player *p, int i) {
  if(gGradRules[i].mode == GRAD_SUBJECT_COUNT)
      return (int)p->numofSubjectType[i].count_passSubject;
  return (int)p->numofSubjectType[i].count_passCredit;
}

/* ── Score letter → 4-scale GPA point ───────────────────────────────────
 *  letter : base char stored in Subject_Node.score_letter ('A','B','C','D','F')
 *  plus   : non-zero if the full grade string has a '+' modifier (e.g., "A+")
 *
 *  True HUST 4.0 scale:  A+ 4.0 | A 4.0 | B+ 3.5 | B 3.0 | C+ 2.5 | C 2.0
 *                        D+ 1.5 | D 1.0 | F/X 0.0
 *  A and A+ both cap at 4.0 — the CPA can never exceed 4.0, so the honor
 *  classification bands (Normal..God of HUST, topping at 4.0) line up exactly.
 * ──────────────────────────────────────────────────────────────────────── */
static float score_to_gpa(char letter, int plus)
{
    switch (letter) {
        case 'A': return 4.0f;                 /* A and A+ both cap at 4.0 */
        case 'B': return plus ? 3.5f : 3.0f;
        case 'C': return plus ? 2.5f : 2.0f;
        case 'D': return plus ? 1.5f : 1.0f;
        default:  return 0.0f;
    }
}

/* ── Academic alert level ────────────────────────────────────────────────
 *  Based on total studied-but-NOT-passed credits (ToTal_credit_npass):
 *    0 — OK        (< 8)
 *    1 — Caution   (8 – 15)
 *    2 — Warning   (16 – 26)
 *    3 — Critical  (>= 27)
 * ──────────────────────────────────────────────────────────────────────── */
static int calc_status_alert(int credit_npass)
{
    if (credit_npass <  8) return 0;
    if (credit_npass < 16) return 1;
    if (credit_npass < 27) return 2;
    return 3;
}

/* ── Cumulative Point Average ────────────────────────────────────────────
 *  pass_only = 0 : include all subjects that were ever studied
 *  pass_only = 1 : include only subjects that were passed
 * ──────────────────────────────────────────────────────────────────────── */
static float calc_cpa(Player *p, int pass_only)
{
    float total   = 0.0f;
    int   credits = 0;
    for (int i = 0; i < sizeSubjectType; i++) {
        Subject_Node *cur = p->numofSubjectType[i].head;
        while (cur) {
            int use = pass_only ? (int)cur->status_pass
                                : (int)(cur->status_ever_been_study & 1);
            if (use) {
                int plus = (cur->status_ever_been_study & 2) != 0;
                total   += score_to_gpa(cur->score_letter, plus) * (float)cur->credit;
                credits += (int)cur->credit;
            }
            cur = cur->next;
        }
    }
    return (credits == 0) ? 0.0f : total / (float)credits;
}

/* ── CPA for a single subject type ───────────────────────────────────────
 *  Same pass_only semantics as calc_cpa().
 * ──────────────────────────────────────────────────────────────────────── */
static float calc_cpa_type(Player *p, int t, int pass_only)
{
    if (t < 0 || t >= sizeSubjectType) return 0.0f;
    float total   = 0.0f;
    int   credits = 0;
    Subject_Node *cur = p->numofSubjectType[t].head;
    while (cur) {
        int use = pass_only ? (int)cur->status_pass
                            : (int)(cur->status_ever_been_study & 1);
        if (use) {
            int plus = (cur->status_ever_been_study & 2) != 0;
            total   += score_to_gpa(cur->score_letter, plus) * (float)cur->credit;
            credits += (int)cur->credit;
        }
        cur = cur->next;
    }
    return (credits == 0) ? 0.0f : total / (float)credits;
}

/* ── Effective credits toward graduation ─────────────────────────────────
 *  Respects the module-choice rules:
 *    • modules I / II / III — only the BEST one counts
 *    • modules IV / V       — only the BEST one counts
 *    • sport (the_thao)     — counted by subject number, not credits
 * ──────────────────────────────────────────────────────────────────────── */
static int calc_effective_credits(Player *p)
{
    int count = 0;
    int done[sizeSubjectType];
    for (int k = 0; k < sizeSubjectType; k++) done[k] = 0;
    for (int i = 0; i < sizeSubjectType; i++) {
        if (done[i]) continue;
        GradRule *r = &gGradRules[i];
        if (r->mode == GRAD_SUBJECT_COUNT) { done[i] = 1; continue; }
        if (r->group_id != 0) {
            int best = 0;
            for (int j = 0; j < sizeSubjectType; j++) {
                if (gGradRules[j].group_id == r->group_id) {
                    int pc = (int)p->numofSubjectType[j].count_passCredit;
                    if (pc > best) best = pc;
                    done[j] = 1;
                }
            }
            count += best;
        } else {
            count += (int)p->numofSubjectType[i].count_passCredit;
            done[i] = 1;
        }
    }
    return count;
}

/* ── Total credits required for graduation ───────────────────────────────
 *  Sums LIMIT_FOR_GRADUATE, counting only one limit for the module groups
 *  (same skip logic used in remaining_credit from the old program).
 * ──────────────────────────────────────────────────────────────────────── */
static int calc_required_credits(Player *p)
{
    int total = 0;
    int done[sizeSubjectType];
    for (int k = 0; k < sizeSubjectType; k++) done[k] = 0;
    for (int i = 0; i < sizeSubjectType; i++) {
        if (done[i]) continue;
        GradRule *r = &gGradRules[i];
        if (r->mode == GRAD_SUBJECT_COUNT) { done[i] = 1; continue; }
        if (r->group_id != 0) {
            int lim = _sl_resolve_limit(p, i);  /* first member's limit */
            for (int j = 0; j < sizeSubjectType; j++) {
                if (gGradRules[j].group_id == r->group_id) done[j] = 1;
            }
            total += lim;
        } else {
            total += _sl_resolve_limit(p, i);
            done[i] = 1;
        }
    }
    return total;
}

/* ── Recompute status_can_grauate and status_alert on *p ─────────────────
 *  Call this after every DB_Query() to keep the player state correct.
 *
 *  Graduation criteria loaded from gGradRules[] (seeded from grad_config.cfg):
 *    • GRAD_SUBJECT_COUNT — must pass limit_val subjects of that type
 *    • group (group_id > 0) — best member's passCredits must meet its own limit
 *    • standalone — passCredits must meet its own resolved limit
 *    • CPA (all studied) >= 2.0
 * ──────────────────────────────────────────────────────────────────────── */
static void update_player_status(Player *p)
{
    /* --- Academic alert --- */
    p->status_alert = (unsigned)calc_status_alert(p->ToTal_credit_npass);

    /* --- Graduation check (data-driven via gGradRules) --- */
    p->status_can_grauate = 1;   /* assume pass; set 0 on first failure */

    int done[sizeSubjectType];
    for (int k = 0; k < sizeSubjectType; k++) done[k] = 0;

    for (int i = 0; i < sizeSubjectType; i++) {
        if (done[i]) continue;
        GradRule *r = &gGradRules[i];

        if (r->mode == GRAD_SUBJECT_COUNT) {
            /* e.g. the_thao: count subjects, not credits */
            if ((int)p->numofSubjectType[i].count_passSubject < r->limit_val) {
                p->status_can_grauate = 0;
                return;
            }
            done[i] = 1;
        } else if (r->group_id != 0) {
            /* pick-best group: the member with the most passCredits
               must reach its own limit */
            int best_pass  = -1;
            int best_limit =  0;
            for (int j = 0; j < sizeSubjectType; j++) {
                if (gGradRules[j].group_id == r->group_id) {
                    int pc  = (int)p->numofSubjectType[j].count_passCredit;
                    int lim = _sl_resolve_limit(p, j);
                    if (pc > best_pass) { best_pass = pc; best_limit = lim; }
                    done[j] = 1;
                }
            }
            if (best_pass < best_limit) {
                p->status_can_grauate = 0;
                return;
            }
        } else {
            /* standalone credit requirement */
            int lim = _sl_resolve_limit(p, i);
            if ((int)p->numofSubjectType[i].count_passCredit < lim) {
                p->status_can_grauate = 0;
                return;
            }
            done[i] = 1;
        }
    }

    /* CPA (all studied) >= 2.0 */
    if (calc_cpa(p, 0) < 2.0f) {
        p->status_can_grauate = 0;
        return;
    }
}

/* ── Detect standalone types with no subjects loaded ───────────────────────
 *  Module groups (group_id > 0) are EXEMPT: the pick-best rule means any
 *  member being present is sufficient, and some majors omit some modules.
 *  Required types {1,2,3,4,5,12,13} with Total_Subject == 0 are flagged.
 *  Module slots 6-11 are flexible and never flagged.
 *  out[i] = 1 if flagged, 0 otherwise.  Returns count of flagged types.
 * ──────────────────────────────────────────────────────────────────────── */
static const int _SL_REQUIRED[] = {1, 2, 3, 4, 5, 12, 13};
#define _SL_N_REQUIRED ((int)(sizeof(_SL_REQUIRED)/sizeof(_SL_REQUIRED[0])))
static int calc_missing_types(Player *p, int out[sizeSubjectType])
{
    int count = 0;
    for (int i = 0; i < sizeSubjectType; i++) out[i] = 0;
    for (int k = 0; k < _SL_N_REQUIRED; k++) {
        int i = _SL_REQUIRED[k];
        if ((int)p->numofSubjectType[i].Total_Subject == 0) {
            out[i] = 1;
            count++;
        }
    }
    return count;
}

/* ── Honor classification (HUST graduation tiers) ─────────────────────────
 *  Bands on the cumulative CPA (now 0..4.0 on the true 4.0 scale):
 *    < 2.0      HONOR_NONE       (not eligible to graduate)
 *    2.0–2.49   HONOR_NORMAL
 *    2.5–3.19   HONOR_GOOD
 *    3.2–3.59   HONOR_EXCELLENT
 *    3.6–4.0    HONOR_GOD        ("God of HUST")
 * ──────────────────────────────────────────────────────────────────────── */
typedef enum {
    HONOR_NONE = 0,
    HONOR_NORMAL,
    HONOR_GOOD,
    HONOR_EXCELLENT,
    HONOR_GOD
} HonorTier;

/* CPA floor required to enter each tier — index by HonorTier. */
static const float kHonorFloor[5] = { 0.0f, 2.0f, 2.5f, 3.2f, 3.6f };

static HonorTier honor_tier(float cpa)
{
    if (cpa >= kHonorFloor[HONOR_GOD])       return HONOR_GOD;
    if (cpa >= kHonorFloor[HONOR_EXCELLENT]) return HONOR_EXCELLENT;
    if (cpa >= kHonorFloor[HONOR_GOOD])      return HONOR_GOOD;
    if (cpa >= kHonorFloor[HONOR_NORMAL])    return HONOR_NORMAL;
    return HONOR_NONE;
}

static const char *honor_name(HonorTier t)
{
    switch (t) {
        case HONOR_NORMAL:    return "Normal";
        case HONOR_GOOD:      return "Good";
        case HONOR_EXCELLENT: return "Excellent";
        case HONOR_GOD:       return "God of HUST";
        default:              return "Below classification";
    }
}

/* Smallest letter grade whose GPA point meets `g` (for "you need at least L"). */
static void gpa_to_letter(float g, char *letter, int *plus)
{
    if      (g > 3.5f) { *letter = 'A'; *plus = 0; }   /* 3.5 <   .. 4.0  */
    else if (g > 3.0f) { *letter = 'B'; *plus = 1; }   /* B+ 3.5          */
    else if (g > 2.5f) { *letter = 'B'; *plus = 0; }   /* B  3.0          */
    else if (g > 2.0f) { *letter = 'C'; *plus = 1; }   /* C+ 2.5          */
    else if (g > 1.5f) { *letter = 'C'; *plus = 0; }   /* C  2.0          */
    else if (g > 1.0f) { *letter = 'D'; *plus = 1; }   /* D+ 1.5          */
    else               { *letter = 'D'; *plus = 0; }   /* D  1.0 (min pass) */
}

/* ── Graduation honor projection ─────────────────────────────────────────
 *  Forecasts the honor tier at graduation from current performance.
 *
 *  Model (a forecast, not an exact transcript recompute):
 *    eff  = calc_effective_credits(p)   credits already passed toward the degree
 *    req  = calc_required_credits(p)    credits needed to graduate
 *    R    = req - eff                   remaining credits still to earn
 *    rate = calc_cpa(p, 1)              pass-only CPA (quality of work so far)
 *    P    = rate * eff                  locked GPA points (approx — pass-only
 *                                       rate applied over effective credits)
 *  "Maintain current average" ⇒ projected final CPA = rate, so the headline
 *  tier is honor_tier(rate). Failed (F) subjects are treated as part of R
 *  (assumed retaken & replaced), not permanent 0-point anchors.
 *
 *  Reachable range given R credits left:
 *    ceiling = (P + 4.0*R) / req   (all remaining = A)
 *    floor   = (P + 1.0*R) / req   (all remaining = D, the minimum pass)
 * ──────────────────────────────────────────────────────────────────────── */
typedef struct {
    int       eff, req, remaining;  /* credits                              */
    float     rate;                 /* pass-only CPA = projected final CPA  */
    float     ceiling, floor;       /* reachable CPA range given R          */
    HonorTier projected;            /* honor_tier(rate)                     */
    HonorTier best, worst;          /* honor_tier(ceiling / floor)          */
} HonorProjection;

static HonorProjection honor_project(Player *p)
{
    HonorProjection hp;
    hp.eff       = calc_effective_credits(p);
    hp.req       = calc_required_credits(p);
    hp.remaining = hp.req - hp.eff;
    if (hp.remaining < 0) hp.remaining = 0;
    hp.rate      = calc_cpa(p, 1);
    int   req    = hp.req > 0 ? hp.req : 1;
    float P      = hp.rate * (float)hp.eff;
    hp.ceiling   = (P + 4.0f * (float)hp.remaining) / (float)req;
    hp.floor     = (P + 1.0f * (float)hp.remaining) / (float)req;
    if (hp.ceiling > 4.0f) hp.ceiling = 4.0f;
    if (hp.floor   < 0.0f) hp.floor   = 0.0f;
    hp.projected = honor_tier(hp.rate);
    hp.best      = honor_tier(hp.ceiling);
    hp.worst     = honor_tier(hp.floor);
    return hp;
}

/* ── Target feasibility for a chosen honor tier ───────────────────────────
 *  Given a projection and a desired tier, what average must the remaining R
 *  credits earn?   needed = (T*req - P) / R   where T = tier's CPA floor.
 *    needed <= 1.0  → SECURED  (any passing grades reach it)
 *    needed >  4.0  → IMPOSSIBLE (even all-A falls short)
 *    otherwise      → REACHABLE at grade `need_letter`(+)
 * ──────────────────────────────────────────────────────────────────────── */
typedef enum {
    TARGET_SECURED = 0,
    TARGET_REACHABLE,
    TARGET_IMPOSSIBLE
} TargetStatus;

typedef struct {
    TargetStatus status;
    float        needed_avg;   /* required average GPA over remaining credits */
    char         need_letter;  /* smallest letter meeting needed_avg          */
    int          need_plus;    /* whether that letter carries a '+'           */
} TargetPlan;

/* Ambition within a chosen tier's band:
 *   FLEX_LOW  → just cross the floor (e.g. Good = 2.50)
 *   FLEX_MED  → mid-band
 *   FLEX_HIGH → top of the band ("every possible" without bumping to the next tier)
 */
typedef enum { FLEX_LOW = 0, FLEX_MED = 1, FLEX_HIGH = 2 } FlexLevel;

/* Top of a tier's band: just under the next tier's floor (God caps at 4.0). */
static float honor_tier_top(HonorTier t)
{
    if (t >= HONOR_GOD) return 4.0f;
    return kHonorFloor[t + 1] - 0.01f;
}

/* Target CPA for a tier at a given ambition level. */
static float honor_flex_target(HonorTier tier, FlexLevel flex)
{
    float lo = kHonorFloor[tier];
    float hi = honor_tier_top(tier);
    switch (flex) {
        case FLEX_HIGH: return hi;
        case FLEX_MED:  return (lo + hi) * 0.5f;
        default:        return lo;
    }
}

/* Core feasibility math against an explicit target CPA `T`. */
static TargetPlan honor_target_plan_at(const HonorProjection *hp, float T)
{
    TargetPlan tp;

    if (hp->remaining <= 0) {
        /* nothing left to change — the current rate decides it */
        tp.status     = (hp->rate >= T) ? TARGET_SECURED : TARGET_IMPOSSIBLE;
        tp.needed_avg = 0.0f;
        gpa_to_letter(1.0f, &tp.need_letter, &tp.need_plus);
        return tp;
    }

    float P    = hp->rate * (float)hp->eff;
    float need = (T * (float)hp->req - P) / (float)hp->remaining;
    tp.needed_avg = need;

    if (need <= 1.0f) {
        tp.status = TARGET_SECURED;            /* just pass remaining work */
        gpa_to_letter(1.0f, &tp.need_letter, &tp.need_plus);
    } else if (need > 4.0f) {
        tp.status = TARGET_IMPOSSIBLE;
        gpa_to_letter(4.0f, &tp.need_letter, &tp.need_plus);
    } else {
        tp.status = TARGET_REACHABLE;
        gpa_to_letter(need, &tp.need_letter, &tp.need_plus);
    }
    return tp;
}

/* Plan to just enter a tier (its floor) — the FLEX_LOW case. */
static TargetPlan honor_target_plan(const HonorProjection *hp, HonorTier target)
{
    return honor_target_plan_at(hp, kHonorFloor[target]);
}
