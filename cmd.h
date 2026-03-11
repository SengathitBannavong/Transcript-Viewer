/*
 * cmd.h -- Command palette dispatcher
 *
 * Included once from main.c, after all g* globals and db.h are declared.
 * NO longer uses easyargs — simple manual tokeniser for direct dispatch.
 *
 * Supported commands
 * ------------------
 *   type  <0-11>             Switch sidebar to that subject-type section
 *   score <CODE> <mid> <fin> Update scores for a subject
 *   clear <CODE>             Reset a subject score back to X / 0.0
 *   logout                   Close DB and return to the name-input screen
 *   help                     Show available commands in the toast
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
                 "Usage: type <0-11>  |  score <CODE> <mid> <fin>  |  clear <CODE>  |  help");
        return;
    }

    const char *verb = argv[1];

    /* ── help ── */
    if (strcmp(verb, "help") == 0) {
        snprintf(out_msg, msg_size,
                 "type 0-11  |  score <CODE> <mid> <fin>  |  clear <CODE>  |  logout  |  help");
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

    /* ── score <CODE> <mid> <fin> ── */
    if (strcmp(verb, "score") == 0) {
        if (argc < 5) {
            snprintf(out_msg, msg_size,
                     "score: usage: score <CODE> <mid> <final>");
            return;
        }
        const char *code = argv[2];
        float  mid_  = (float)atof(argv[3]);
        float  fin_  = (float)atof(argv[4]);
        if (mid_ < 0.f || mid_ > 10.f || fin_ < 0.f || fin_ > 10.f) {
            snprintf(out_msg, msg_size, "score: values must be in 0.0-10.0");
            return;
        }
        if (!DB_SubjectExists(code)) {
            snprintf(out_msg, msg_size, "score: subject '%s' not found", code);
            return;
        }
        int changed = DB_UpdateScore(code, mid_, fin_);
        if (changed) {
            DB_Query(&gPlayer);
            snprintf(out_msg, msg_size, "Updated %s: mid=%.1f final=%.1f", code, mid_, fin_);
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
        DB_Query(&gPlayer);
        snprintf(out_msg, msg_size, "Cleared score for %s", code);
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
