/*
 * Clay DB Viewer — modern UI with Clay + Raylib
 * Mock employee data — single-file build
 *   make
 */

#define CLAY_IMPLEMENTATION
#include "clay.h"
#include "raylib.h"
#include "clay_renderer_raylib.c"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>

/* ─── Window / layout constants ─────────────────────────────────────── */
#define WIN_W      1400
#define WIN_H       820
#define SIDEBAR_W   240
#define ROW_H        56
#define HDR_H        50

/* ─── Colors (dark modern palette) ──────────────────────────────────── */
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

/* ─── Data model ─────────────────────────────────────────────────────── */
typedef struct {
    int         id;
    const char *name;
    const char *department;
    const char *role;
    int         salary;
    int         status;   /* 0=Active  1=Inactive  2=On Leave */
    const char *hireDate;
    const char *initials;
} Employee;

static Employee gEmployees[] = {
    { 1, "Alice Johnson",  "Engineering",  "Senior Developer",   132000, 0, "2019-03-15", "AJ"},
    { 2, "Bob Martinez",   "Marketing",    "Brand Manager",       94000, 0, "2020-07-22", "BM"},
    { 3, "Carol White",    "HR",           "HR Specialist",       72000, 0, "2021-01-08", "CW"},
    { 4, "David Chen",     "Engineering",  "DevOps Engineer",    118000, 0, "2018-11-03", "DC"},
    { 5, "Elsa Nguyen",    "Design",       "UX Designer",         88000, 2, "2020-04-17", "EN"},
    { 6, "Frank Lee",      "Finance",      "Financial Analyst",   95000, 0, "2017-06-30", "FL"},
    { 7, "Grace Kim",      "Engineering",  "Frontend Developer", 105000, 0, "2021-09-12", "GK"},
    { 8, "Henry Brown",    "Sales",        "Account Executive",   82000, 1, "2019-05-25", "HB"},
    { 9, "Iris Patel",     "Design",       "Product Designer",    91000, 0, "2022-02-14", "IP"},
    {10, "James Wilson",   "Engineering",  "Backend Developer",  115000, 0, "2020-08-19", "JW"},
    {11, "Karen Davis",    "Marketing",    "Content Strategist",  78000, 0, "2021-11-03", "KD"},
    {12, "Liam Thompson",  "Finance",      "CFO",                195000, 0, "2015-01-20", "LT"},
    {13, "Maya Rodriguez", "HR",           "HR Director",        135000, 0, "2016-04-07", "MR"},
    {14, "Nathan Clark",   "Sales",        "Sales Director",     158000, 0, "2017-09-14", "NC"},
    {15, "Olivia Scott",   "Engineering",  "QA Engineer",         85000, 2, "2022-06-01", "OS"},
    {16, "Paul Harris",    "Engineering",  "Tech Lead",          145000, 0, "2018-03-22", "PH"},
    {17, "Quinn Adams",    "Marketing",    "Social Media Mgr",    69000, 1, "2023-01-15", "QA"},
    {18, "Rachel Turner",  "Design",       "Creative Director",  128000, 0, "2019-07-08", "RT"},
    {19, "Sam Walker",     "Finance",      "Accountant",          75000, 0, "2020-10-29", "SW"},
    {20, "Tina Morgan",    "Engineering",  "Mobile Developer",   110000, 0, "2021-04-18", "TM"},
};
#define EMP_COUNT ((int)(sizeof(gEmployees) / sizeof(gEmployees[0])))

/* ─── Globals ────────────────────────────────────────────────────────── */
static Font gFonts[1];
static bool gCustomFont = false;
static int  gActiveNav  = 1;

/* per-frame dynamic string buffer */
#define DYN_BUF_SIZE 32768
static char gDynBuf[DYN_BUF_SIZE];
static int  gDynPos;

/* ─── Helpers ────────────────────────────────────────────────────────── */
/* Clay_String from a static/permanent char* */
static Clay_String CS(const char *s) {
    return (Clay_String){ .isStaticallyAllocated = true,
                          .length = (int)strlen(s), .chars = s };
}

/* Clay_String from a formatted dynamic string (backed by gDynBuf) */
static Clay_String DS(const char *fmt, ...) {
    char   *start = gDynBuf + gDynPos;
    int     avail = DYN_BUF_SIZE - gDynPos - 1;
    va_list ap;
    va_start(ap, fmt);
    int len = vsnprintf(start, (size_t)(avail > 0 ? avail : 0), fmt, ap);
    va_end(ap);
    if (len < 0) len = 0;
    if (len >= avail) { /* fallback — wrap buffer */
        gDynPos = 0;
        start   = gDynBuf;
        va_start(ap, fmt);
        len = vsnprintf(start, DYN_BUF_SIZE - 1, fmt, ap);
        va_end(ap);
        if (len < 0) len = 0;
    }
    gDynPos += len + 1;
    return (Clay_String){ .isStaticallyAllocated = false,
                          .length = len, .chars = start };
}

/*
 * TC — store a text config into Clay's internal arena and return the pointer.
 * The macro expands to Clay__StoreTextElementConfig which copies the struct
 * by value into Clay-managed memory valid for the entire frame.
 */
#define FONT_SCALE 1.4f
#define TC(color, size) \
    Clay__StoreTextElementConfig((Clay_TextElementConfig){ \
        .textColor = (color), \
        .fontSize  = (uint16_t)((size) * FONT_SCALE), \
        .fontId    = 0, \
        .wrapMode  = CLAY_TEXT_WRAP_NONE })

/* ─── UI Components ──────────────────────────────────────────────────── */

static void RenderNavItem(int idx, const char *label, const char *icon)
{
    bool active = (idx == gActiveNav);
    Clay_Color bg  = active ? C_ACCENT_BG : C_TRANS;
    Clay_Color tc  = active ? C_WHITE     : C_SUBTEXT;
    Clay_Color bdr = active ? C_ACCENT    : C_TRANS;

    CLAY(CLAY_IDI("NavItem", idx), {
        .layout = {
            .sizing          = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(44) },
            .padding         = { 10, 10, 0, 0 },
            .childGap        = 10,
            .childAlignment  = { .y = CLAY_ALIGN_Y_CENTER },
            .layoutDirection = CLAY_LEFT_TO_RIGHT,
        },
        .backgroundColor = bg,
        .cornerRadius    = CLAY_CORNER_RADIUS(6),
        .border          = { .color = bdr, .width = { .left = active ? 3 : 0 } },
    }) {
        /* icon chip */
        CLAY(CLAY_IDI("NavIcon", idx), {
            .layout = {
                .sizing         = { CLAY_SIZING_FIXED(24), CLAY_SIZING_FIXED(24) },
                .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER },
            },
            .backgroundColor = active ? C_ACCENT_DIM : C_BORDER,
            .cornerRadius    = CLAY_CORNER_RADIUS(5),
        }) {
            CLAY_TEXT(CS(icon), TC(active ? C_WHITE : C_SUBTEXT, 11));
        }
        CLAY_TEXT(CS(label), TC(tc, 13));
    }
}

/* ─── Sidebar ────────────────────────────────────────────────────────── */
static void RenderSidebar(void)
{
    CLAY(CLAY_ID("Sidebar"), {
        .layout = {
            .sizing          = { CLAY_SIZING_FIXED(SIDEBAR_W), CLAY_SIZING_GROW(0) },
            .padding         = { 12, 12, 16, 16 },
            .childGap        = 4,
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
        },
        .backgroundColor = C_SIDEBAR,
        .border          = { .color = C_BORDER, .width = { .right = 1 } },
    }) {
        /* ── Logo area ── */
        CLAY(CLAY_ID("Logo"), {
            .layout = {
                .sizing          = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(58) },
                .padding         = { 4, 4, 0, 0 },
                .childGap        = 10,
                .childAlignment  = { .y = CLAY_ALIGN_Y_CENTER },
                .layoutDirection = CLAY_LEFT_TO_RIGHT,
            },
        }) {
            CLAY(CLAY_ID("LogoBadge"), {
                .layout = {
                    .sizing         = { CLAY_SIZING_FIXED(34), CLAY_SIZING_FIXED(34) },
                    .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER },
                },
                .backgroundColor = C_ACCENT,
                .cornerRadius    = CLAY_CORNER_RADIUS(8),
            }) {
                CLAY_TEXT(CLAY_STRING("DB"), TC(C_WHITE, 12));
            }
            CLAY(CLAY_ID("LogoText"), {
                .layout = { .layoutDirection = CLAY_TOP_TO_BOTTOM, .childGap = 2 },
            }) {
                CLAY_TEXT(CLAY_STRING("Clay DB"),  TC(C_TEXT,    15));
                CLAY_TEXT(CLAY_STRING("v1.0"),     TC(C_SUBTEXT, 11));
            }
        }

        /* separator */
        CLAY(CLAY_ID("Sep1"), {
            .layout          = { .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(1) } },
            .backgroundColor = C_BORDER,
        }) {}

        /* section label */
        CLAY(CLAY_ID("MenuLabel"), {
            .layout = {
                .sizing         = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(30) },
                .padding        = { 4, 4, 0, 0 },
                .childAlignment = { .y = CLAY_ALIGN_Y_CENTER },
            },
        }) {
            CLAY_TEXT(CLAY_STRING("MENU"), TC(C_SUBTEXT, 10));
        }

        RenderNavItem(0, "Dashboard", "");
        RenderNavItem(1, "Employees", "");
        RenderNavItem(2, "Reports",   "");
        RenderNavItem(3, "Analytics", "");

        /* spacer */
        CLAY(CLAY_ID("NavSpacer"), {
            .layout = { .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) } },
        }) {}

        /* separator */
        CLAY(CLAY_ID("Sep2"), {
            .layout          = { .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(1) } },
            .backgroundColor = C_BORDER,
        }) {}

        RenderNavItem(4, "Settings", "");

        /* ── User card ── */
        CLAY(CLAY_ID("UserCard"), {
            .layout = {
                .sizing          = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(54) },
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
                    .sizing         = { CLAY_SIZING_FIXED(32), CLAY_SIZING_FIXED(32) },
                    .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER },
                },
                .backgroundColor = C_ACCENT,
                .cornerRadius    = CLAY_CORNER_RADIUS(16),
            }) {
                CLAY_TEXT(CLAY_STRING("JD"), TC(C_WHITE, 11));
            }
            CLAY(CLAY_ID("UInfo"), {
                .layout = { .layoutDirection = CLAY_TOP_TO_BOTTOM, .childGap = 2 },
            }) {
                CLAY_TEXT(CLAY_STRING("John Doe"),      TC(C_TEXT,    12));
                CLAY_TEXT(CLAY_STRING("Administrator"), TC(C_SUBTEXT, 10));
            }
        }
    }
}

/* ─── Stat card ──────────────────────────────────────────────────────── */
static void RenderStatCard(int idx, const char *title, const char *value,
                            const char *change, Clay_Color accent)
{
    Clay_Color changeColor = C_SUBTEXT;
    if (change && change[0] == '+') changeColor = C_GREEN;
    else if (change && change[0] == '-') changeColor = C_RED;

    CLAY(CLAY_IDI("StatCard", idx), {
        .layout = {
            .sizing          = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
            .padding         = { 16, 16, 14, 14 },
            .childGap        = 8,
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
        },
        .backgroundColor = C_CARD,
        .cornerRadius    = CLAY_CORNER_RADIUS(8),
        .border          = { .color = C_BORDER,
                             .width = { .left=1,.right=1,.top=1,.bottom=1 } },
    }) {
        /* accent bar + title */
        CLAY(CLAY_IDI("SCTitle", idx), {
            .layout = {
                .sizing          = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
                .childGap        = 8,
                .childAlignment  = { .y = CLAY_ALIGN_Y_CENTER },
                .layoutDirection = CLAY_LEFT_TO_RIGHT,
            },
        }) {
            CLAY(CLAY_IDI("SCBar", idx), {
                .layout          = { .sizing = { CLAY_SIZING_FIXED(4), CLAY_SIZING_FIXED(16) } },
                .backgroundColor = accent,
                .cornerRadius    = CLAY_CORNER_RADIUS(2),
            }) {}
            CLAY_TEXT(CS(title), TC(C_SUBTEXT, 12));
        }
        /* big number */
        CLAY_TEXT(CS(value), TC(C_TEXT, 26));
        /* change string */
        CLAY_TEXT(CS(change), TC(changeColor, 11));
    }
}

/* ─── Table header ───────────────────────────────────────────────────── */
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
#define HDR_CELL(cid, w, label)                                             \
        CLAY(CLAY_ID(cid), {                                                \
            .layout = {                                                     \
                .sizing         = { w, CLAY_SIZING_GROW(0) },              \
                .padding        = { 12, 12, 0, 0 },                        \
                .childAlignment = { .y = CLAY_ALIGN_Y_CENTER },            \
            },                                                              \
        }) {                                                                \
            CLAY_TEXT(CLAY_STRING(label), TC(C_SUBTEXT, 11));              \
        }

        HDR_CELL("HC0", CLAY_SIZING_FIXED(64),  "#")
        HDR_CELL("HC1", CLAY_SIZING_GROW(0),    "EMPLOYEE")
        HDR_CELL("HC2", CLAY_SIZING_FIXED(130), "DEPARTMENT")
        HDR_CELL("HC3", CLAY_SIZING_FIXED(160), "ROLE")
        HDR_CELL("HC4", CLAY_SIZING_FIXED(112), "SALARY")
        HDR_CELL("HC5", CLAY_SIZING_FIXED(110), "STATUS")
        HDR_CELL("HC6", CLAY_SIZING_FIXED(112), "HIRE DATE")
#undef HDR_CELL
    }
}

/* ─── Table row ──────────────────────────────────────────────────────── */
static void RenderTableRow(Employee *emp, int idx)
{
    bool isOdd = idx % 2 != 0;

    CLAY(CLAY_IDI("Row", idx), {
        .layout = {
            .sizing          = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(ROW_H) },
            .layoutDirection = CLAY_LEFT_TO_RIGHT,
        },
        .backgroundColor = Clay_Hovered() ? C_ROW_HOVER :
                           (isOdd ? C_ROW_ODD : C_ROW_EVEN),
        .border = { .color = C_BORDER, .width = { .bottom = 1 } },
    }) {
        /* Each cell gets a unique ID via idx*10+col — avoids CLAY_IDI_LOCAL
         * parent-scope ambiguity in loops.                                 */
        /* ── ID ── */
        CLAY(CLAY_IDI("RC", idx * 10 + 0), {
            .layout = {
                .sizing         = { CLAY_SIZING_FIXED(64), CLAY_SIZING_GROW(0) },
                .padding        = { 12, 12, 0, 0 },
                .childAlignment = { .y = CLAY_ALIGN_Y_CENTER },
            },
        }) {
            CLAY_TEXT(DS("%d", emp->id), TC(C_SUBTEXT, 12));
        }

        /* ── Name + avatar ── */
        CLAY(CLAY_IDI("RC", idx * 10 + 1), {
            .layout = {
                .sizing          = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
                .padding         = { 10, 10, 0, 0 },
                .childGap        = 8,
                .childAlignment  = { .y = CLAY_ALIGN_Y_CENTER },
                .layoutDirection = CLAY_LEFT_TO_RIGHT,
            },
        }) {
            CLAY(CLAY_IDI("RowAv", idx), {
                .layout = {
                    .sizing         = { CLAY_SIZING_FIXED(28), CLAY_SIZING_FIXED(28) },
                    .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER },
                },
                .backgroundColor = C_ACCENT_DIM,
                .cornerRadius    = CLAY_CORNER_RADIUS(14),
            }) {
                CLAY_TEXT(CS(emp->initials), TC(C_WHITE, 10));
            }
            CLAY_TEXT(CS(emp->name), TC(C_TEXT, 13));
        }

        /* ── Department ── */
        CLAY(CLAY_IDI("RC", idx * 10 + 2), {
            .layout = {
                .sizing         = { CLAY_SIZING_FIXED(130), CLAY_SIZING_GROW(0) },
                .padding        = { 10, 10, 0, 0 },
                .childAlignment = { .y = CLAY_ALIGN_Y_CENTER },
            },
        }) {
            CLAY_TEXT(CS(emp->department), TC(C_SUBTEXT, 12));
        }

        /* ── Role ── */
        CLAY(CLAY_IDI("RC", idx * 10 + 3), {
            .layout = {
                .sizing         = { CLAY_SIZING_FIXED(160), CLAY_SIZING_GROW(0) },
                .padding        = { 10, 10, 0, 0 },
                .childAlignment = { .y = CLAY_ALIGN_Y_CENTER },
            },
        }) {
            CLAY_TEXT(CS(emp->role), TC(C_TEXT, 12));
        }

        /* ── Salary ── */
        CLAY(CLAY_IDI("RC", idx * 10 + 4), {
            .layout = {
                .sizing         = { CLAY_SIZING_FIXED(112), CLAY_SIZING_GROW(0) },
                .padding        = { 10, 10, 0, 0 },
                .childAlignment = { .y = CLAY_ALIGN_Y_CENTER },
            },
        }) {
            int k = emp->salary / 1000;
            int r = (emp->salary % 1000) / 100;
            Clay_String salStr = (r > 0) ? DS("$%d.%dK", k, r) : DS("$%dK", k);
            CLAY_TEXT(salStr, TC(C_TEXT, 13));
        }

        /* ── Status badge ── */
        CLAY(CLAY_IDI("RC", idx * 10 + 5), {
            .layout = {
                .sizing         = { CLAY_SIZING_FIXED(110), CLAY_SIZING_GROW(0) },
                .padding        = { 10, 10, 0, 0 },
                .childAlignment = { .y = CLAY_ALIGN_Y_CENTER },
            },
        }) {
            Clay_Color badgeFg, badgeBg;
            const char *badgeLabel;
            switch (emp->status) {
                case 1:  badgeFg = C_RED;    badgeBg = C_RED_BG;
                         badgeLabel = "Inactive"; break;
                case 2:  badgeFg = C_YELLOW; badgeBg = C_YELLOW_BG;
                         badgeLabel = "On Leave"; break;
                default: badgeFg = C_GREEN;  badgeBg = C_GREEN_BG;
                         badgeLabel = "Active";
            }
            CLAY(CLAY_IDI("RowBdg", idx), {
                .layout = {
                    .sizing         = { CLAY_SIZING_FIT(0), CLAY_SIZING_FIXED(22) },
                    .padding        = { 10, 10, 4, 4 },
                    .childAlignment = { .x = CLAY_ALIGN_X_CENTER,
                                        .y = CLAY_ALIGN_Y_CENTER },
                },
                .backgroundColor = badgeBg,
                .cornerRadius    = CLAY_CORNER_RADIUS(11),
            }) {
                CLAY_TEXT(CS(badgeLabel), TC(badgeFg, 11));
            }
        }

        /* ── Hire date ── */
        CLAY(CLAY_IDI("RC", idx * 10 + 6), {
            .layout = {
                .sizing         = { CLAY_SIZING_FIXED(112), CLAY_SIZING_GROW(0) },
                .padding        = { 10, 10, 0, 0 },
                .childAlignment = { .y = CLAY_ALIGN_Y_CENTER },
            },
        }) {
            CLAY_TEXT(CS(emp->hireDate), TC(C_SUBTEXT, 12));
        }
    }
}

/* ─── Full main content area ─────────────────────────────────────────── */
static void RenderMainContent(void)
{
    CLAY(CLAY_ID("Main"), {
        .layout = {
            .sizing          = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
            .padding         = { 24, 24, 24, 24 },
            .childGap        = 20,
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
        },
        .backgroundColor = C_BG,
    }) {
        /* ── Page header ── */
        CLAY(CLAY_ID("PageHdr"), {
            .layout = {
                .sizing          = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
                .childAlignment  = { .y = CLAY_ALIGN_Y_CENTER },
                .layoutDirection = CLAY_LEFT_TO_RIGHT,
            },
        }) {
            /* left: title + subtitle */
            CLAY(CLAY_ID("PHLeft"), {
                .layout = {
                    .sizing          = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
                    .layoutDirection = CLAY_TOP_TO_BOTTOM,
                    .childGap        = 4,
                },
            }) {
                CLAY_TEXT(CLAY_STRING("Employees"), TC(C_TEXT, 22));
                CLAY_TEXT(DS("%d records in database", EMP_COUNT),
                          TC(C_SUBTEXT, 13));
            }

            /* right: search + buttons */
            CLAY(CLAY_ID("SearchBox"), {
                .layout = {
                    .sizing         = { CLAY_SIZING_FIXED(200), CLAY_SIZING_FIXED(36) },
                    .padding        = { 12, 12, 0, 0 },
                    .childAlignment = { .y = CLAY_ALIGN_Y_CENTER },
                },
                .backgroundColor = C_CARD,
                .cornerRadius    = CLAY_CORNER_RADIUS(6),
                .border          = { .color = C_BORDER,
                                     .width = { .left=1,.right=1,.top=1,.bottom=1 } },
            }) {
                CLAY_TEXT(CLAY_STRING("Search..."), TC(C_SUBTEXT, 13));
            }

            CLAY(CLAY_ID("BtnGap1"), {
                .layout = { .sizing = { CLAY_SIZING_FIXED(8), CLAY_SIZING_FIXED(1) } },
            }) {}

            CLAY(CLAY_ID("FilterBtn"), {
                .layout = {
                    .sizing         = { CLAY_SIZING_FIT(0), CLAY_SIZING_FIXED(36) },
                    .padding        = { 14, 14, 0, 0 },
                    .childAlignment = { .y = CLAY_ALIGN_Y_CENTER },
                },
                .backgroundColor = C_CARD,
                .cornerRadius    = CLAY_CORNER_RADIUS(6),
                .border          = { .color = C_BORDER,
                                     .width = { .left=1,.right=1,.top=1,.bottom=1 } },
            }) {
                CLAY_TEXT(CLAY_STRING("Filter"), TC(C_TEXT, 13));
            }

            CLAY(CLAY_ID("BtnGap2"), {
                .layout = { .sizing = { CLAY_SIZING_FIXED(8), CLAY_SIZING_FIXED(1) } },
            }) {}

            CLAY(CLAY_ID("AddBtn"), {
                .layout = {
                    .sizing         = { CLAY_SIZING_FIT(0), CLAY_SIZING_FIXED(36) },
                    .padding        = { 16, 16, 0, 0 },
                    .childAlignment = { .y = CLAY_ALIGN_Y_CENTER },
                },
                .backgroundColor = C_ACCENT,
                .cornerRadius    = CLAY_CORNER_RADIUS(6),
            }) {
                CLAY_TEXT(CLAY_STRING("+ Add Employee"), TC(C_WHITE, 13));
            }
        }

        /* ── Stats row ── */
        CLAY(CLAY_ID("StatsRow"), {
            .layout = {
                .sizing          = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(120) },
                .childGap        = 16,
                .layoutDirection = CLAY_LEFT_TO_RIGHT,
            },
        }) {
            RenderStatCard(0, "Total Employees", "248", "+12 this month",   C_ACCENT);
            RenderStatCard(1, "Active",          "203", "81.9% of total",   C_GREEN);
            RenderStatCard(2, "On Leave",         "31", "12.5% of total",   C_YELLOW);
            RenderStatCard(3, "New This Month",   "12", "+8 from last month", C_ACCENT);
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
                .clip = { .vertical = true },
            }) {
                for (int i = 0; i < EMP_COUNT; i++) {
                    RenderTableRow(&gEmployees[i], i);
                }
            }

            /* ── Pagination footer ── */
            CLAY(CLAY_ID("Pagination"), {
                .layout = {
                    .sizing          = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(48) },
                    .padding         = { 16, 16, 0, 0 },
                    .childGap        = 6,
                    .childAlignment  = { .y = CLAY_ALIGN_Y_CENTER },
                    .layoutDirection = CLAY_LEFT_TO_RIGHT,
                },
                .backgroundColor = C_TBL_HDR,
                .border          = { .color = C_BORDER, .width = { .top = 1 } },
            }) {
                /* summary text */
                CLAY_TEXT(DS("Showing 1–%d of %d employees", EMP_COUNT, EMP_COUNT),
                          TC(C_SUBTEXT, 12));

                /* grow spacer */
                CLAY(CLAY_ID("PgSpacer"), {
                    .layout = { .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(1) } },
                }) {}

                /* page buttons — decorative */
                const char *pages[] = {"<", "1", "2", "3", ">"};
                for (int p = 0; p < 5; p++) {
                    bool isNum = (p >= 1 && p <= 3);
                    bool isOne = (p == 1);
                    CLAY(CLAY_IDI("PgBtn", p), {
                        .layout = {
                            .sizing         = { CLAY_SIZING_FIXED(30), CLAY_SIZING_FIXED(30) },
                            .childAlignment = { .x = CLAY_ALIGN_X_CENTER,
                                                .y = CLAY_ALIGN_Y_CENTER },
                        },
                        .backgroundColor = isOne ? C_ACCENT : C_CARD,
                        .cornerRadius    = CLAY_CORNER_RADIUS(6),
                        .border          = { .color = isOne ? C_ACCENT : C_BORDER,
                                             .width = { .left=1,.right=1,.top=1,.bottom=1 } },
                    }) {
                        Clay_Color pgTc = isOne  ? C_WHITE :
                                         isNum ? C_TEXT  : C_SUBTEXT;
                        CLAY_TEXT(CS(pages[p]), TC(pgTc, 13));
                    }
                }
            }
        }
    }
}

/* ─── Root layout ────────────────────────────────────────────────────── */
static void BuildLayout(void)
{
    CLAY(CLAY_ID("Root"), {
        .layout = {
            .sizing          = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
            .layoutDirection = CLAY_LEFT_TO_RIGHT,
        },
        .backgroundColor = C_BG,
    }) {
        RenderSidebar();
        RenderMainContent();
    }
}

/* ─── Error handler ──────────────────────────────────────────────────── */
static void HandleClayError(Clay_ErrorData err)
{
    fprintf(stderr, "[Clay error %d] %.*s\n",
            err.errorType, err.errorText.length, err.errorText.chars);
}

/* ─── Entry point ────────────────────────────────────────────────────── */
int main(void)
{
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
    InitWindow(WIN_W, WIN_H, "Clay DB Viewer  —  Employee Dashboard");
    SetTargetFPS(60);

    /* ── Font loading (try system fonts, fall back to Raylib default) ── */
    const char *fontCandidates[] = {
        "../Font/Space_Mono/SpaceMono-Bold.ttf",
        "/usr/share/fonts/truetype/ubuntu/Ubuntu-R.ttf",
        "/usr/share/fonts/truetype/ubuntu/Ubuntu-Regular.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/TTF/DejaVuSans.ttf",
        "/usr/share/fonts/TTF/FiraCode-Regular.ttf",
        "/usr/share/fonts/opentype/noto/NotoSans-Regular.ttf",
        NULL
    };
    gFonts[0] = (Font){0};
    for (int i = 0; fontCandidates[i]; i++) {
        if (FileExists(fontCandidates[i])) {
            Font f = LoadFontEx(fontCandidates[i], 72, NULL, 250);
            if (f.texture.id != 0) {
                gFonts[0] = f;
                gCustomFont = true;
                SetTextureFilter(gFonts[0].texture, TEXTURE_FILTER_BILINEAR);
                break;
            }
        }
    }
    if (!gCustomFont) {
        gFonts[0] = GetFontDefault();
    }

    /* ── Clay initialisation ── */
    size_t memSize = Clay_MinMemorySize();
    void  *mem     = malloc(memSize);
    Clay_Arena arena = Clay_CreateArenaWithCapacityAndMemory(memSize, mem);
    Clay_Initialize(arena,
                    (Clay_Dimensions){ (float)WIN_W, (float)WIN_H },
                    (Clay_ErrorHandler){ HandleClayError, NULL });
    Clay_SetMeasureTextFunction(Raylib_MeasureText, gFonts);

    /* ── Main loop ── */
    while (!WindowShouldClose()) {
        int sw = GetScreenWidth();
        int sh = GetScreenHeight();

        Clay_SetLayoutDimensions((Clay_Dimensions){ (float)sw, (float)sh });
        Clay_SetPointerState(
            (Clay_Vector2){ (float)GetMouseX(), (float)GetMouseY() },
            IsMouseButtonDown(MOUSE_LEFT_BUTTON));
        Clay_UpdateScrollContainers(
            true,
            (Clay_Vector2){ 0.f, -GetMouseWheelMove() * 50.f },
            GetFrameTime());

        gDynPos = 0;  /* reset per-frame string buffer */

        Clay_BeginLayout();
        BuildLayout();
        Clay_RenderCommandArray cmds = Clay_EndLayout();

        BeginDrawing();
        ClearBackground((Color){ C_BG.r, C_BG.g, C_BG.b, 255 });
        Clay_Raylib_Render(cmds, gFonts);
        /* tiny fps counter */
        char fps[32];
        snprintf(fps, sizeof(fps), "FPS: %d", GetFPS());
        DrawTextEx(gFonts[0], fps,
                   (Vector2){ (float)(sw - 80), 6.f }, 14.f, 1.f,
                   (Color){ 80, 80, 120, 180 });
        EndDrawing();
    }

    if (gCustomFont) UnloadFont(gFonts[0]);
    free(mem);
    Clay_Raylib_Close();
    return 0;
}
