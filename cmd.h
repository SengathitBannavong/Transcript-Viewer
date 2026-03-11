/*
 * cmd.h -- Command palette argument schema + dispatcher
 *
 * Included once from main.c, after all g* globals are declared.
 * Uses the easyargs single-header library to define args_t and parse
 * whitespace-tokenised input from the Ctrl+K palette.
 *
 * Supported commands
 * ------------------
 *   nav    -n <0-4>      Navigate to sidebar item
 *   filter -d <dept>     Filter the employee table by department
 *   clear                Clear active department filter
 *   help                 Show available commands in the toast
 */

#pragma once

#include <string.h>
#include <stdio.h>

/* ── Argument schema (defines args_t via easyargs macro magic) ───────── */

#define REQUIRED_ARGS \
    REQUIRED_STRING_ARG(verb, "command", "Command: nav | filter | clear | help")

#define OPTIONAL_ARGS \
    OPTIONAL_INT_ARG(nav_idx, -1, "-n", "N",    "Sidebar index to activate (0-4)") \
    OPTIONAL_STRING_ARG(dept,  "", "-d", "dept", "Department name (for filter)")

/* No BOOLEAN_ARGS — "help" is handled as a verb for simplicity */

#include "easyargs.h"

/* ── Tokeniser ───────────────────────────────────────────────────────── */
/*
 * Split a mutable buffer on whitespace, filling argv (max max_argc slots).
 * argv[0] is always the fake program name "palette".
 * Returns the resulting argc.
 */
static inline int cmd_tokenize(char *buf, char **argv, int max_argc)
{
    int argc  = 0;
    argv[argc++] = (char *)"palette";   /* fake argv[0] */
    char *p   = buf;
    while (*p && argc < max_argc) {
        while (*p == ' ') p++;          /* skip spaces  */
        if (!*p) break;
        argv[argc++] = p;
        while (*p && *p != ' ') p++;
        if (*p) *p++ = '\0';
    }
    return argc;
}

/* ── Dispatcher ──────────────────────────────────────────────────────── */
/*
 * Parse `input` and execute the command.
 *   out_nav    — pointer to gActiveNav; updated by "nav"
 *   out_filter — pointer to gFilterDept (char[64]); updated by "filter"/"clear"
 *   out_msg    — human-readable result written here (for the toast)
 *   msg_size   — sizeof(out_msg) buffer
 */
static inline void ExecuteCommand(const char *input,
                                   int        *out_nav,
                                   char       *out_filter,
                                   char       *out_msg,
                                   int         msg_size)
{
    /* work on a mutable copy so tokeniser can null-terminate in-place */
    char buf[256];
    strncpy(buf, input, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    char *argv[16];
    int   argc = cmd_tokenize(buf, argv, 16);

    args_t args = make_default_args();

    if (!parse_args(argc, argv, &args)) {
        snprintf(out_msg, msg_size,
                 "Usage: nav -n <0-4>  |  filter -d <dept>  |  clear  |  help");
        return;
    }

    /* ── help ── */
    if (strcmp(args.verb, "help") == 0) {
        snprintf(out_msg, msg_size,
                 "nav -n 0-4   filter -d <dept>   clear   help");
        return;
    }

    /* ── nav ── */
    if (strcmp(args.verb, "nav") == 0) {
        if (args.nav_idx < 0 || args.nav_idx > 4) {
            snprintf(out_msg, msg_size,
                     "nav: -n must be 0-4  (0=Dashboard 1=Employees 2=Reports 3=Analytics 4=Settings)");
            return;
        }
        *out_nav = args.nav_idx;
        static const char *names[] = {
            "Dashboard", "Employees", "Reports", "Analytics", "Settings"
        };
        snprintf(out_msg, msg_size, "Navigated to %s", names[args.nav_idx]);
        return;
    }

    /* ── filter ── */
    if (strcmp(args.verb, "filter") == 0) {
        if (!args.dept || args.dept[0] == '\0') {
            snprintf(out_msg, msg_size,
                     "filter: provide -d <dept>  e.g. filter -d Engineering");
            return;
        }
        strncpy(out_filter, args.dept, 63);
        out_filter[63] = '\0';
        snprintf(out_msg, msg_size, "Filtering by '%s'", args.dept);
        return;
    }

    /* ── clear ── */
    if (strcmp(args.verb, "clear") == 0) {
        out_filter[0] = '\0';
        snprintf(out_msg, msg_size, "Filter cleared — showing all employees");
        return;
    }

    /* ── unknown ── */
    snprintf(out_msg, msg_size,
             "Unknown command '%s'   try: nav | filter | clear | help",
             args.verb);
}
