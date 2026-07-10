#pragma once

#include "app_data.h"

typedef struct {
    int       eff, req, remaining;  /* credits                              */
    float     rate;                 /* pass-only CPA = projected final CPA  */
    float     ceiling, floor;       /* reachable CPA range given R          */
    HonorTier projected;            /* honor_tier(rate)                     */
    HonorTier best, worst;          /* honor_tier(ceiling / floor)          */
} HonorProjection;

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

/* Per-term statistics series for the By Term charts. */
#define MAX_TERM_SERIES 40
typedef struct {
    int   count;
    int   terms[MAX_TERM_SERIES];      /* raw term code, e.g. 20231          */
    float sem_gpa[MAX_TERM_SERIES];    /* semester GPA (credit-weighted)     */
    int   reg_cred[MAX_TERM_SERIES];   /* credits registered that term       */
    int   pass_cred[MAX_TERM_SERIES];  /* credits passed that term           */
    int   cum_cred[MAX_TERM_SERIES];   /* cumulative credits passed          */
    float cum_cpa[MAX_TERM_SERIES];    /* cumulative CPA (best-attempt-so-far)*/
} TermSeries;
void compute_term_series(const AttemptRow *att, int n, TermSeries *out);

/* ── Quality-points breakdown for the Quality Points Analyzer ───────────────
 *  quality_points = Σ (grade_point × credits) over the subjects counted.
 *  0-credit courses (e.g. sports/PE) contribute nothing to either field, so
 *  they never move the CPA — but callers may still list them separately.
 * ──────────────────────────────────────────────────────────────────────── */
typedef struct {
    int   credits;   /* CPA-affecting credits counted                        */
    float qp;        /* quality points = Σ grade_point × credits             */
    float cpa;       /* qp / credits (0.0 when credits == 0)                 */
} QpCategory;

/* Quality points for one subject category / the whole transcript.
 *   pass_only : 0 = all studied attempts, 1 = passed subjects only
 *   simulated : 0 = real grades, 1 = apply the committed sandbox overrides
 *               (gApp.sandbox_overrides — the same set the Planner projects) */
QpCategory calc_qp_type(Player *p, int t, int pass_only, int simulated);
QpCategory calc_qp_overall(Player *p, int pass_only, int simulated);

int _sl_resolve_limit(Player *p, int i);
int _sl_resolve_pass(Player *p, int i);
float score_to_gpa(char letter, int plus);
int calc_status_alert(int credit_npass);
float calc_cpa(Player *p, int pass_only);
float calc_cpa_type(Player *p, int t, int pass_only);
int calc_effective_credits(Player *p);
int calc_required_credits(Player *p);
void update_player_status(Player *p);
int calc_missing_types(Player *p, int out[sizeSubjectType]);
HonorTier honor_tier(float cpa);
const char *honor_name(HonorTier t);
void gpa_to_letter(float g, char *letter, int *plus);
HonorProjection honor_project(Player *p);
float honor_flex_target(HonorTier tier, FlexLevel flex);
float honor_tier_top(HonorTier t);
TargetPlan honor_target_plan(const HonorProjection *hp, HonorTier target);
TargetPlan honor_target_plan_at(const HonorProjection *hp, float T);

extern const float kHonorFloor[5];
