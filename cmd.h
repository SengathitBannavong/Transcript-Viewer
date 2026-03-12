/*
 * cmd.h -- Command palette dispatcher
 *
 * Included once from main.c, after all g* globals and db.h are declared.
 * NO longer uses easyargs — simple manual tokeniser for direct dispatch.
 *
 * Supported commands
 * ------------------
 *   type  <0-11>                      Switch sidebar to that subject-type section
 *   score <CODE> <mid> <fin> [ratio]  Update scores (ratio: 1=50/50, 2=40/60, 3=30/70)
 *   clear <CODE>                      Reset a subject score back to X / 0.0
 *   cpa                               Show overall CPA (all studied + pass only)
 *   logout                            Close DB and return to the name-input screen
 *   help                              Show available commands in the toast
 */

#pragma once

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* ── Tokeniser ───────────────────────────────────────────────────────── */
static inline int cmd_tokenize(char *buf, char **argv, int max_argc)
{
    int argc = 0;
    argv[argc++] = (char *)"palette";
    char *p = buf;
    while (*p && argc < max_argc) {
        while (*p == ' ') p++;
        if (!*p) break;
        argv[argc++] = p;
        while (*p && *p != ' ') p++;
        if (*p) *p++ = '\0';
    }
    return argc;
}

/* ── Dispatcher ──────────────────────────────────────────────────────── */
static inline void ExecuteCommand(const char *input,
                                   int        *out_nav,
                                   char       *out_filter,   /* kept for compat */
                                   char       *out_msg,
                                   int         msg_size)
{
    (void)out_filter;

    char buf[256];
    strncpy(buf, input, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    char *argv[16];
    int   argc = cmd_tokenize(buf, argv, 16);

    if (argc < 2) {
        snprintf(out_msg, msg_size,
                 "type <1-13>  |  score <CODE> <mid> <fin>  |  clear <CODE>  |  reload  |  help");
        return;
    }

    const char *verb = argv[1];

    /* ── help ── */
    if (strcmp(verb, "help") == 0) {
        snprintf(out_msg, msg_size,
                 "type 1-13  |  score <CODE> <mid> <fin> [ratio 1-3]  |  clear <CODE>  |  cpa  |  reload  |  logout");
        return;
    }

    /* ── type <N> ── */
    if (strcmp(verb, "type") == 0) {
        if (argc < 3) {
            snprintf(out_msg, msg_size, "type: usage: type <0-%d>", sizeSubjectType - 1);
            return;
        }
        int idx = atoi(argv[2]);
        if (idx < 0 || idx >= sizeSubjectType) {
            snprintf(out_msg, msg_size, "type: index %d out of range (0-%d)",
                     idx, sizeSubjectType - 1);
            return;
        }
        *out_nav = idx;
        snprintf(out_msg, msg_size, "Showing type %d: %s", idx, gTypeName[idx]);
        return;
    }

    /* ── score <CODE> <mid> <fin> [ratio] ── */
    if (strcmp(verb, "score") == 0) {
        if (argc < 5) {
            snprintf(out_msg, msg_size,
                     "score: usage: score <CODE> <mid> <final> [ratio]  (ratio: 1=50/50  2=40/60  3=30/70)");
            return;
        }
        const char *code = argv[2];
        float  mid_  = (float)atof(argv[3]);
        float  fin_  = (float)atof(argv[4]);
        int    ratio = (argc >= 6) ? atoi(argv[5]) : 3;  /* default 30/70 */
        if (ratio < 1 || ratio > 3) ratio = 3;
        if (mid_ < 0.f || mid_ > 10.f || fin_ < 0.f || fin_ > 10.f) {
            snprintf(out_msg, msg_size, "score: values must be in 0.0-10.0");
            return;
        }
        if (!DB_SubjectExists(code)) {
            snprintf(out_msg, msg_size, "score: subject '%s' not found", code);
            return;
        }
        static const char *ratio_labels[] = { "", "50/50", "40/60", "30/70" };
        int changed = DB_UpdateScoreRatio(code, mid_, fin_, ratio);
        if (changed) {
            RefreshPlayer();
            snprintf(out_msg, msg_size,
                     "Updated %s: mid=%.1f  final=%.1f  ratio=%s",
                     code, mid_, fin_, ratio_labels[ratio]);
        } else {
            snprintf(out_msg, msg_size, "score: update failed for '%s'", code);
        }
        return;
    }

    /* ── clear <CODE> ── */
    if (strcmp(verb, "clear") == 0) {
        if (argc < 3) {
            snprintf(out_msg, msg_size, "clear: usage: clear <CODE>");
            return;
        }
        const char *code = argv[2];
        if (!DB_SubjectExists(code)) {
            snprintf(out_msg, msg_size, "clear: subject '%s' not found", code);
            return;
        }
        DB_ClearScore(code);
        RefreshPlayer();
        snprintf(out_msg, msg_size, "Cleared score for %s", code);
        return;
    }

    /* ── cpa ── */
    if (strcmp(verb, "cpa") == 0) {
        float cpa_all  = calc_cpa(&gPlayer, 0);
        float cpa_pass = calc_cpa(&gPlayer, 1);
        int   eff      = calc_effective_credits(&gPlayer);
        int   req      = calc_required_credits(&gPlayer);
        snprintf(out_msg, msg_size,
                 "CPA all=%.3f  pass=%.3f  |  Credits %d/%d  |  Alert=%d",
                 cpa_all, cpa_pass, eff, req, (int)gPlayer.status_alert);
        return;
    }

    /* ── reload ── */
    if (strcmp(verb, "reload") == 0) {
        int prev_warns = gDataWarnCount;
        DB_ReloadData();   /* re-parse subjects.dat + grad_config.cfg, re-validate */
        RefreshPlayer();   /* rebuild gTypeName[] + gPlayer from the updated DB    */
        if (gDataWarnCount == 0) {
            snprintf(out_msg, msg_size,
                     "Reload OK. all data valid (was %d warning%s)",
                     prev_warns, prev_warns == 1 ? "" : "s");
        } else {
            snprintf(out_msg, msg_size,
                     "Reload done. %d warning%s remain (check data banner)",
                     gDataWarnCount, gDataWarnCount == 1 ? "" : "s");
        }
        return;
    }

    /* ── logout ── */
    if (strcmp(verb, "logout") == 0) {
        DB_Close();
        memset(&gPlayer, 0, sizeof(gPlayer));
        gNameLen    = 0;
        gUserName[0] = '\0';
        gDBReady    = false;
        gNameInput  = true;
        SetWindowTitle("Transcript Viewer");
        snprintf(out_msg, msg_size, "Logged out");
        return;
    }

    /* ── unknown ── */
    snprintf(out_msg, msg_size,
             "Unknown command '%s'  try: type | score | clear | logout | help", verb);
}
