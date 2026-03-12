/*
 * ui.c — Clay rendering layer for the Student Transcript Viewer
 *
 * NOT compiled independently. Included into main.c AFTER all globals
 * and db.h/app_data.h are declared.
 *
 * Globals expected from main.c:
 *   int   gScreenW, gScreenH   — current window dimensions
 *   int   gActiveNav           — active subject-type section (0 = sizeSubjectType-1)
 *   char  gDynBuf[], gDynPos   — per-frame dynamic string arena
 *   bool  gPopupOpen           — command palette visibility
 *   char  gCmdBuf[], gCmdLen   — current typed text
 *   bool  gHasResult           — whether a toast should be shown
 *   float gResultShowUntil     — GetTime() deadline for the toast
 *   char  gResultMsg[]         — result message shown in toast
 *   Player gPlayer             — filled by DB_Query()
 */

/* ─── Layout constants ───────────────────────────────────────────────── */
#define SIDEBAR_W   210
#define ROW_H        44
#define HDR_H        42

/* ─── Colors ─────────────────────────────────────────────────────────── */
#define C_BG         ((Clay_Color){ 11,  11,  20, 255})
#define C_SIDEBAR    ((Clay_Color){ 15,  15,  28, 255})
#define C_CARD       ((Clay_Color){ 20,  20,  38, 255})
#define C_TBL_HDR    ((Clay_Color){ 25,  25,  46, 255})
#define C_ROW_ODD    ((Clay_Color){ 18,  18,  34, 255})
#define C_ROW_EVEN   ((Clay_Color){ 15,  15,  28, 255})
#define C_ROW_HOVER  ((Clay_Color){ 32,  32,  64, 255})
#define C_BORDER     ((Clay_Color){ 36,  36,  68, 255})
#define C_ACCENT     ((Clay_Color){ 99, 102, 241, 255})
#define C_ACCENT_DIM ((Clay_Color){ 50,  52, 130, 255})
#define C_ACCENT_BG  ((Clay_Color){ 22,  23,  58, 255})
#define C_TEXT       ((Clay_Color){224, 224, 242, 255})
#define C_SUBTEXT    ((Clay_Color){110, 110, 155, 255})
#define C_WHITE      ((Clay_Color){255, 255, 255, 255})
#define C_GREEN      ((Clay_Color){ 34, 197,  94, 255})
#define C_GREEN_BG   ((Clay_Color){ 16,  60,  36, 255})
#define C_RED        ((Clay_Color){239,  68,  68, 255})
#define C_RED_BG     ((Clay_Color){ 70,  18,  18, 255})
#define C_YELLOW     ((Clay_Color){234, 179,   8, 255})
#define C_YELLOW_BG  ((Clay_Color){ 60,  50,  10, 255})
#define C_TRANS      ((Clay_Color){  0,   0,   0,   0})

/* ─── Text config helper ─────────────────────────────────────────────── */
/* gFontScale is declared in main.c and loaded from assets/ui.cfg        */
#define TC(color, size) \
    Clay__StoreTextElementConfig((Clay_TextElementConfig){ \
        .textColor = (color), \
        .fontSize  = (uint16_t)((size) * gFontScale), \
        .fontId    = 0, \
        .wrapMode  = CLAY_TEXT_WRAP_NONE })

/* ─── String helpers ─────────────────────────────────────────────────── */
static Clay_String CS(const char *s)
{
    return (Clay_String){ .isStaticallyAllocated = true,
                          .length = (int)strlen(s), .chars = s };
}

static Clay_String DS(const char *fmt, ...)
{
    char   *start = gDynBuf + gDynPos;
    int     avail = DYN_BUF_SIZE - gDynPos - 1;
    va_list ap;
    va_start(ap, fmt);
    int len = vsnprintf(start, (size_t)(avail > 0 ? avail : 0), fmt, ap);
    va_end(ap);
    if (len < 0) len = 0;
    if (len >= avail) {
        gDynPos = 0; start = gDynBuf;
        va_start(ap, fmt);
        len = vsnprintf(start, DYN_BUF_SIZE - 1, fmt, ap);
        va_end(ap);
        if (len < 0) len = 0;
    }
    gDynPos += len + 1;
    return (Clay_String){ .isStaticallyAllocated = false,
                          .length = len, .chars = start };
}

/* ═══════════════════════════════════════════════════════════════════════
 *  SIDEBAR
 * ═══════════════════════════════════════════════════════════════════════ */

static void RenderNavItem(int idx, const char *label, int warn)
{
    bool active = (idx == gActiveNav);
    Clay_Color bg  = active ? C_ACCENT_BG : C_TRANS;
    Clay_Color tc  = active ? C_WHITE     : C_SUBTEXT;
    Clay_Color bdr = active ? C_ACCENT    : C_TRANS;

    CLAY(CLAY_IDI("NavItem", idx), {
        .layout = {
            .sizing          = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(38) },
            .padding         = { 10, 10, 0, 0 },
            .childGap        = 8,
            .childAlignment  = { .y = CLAY_ALIGN_Y_CENTER },
            .layoutDirection = CLAY_LEFT_TO_RIGHT,
        },
        .backgroundColor = (!active && Clay_Hovered()) ? C_ROW_HOVER : bg,
        .cornerRadius    = CLAY_CORNER_RADIUS(5),
        .border          = { .color = bdr, .width = { .left = active ? 3 : 0 } },
    }) {
        if (Clay_Hovered() && IsMouseButtonReleased(MOUSE_LEFT_BUTTON))
            gActiveNav = idx;

        /* index badge */
        CLAY(CLAY_IDI("NavBdg", idx), {
            .layout = {
                .sizing         = { CLAY_SIZING_FIXED(20), CLAY_SIZING_FIXED(20) },
                .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER },
            },
            .backgroundColor = active ? C_ACCENT_DIM : C_BORDER,
            .cornerRadius    = CLAY_CORNER_RADIUS(4),
        }) {
            CLAY_TEXT(DS("%d", idx), TC(active ? C_WHITE : C_SUBTEXT, 9));
        }
        CLAY_TEXT(CS(label), TC(tc, 11));

        /* push warning badge to the right edge */
        if (warn) {
            CLAY(CLAY_IDI("NavWSp", idx), {
                .layout = { .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(1) } },
            }) {}
            CLAY(CLAY_IDI("NavWarn", idx), {
                .layout = {
                    .sizing         = { CLAY_SIZING_FIXED(15), CLAY_SIZING_FIXED(15) },
                    .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER },
                },
                .backgroundColor = C_RED,
                .cornerRadius    = CLAY_CORNER_RADIUS(8),
            }) {
                CLAY_TEXT(CLAY_STRING("!"), TC(C_WHITE, 9));
            }
        }
    }
}

static void RenderSidebar(void)
{
    CLAY(CLAY_ID("Sidebar"), {
        .layout = {
            .sizing          = { CLAY_SIZING_FIXED(SIDEBAR_W), CLAY_SIZING_GROW(0) },
            .padding         = { 10, 10, 12, 12 },
            .childGap        = 3,
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
        },
        .backgroundColor = C_SIDEBAR,
        .border          = { .color = C_BORDER, .width = { .right = 1 } },
    }) {
        /* Logo */
        CLAY(CLAY_ID("Logo"), {
            .layout = {
                .sizing          = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(54) },
                .padding         = { 4, 4, 0, 0 },
                .childGap        = 8,
                .childAlignment  = { .y = CLAY_ALIGN_Y_CENTER },
                .layoutDirection = CLAY_LEFT_TO_RIGHT,
            },
        }) {
            CLAY(CLAY_ID("LogoBadge"), {
                .layout = {
                    .sizing         = { CLAY_SIZING_FIXED(30), CLAY_SIZING_FIXED(30) },
                    .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER },
                },
                .backgroundColor = C_ACCENT,
                .cornerRadius    = CLAY_CORNER_RADIUS(7),
            }) { CLAY_TEXT(CLAY_STRING("TC"), TC(C_WHITE, 11)); }
            CLAY(CLAY_ID("LogoText"), {
                .layout = { .layoutDirection = CLAY_TOP_TO_BOTTOM, .childGap = 2 },
            }) {
                CLAY_TEXT(CLAY_STRING("Transcript"),  TC(C_TEXT,    13));
                CLAY_TEXT(CLAY_STRING("Viewer v1.0"), TC(C_SUBTEXT,  9));
            }
        }

        /* separator */
        CLAY(CLAY_ID("Sep1"), {
            .layout          = { .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(1) } },
            .backgroundColor = C_BORDER,
        }) {}

        CLAY(CLAY_ID("SectionLabel"), {
            .layout = {
                .sizing         = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(26) },
                .padding        = { 4, 4, 0, 0 },
                .childAlignment = { .y = CLAY_ALIGN_Y_CENTER },
            },
        }) { CLAY_TEXT(CLAY_STRING("SUBJECT TYPES"), TC(C_SUBTEXT, 9)); }

        /* Dashboard nav item (index 0) */
        RenderNavItem(0, "Dashboard", 0);

        /* one nav item per subject type — only show types that have subjects */
        {
            int _miss[sizeSubjectType];
            calc_missing_types(&gPlayer, _miss);
            for (int i = 1; i < sizeSubjectType; i++) {
                if (gTypeName[i][0] != 0 && gPlayer.numofSubjectType[i].Total_Subject > 0)
                    RenderNavItem(i, gTypeName[i], _miss[i]);
            }
        }

        /* spacer */
        CLAY(CLAY_ID("NavSpacer"), {
            .layout = { .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) } },
        }) {}

        CLAY(CLAY_ID("Sep2"), {
            .layout          = { .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(1) } },
            .backgroundColor = C_BORDER,
        }) {}

        /* user card */
        CLAY(CLAY_ID("UserCard"), {
            .layout = {
                .sizing          = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(50) },
                .padding         = { 8, 8, 0, 0 },
                .childGap        = 8,
                .childAlignment  = { .y = CLAY_ALIGN_Y_CENTER },
                .layoutDirection = CLAY_LEFT_TO_RIGHT,
            },
            .backgroundColor = C_CARD,
            .cornerRadius    = CLAY_CORNER_RADIUS(6),
        }) {
            CLAY(CLAY_ID("UAvatarCircle"), {
                .layout = {
                    .sizing         = { CLAY_SIZING_FIXED(30), CLAY_SIZING_FIXED(30) },
                    .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER },
                },
                .backgroundColor = C_ACCENT,
                .cornerRadius    = CLAY_CORNER_RADIUS(15),
            }) {
                /* Build up-to-2-char initials from gUserName */
                char initials[3] = { '?', '\0', '\0' };
                if (gUserName[0]) {
                    initials[0] = (char)(gUserName[0] >= 'a' && gUserName[0] <= 'z'
                                        ? gUserName[0] - 32 : gUserName[0]);
                    for (int _i = 1; gUserName[_i]; _i++) {
                        if (gUserName[_i-1] == '_' || gUserName[_i-1] == '-' ||
                                gUserName[_i-1] == ' ') {
                            initials[1] = (char)(gUserName[_i] >= 'a' && gUserName[_i] <= 'z'
                                                ? gUserName[_i] - 32 : gUserName[_i]);
                            break;
                        }
                    }
                    if (!initials[1] && gUserName[1])
                        initials[1] = (char)(gUserName[1] >= 'a' && gUserName[1] <= 'z'
                                            ? gUserName[1] - 32 : gUserName[1]);
                }
                CLAY_TEXT(DS("%s", initials), TC(C_WHITE, 10));
            }
            CLAY(CLAY_ID("UInfo"), {
                .layout = { .layoutDirection = CLAY_TOP_TO_BOTTOM, .childGap = 2 },
            }) {
                CLAY_TEXT(DS("%s", gUserName[0] ? gUserName : "--"), TC(C_TEXT,    11));
                CLAY_TEXT(CLAY_STRING("Student"),                    TC(C_SUBTEXT,  9));
            }
        }
    }
}

/* ═══════════════════════════════════════════════════════════════════════
 *  SUMMARY CARDS ROW  (Total credits passed / subject type stats)
 * ═══════════════════════════════════════════════════════════════════════ */

static void RenderSummaryCard(int idx, const char *title,
                               Clay_String val, Clay_Color accent)
{
    CLAY(CLAY_IDI("SumCard", idx), {
        .layout = {
            .sizing          = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
            .padding         = { 14, 14, 10, 10 },
            .childGap        = 6,
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
        },
        .backgroundColor = C_CARD,
        .cornerRadius    = CLAY_CORNER_RADIUS(7),
        .border          = { .color = C_BORDER,
                             .width = { .left=1,.right=1,.top=1,.bottom=1 } },
    }) {
        CLAY(CLAY_IDI("SCBar", idx), {
            .layout = {
                .sizing         = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
                .childGap       = 6,
                .childAlignment = { .y = CLAY_ALIGN_Y_CENTER },
                .layoutDirection= CLAY_LEFT_TO_RIGHT,
            },
        }) {
            CLAY(CLAY_IDI("SCAccent", idx), {
                .layout          = { .sizing = { CLAY_SIZING_FIXED(3), CLAY_SIZING_FIXED(14) } },
                .backgroundColor = accent,
                .cornerRadius    = CLAY_CORNER_RADIUS(2),
            }) {}
            CLAY_TEXT(CS(title), TC(C_SUBTEXT, 10));
        }
        CLAY_TEXT(val, TC(C_TEXT, 22));
    }
}

/* ═══════════════════════════════════════════════════════════════════════
 *  SUBJECT TABLE  (header + rows for one Subject_Type)
 * ═══════════════════════════════════════════════════════════════════════ */

static void RenderTableHeader(void)
{
    CLAY(CLAY_ID("TblHdr"), {
        .layout = {
            .sizing          = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(HDR_H) },
            .layoutDirection = CLAY_LEFT_TO_RIGHT,
        },
        .backgroundColor = C_TBL_HDR,
        .border          = { .color = C_BORDER, .width = { .bottom = 1 } },
    }) {
#define HDR_CELL(cid, w, lbl) \
        CLAY(CLAY_ID(cid), { \
            .layout = { \
                .sizing         = { w, CLAY_SIZING_GROW(0) }, \
                .padding        = { 10, 10, 0, 0 }, \
                .childAlignment = { .y = CLAY_ALIGN_Y_CENTER }, \
            }, \
        }) { CLAY_TEXT(CLAY_STRING(lbl), TC(C_SUBTEXT, 10)); }

        HDR_CELL("HC0", CLAY_SIZING_FIXED(56),  "#")
        HDR_CELL("HC1", CLAY_SIZING_GROW(0),    "SUBJECT NAME")
        HDR_CELL("HC2", CLAY_SIZING_FIXED(80),  "ID")
        HDR_CELL("HC3", CLAY_SIZING_FIXED(60),  "SCORE")
        HDR_CELL("HC4", CLAY_SIZING_FIXED(72),  "MIDTERM")
        HDR_CELL("HC5", CLAY_SIZING_FIXED(72),  "FINAL")
        HDR_CELL("HC6", CLAY_SIZING_FIXED(60),  "PASS")
        HDR_CELL("HC7", CLAY_SIZING_FIXED(62),  "CREDITS")
        HDR_CELL("HC8", CLAY_SIZING_FIXED(58),  "TERM")
#undef HDR_CELL
    }
}

/* Score letter → color */
static Clay_Color score_color(const char *letter)
{
    if (!letter || letter[0] == 'X') return C_SUBTEXT;
    if (letter[0] == 'F') return C_RED;
    if (letter[0] == 'A') return C_GREEN;
    if (letter[0] == 'B') return C_ACCENT;
    if (letter[0] == 'C') return C_YELLOW;
    return C_TEXT;  /* D+ or D */
}

static void RenderTableRow(Subject_Node *node, int idx)
{
    bool isOdd = idx % 2 != 0;

    /* Open edit popup on row click */
    bool rowHovered = Clay_Hovered();

    CLAY(CLAY_IDI("Row", idx), {
        .layout = {
            .sizing          = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(ROW_H) },
            .layoutDirection = CLAY_LEFT_TO_RIGHT,
        },
        .backgroundColor = Clay_Hovered() ? C_ROW_HOVER
                         : (isOdd ? C_ROW_ODD : C_ROW_EVEN),
        .border = { .color = C_BORDER, .width = { .bottom = 1 } },
    }) {
        (void)rowHovered;
        if (Clay_Hovered() && IsMouseButtonReleased(MOUSE_LEFT_BUTTON) && !gEditOpen) {
            gEditOpen  = true;
            gEditField = 0;
            gEditRatio = 3;
            strncpy(gEditCode, node->ID,   MAXSIZEID - 1);
            gEditCode[MAXSIZEID - 1] = '\0';
            strncpy(gEditSubjectName, node->name, MAXSIZENAME - 1);
            gEditSubjectName[MAXSIZENAME - 1] = '\0';
            /* Pre-fill buffers with existing scores */
            if (node->score_number_mid > 0.001f)
                gEditMidLen = snprintf(gEditMidBuf, sizeof(gEditMidBuf), "%.2f", node->score_number_mid);
            else { gEditMidBuf[0] = '\0'; gEditMidLen = 0; }
            if (node->score_number_final > 0.001f)
                gEditFinLen = snprintf(gEditFinBuf, sizeof(gEditFinBuf), "%.2f", node->score_number_final);
            else { gEditFinBuf[0] = '\0'; gEditFinLen = 0; }
        }
        /* ── # ── */
        CLAY(CLAY_IDI("RC", idx * 10 + 0), {
            .layout = {
                .sizing         = { CLAY_SIZING_FIXED(56), CLAY_SIZING_GROW(0) },
                .padding        = { 10, 10, 0, 0 },
                .childAlignment = { .y = CLAY_ALIGN_Y_CENTER },
            },
        }) { CLAY_TEXT(DS("%d", idx + 1), TC(C_SUBTEXT, 11)); }

        /* ── Subject Name ── */
        CLAY(CLAY_IDI("RC", idx * 10 + 1), {
            .layout = {
                .sizing         = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
                .padding        = { 10, 2, 0, 0 },
                .childAlignment = { .y = CLAY_ALIGN_Y_CENTER },
            },
        }) { CLAY_TEXT(CS(node->name), TC(C_TEXT, 11)); }

        /* ── ID ── */
        CLAY(CLAY_IDI("RC", idx * 10 + 2), {
            .layout = {
                .sizing         = { CLAY_SIZING_FIXED(80), CLAY_SIZING_GROW(0) },
                .padding        = { 8, 8, 0, 0 },
                .childAlignment = { .y = CLAY_ALIGN_Y_CENTER },
            },
        }) { CLAY_TEXT(CS(node->ID), TC(C_ACCENT, 11)); }

        /* ── Score letter ── */
        CLAY(CLAY_IDI("RC", idx * 10 + 3), {
            .layout = {
                .sizing         = { CLAY_SIZING_FIXED(60), CLAY_SIZING_GROW(0) },
                .padding        = { 8, 8, 0, 0 },
                .childAlignment = { .y = CLAY_ALIGN_Y_CENTER },
            },
        }) {
            /* Reconstruct full grade string (bit1 = has '+' modifier) */
            bool plus = (node->status_ever_been_study & 2) != 0;
            char grade[4];
            if (node->score_letter == 'X') {
                grade[0] = 'X'; grade[1] = '\0';
            } else if (plus) {
                grade[0] = node->score_letter; grade[1] = '+'; grade[2] = '\0';
            } else {
                grade[0] = node->score_letter; grade[1] = '\0';
            }
            char tmp[4] = { node->score_letter, '\0', '\0', '\0' };
            Clay_Color sc = score_color(tmp);
            CLAY_TEXT(DS("%s", grade), TC(sc, 13));
        }

        /* ── Midterm ── */
        CLAY(CLAY_IDI("RC", idx * 10 + 4), {
            .layout = {
                .sizing         = { CLAY_SIZING_FIXED(72), CLAY_SIZING_GROW(0) },
                .padding        = { 8, 8, 0, 0 },
                .childAlignment = { .y = CLAY_ALIGN_Y_CENTER },
            },
        }) {
            Clay_String ms = (node->score_number_mid > 0.001f)
                             ? DS("%.2f", node->score_number_mid)
                             : CLAY_STRING("--");
            CLAY_TEXT(ms, TC(C_TEXT, 11));
        }

        /* ── Final ── */
        CLAY(CLAY_IDI("RC", idx * 10 + 5), {
            .layout = {
                .sizing         = { CLAY_SIZING_FIXED(72), CLAY_SIZING_GROW(0) },
                .padding        = { 8, 8, 0, 0 },
                .childAlignment = { .y = CLAY_ALIGN_Y_CENTER },
            },
        }) {
            Clay_String fs = (node->score_number_final > 0.001f)
                             ? DS("%.2f", node->score_number_final)
                             : CLAY_STRING("--");
            CLAY_TEXT(fs, TC(C_TEXT, 11));
        }

        /* ── Pass badge ── */
        CLAY(CLAY_IDI("RC", idx * 10 + 6), {
            .layout = {
                .sizing         = { CLAY_SIZING_FIXED(60), CLAY_SIZING_GROW(0) },
                .padding        = { 8, 8, 0, 0 },
                .childAlignment = { .y = CLAY_ALIGN_Y_CENTER },
            },
        }) {
            bool    pass_flag = node->status_pass == 1;
            bool    no_score  = (node->score_letter == 'X');
            Clay_Color fg = no_score ? C_SUBTEXT : (pass_flag ? C_GREEN : C_RED);
            Clay_Color bg = no_score ? C_TRANS   : (pass_flag ? C_GREEN_BG : C_RED_BG);
            CLAY(CLAY_IDI("PassBdg", idx), {
                .layout = {
                    .sizing         = { CLAY_SIZING_FIT(0), CLAY_SIZING_FIXED(20) },
                    .padding        = { 8, 8, 2, 2 },
                    .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER },
                },
                .backgroundColor = bg,
                .cornerRadius    = CLAY_CORNER_RADIUS(10),
            }) {
                CLAY_TEXT(no_score ? CLAY_STRING("--")
                                   : (pass_flag ? CLAY_STRING("YES") : CLAY_STRING("NO")),
                          TC(fg, 10));
            }
        }

        /* ── Credits ── */
        CLAY(CLAY_IDI("RC", idx * 10 + 7), {
            .layout = {
                .sizing         = { CLAY_SIZING_FIXED(62), CLAY_SIZING_GROW(0) },
                .padding        = { 8, 8, 0, 0 },
                .childAlignment = { .y = CLAY_ALIGN_Y_CENTER },
            },
        }) { CLAY_TEXT(DS("%u", node->credit), TC(C_TEXT, 11)); }

        /* ── Term ── */
        CLAY(CLAY_IDI("RC", idx * 10 + 8), {
            .layout = {
                .sizing         = { CLAY_SIZING_FIXED(58), CLAY_SIZING_GROW(0) },
                .padding        = { 8, 8, 0, 0 },
                .childAlignment = { .y = CLAY_ALIGN_Y_CENTER },
            },
        }) {
            unsigned int t = node->term_recomment_to_studie;
            CLAY_TEXT(t > 0 ? DS("%u", t) : CLAY_STRING("--"),
                      TC(C_SUBTEXT, 11));
        }
    }
}

/* ═══════════════════════════════════════════════════════════════════════
 *  MAIN CONTENT
 * ═══════════════════════════════════════════════════════════════════════ */

static void RenderMainContent(void)
{
    Subject_Type *st = &gPlayer.numofSubjectType[gActiveNav];

    CLAY(CLAY_ID("Main"), {
        .layout = {
            .sizing          = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
            .padding         = { 20, 20, 18, 18 },
            .childGap        = 16,
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
        },
        .backgroundColor = C_BG,
    }) {

        /* ── Page header ── */
        CLAY(CLAY_ID("PageHdr"), {
            .layout = {
                .sizing          = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
                .childGap        = 12,
                .childAlignment  = { .y = CLAY_ALIGN_Y_CENTER },
                .layoutDirection = CLAY_LEFT_TO_RIGHT,
            },
        }) {
            CLAY(CLAY_ID("PHLeft"), {
                .layout = {
                    .sizing          = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
                    .layoutDirection = CLAY_TOP_TO_BOTTOM,
                    .childGap        = 4,
                },
            }) {
                CLAY_TEXT(CS(gTypeName[gActiveNav]), TC(C_TEXT, 20));
                CLAY_TEXT(
                    DS("Player: %s   |   %d subjects  |  passed %d/%d credits  |  CPA %.2f",
                       gUserName,
                       st->Total_Subject,
                       st->count_passCredit,
                       st->Total_Credit,
                       calc_cpa(&gPlayer, 0)),
                    TC(C_SUBTEXT, 11));
            }

            /* Ctrl+K hint pill */
            CLAY(CLAY_ID("CmdHint"), {
                .layout = {
                    .sizing         = { CLAY_SIZING_FIT(0), CLAY_SIZING_FIXED(30) },
                    .padding        = { 12, 12, 0, 0 },
                    .childAlignment = { .y = CLAY_ALIGN_Y_CENTER },
                },
                .backgroundColor = C_CARD,
                .cornerRadius    = CLAY_CORNER_RADIUS(6),
                .border          = { .color = C_BORDER,
                                     .width = { .left=1,.right=1,.top=1,.bottom=1 } },
            }) {
                CLAY_TEXT(CLAY_STRING("Ctrl+K  Command"), TC(C_SUBTEXT, 11));
            }
        }

        /* ── Summary cards ── */
        CLAY(CLAY_ID("SumRow"), {
            .layout = {
                .sizing          = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(88) },
                .childGap        = 12,
                .layoutDirection = CLAY_LEFT_TO_RIGHT,
            },
        }) {
            /* global credit stats */
            RenderSummaryCard(0, "Credits Passed",
                DS("%d", gPlayer.ToTal_credit_pass), C_GREEN);

            RenderSummaryCard(1, "Credits Failed",
                DS("%d", gPlayer.ToTal_credit_npass), C_RED);

            /* GPA (all studied + pass only) */
            float cpa_all  = calc_cpa(&gPlayer, 0);
            float cpa_pass = calc_cpa(&gPlayer, 1);
            RenderSummaryCard(2, "CPA (All studied)",
                DS("%.3f", cpa_all), C_ACCENT);

            RenderSummaryCard(3, "CPA (Pass only)",
                DS("%.3f", cpa_pass), C_YELLOW);

            /* overall graduation progress */
            {
                int eff = calc_effective_credits(&gPlayer);
                int req = calc_required_credits(&gPlayer);
                RenderSummaryCard(4, "Total Credits (eff/req)",
                    DS("%d / %d", eff, req),
                    eff >= req ? C_GREEN : C_YELLOW);
            }

            /* current section credits vs required limit */
            {
                int sec_want = _sl_resolve_limit(&gPlayer, gActiveNav);
                int sec_pass = (int)st->count_passCredit;
                RenderSummaryCard(5, "Section Credits (pass/want)",
                    DS("%d / %d", sec_pass, sec_want),
                    sec_pass >= sec_want ? C_GREEN : C_ACCENT);
            }

            /* graduation status */
            CLAY(CLAY_ID("GradCard"), {
                .layout = {
                    .sizing          = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
                    .padding         = { 14, 14, 10, 10 },
                    .childGap        = 4,
                    .layoutDirection = CLAY_TOP_TO_BOTTOM,
                },
                .backgroundColor = gPlayer.status_can_grauate ? C_GREEN_BG : C_RED_BG,
                .cornerRadius    = CLAY_CORNER_RADIUS(7),
                .border          = { .color = gPlayer.status_can_grauate ? C_GREEN : C_RED,
                                     .width = { .left=1,.right=1,.top=1,.bottom=1 } },
            }) {
                CLAY_TEXT(CLAY_STRING("Graduation"), TC(C_SUBTEXT, 10));
                if (gPlayer.status_can_grauate) {
                    CLAY_TEXT(CLAY_STRING("READY"), TC(C_GREEN, 20));
                } else {
                    int eff = calc_effective_credits(&gPlayer);
                    int req = calc_required_credits(&gPlayer);
                    CLAY_TEXT(DS("%d/%d cr", eff, req), TC(C_RED, 18));
                    CLAY_TEXT(CLAY_STRING("Not ready"), TC(C_SUBTEXT, 10));
                }
            }
        }

        /* ── Academic alert banner (hidden when alert level = 0) ── */
        if (gPlayer.status_alert > 0) {
            static const Clay_Color alert_fg[4] = {
                {  0,  0,  0,  0},            /* 0: unused      */
                {234,179,  8,255},            /* 1: C_YELLOW    */
                { 99,102,241,255},            /* 2: C_ACCENT    */
                {239, 68, 68,255},            /* 3: C_RED       */
            };
            static const Clay_Color alert_bg[4] = {
                {  0,  0,  0,  0},
                { 60, 50, 10,255},            /* C_YELLOW_BG    */
                { 22, 23, 58,255},            /* C_ACCENT_BG    */
                { 70, 18, 18,255},            /* C_RED_BG       */
            };
            static const char *alert_label[4] = {
                "", "Caution", "Warning", "Critical"
            };
            CLAY(CLAY_ID("AlertBanner"), {
                .layout = {
                    .sizing         = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(32) },
                    .padding        = { 14, 14, 0, 0 },
                    .childAlignment = { .y = CLAY_ALIGN_Y_CENTER },
                },
                .backgroundColor = alert_bg[gPlayer.status_alert],
                .cornerRadius    = CLAY_CORNER_RADIUS(6),
                .border = { .color = alert_fg[gPlayer.status_alert],
                            .width = { .left = 3 } },
            }) {
                CLAY_TEXT(
                    DS("Academic Alert  [Level %d — %s]  %d studied-but-failed credits",
                       (int)gPlayer.status_alert,
                       alert_label[gPlayer.status_alert],
                       gPlayer.ToTal_credit_npass),
                    TC(alert_fg[gPlayer.status_alert], 11));
            }
        }

        /* ── Data-file validation warnings (from DB_ValidateData) ── */
        if (gDataWarnCount > 0) {
            CLAY(CLAY_ID("DataWarnBanner"), {
                .layout = {
                    .sizing          = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
                    .padding         = { 14, 14, 8, 8 },
                    .childGap        = 4,
                    .layoutDirection = CLAY_TOP_TO_BOTTOM,
                },
                .backgroundColor = C_RED_BG,
                .cornerRadius    = CLAY_CORNER_RADIUS(6),
                .border          = { .color = C_RED, .width = { .left = 3 } },
            }) {
                CLAY_TEXT(CLAY_STRING("Data file validation errors:"), TC(C_RED, 11));
                const char *_w = gDataWarningsBuf;
                for (int _k = 0; _k < gDataWarnCount; _k++) {
                    CLAY_TEXT(DS("  * %s", _w), TC(C_WHITE, 11));
                    _w += strlen(_w) + 1;
                }
            }
        }

        /* ── Table card ── */
        CLAY(CLAY_ID("TblCard"), {
            .layout = {
                .sizing          = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
            },
            .backgroundColor = C_CARD,
            .cornerRadius    = CLAY_CORNER_RADIUS(8),
            .border          = { .color = C_BORDER,
                                 .width = { .left=1,.right=1,.top=1,.bottom=1 } },
        }) {
            RenderTableHeader();

            /* scrollable body */
            CLAY(CLAY_ID("TblBody"), {
                .layout = {
                    .sizing          = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
                    .layoutDirection = CLAY_TOP_TO_BOTTOM,
                },
                .clip = { .vertical = true, .childOffset = Clay_GetScrollOffset() },
            }) {
                Subject_Node *node = st->head;
                int i = 0;
                while (node) {
                    RenderTableRow(node, i++);
                    node = node->next;
                }
                if (i == 0) {
                    /* empty placeholder */
                    CLAY(CLAY_ID("EmptyRow"), {
                        .layout = {
                            .sizing         = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(64) },
                            .childAlignment = { .x = CLAY_ALIGN_X_CENTER,
                                                .y = CLAY_ALIGN_Y_CENTER },
                        },
                    }) {
                        CLAY_TEXT(CLAY_STRING("No subjects in this category."),
                                  TC(C_SUBTEXT, 12));
                    }
                }
            }

            /* Footer */
            CLAY(CLAY_ID("TblFoot"), {
                .layout = {
                    .sizing          = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(40) },
                    .padding         = { 14, 14, 0, 0 },
                    .childGap        = 8,
                    .childAlignment  = { .y = CLAY_ALIGN_Y_CENTER },
                    .layoutDirection = CLAY_LEFT_TO_RIGHT,
                },
                .backgroundColor = C_TBL_HDR,
                .border          = { .color = C_BORDER, .width = { .top = 1 } },
            }) {
                CLAY_TEXT(
                    DS("Showing %d subjects  |  %d passed  |  %d/%d credits  |  CPA %.2f",
                       st->Total_Subject,
                       st->count_passSubject,
                       st->count_passCredit,
                       st->Total_Credit,
                       calc_cpa_type(&gPlayer, gActiveNav, 0)),
                    TC(C_SUBTEXT, 10));

                CLAY(CLAY_ID("FtSpacer"), {
                    .layout = { .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(1) } },
                }) {}

                CLAY_TEXT(
                    DS("Total passed credits: %d", gPlayer.ToTal_credit_pass),
                    TC(C_ACCENT, 10));
            }
        }
    }
}

/* ═══════════════════════════════════════════════════════════════════════
 *  DASHBOARD PAGE
 * ═══════════════════════════════════════════════════════════════════════ */

/* Colors used for each grade band in charts */
#define C_GRADE_A  ((Clay_Color){ 34, 197,  94, 255})
#define C_GRADE_B  ((Clay_Color){ 99, 102, 241, 255})
#define C_GRADE_C  ((Clay_Color){234, 179,   8, 255})
#define C_GRADE_D  ((Clay_Color){249, 115,  22, 255})
#define C_GRADE_F  ((Clay_Color){239,  68,  68, 255})
#define C_GRADE_X  ((Clay_Color){ 60,  60,  90, 255})

/* Per-type colors cycle for the progress bars */
static const Clay_Color kTypeColors[13] = {
    {  99, 102, 241, 255}, /* 1  co_so_nganh        */
    {  34, 197,  94, 255}, /* 2  dai_cuong          */
    { 249, 115,  22, 255}, /* 3  the_thao           */
    { 234, 179,   8, 255}, /* 4  ly_luat_chinh_tri  */
    {  20, 184, 166, 255}, /* 5  tu_chon            */
    { 168,  85, 247, 255}, /* 6  modunI             */
    { 236,  72, 153, 255}, /* 7  modunII            */
    {  14, 165, 233, 255}, /* 8  modunIII           */
    { 132, 204,  22, 255}, /* 9  modunIV            */
    { 251, 191,  36, 255}, /* 10 modunV             */
    { 239,  68,  68, 255}, /* 11 modunVI            */
    {  52, 211, 153, 255}, /* 12 thuc_tap           */
    { 248, 113, 113, 255}, /* 13 do_an_tot_nghiep   */
};

/*
 * Donut placeholder element IDs — main.c reads their bounding boxes after
 * Clay_Raylib_Render and draws the Raylib rings on top.
 * Index 0 = grade-distribution donut; indices 1-13 = per-type mini-ring.
 */
#define DONUT_GRADE_ID  "DashDonutGrade"
#define DONUT_CPA_ID    "DashDonutCPA"

static void RenderDashStatCard(int idx, const char *title,
                                Clay_String val, Clay_Color accent,
                                const char *sub)
{
    CLAY(CLAY_IDI("DStatCard", idx), {
        .layout = {
            .sizing          = { CLAY_SIZING_PERCENT(0.2f), CLAY_SIZING_FIT(0) },
            .padding         = { 16, 16, 14, 14 },
            .childGap        = 5,
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
        },
        .backgroundColor = C_CARD,
        .cornerRadius    = CLAY_CORNER_RADIUS(8),
        .border          = { .color = accent,
                             .width = { .left=3,.right=0,.top=0,.bottom=0 } },
    }) {
        CLAY_TEXT(CS(title), TC(C_SUBTEXT, 10));
        CLAY_TEXT(val, TC(C_TEXT, 24));
        if (sub && sub[0])
            CLAY_TEXT(CS(sub), TC(C_SUBTEXT, 9));
    }
}

static void RenderDashboard(void)
{
    /* pre-compute grade counts across ALL subject types */
    int cnt_A=0, cnt_B=0, cnt_C=0, cnt_D=0, cnt_F=0, cnt_X=0;
    int total_studied = 0;
    for (int t = 1; t < sizeSubjectType; t++) {
        Subject_Node *n = gPlayer.numofSubjectType[t].head;
        while (n) {
            if (n->status_ever_been_study & 1) {
                switch (n->score_letter) {
                    case 'A': cnt_A++; break;
                    case 'B': cnt_B++; break;
                    case 'C': cnt_C++; break;
                    case 'D': cnt_D++; break;
                    case 'F': cnt_F++; break;
                    default:  cnt_X++; break;
                }
                total_studied++;
            } else {
                cnt_X++;
            }
            n = n->next;
        }
    }
    /* All subjects regardless of study */
    int total_subjects = 0;
    for (int t = 1; t < sizeSubjectType; t++)
        total_subjects += gPlayer.numofSubjectType[t].Total_Subject;
    if (total_studied == 0) total_studied = 1; /* avoid div-0 for arcs */

    float cpa_all  = calc_cpa(&gPlayer, 0);
    int   eff      = calc_effective_credits(&gPlayer);
    int   req      = calc_required_credits(&gPlayer);

    CLAY(CLAY_ID("Dash"), {
        .layout = {
            .sizing          = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
            .padding         = { 20, 20, 16, 16 },
            .childGap        = 16,
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
        },
        .backgroundColor = C_BG,
        .clip = { .vertical = true, .childOffset = Clay_GetScrollOffset() },
    }) {

        /* ── Page title ── */
        CLAY(CLAY_ID("DashHdr"), {
            .layout = {
                .sizing          = { CLAY_SIZING_FIT(0), CLAY_SIZING_FIT(0) },
                .childGap        = 6,
                .childAlignment  = { .y = CLAY_ALIGN_Y_CENTER },
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
            },
        }) {
            CLAY_TEXT(CLAY_STRING("Dashboard"), TC(C_TEXT, 20));
            CLAY_TEXT(
                DS("Overview for %s  |  %d total subjects",
                   gUserName, total_subjects),
                TC(C_SUBTEXT, 11));
        }

        /* ── Top stat cards ── */
        CLAY(CLAY_ID("DashCards"), {
            .layout = {
                .sizing          = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
                .childGap        = 12,
                .layoutDirection = CLAY_LEFT_TO_RIGHT,
            },
        }) {
            RenderDashStatCard(0, "Credits Passed",
                DS("%d", gPlayer.ToTal_credit_pass), C_GREEN,
                "studied & passed");
            RenderDashStatCard(1, "Credits Failed",
                DS("%d", gPlayer.ToTal_credit_npass), C_RED,
                "studied but failed");
            RenderDashStatCard(2, "CPA (All studied)",
                DS("%.3f", cpa_all), C_ACCENT,
                "4.0 scale");
            RenderDashStatCard(3, "Progress",
                DS("%d / %d cr", eff, req),
                eff >= req ? C_GREEN : C_YELLOW,
                eff >= req ? "Ready to graduate" : "Effective / Required");
            /* graduation status card */
            CLAY(CLAY_IDI("DStatCard", 4), {
                .layout = {
                    .sizing          = { CLAY_SIZING_PERCENT(0.2f), CLAY_SIZING_FIT(0) },
                    .padding         = { 16, 16, 14, 14 },
                    .childGap        = 5,
                    .layoutDirection = CLAY_TOP_TO_BOTTOM,
                },
                .backgroundColor = gPlayer.status_can_grauate ? C_GREEN_BG : C_RED_BG,
                .cornerRadius    = CLAY_CORNER_RADIUS(8),
                .border          = {
                    .color = gPlayer.status_can_grauate ? C_GREEN : C_RED,
                    .width = { .left=3 },
                },
            }) {
                CLAY_TEXT(CLAY_STRING("Graduation"), TC(C_SUBTEXT, 10));
                CLAY_TEXT(
                    gPlayer.status_can_grauate ? CLAY_STRING("READY")
                                               : CLAY_STRING("NOT READY"),
                    TC(gPlayer.status_can_grauate ? C_GREEN : C_RED, 22));
            }
        }

        /* ── Middle row: charts ── */
        CLAY(CLAY_ID("DashMid"), {
            .layout = {
                .sizing          = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
                .childGap        = 16,
                .layoutDirection = CLAY_LEFT_TO_RIGHT,
            },
        }) {

            /* ── Grade distribution donut (placeholder box — drawn by Raylib) ── */
            CLAY(CLAY_ID("DashGradeCard"), {
                .layout = {
                    .sizing          = { CLAY_SIZING_PERCENT(0.4f), CLAY_SIZING_GROW(0) },
                    .padding         = { 14, 14, 12, 12 },
                    .childGap        = 8,
                    .layoutDirection = CLAY_TOP_TO_BOTTOM,
                },
                .backgroundColor = C_CARD,
                .cornerRadius    = CLAY_CORNER_RADIUS(8),
                .border          = { .color = C_BORDER,
                                     .width = { .left=1,.right=1,.top=1,.bottom=1 } },
            }) {
                CLAY_TEXT(CLAY_STRING("Grade Distribution"), TC(C_SUBTEXT, 10));
                /* donut placeholder — Raylib draws into this element's bounding box */
                CLAY(CLAY_ID(DONUT_GRADE_ID), {
                    .layout = {
                        .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
                    },
                    .backgroundColor = C_TRANS,
                }) {}
                /* legend */
                CLAY(CLAY_ID("DGLeg"), {
                    .layout = {
                        .sizing          = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
                        .childGap        = 6,
                        .layoutDirection = CLAY_LEFT_TO_RIGHT,
                    },
                }) {
#define DLEG(lid, col, lbl) \
    CLAY(CLAY_ID(lid), { \
        .layout = { .childGap=4, .childAlignment={ .y=CLAY_ALIGN_Y_CENTER }, \
                    .layoutDirection=CLAY_LEFT_TO_RIGHT }, \
    }) { \
        CLAY(CLAY_ID(lid"Dot"), { \
            .layout={ .sizing={ CLAY_SIZING_FIXED(8), CLAY_SIZING_FIXED(8) } }, \
            .backgroundColor=(col), .cornerRadius=CLAY_CORNER_RADIUS(4), \
        }) {} \
        CLAY_TEXT(CS(lbl), TC(C_SUBTEXT, 9)); \
    }
                    DLEG("LegA", C_GRADE_A, "A")
                    DLEG("LegB", C_GRADE_B, "B")
                    DLEG("LegC", C_GRADE_C, "C")
                    DLEG("LegD", C_GRADE_D, "D")
                    DLEG("LegF", C_GRADE_F, "F")
                    DLEG("LegX", C_GRADE_X, "X")
#undef DLEG
                }
            }

            /* ── CPA gauge donut ── */
            CLAY(CLAY_ID("DashCPACard"), {
                .layout = {
                    .sizing          = { CLAY_SIZING_PERCENT(0.3f), CLAY_SIZING_GROW(0) },
                    .padding         = { 14, 14, 12, 12 },
                    .childGap        = 8,
                    .layoutDirection = CLAY_TOP_TO_BOTTOM,
                },
                .backgroundColor = C_CARD,
                .cornerRadius    = CLAY_CORNER_RADIUS(8),
                .border          = { .color = C_BORDER,
                                     .width = { .left=1,.right=1,.top=1,.bottom=1 } },
            }) {
                CLAY_TEXT(CLAY_STRING("CPA Gauge"), TC(C_SUBTEXT, 10));
                CLAY(CLAY_ID(DONUT_CPA_ID), {
                    .layout = {
                        .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
                    },
                    .backgroundColor = C_TRANS,
                }) {}
                CLAY_TEXT(DS("CPA  %.3f / 4.00", cpa_all), TC(C_ACCENT, 10));
            }

            /* ── Per-type credit progress bars ── */
            CLAY(CLAY_ID("DashBarsCard"), {
                .layout = {
                    .sizing          = { CLAY_SIZING_PERCENT(0.3f), CLAY_SIZING_GROW(0) },
                    .padding         = { 16, 16, 12, 12 },
                    .childGap        = 7,
                    .layoutDirection = CLAY_TOP_TO_BOTTOM,
                },
                .backgroundColor = C_CARD,
                .cornerRadius    = CLAY_CORNER_RADIUS(8),
                .border          = { .color = C_BORDER,
                                     .width = { .left=1,.right=1,.top=1,.bottom=1 } },
            }) {
                CLAY_TEXT(CLAY_STRING("Credits by Type"), TC(C_SUBTEXT, 10));
                for (int t = 1; t < sizeSubjectType; t++) {
                        Subject_Type *st = &gPlayer.numofSubjectType[t];
                        if (gTypeName[t][0] == 0 || st->Total_Subject == 0) continue;

                        int pass = (int)st->count_passCredit;
                        int tot  = st->Total_Credit > 0 ? st->Total_Credit : 1;
                        int lim  = _sl_resolve_limit(&gPlayer, t);
                        if (lim <= 0) lim = tot;

                        float pct = (float)pass / (float)(lim > 0 ? lim : 1);
                        if (pct > 1.f) pct = 1.f;

                        Clay_Color col = kTypeColors[t - 1];

                        CLAY(CLAY_IDI("DBar", t), {
                            .layout = {
                                .sizing          = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
                                .childGap        = 4,
                                .layoutDirection = CLAY_TOP_TO_BOTTOM,
                            },
                        }) {
                            /* label row */
                            CLAY(CLAY_IDI("DBarLbl", t), {
                                .layout = {
                                    .sizing          = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
                                    .childGap        = 4,
                                    .childAlignment  = { .y = CLAY_ALIGN_Y_CENTER },
                                    .layoutDirection = CLAY_LEFT_TO_RIGHT,
                                },
                            }) {
                                CLAY_TEXT(CS(gTypeName[t]), TC(C_TEXT, 9));
                                CLAY(CLAY_IDI("DBarSp", t), {
                                    .layout = { .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(1) } },
                                }) {}
                                CLAY_TEXT(
                                    DS("%d/%d cr", pass, lim),
                                    TC(pct >= 1.f ? C_GREEN : C_SUBTEXT, 9));
                            }
                            /* track */
                            CLAY(CLAY_IDI("DBarTrack", t), {
                                .layout = {
                                    .sizing          = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(8) },
                                    .layoutDirection = CLAY_LEFT_TO_RIGHT,
                                },
                                .backgroundColor = C_BORDER,
                                .cornerRadius    = CLAY_CORNER_RADIUS(4),
                            }) {
                                /* filled portion */
                                CLAY(CLAY_IDI("DBarFill", t), {
                                    .layout = {
                                        .sizing = { CLAY_SIZING_PERCENT(pct > 0.f ? pct : 0.01f),
                                                    CLAY_SIZING_GROW(0) },
                                    },
                                    .backgroundColor = col,
                                    .cornerRadius    = CLAY_CORNER_RADIUS(4),
                                }) {}
                            }
                        }
                    }
            }
        }

        /* ── Bottom: graduation checklist ── */
        CLAY(CLAY_ID("DashCheck"), {
            .layout = {
                .sizing          = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
                .padding         = { 16, 16, 12, 12 },
                .childGap        = 0,
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
            },
            .backgroundColor = C_CARD,
            .cornerRadius    = CLAY_CORNER_RADIUS(8),
            .border          = { .color = C_BORDER,
                                 .width = { .left=1,.right=1,.top=1,.bottom=1 } },
        }) {
            /* header */
            CLAY(CLAY_ID("DCheckHdr"), {
                .layout = {
                    .sizing          = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(32) },
                    .padding         = { 0, 0, 0, 8 },
                    .childGap        = 12,
                    .childAlignment  = { .y = CLAY_ALIGN_Y_CENTER },
                    .layoutDirection = CLAY_LEFT_TO_RIGHT,
                },
                .border = { .color = C_BORDER, .width = { .bottom = 1 } },
            }) {
                CLAY_TEXT(CLAY_STRING("Graduation Checklist"), TC(C_TEXT, 12));
                CLAY(CLAY_ID("DChkSp"), { .layout = { .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(1) } } }) {}
                CLAY_TEXT(
                    DS("%s",  gPlayer.status_can_grauate
                        ? "All requirements met" : "Requirements pending"),
                    TC(gPlayer.status_can_grauate ? C_GREEN : C_YELLOW, 10));
            }
            /* one row per type */
            {
                int _miss2[sizeSubjectType];
                calc_missing_types(&gPlayer, _miss2);
                for (int t = 1; t < sizeSubjectType; t++) {
                    Subject_Type *st2 = &gPlayer.numofSubjectType[t];
                    if (gTypeName[t][0] == 0 || st2->Total_Subject == 0) continue;

                    int pass2 = (int)st2->count_passCredit;
                    int lim2  = _sl_resolve_limit(&gPlayer, t);
                    bool done = (_miss2[t] == 0);
                    Clay_Color rowbg = done ? (Clay_Color){16,40,24,255}
                                            : C_TRANS;
                    Clay_Color fg2   = done ? C_GREEN : C_SUBTEXT;

                    CLAY(CLAY_IDI("DChkRow", t), {
                        .layout = {
                            .sizing          = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(36) },
                            .padding         = { 6, 6, 0, 0 },
                            .childGap        = 10,
                            .childAlignment  = { .y = CLAY_ALIGN_Y_CENTER },
                            .layoutDirection = CLAY_LEFT_TO_RIGHT,
                        },
                        .backgroundColor = rowbg,
                        .border = { .color = C_BORDER, .width = { .bottom=1 } },
                    }) {
                        /* checkmark box */
                        CLAY(CLAY_IDI("DChkBox", t), {
                            .layout = {
                                .sizing         = { CLAY_SIZING_FIXED(18), CLAY_SIZING_FIXED(18) },
                                .childAlignment = { .x=CLAY_ALIGN_X_CENTER, .y=CLAY_ALIGN_Y_CENTER },
                            },
                            .backgroundColor = done ? C_GREEN : C_BORDER,
                            .cornerRadius    = CLAY_CORNER_RADIUS(4),
                        }) {
                            if (done)
                                CLAY_TEXT(CLAY_STRING("v"), TC(C_WHITE, 9));
                        }
                        /* name */
                        CLAY_TEXT(CS(gTypeName[t]), TC(C_TEXT, 11));
                        /* spacer */
                        CLAY(CLAY_IDI("DChkRowSp", t), {
                            .layout = { .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(1) } },
                        }) {}
                        /* credit progress */
                        CLAY_TEXT(
                            DS("%d / %d cr", pass2, lim2 > 0 ? lim2 : st2->Total_Credit),
                            TC(fg2, 10));
                        /* subjects */
                        CLAY_TEXT(
                            DS("%d subj", st2->Total_Subject),
                            TC(C_SUBTEXT, 9));
                        /* CPA for this type */
                        float tcpa = calc_cpa_type(&gPlayer, t, 0);
                        CLAY_TEXT(
                            DS("CPA %.2f", tcpa),
                            TC(tcpa >= 2.0f ? C_ACCENT : C_RED, 9));
                    }
                }
            }
        }
    }
}

/* ═══════════════════════════════════════════════════════════════════════
 *  ROW EDIT POPUP
 * ═══════════════════════════════════════════════════════════════════════ */

static void RenderEditPopup(void)
{
    static const char *ratio_labels[4] = { "", "50 / 50", "40 / 60", "30 / 70" };

    /* full-screen backdrop */
    CLAY(CLAY_ID("EditBackdrop"), {
        .layout = {
            .sizing = { CLAY_SIZING_FIXED((float)gScreenW),
                        CLAY_SIZING_FIXED((float)gScreenH) },
        },
        .backgroundColor = (Clay_Color){ 0, 0, 0, 160 },
        .floating = {
            .attachTo = CLAY_ATTACH_TO_ROOT,
            .zIndex   = 15,
        },
    }) {}

    /* popup card */
    CLAY(CLAY_ID("EditCard"), {
        .layout = {
            .sizing          = { CLAY_SIZING_FIXED(520), CLAY_SIZING_FIT(0) },
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
            .childGap        = 0,
        },
        .backgroundColor = C_CARD,
        .cornerRadius    = CLAY_CORNER_RADIUS(12),
        .border          = { .color = C_ACCENT,
                             .width = { .left=1,.right=1,.top=1,.bottom=1 } },
        .floating = {
            .attachTo     = CLAY_ATTACH_TO_ROOT,
            .attachPoints = { .element = CLAY_ATTACH_POINT_CENTER_CENTER,
                              .parent  = CLAY_ATTACH_POINT_CENTER_CENTER },
            .zIndex       = 20,
        },
    }) {

        /* ── Header ── */
        CLAY(CLAY_ID("EditHdr"), {
            .layout = {
                .sizing          = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(54) },
                .padding         = { 18, 18, 0, 0 },
                .childGap        = 6,
                .childAlignment  = { .y = CLAY_ALIGN_Y_CENTER },
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
            },
            .backgroundColor = C_TBL_HDR,
            .border          = { .color = C_BORDER, .width = { .bottom = 1 } },
        }) {
            CLAY_TEXT(CS(gEditSubjectName), TC(C_TEXT, 13));
            CLAY_TEXT(DS("Code: %s", gEditCode),  TC(C_ACCENT, 10));
        }

        /* ── Body ── */
        CLAY(CLAY_ID("EditBody"), {
            .layout = {
                .sizing          = { CLAY_SIZING_FIT(0), CLAY_SIZING_FIT(0) },
                .padding         = { 18, 18, 14, 14 },
                .childGap        = 12,
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
            },
        }) {

            /* Score input row */
            CLAY(CLAY_ID("EditScoreRow"), {
                .layout = {
                    .sizing          = { CLAY_SIZING_FIXED(480), CLAY_SIZING_FIT(0) },
                    .childGap        = 12,
                    .layoutDirection = CLAY_LEFT_TO_RIGHT,
                },
            }) {
                /* Midterm field */
                CLAY(CLAY_ID("EditMidCol"), {
                    .layout = {
                        .sizing          = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
                        .childGap        = 6,
                        .layoutDirection = CLAY_TOP_TO_BOTTOM,
                    },
                }) {
                    CLAY_TEXT(CLAY_STRING("Midterm  (0 — 10)"), TC(C_SUBTEXT, 10));
                    CLAY(CLAY_ID("EditMidBox"), {
                        .layout = {
                            .sizing          = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(38) },
                            .padding         = { 10, 10, 0, 0 },
                            .childAlignment  = { .y = CLAY_ALIGN_Y_CENTER },
                            .layoutDirection = CLAY_LEFT_TO_RIGHT,
                        },
                        .backgroundColor = (gEditField == 0) ? C_ACCENT_BG : C_TBL_HDR,
                        .cornerRadius    = CLAY_CORNER_RADIUS(6),
                        .border = { .color = (gEditField == 0) ? C_ACCENT : C_BORDER,
                                    .width = { .left=1,.right=1,.top=1,.bottom=1 } },
                    }) {
                        bool cursorOn = ((int)(GetTime() * 2) % 2) == 0;
                        if (gEditMidLen > 0)
                            CLAY_TEXT(cursorOn && gEditField==0
                                        ? DS("%s|", gEditMidBuf)
                                        : DS("%s",  gEditMidBuf),
                                      TC(C_TEXT, 13));
                        else
                            CLAY_TEXT(gEditField==0 && cursorOn
                                        ? CLAY_STRING("|") : CLAY_STRING("—"),
                                      TC(C_SUBTEXT, 13));
                        /* click to focus */
                        if (Clay_Hovered() && IsMouseButtonReleased(MOUSE_LEFT_BUTTON))
                            gEditField = 0;
                    }
                }

                /* Final field */
                CLAY(CLAY_ID("EditFinCol"), {
                    .layout = {
                        .sizing          = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
                        .childGap        = 6,
                        .layoutDirection = CLAY_TOP_TO_BOTTOM,
                    },
                }) {
                    CLAY_TEXT(CLAY_STRING("Final  (0 — 10)"), TC(C_SUBTEXT, 10));
                    CLAY(CLAY_ID("EditFinBox"), {
                        .layout = {
                            .sizing          = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(38) },
                            .padding         = { 10, 10, 0, 0 },
                            .childAlignment  = { .y = CLAY_ALIGN_Y_CENTER },
                            .layoutDirection = CLAY_LEFT_TO_RIGHT,
                        },
                        .backgroundColor = (gEditField == 1) ? C_ACCENT_BG : C_TBL_HDR,
                        .cornerRadius    = CLAY_CORNER_RADIUS(6),
                        .border = { .color = (gEditField == 1) ? C_ACCENT : C_BORDER,
                                    .width = { .left=1,.right=1,.top=1,.bottom=1 } },
                    }) {
                        bool cursorOn = ((int)(GetTime() * 2) % 2) == 0;
                        if (gEditFinLen > 0)
                            CLAY_TEXT(cursorOn && gEditField==1
                                        ? DS("%s|", gEditFinBuf)
                                        : DS("%s",  gEditFinBuf),
                                      TC(C_TEXT, 13));
                        else
                            CLAY_TEXT(gEditField==1 && cursorOn
                                        ? CLAY_STRING("|") : CLAY_STRING("—"),
                                      TC(C_SUBTEXT, 13));
                        /* click to focus */
                        if (Clay_Hovered() && IsMouseButtonReleased(MOUSE_LEFT_BUTTON))
                            gEditField = 1;
                    }
                }
            }

            /* Tab hint */
            CLAY_TEXT(CLAY_STRING("Tab  switch field"), TC(C_SUBTEXT, 9));

            /* Ratio selector */
            CLAY(CLAY_ID("EditRatioRow"), {
                .layout = {
                    .sizing          = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
                    .childGap        = 8,
                    .childAlignment  = { .y = CLAY_ALIGN_Y_CENTER },
                    .layoutDirection = CLAY_LEFT_TO_RIGHT,
                },
            }) {
                CLAY_TEXT(CLAY_STRING("Ratio:"), TC(C_SUBTEXT, 10));
                for (int r = 1; r <= 3; r++) {
                    bool sel = (gEditRatio == r);
                    CLAY(CLAY_IDI("RatioBtn", r), {
                        .layout = {
                            .sizing         = { CLAY_SIZING_FIT(0), CLAY_SIZING_FIXED(28) },
                            .padding        = { 12, 12, 4, 4 },
                            .childAlignment = { .x = CLAY_ALIGN_X_CENTER,
                                                .y = CLAY_ALIGN_Y_CENTER },
                        },
                        .backgroundColor = sel ? C_ACCENT
                                         : (Clay_Hovered() ? C_ROW_HOVER : C_BORDER),
                        .cornerRadius    = CLAY_CORNER_RADIUS(5),
                    }) {
                        if (Clay_Hovered() && IsMouseButtonReleased(MOUSE_LEFT_BUTTON))
                            gEditRatio = r;
                        CLAY_TEXT(DS("%s", ratio_labels[r]),
                                  TC(sel ? C_WHITE : C_TEXT, 10));
                    }
                }
            }
        }

        /* ── Footer buttons ── */
        CLAY(CLAY_ID("EditFooter"), {
            .layout = {
                .sizing          = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(52) },
                .padding         = { 18, 18, 0, 0 },
                .childGap        = 10,
                .childAlignment  = { .y = CLAY_ALIGN_Y_CENTER },
                .layoutDirection = CLAY_LEFT_TO_RIGHT,
            },
            .backgroundColor = C_TBL_HDR,
            .border          = { .color = C_BORDER, .width = { .top = 1 } },
        }) {

            /* Save */
            CLAY(CLAY_ID("EditSave"), {
                .layout = {
                    .sizing         = { CLAY_SIZING_FIT(0), CLAY_SIZING_FIXED(34) },
                    .padding        = { 18, 18, 6, 6 },
                    .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER },
                },
                .backgroundColor = Clay_Hovered() ? C_GREEN : C_GREEN_BG,
                .cornerRadius    = CLAY_CORNER_RADIUS(6),
                .border          = { .color = C_GREEN,
                                     .width = { .left=1,.right=1,.top=1,.bottom=1 } },
            }) {
                if (Clay_Hovered() && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
                    float mid = (float)atof(gEditMidBuf);
                    float fin = (float)atof(gEditFinBuf);
                    if (mid >= 0.f && mid <= 10.f && fin >= 0.f && fin <= 10.f
                            && DB_SubjectExists(gEditCode)) {
                        DB_UpdateScoreRatio(gEditCode, mid, fin, gEditRatio);
                        RefreshPlayer();
                        snprintf(gResultMsg, sizeof(gResultMsg),
                                 "Saved %s: mid=%.2f  final=%.2f  ratio=%s",
                                 gEditCode, mid, fin, ratio_labels[gEditRatio]);
                        gHasResult       = true;
                        gResultShowUntil = (float)GetTime() + 5.f;
                        gEditOpen = false;
                    }
                }
                CLAY_TEXT(CLAY_STRING("Save"), TC(C_WHITE, 11));
            }

            /* Reset */
            CLAY(CLAY_ID("EditReset"), {
                .layout = {
                    .sizing         = { CLAY_SIZING_FIT(0), CLAY_SIZING_FIXED(34) },
                    .padding        = { 18, 18, 6, 6 },
                    .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER },
                },
                .backgroundColor = Clay_Hovered() ? C_RED : C_RED_BG,
                .cornerRadius    = CLAY_CORNER_RADIUS(6),
                .border          = { .color = C_RED,
                                     .width = { .left=1,.right=1,.top=1,.bottom=1 } },
            }) {
                if (Clay_Hovered() && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
                    DB_ClearScore(gEditCode);
                    RefreshPlayer();
                    snprintf(gResultMsg, sizeof(gResultMsg),
                             "Reset score for %s", gEditCode);
                    gHasResult       = true;
                    gResultShowUntil = (float)GetTime() + 5.f;
                    gEditOpen = false;
                }
                CLAY_TEXT(CLAY_STRING("Reset"), TC(C_WHITE, 11));
            }

            /* spacer */
            CLAY(CLAY_ID("EditBtnSp"), {
                .layout = { .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(1) } },
            }) {}

            /* Cancel */
            CLAY(CLAY_ID("EditCancel"), {
                .layout = {
                    .sizing         = { CLAY_SIZING_FIT(0), CLAY_SIZING_FIXED(34) },
                    .padding        = { 18, 18, 6, 6 },
                    .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER },
                },
                .backgroundColor = Clay_Hovered() ? C_ROW_HOVER : C_BORDER,
                .cornerRadius    = CLAY_CORNER_RADIUS(6),
            }) {
                if (Clay_Hovered() && IsMouseButtonReleased(MOUSE_LEFT_BUTTON))
                    gEditOpen = false;
                CLAY_TEXT(CLAY_STRING("Cancel"), TC(C_TEXT, 11));
            }
        }
    }
}

/* ═══════════════════════════════════════════════════════════════════════
 *  NAME INPUT SCREEN  (shown on startup before DB is opened)
 * ═══════════════════════════════════════════════════════════════════════ */

static void RenderNameInput(void)
{
    /* full-screen background */
    CLAY(CLAY_ID("NIBackdrop"), {
        .layout = {
            .sizing = { CLAY_SIZING_FIXED((float)gScreenW),
                        CLAY_SIZING_FIXED((float)gScreenH) },
        },
        .backgroundColor = C_BG,
        .floating = {
            .attachTo = CLAY_ATTACH_TO_ROOT,
            .zIndex   = 20,
        },
    }) {}

    /* centered card */
    CLAY(CLAY_ID("NICard"), {
        .layout = {
            .sizing          = { CLAY_SIZING_FIXED(480), CLAY_SIZING_FIT(0) },
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
            .childGap        = 0,
        },
        .backgroundColor = C_CARD,
        .cornerRadius    = CLAY_CORNER_RADIUS(12),
        .border          = { .color = C_ACCENT,
                             .width = { .left=1,.right=1,.top=1,.bottom=1 } },
        .floating = {
            .attachTo     = CLAY_ATTACH_TO_ROOT,
            .attachPoints = { .element = CLAY_ATTACH_POINT_CENTER_CENTER,
                              .parent  = CLAY_ATTACH_POINT_CENTER_CENTER },
            .zIndex       = 25,
        },
    }) {
        /* header */
        CLAY(CLAY_ID("NIHdr"), {
            .layout = {
                .sizing          = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(66) },
                .padding         = { 22, 22, 0, 0 },
                .childGap        = 4,
                .childAlignment  = { .y = CLAY_ALIGN_Y_CENTER },
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
            },
            .backgroundColor = C_TBL_HDR,
            .border          = { .color = C_BORDER, .width = { .bottom = 1 } },
        }) {
            CLAY_TEXT(CLAY_STRING("Transcript Viewer"), TC(C_TEXT, 15));
            CLAY_TEXT(CLAY_STRING("Enter your username to continue"),
                      TC(C_SUBTEXT, 10));
        }

        /* label + input box */
        CLAY(CLAY_ID("NIInputArea"), {
            .layout = {
                .sizing          = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
                .padding         = { 22, 22, 14, 14 },
                .childGap        = 8,
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
            },
        }) {
            CLAY_TEXT(CLAY_STRING("Username  (max 25 chars)"), TC(C_SUBTEXT, 10));
            CLAY(CLAY_ID("NIBox"), {
                .layout = {
                    .sizing          = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(40) },
                    .padding         = { 12, 12, 0, 0 },
                    .childAlignment  = { .y = CLAY_ALIGN_Y_CENTER },
                    .layoutDirection = CLAY_LEFT_TO_RIGHT,
                },
                .backgroundColor = C_ACCENT_BG,
                .cornerRadius    = CLAY_CORNER_RADIUS(6),
                .border          = { .color = C_ACCENT,
                                     .width = { .left=1,.right=1,.top=1,.bottom=1 } },
            }) {
                bool cursorOn = ((int)(GetTime() * 2) % 2) == 0;
                if (gNameLen > 0)
                    CLAY_TEXT(cursorOn ? DS("%s|", gUserName) : DS("%s ", gUserName),
                              TC(C_TEXT, 13));
                else
                    CLAY_TEXT(cursorOn ? CLAY_STRING("|") : CLAY_STRING(" "),
                              TC(C_SUBTEXT, 13));
            }
        }

        /* status line */
        CLAY(CLAY_ID("NIStatus"), {
            .layout = {
                .sizing          = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(34) },
                .padding         = { 22, 22, 0, 0 },
                .childAlignment  = { .y = CLAY_ALIGN_Y_CENTER },
                .layoutDirection = CLAY_LEFT_TO_RIGHT,
            },
            .border = { .color = C_BORDER, .width = { .top = 1 } },
        }) {
            if (gNameLen > 0) {
                if (DB_Exists(gUserName))
                    CLAY_TEXT(DS("db_%s.db  found | load existing data", gUserName),
                              TC(C_GREEN, 10));
                else
                    CLAY_TEXT(DS("db_%s.db  will be created", gUserName),
                              TC(C_ACCENT, 10));
            } else {
                CLAY_TEXT(CLAY_STRING("Type your username and press Enter"),
                          TC(C_SUBTEXT, 10));
            }
        }

        /* footer */
        CLAY(CLAY_ID("NIFooter"), {
            .layout = {
                .sizing          = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(36) },
                .padding         = { 22, 22, 0, 0 },
                .childGap        = 24,
                .childAlignment  = { .y = CLAY_ALIGN_Y_CENTER },
                .layoutDirection = CLAY_LEFT_TO_RIGHT,
            },
            .backgroundColor = C_TBL_HDR,
            .border          = { .color = C_BORDER, .width = { .top = 1 } },
        }) {
            CLAY_TEXT(CLAY_STRING("Enter  Confirm"), TC(C_SUBTEXT, 10));
            CLAY_TEXT(CLAY_STRING("ESC  Quit"),      TC(C_SUBTEXT, 10));
        }
    }
}

/* ═══════════════════════════════════════════════════════════════════════
 *  COMMAND PALETTE OVERLAY
 * ═══════════════════════════════════════════════════════════════════════ */

static void RenderCommandPopup(void)
{
    /* full-screen backdrop */
    CLAY(CLAY_ID("PopupBackdrop"), {
        .layout = {
            .sizing = { CLAY_SIZING_FIXED((float)gScreenW),
                        CLAY_SIZING_FIXED((float)gScreenH) },
        },
        .backgroundColor = (Clay_Color){ 0, 0, 0, 150 },
        .floating = {
            .attachTo = CLAY_ATTACH_TO_ROOT,
            .zIndex   = 5,
        },
    }) {}

    /* popup card */
    CLAY(CLAY_ID("PopupCard"), {
        .layout = {
            .sizing          = { CLAY_SIZING_FIXED(580), CLAY_SIZING_FIT(0) },
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
            .childGap        = 0,
        },
        .backgroundColor = C_CARD,
        .cornerRadius    = CLAY_CORNER_RADIUS(12),
        .border          = { .color = C_ACCENT,
                             .width = { .left=1,.right=1,.top=1,.bottom=1 } },
        .floating = {
            .attachTo     = CLAY_ATTACH_TO_ROOT,
            .attachPoints = { .element = CLAY_ATTACH_POINT_CENTER_CENTER,
                              .parent  = CLAY_ATTACH_POINT_CENTER_CENTER },
            .zIndex       = 10,
        },
    }) {
        /* header */
        CLAY(CLAY_ID("PUHdr"), {
            .layout = {
                .sizing          = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(44) },
                .padding         = { 16, 16, 0, 0 },
                .childGap        = 10,
                .childAlignment  = { .y = CLAY_ALIGN_Y_CENTER },
                .layoutDirection = CLAY_LEFT_TO_RIGHT,
            },
            .backgroundColor = C_TBL_HDR,
            .border          = { .color = C_BORDER, .width = { .bottom = 1 } },
        }) {
            CLAY(CLAY_ID("PUIconBadge"), {
                .layout = {
                    .sizing         = { CLAY_SIZING_FIXED(24), CLAY_SIZING_FIXED(24) },
                    .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER },
                },
                .backgroundColor = C_ACCENT,
                .cornerRadius    = CLAY_CORNER_RADIUS(5),
            }) { CLAY_TEXT(CLAY_STRING("K"), TC(C_WHITE, 10)); }
            CLAY_TEXT(CLAY_STRING("Command Palette"), TC(C_TEXT, 12));
            CLAY(CLAY_ID("PUHdrSp"), {
                .layout = { .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(1) } },
            }) {}
            CLAY(CLAY_ID("PUHdrHint"), {
                .layout = {
                    .sizing         = { CLAY_SIZING_FIT(0), CLAY_SIZING_FIXED(22) },
                    .padding        = { 8, 8, 2, 2 },
                    .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER },
                },
                .backgroundColor = C_BORDER,
                .cornerRadius    = CLAY_CORNER_RADIUS(4),
            }) { CLAY_TEXT(CLAY_STRING("Ctrl+K"), TC(C_SUBTEXT, 10)); }
        }

        /* input row */
        CLAY(CLAY_ID("PUInputRow"), {
            .layout = {
                .sizing          = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(56) },
                .padding         = { 16, 16, 0, 0 },
                .childGap        = 10,
                .childAlignment  = { .y = CLAY_ALIGN_Y_CENTER },
                .layoutDirection = CLAY_LEFT_TO_RIGHT,
            },
        }) {
            CLAY_TEXT(CLAY_STRING(">"), TC(C_ACCENT, 17));
            if (gCmdLen > 0) {
                bool cursorOn = ((int)(GetTime() * 2) % 2) == 0;
                CLAY_TEXT(cursorOn ? DS("%s|", gCmdBuf) : DS("%s ", gCmdBuf),
                          TC(C_TEXT, 14));
            } else {
                CLAY_TEXT(CLAY_STRING("please type \"help\" to see all command"),
                          TC(C_SUBTEXT, 13));
            }
        }

        /* footer */
        CLAY(CLAY_ID("PUFooter"), {
            .layout = {
                .sizing          = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(34) },
                .padding         = { 16, 16, 0, 0 },
                .childGap        = 20,
                .childAlignment  = { .y = CLAY_ALIGN_Y_CENTER },
                .layoutDirection = CLAY_LEFT_TO_RIGHT,
            },
            .backgroundColor = C_TBL_HDR,
            .border          = { .color = C_BORDER, .width = { .top = 1 } },
        }) {
            CLAY_TEXT(CLAY_STRING("Enter for Execute"),    TC(C_SUBTEXT, 10));
            CLAY_TEXT(CLAY_STRING("Backspace for Delete"), TC(C_SUBTEXT, 10));
            CLAY_TEXT(CLAY_STRING("ESC for Stop Program"), TC(C_SUBTEXT, 10));
        }
    }
}

/* ═══════════════════════════════════════════════════════════════════════
 *  RESULT TOAST
 * ═══════════════════════════════════════════════════════════════════════ */

static void RenderResultToast(void)
{
    float remaining = gResultShowUntil - (float)GetTime();
    if (remaining <= 0.f) return;

    uint8_t alpha = (remaining < 1.f) ? (uint8_t)(remaining * 255.f) : 255;

    Clay_Color toastBg  = (Clay_Color){ 20,  20,  38, alpha };
    Clay_Color toastBdr = (Clay_Color){ 99, 102, 241, alpha };
    Clay_Color textClr  = (Clay_Color){224, 224, 242, alpha };

    CLAY(CLAY_ID("ResultToast"), {
        .layout = {
            .sizing          = { CLAY_SIZING_FIT(0), CLAY_SIZING_FIXED(44) },
            .padding         = { 18, 18, 0, 0 },
            .childGap        = 10,
            .childAlignment  = { .y = CLAY_ALIGN_Y_CENTER },
            .layoutDirection = CLAY_LEFT_TO_RIGHT,
        },
        .backgroundColor = toastBg,
        .cornerRadius    = CLAY_CORNER_RADIUS(8),
        .border          = { .color = toastBdr,
                             .width = { .left=1,.right=1,.top=1,.bottom=1 } },
        .floating = {
            .attachTo     = CLAY_ATTACH_TO_ROOT,
            .attachPoints = { .element = CLAY_ATTACH_POINT_CENTER_TOP,
                              .parent  = CLAY_ATTACH_POINT_CENTER_TOP },
            .offset       = { 0.f, 14.f },
            .zIndex       = 8,
        },
    }) {
        CLAY_TEXT(CLAY_STRING("->"),    TC(toastBdr, 12));
        CLAY_TEXT(DS("%s", gResultMsg), TC(textClr,  13));
    }
}
