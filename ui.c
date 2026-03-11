/*
 * ui.c — Clay rendering layer for Clay DB Viewer
 *
 * NOT compiled independently. Included into main.c AFTER all globals
 * are declared, so every `g*` variable below is provided by main.c.
 *
 * Globals expected from main.c:
 *   int   gScreenW, gScreenH   — current window dimensions
 *   int   gActiveNav           — currently highlighted sidebar item
 *   char  gDynBuf[], gDynPos   — per-frame dynamic string arena
 *   bool  gPopupOpen           — command palette visibility
 *   char  gCmdBuf[], gCmdLen   — current typed text
 *   bool  gHasResult           — whether a toast should be shown
 *   float gResultShowUntil     — GetTime() deadline for the toast
 *   char  gFilterDept[]        — active department filter ("" = show all)
 *   char  gResultMsg[]         — result message shown in toast
 */

/* ─── Layout constants ───────────────────────────────────────────────── */
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

/* ─── Text config helper ─────────────────────────────────────────────── */
#define FONT_SCALE 1.4f
#define TC(color, size) \
    Clay__StoreTextElementConfig((Clay_TextElementConfig){ \
        .textColor = (color), \
        .fontSize  = (uint16_t)((size) * FONT_SCALE), \
        .fontId    = 0, \
        .wrapMode  = CLAY_TEXT_WRAP_NONE })

/* ─── String helpers ─────────────────────────────────────────────────── */

/* Clay_String from a static/permanent char* */
static Clay_String CS(const char *s)
{
    return (Clay_String){ .isStaticallyAllocated = true,
                          .length = (int)strlen(s), .chars = s };
}

/* Clay_String from a printf-style format, backed by the per-frame gDynBuf */
static Clay_String DS(const char *fmt, ...)
{
    char   *start = gDynBuf + gDynPos;
    int     avail = DYN_BUF_SIZE - gDynPos - 1;
    va_list ap;
    va_start(ap, fmt);
    int len = vsnprintf(start, (size_t)(avail > 0 ? avail : 0), fmt, ap);
    va_end(ap);
    if (len < 0) len = 0;
    if (len >= avail) {          /* wrap buffer on overflow */
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

/* ═══════════════════════════════════════════════════════════════════════
 *  SIDEBAR
 * ═══════════════════════════════════════════════════════════════════════ */

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
        /* show a subtle hover tint for inactive items */
        .backgroundColor = (!active && Clay_Hovered()) ? C_ROW_HOVER : bg,
        .cornerRadius    = CLAY_CORNER_RADIUS(6),
        .border          = { .color = bdr, .width = { .left = active ? 3 : 0 } },
    }) {
        /* ── click: update active nav ── */
        if (Clay_Hovered() && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
            gActiveNav = idx;
        }

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
        /* ── Logo ── */
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
                CLAY_TEXT(CLAY_STRING("Clay DB"), TC(C_TEXT,    15));
                CLAY_TEXT(CLAY_STRING("v1.0"),    TC(C_SUBTEXT, 11));
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

/* ═══════════════════════════════════════════════════════════════════════
 *  MAIN CONTENT
 * ═══════════════════════════════════════════════════════════════════════ */

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
        /* accent bar + title row */
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
        CLAY_TEXT(CS(value),  TC(C_TEXT,    26));
        CLAY_TEXT(CS(change), TC(changeColor, 11));
    }
}

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
#define HDR_CELL(cid, w, label)                                               \
        CLAY(CLAY_ID(cid), {                                                  \
            .layout = {                                                       \
                .sizing         = { w, CLAY_SIZING_GROW(0) },                \
                .padding        = { 12, 12, 0, 0 },                          \
                .childAlignment = { .y = CLAY_ALIGN_Y_CENTER },              \
            },                                                                \
        }) {                                                                  \
            CLAY_TEXT(CLAY_STRING(label), TC(C_SUBTEXT, 11));                \
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

static void RenderTableRow(Employee *emp, int idx)
{
    bool isOdd = idx % 2 != 0;

    CLAY(CLAY_IDI("Row", idx), {
        .layout = {
            .sizing          = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(ROW_H) },
            .layoutDirection = CLAY_LEFT_TO_RIGHT,
        },
        .backgroundColor = Clay_Hovered() ? C_ROW_HOVER
                         : (isOdd ? C_ROW_ODD : C_ROW_EVEN),
        .border = { .color = C_BORDER, .width = { .bottom = 1 } },
    }) {
        /* ── # ── */
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
            CLAY(CLAY_ID("PHLeft"), {
                .layout = {
                    .sizing          = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
                    .layoutDirection = CLAY_TOP_TO_BOTTOM,
                    .childGap        = 4,
                },
            }) {
                CLAY_TEXT(CLAY_STRING("Employees"), TC(C_TEXT, 22));
                /* subtitle: show filter status when active */
                int hdrMatch = 0;
                if (gFilterDept[0] != '\0') {
                    for (int _i = 0; _i < EMP_COUNT; _i++)
                        if (strcmp(gEmployees[_i].department, gFilterDept) == 0) hdrMatch++;
                    CLAY_TEXT(DS("Filtered: %d of %d  (dept: %s)",
                                 hdrMatch, EMP_COUNT, gFilterDept),
                              TC(C_ACCENT, 12));
                } else {
                    CLAY_TEXT(DS("%d records in database", EMP_COUNT), TC(C_SUBTEXT, 13));
                }
            }

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
            RenderStatCard(0, "Total Employees", "248", "+12 this month",    C_ACCENT);
            RenderStatCard(1, "Active",          "203", "81.9% of total",    C_GREEN);
            RenderStatCard(2, "On Leave",         "31", "12.5% of total",    C_YELLOW);
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

            CLAY(CLAY_ID("TblBody"), {
                .layout = {
                    .sizing          = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
                    .layoutDirection = CLAY_TOP_TO_BOTTOM,
                },
                .clip = { .vertical = true },
            }) {
                /* only render rows that match the active department filter */
                bool filtering = gFilterDept[0] != '\0';
                for (int i = 0; i < EMP_COUNT; i++) {
                    if (filtering &&
                        strcmp(gEmployees[i].department, gFilterDept) != 0)
                        continue;
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
                /* pagination label: adapts to active filter */
                if (gFilterDept[0] != '\0') {
                    int mc = 0;
                    for (int _i = 0; _i < EMP_COUNT; _i++)
                        if (strcmp(gEmployees[_i].department, gFilterDept) == 0) mc++;
                    CLAY_TEXT(DS("Showing %d match%s for '%s'  (cmd: clear to reset)",
                                 mc, mc == 1 ? "" : "es", gFilterDept),
                              TC(C_ACCENT, 12));
                } else {
                    CLAY_TEXT(DS("Showing 1\xe2\x80\x93%d of %d employees", EMP_COUNT, EMP_COUNT),
                              TC(C_SUBTEXT, 12));
                }

                CLAY(CLAY_ID("PgSpacer"), {
                    .layout = { .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(1) } },
                }) {}

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
                        Clay_Color pgTc = isOne ? C_WHITE
                                        : isNum ? C_TEXT : C_SUBTEXT;
                        CLAY_TEXT(CS(pages[p]), TC(pgTc, 13));
                    }
                }
            }
        }
    }
}

/* ═══════════════════════════════════════════════════════════════════════
 *  COMMAND PALETTE OVERLAY
 * ═══════════════════════════════════════════════════════════════════════ */

static void RenderCommandPopup(void)
{
    /*
     * Backdrop: full-screen dim.  Floating elements do not size with GROW
     * when attached to root — use FIXED with current screen dimensions
     * (gScreenW / gScreenH updated each frame in main loop).
     */
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

    /* ── Popup card — centred ── */
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
        /* ── Header bar ── */
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
            }) {
                CLAY_TEXT(CLAY_STRING("K"), TC(C_WHITE, 10));
            }
            CLAY_TEXT(CLAY_STRING("Command Palette"), TC(C_TEXT, 13));
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
            }) {
                CLAY_TEXT(CLAY_STRING("Ctrl+K"), TC(C_SUBTEXT, 10));
            }
        }

        /* ── Input row ── */
        CLAY(CLAY_ID("PUInputRow"), {
            .layout = {
                .sizing          = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(60) },
                .padding         = { 18, 18, 0, 0 },
                .childGap        = 12,
                .childAlignment  = { .y = CLAY_ALIGN_Y_CENTER },
                .layoutDirection = CLAY_LEFT_TO_RIGHT,
            },
        }) {
            CLAY_TEXT(CLAY_STRING(">"), TC(C_ACCENT, 18));

            if (gCmdLen > 0) {
                bool cursorOn = ((int)(GetTime() * 2) % 2) == 0;
                CLAY_TEXT(cursorOn ? DS("%s|", gCmdBuf) : DS("%s ", gCmdBuf),
                          TC(C_TEXT, 15));
            } else {
                CLAY_TEXT(CLAY_STRING("Type a command and press Enter..."),
                          TC(C_SUBTEXT, 15));
            }
        }

        /* ── Footer hints ── */
        CLAY(CLAY_ID("PUFooter"), {
            .layout = {
                .sizing          = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(36) },
                .padding         = { 18, 18, 0, 0 },
                .childGap        = 24,
                .childAlignment  = { .y = CLAY_ALIGN_Y_CENTER },
                .layoutDirection = CLAY_LEFT_TO_RIGHT,
            },
            .backgroundColor = C_TBL_HDR,
            .border          = { .color = C_BORDER, .width = { .top = 1 } },
        }) {
            CLAY_TEXT(CLAY_STRING("Enter  Execute"),    TC(C_SUBTEXT, 11));
            CLAY_TEXT(CLAY_STRING("Backspace  Delete"), TC(C_SUBTEXT, 11));
            CLAY_TEXT(CLAY_STRING("ESC  Dismiss"),      TC(C_SUBTEXT, 11));
        }
    }
}

/* ═══════════════════════════════════════════════════════════════════════
 *  RESULT TOAST  (top-centre, fades out)
 * ═══════════════════════════════════════════════════════════════════════ */

static void RenderResultToast(void)
{
    float remaining = gResultShowUntil - (float)GetTime();
    if (remaining <= 0.f) return;

    /* fade out over the last second */
    uint8_t alpha = (remaining < 1.f) ? (uint8_t)(remaining * 255.f) : 255;

    Clay_Color toastBg  = (Clay_Color){ 20,  20,  38, alpha };
    Clay_Color toastBdr = (Clay_Color){ 99, 102, 241, alpha };
    Clay_Color textClr  = (Clay_Color){224, 224, 242, alpha };

    CLAY(CLAY_ID("ResultToast"), {
        .layout = {
            .sizing          = { CLAY_SIZING_FIT(0), CLAY_SIZING_FIXED(46) },
            .padding         = { 20, 20, 0, 0 },
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
            .offset       = { 0.f, 16.f },
            .zIndex       = 8,
        },
    }) {
        CLAY_TEXT(CLAY_STRING("->"),   TC(toastBdr, 13));
        CLAY_TEXT(DS("%s", gResultMsg), TC(textClr,  14));
    }
}
