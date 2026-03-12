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
 *   LIMIT_FOR_GRADUATE[]         — required credits/subjects per type (dynamic)
 *   init_grad_limits(p)          — populate LIMIT_FOR_GRADUATE from Subject_Type.Total_Credit
 *   score_to_gpa(letter, plus)   — 4-scale GPA point for a stored grade
 *   calc_cpa(player, pass_only)  — overall CPA (0=all studied, 1=pass only)
 *   calc_cpa_type(player,t,p)    — CPA for one subject type
 *   calc_status_alert(npass)     — academic alert level (0-3)
 *   calc_effective_credits(p)    — credits toward graduation (module choice)
 *   calc_required_credits()      — total credits required to graduate
 *   update_player_status(p)      — recompute status_can_grauate + status_alert
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
static int LIMIT_FOR_GRADUATE[sizeSubjectType] = {
     0, /* co_so_nganh       — set by init_grad_limits() */
     0, /* dai_cuong         — set by init_grad_limits() */
     4, /* the_thao          — fixed: min sports subjects (not credits) */
     0, /* ly_luat_chinh_tri — set by init_grad_limits() */
     9, /* tu_chon           — fixed: min elective credits */
     0, /* thuc_tap          — set by init_grad_limits() */
     0, /* modunI            — set by init_grad_limits() */
     0, /* modunII           — set by init_grad_limits() */
     0, /* modunIII          — set by init_grad_limits() */
     0, /* modunIV           — set by init_grad_limits() */
     0, /* modunV            — set by init_grad_limits() */
     0, /* do_an_tot_nghiep  — set by init_grad_limits() */
};

/* ── Populate LIMIT_FOR_GRADUATE from curriculum data ────────────────────
 *  Must be called after DB_Query() has filled Subject_Type.Total_Credit.
 *  Skips the_thao and tu_chon which have fixed minimum requirements.
 * ──────────────────────────────────────────────────────────────────────── */
static void init_grad_limits(Player *p)
{
    for (int i = 0; i < sizeSubjectType; i++) {
        if (i == the_thao || i == tu_chon) continue;
        LIMIT_FOR_GRADUATE[i] = (int)p->numofSubjectType[i].Total_Credit;
    }
}

/* ── Score letter → 4-scale GPA point ───────────────────────────────────
 *  letter : base char stored in Subject_Node.score_letter ('A','B','C','D','F')
 *  plus   : non-zero if the full grade string has a '+' modifier (e.g., "A+")
 *
 *  Scale:  A+ 4.5 | A 4.0 | B+ 3.5 | B 3.0 | C+ 2.5 | C 2.0
 *          D+ 1.5 | D 1.0 | F/X 0.0
 * ──────────────────────────────────────────────────────────────────────── */
static float score_to_gpa(char letter, int plus)
{
    switch (letter) {
        case 'A': return plus ? 4.5f : 4.0f;
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

/* internal helper */
static int _sl_max3(int a, int b, int c)
{
    int r = a; if (r < b) r = b; if (r < c) r = c; return r;
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
    for (int i = 0; i < sizeSubjectType; i++) {
        if (i == the_thao) continue;   /* sport: checked separately */
        if (i == modunI) {
            /* pick best of modunI / modunII / modunIII */
            count += _sl_max3(
                (int)p->numofSubjectType[modunI].count_passCredit,
                (int)p->numofSubjectType[modunII].count_passCredit,
                (int)p->numofSubjectType[modunIII].count_passCredit);
            /* pick best of modunIV / modunV */
            count += _sl_max3(
                (int)p->numofSubjectType[modunIV].count_passCredit,
                (int)p->numofSubjectType[modunV].count_passCredit, 0);
            i = modunV;
            continue;
        }
        count += (int)p->numofSubjectType[i].count_passCredit;
    }
    return count;
}

/* ── Total credits required for graduation ───────────────────────────────
 *  Sums LIMIT_FOR_GRADUATE, counting only one limit for the module groups
 *  (same skip logic used in remaining_credit from the old program).
 * ──────────────────────────────────────────────────────────────────────── */
static int calc_required_credits(void)
{
    int total = 0;
    for (int i = 0; i < sizeSubjectType; i++) {
        if (i == the_thao) continue;            /* sport: not counted by credits */
        total += LIMIT_FOR_GRADUATE[i];
        if (i == modunI)  i = modunIII;         /* skip II, III (same limit)     */
        if (i == modunIV) i = modunV;           /* skip V (same limit)           */
    }
    return total;
}

/* ── Recompute status_can_grauate and status_alert on *p ─────────────────
 *  Call this after every DB_Query() to keep the player state correct.
 *
 *  Graduation criteria (all must be met):
 *    1. Sport: count_passSubject >= LIMIT_FOR_GRADUATE[the_thao]
 *    2. Each non-sport, non-module type: count_passCredit >= limit
 *    3. Modules: max(I,II,III passCredit) >= limit[modunI]
 *                max(IV,V passCredit)    >= limit[modunIV]
 *    4. CPA (all studied) >= 2.0
 * ──────────────────────────────────────────────────────────────────────── */
static void update_player_status(Player *p)
{
    /* Sync graduation limits with the actual curriculum in the DB */
    init_grad_limits(p);

    /* --- Academic alert --- */
    p->status_alert = (unsigned)calc_status_alert(p->ToTal_credit_npass);

    /* --- Graduation check --- */

    /* 1. Sport: need enough SUBJECTS passed (not credits) */
    if ((int)p->numofSubjectType[the_thao].count_passSubject
            < LIMIT_FOR_GRADUATE[the_thao]) {
        p->status_can_grauate = 0;
        return;
    }

    /* 2. All other types (including module logic) */
    for (int i = 0; i < sizeSubjectType; i++) {
        if (i == the_thao) continue;

        if (i == modunI) {
            int best123 = _sl_max3(
                (int)p->numofSubjectType[modunI].count_passCredit,
                (int)p->numofSubjectType[modunII].count_passCredit,
                (int)p->numofSubjectType[modunIII].count_passCredit);
            int best45 = _sl_max3(
                (int)p->numofSubjectType[modunIV].count_passCredit,
                (int)p->numofSubjectType[modunV].count_passCredit, 0);
            if (best123 < LIMIT_FOR_GRADUATE[modunI] ||
                best45  < LIMIT_FOR_GRADUATE[modunIV]) {
                p->status_can_grauate = 0;
                return;
            }
            i = modunV;
            continue;
        }

        if ((int)p->numofSubjectType[i].count_passCredit < LIMIT_FOR_GRADUATE[i]) {
            p->status_can_grauate = 0;
            return;
        }
    }

    /* 3. CPA (all studied) >= 2.0 */
    if (calc_cpa(p, 0) < 2.0f) {
        p->status_can_grauate = 0;
        return;
    }

    p->status_can_grauate = 1;
}
