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
