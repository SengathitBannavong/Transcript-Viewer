#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "ui.h"
#include "app_data.h"
#include "db.h"
#include "score_logic.h"
#include "raylib.h"
#include "ui_strings.h"
#include "app_config.h"

AppTheme gTheme;

static const AppTheme kThemePresets[3] = {
    /* 0: Academic Paper (Warm Light) */
    {
        .bg         = {241, 237, 229, 255},
        .sidebar    = {234, 229, 219, 255},
        .card       = {250, 248, 243, 255},
        .tbl_hdr    = {236, 231, 221, 255},
        .row_odd    = {247, 244, 238, 255},
        .row_even   = {252, 250, 246, 255},
        .row_hover  = {244, 230, 227, 255},
        .border     = {221, 214, 201, 255},
        .accent     = {140,  47,  42, 255},
        .accent_dim = {176,  92,  86, 255},
        .accent_bg  = {243, 228, 225, 255},
        .text       = { 27,  26,  23, 255},
        .subtext    = {122, 116, 104, 255},
        .white      = {250, 248, 244, 255},
        .green      = { 47, 120,  75, 255},
        .green_bg   = {222, 236, 224, 255},
        .red        = {178,  52,  42, 255},
        .red_bg     = {246, 225, 221, 255},
        .yellow     = {166, 124,  36, 255},
        .yellow_bg  = {245, 236, 212, 255}
    },
    /* 1: Midnight Slate (Dark Mode) */
    {
        .bg         = { 20,  24,  33, 255},
        .sidebar    = { 15,  18,  25, 255},
        .card       = { 28,  34,  46, 255},
        .tbl_hdr    = { 35,  42,  56, 255},
        .row_odd    = { 25,  30,  42, 255},
        .row_even   = { 28,  34,  46, 255},
        .row_hover  = { 42,  52,  70, 255},
        .border     = { 45,  54,  72, 255},
        .accent     = { 79, 140, 243, 255},
        .accent_dim = {100, 160, 255, 255},
        .accent_bg  = { 35,  56,  90, 255},
        .text       = {235, 240, 250, 255},
        .subtext    = {140, 150, 170, 255},
        .white      = {255, 255, 255, 255},
        .green      = { 60, 179, 113, 255},
        .green_bg   = { 24,  55,  38, 255},
        .red        = {239,  83,  80, 255},
        .red_bg     = { 62,  24,  24, 255},
        .yellow     = {255, 202,  40, 255},
        .yellow_bg  = { 60,  50,  20, 255}
    },
    /* 2: Nordic Forest (Teal/Sage) */
    {
        .bg         = {235, 242, 238, 255},
        .sidebar    = {222, 232, 226, 255},
        .card       = {245, 249, 247, 255},
        .tbl_hdr    = {228, 238, 232, 255},
        .row_odd    = {240, 246, 243, 255},
        .row_even   = {245, 249, 247, 255},
        .row_hover  = {210, 228, 218, 255},
        .border     = {200, 215, 207, 255},
        .accent     = { 47, 102,  85, 255},
        .accent_dim = { 65, 128, 110, 255},
        .accent_bg  = {218, 235, 227, 255},
        .text       = { 25,  38,  34, 255},
        .subtext    = {110, 128, 120, 255},
        .white      = {250, 253, 251, 255},
        .green      = { 47, 120,  75, 255},
        .green_bg   = {222, 236, 224, 255},
        .red        = {178,  52,  42, 255},
        .red_bg     = {246, 225, 221, 255},
        .yellow     = {166, 124,  36, 255},
        .yellow_bg  = {245, 236, 212, 255}
    }
};

void Theme_Apply(int theme_id)
{
    if (theme_id < 0 || theme_id >= 3) theme_id = 0;
    gTheme = kThemePresets[theme_id];
}

#define gScreenW (gApp.screen_w)
#define gScreenH (gApp.screen_h)
#define gActiveNav (gApp.active_nav)
#define gDynBuf (gApp.dyn_buf)
#define gDynPos (gApp.dyn_pos)
#define gPopupOpen (gApp.popup_open)
#define gCmdBuf (gApp.cmd_buf)
#define gCmdLen (gApp.cmd_len)
#define gHasResult (gApp.has_result)
#define gResultShowUntil (gApp.result_show_until)
#define gResultMsg (gApp.result_msg)
#define gPlayer (gApp.player)
#define gFonts (gApp.fonts)
#define gCustomFont (gApp.custom_font)
#define gIsMobile (gApp.is_mobile)
#define gDrawerOpen (gApp.drawer_open)
#define gRowHover (gApp.row_hover)
#define gIsTouch (gApp.is_touch)
#define gPlanTarget (gApp.plan_target)
#define gPlanFlex (gApp.plan_flex)
#define gEditOpen (gApp.edit_open)
#define gEditCode (gApp.edit_code)
#define gEditMidBuf (gApp.edit_mid_buf)
#define gEditMidLen (gApp.edit_mid_len)
#define gEditFinBuf (gApp.edit_fin_buf)
#define gEditFinLen (gApp.edit_fin_len)
#define gEditField (gApp.edit_field)
#define gEditRatio (gApp.edit_ratio)
#define gEditSubjectName (gApp.edit_subject_name)
#define gEditError (gApp.edit_error)
#define gFontScale (gApp.font_scale)
#define gTargetFPS (gApp.target_fps)
#define gNameInput (gApp.name_input)
#define gDBReady (gApp.db_ready)
#define gUserName (gApp.user_name)
#define gNameLen (gApp.name_len)
#define gTypeName (gApp.type_name)
#define gGradRules (gApp.grad_rules)
#define gDataWarningsBuf (gApp.data_warnings_buf)
#define gDataWarnCount (gApp.data_warn_count)


/* ─── Text config helper ─────────────────────────────────────────────── */
/* gFontScale is declared in main.c and loaded from assets/ui.cfg        */
#define TC(color, size) \
    Clay__StoreTextElementConfig((Clay_TextElementConfig){ \
        .textColor = (color), \
        .fontSize  = (uint16_t)((size) * gFontScale), \
        .fontId    = 0, \
        .wrapMode  = CLAY_TEXT_WRAP_NONE })

/* Word-wrapping variant — for long text that must stay inside its container
 * (e.g. a subject title in the edit popup header). */
#define TCW(color, size) \
    Clay__StoreTextElementConfig((Clay_TextElementConfig){ \
        .textColor = (color), \
        .fontSize  = (uint16_t)((size) * gFontScale), \
        .fontId    = 0, \
        .wrapMode  = CLAY_TEXT_WRAP_WORDS })

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

/* Auto-wrap a static string into lines of ~20-30 characters, breaking on
 * word boundaries where possible (falling back to a hard break at 25 chars
 * when a single word is longer). Inserts '\n' between lines and returns a
 * Clay_String backed by the shared dynamic buffer (transient, render-only).
 * Use for long text in narrow fixed panels that render with WRAP_NONE; for
 * text inside a flexible container, prefer Clay's TCW (CLAY_TEXT_WRAP_WORDS). */
static Clay_String AutoWrapString(const char *src)
{
    char *start = gApp.dyn_buf + gApp.dyn_pos;
    int avail = DYN_BUF_SIZE - gApp.dyn_pos - 1;
    int src_len = (int)strlen(src);
    if (avail < src_len * 2 + 64) {
        gApp.dyn_pos = 0;
        start = gApp.dyn_buf;
        avail = DYN_BUF_SIZE - 1;
    }

    int dst_idx = 0;
    int i = 0;

    while (i < src_len) {
        // Skip leading spaces for the current line
        while (i < src_len && src[i] == ' ') i++;
        if (i >= src_len) break;

        // If remaining text fits in 30 chars, copy it
        if (src_len - i <= 30) {
            while (i < src_len) {
                start[dst_idx++] = src[i++];
            }
            break;
        }

        // Look for a space between index i + 20 and i + 30
        int best_break = -1;
        for (int j = 20; j <= 30; j++) {
            if (i + j < src_len && src[i + j] == ' ') {
                best_break = i + j;
            }
        }

        if (best_break != -1) {
            for (int k = i; k < best_break; k++) {
                start[dst_idx++] = src[k];
            }
            start[dst_idx++] = '\n';
            i = best_break + 1; // Skip the space
        } else {
            // No space in the 20-30 range. Look for the last space within 30 chars
            int last_space = -1;
            for (int j = 0; j <= 30; j++) {
                if (i + j < src_len && src[i + j] == ' ') {
                    last_space = i + j;
                }
            }
            if (last_space != -1) {
                for (int k = i; k < last_space; k++) {
                    start[dst_idx++] = src[k];
                }
                start[dst_idx++] = '\n';
                i = last_space + 1;
            } else {
                // No space at all, force break at 25 chars
                int break_at = 25;
                for (int k = i; k < i + break_at; k++) {
                    start[dst_idx++] = src[k];
                }
                start[dst_idx++] = '\n';
                i += break_at;
            }
        }
    }

    // Remove trailing newline if any
    while (dst_idx > 0 && start[dst_idx - 1] == '\n') {
        dst_idx--;
    }

    start[dst_idx] = '\0';
    gApp.dyn_pos += dst_idx + 1;
    return (Clay_String){ .isStaticallyAllocated = false, .length = dst_idx, .chars = start };
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
        if (Clay_Hovered() && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
            gActiveNav = idx;
            if (gIsMobile) gDrawerOpen = false;
        }

        /* index badge */ /* nav label sized at 10 to fit long type names */
        CLAY(CLAY_IDI("NavBdg", idx), {
            .layout = {
                .sizing         = { CLAY_SIZING_FIXED(20), CLAY_SIZING_FIXED(20) },
                .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER },
            },
            .backgroundColor = active ? C_ACCENT_DIM : C_BORDER,
            .cornerRadius    = CLAY_CORNER_RADIUS(4),
        }) {
            if (idx == NAV_PLANNER)
                CLAY_TEXT(CLAY_STRING(STR_NAV_P_BADGE), TC(active ? C_WHITE : C_SUBTEXT, 9));
            else if (idx == NAV_SETTINGS)
                CLAY_TEXT(CLAY_STRING(STR_NAV_S_BADGE), TC(active ? C_WHITE : C_SUBTEXT, 9));
            else
                CLAY_TEXT(DS("%d", idx), TC(active ? C_WHITE : C_SUBTEXT, 9));
        }
        CLAY_TEXT(CS(label), TC(tc, 10));

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
                .cornerRadius    = CLAY_CORNER_RADIUS(4),
            }) {
                CLAY_TEXT(CLAY_STRING(STR_NAV_WARN_BADGE), TC(C_WHITE, 9));
            }
        }
    }
}

void RenderSidebar(void)
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
                .cornerRadius    = CLAY_CORNER_RADIUS(4),
            }) { CLAY_TEXT(CLAY_STRING(STR_LOGO_BADGE), TC(C_WHITE, 11)); }
            CLAY(CLAY_ID("LogoText"), {
                .layout = { .layoutDirection = CLAY_TOP_TO_BOTTOM, .childGap = 2 },
            }) {
                CLAY_TEXT(CLAY_STRING(STR_LOGO_TITLE),  TC(C_TEXT,    13));
                CLAY_TEXT(CLAY_STRING(STR_LOGO_SUBTITLE), TC(C_SUBTEXT,  9));
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
        }) { CLAY_TEXT(CLAY_STRING(STR_SECTION_SUBJECT_TYPES), TC(C_SUBTEXT, 9)); }

        /* Dashboard + Planner — overview views, not subject types */
        RenderNavItem(0, STR_NAV_DASHBOARD, 0);
        RenderNavItem(NAV_PLANNER, STR_NAV_PLANNER, 0);

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

        /* Settings — pinned to the bottom of the nav list */
        RenderNavItem(NAV_SETTINGS, STR_NAV_SETTINGS, 0);

        CLAY(CLAY_ID("Sep2"), {
            .layout          = { .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(1) } },
            .backgroundColor = C_BORDER,
        }) {}

        /* ── DB export / import ──────────────────────────────────────────
         * Save a portable copy of the transcript .db or load one (e.g. to move
         * between devices). Web: browser download / file picker. Desktop: a
         * native file dialog (zenity/kdialog), or drag-and-drop a .db onto the
         * window to import. */
        CLAY(CLAY_ID("DBIO"), {
            .layout = {
                .sizing          = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(32) },
                .childGap        = 6,
                .childAlignment  = { .y = CLAY_ALIGN_Y_CENTER },
                .layoutDirection = CLAY_LEFT_TO_RIGHT,
            },
        }) {
            /* Export → browser download */
            CLAY(CLAY_ID("DBExport"), {
                .layout = {
                    .sizing         = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(30) },
                    .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER },
                },
                .backgroundColor = Clay_Hovered() ? (Clay_Color){38,98,61,255} : C_CARD,
                .cornerRadius    = CLAY_CORNER_RADIUS(5),
                .border          = { .color = C_BORDER,
                                     .width = { .left=1,.right=1,.top=1,.bottom=1 } },
            }) {
                if (gDBReady && Clay_Hovered() && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
#if defined(PLATFORM_WEB)
                    DB_ExportDownload(gUserName);
                    snprintf(gResultMsg, sizeof(gResultMsg),
                             STR_DB_EXPORT_WEB_MSG, gUserName);
#else
                    char dest[1024];
                    int rc = DB_ExportFile(gUserName, dest, sizeof dest);
                    if (rc == 1)       snprintf(gResultMsg, sizeof(gResultMsg), STR_DB_EXPORT_SUCCESS, dest);
                    else if (rc == -1) snprintf(gResultMsg, sizeof(gResultMsg), STR_DB_EXPORT_CANCEL);
                    else               snprintf(gResultMsg, sizeof(gResultMsg), STR_DB_EXPORT_FAIL);
#endif
                    ShowToastFor(4.f);
                }
                CLAY_TEXT(CLAY_STRING(STR_DB_EXPORT_BTN), TC(C_TEXT, 10));
            }
            /* Import ← file picker */
            CLAY(CLAY_ID("DBImport"), {
                .layout = {
                    .sizing         = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(30) },
                    .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER },
                },
                .backgroundColor = Clay_Hovered() ? C_ACCENT_DIM : C_CARD,
                .cornerRadius    = CLAY_CORNER_RADIUS(5),
                .border          = { .color = C_BORDER,
                                     .width = { .left=1,.right=1,.top=1,.bottom=1 } },
            }) {
                if (gDBReady && Clay_Hovered() && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
#if defined(PLATFORM_WEB)
                    DB_ImportPick(gUserName);
#else
                    char src[1024];
                    if (DB_PickOpenPath(src, sizeof src)) {
                        if (DB_ImportFile(gUserName, src)) {
                            RefreshPlayer();
                            snprintf(gResultMsg, sizeof(gResultMsg), STR_DB_IMPORT_SUCCESS, src);
                        } else {
                            snprintf(gResultMsg, sizeof(gResultMsg), STR_DB_IMPORT_FAIL);
                        }
                    } else if (tv_have("zenity") || tv_have("kdialog")) {
                        snprintf(gResultMsg, sizeof(gResultMsg), STR_DB_IMPORT_CANCEL);
                    } else {
                        snprintf(gResultMsg, sizeof(gResultMsg),
                                 STR_DB_IMPORT_NO_DIALOG);
                    }
                    ShowToastFor(4.f);
#endif
                }
                CLAY_TEXT(CLAY_STRING(STR_DB_IMPORT_BTN), TC(C_TEXT, 10));
            }
        }

        /* Reload Config */
        CLAY(CLAY_ID("ConfigReload"), {
            .layout = {
                .sizing          = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(32) },
                .childGap        = 6,
                .childAlignment  = { .y = CLAY_ALIGN_Y_CENTER },
                .layoutDirection = CLAY_LEFT_TO_RIGHT,
            },
        }) {
            CLAY(CLAY_ID("ConfigReloadBtn"), {
                .layout = {
                    .sizing         = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(30) },
                    .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER },
                },
                .backgroundColor = Clay_Hovered() ? C_ACCENT_DIM : C_CARD,
                .cornerRadius    = CLAY_CORNER_RADIUS(5),
                .border          = { .color = C_BORDER,
                                     .width = { .left=1,.right=1,.top=1,.bottom=1 } },
            }) {
                if (gDBReady && Clay_Hovered() && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
                    int prev_warns = gDataWarnCount;
                    DB_ReloadData();
                    RefreshPlayer();
                    if (gDataWarnCount == 0) {
                        snprintf(gResultMsg, sizeof(gResultMsg),
                                 STR_DB_RELOAD_SUCCESS,
                                 prev_warns);
                    } else {
                        snprintf(gResultMsg, sizeof(gResultMsg),
                                 STR_DB_RELOAD_WARN,
                                 gDataWarnCount, gDataWarnCount == 1 ? "" : "s");
                    }
                    ShowToastFor(5.f);
                }
                CLAY_TEXT(CLAY_STRING(STR_DB_RELOAD_BTN), TC(C_TEXT, 10));
            }
        }

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
                CLAY_TEXT(DS("%s", gUserName[0] ? gUserName : STR_USER_EMPTY), TC(C_TEXT,    11));
                CLAY_TEXT(CLAY_STRING(STR_USER_ROLE),                    TC(C_SUBTEXT,  9));
            }
        }
    }
}

/* ═══════════════════════════════════════════════════════════════════════
 *  MOBILE TOP BAR + DRAWER  (shown below BP_MOBILE in place of the sidebar)
 * ═══════════════════════════════════════════════════════════════════════ */

void RenderTopBar(void)
{
    CLAY(CLAY_ID("TopBar"), {
        .layout = {
            .sizing          = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(52) },
            .padding         = { 12, 14, 0, 0 },
            .childGap        = 12,
            .childAlignment  = { .y = CLAY_ALIGN_Y_CENTER },
            .layoutDirection = CLAY_LEFT_TO_RIGHT,
        },
        .backgroundColor = C_SIDEBAR,
        .border          = { .color = C_BORDER, .width = { .bottom = 1 } },
    }) {
        /* Hamburger — drawn from three bars so it needs no special glyph */
        CLAY(CLAY_ID("BurgerBtn"), {
            .layout = {
                .sizing         = { CLAY_SIZING_FIXED(38), CLAY_SIZING_FIXED(36) },
                .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER },
            },
            .backgroundColor = Clay_Hovered() ? C_ROW_HOVER : C_CARD,
            .cornerRadius    = CLAY_CORNER_RADIUS(6),
            .border          = { .color = C_BORDER, .width = { .left=1,.right=1,.top=1,.bottom=1 } },
        }) {
            if (Clay_Hovered() && IsMouseButtonReleased(MOUSE_LEFT_BUTTON))
                gDrawerOpen = !gDrawerOpen;
            CLAY(CLAY_ID("BurgerBars"), {
                .layout = {
                    .sizing          = { CLAY_SIZING_FIXED(18), CLAY_SIZING_FIT(0) },
                    .childGap        = 4,
                    .layoutDirection = CLAY_TOP_TO_BOTTOM,
                },
            }) {
                for (int b = 0; b < 3; b++)
                    CLAY(CLAY_IDI("BurgerBar", b), {
                        .layout = { .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(2) } },
                        .backgroundColor = C_TEXT,
                        .cornerRadius    = CLAY_CORNER_RADIUS(1),
                    }) {}
            }
        }

        /* Active section title */
        const char *title = (gActiveNav == 0)          ? STR_NAV_DASHBOARD
                          : (gActiveNav == NAV_PLANNER) ? STR_NAV_PLANNER
                          :                               gTypeName[gActiveNav];
        CLAY_TEXT(CS(title), TC(C_TEXT, 16));
    }
}

/* Sidebar-as-drawer: full-screen scrim (tap to dismiss) + the sidebar floating
 * in from the left. Rendered as a top-level overlay when gDrawerOpen on mobile. */
void RenderDrawer(void)
{
    /* scrim */
    CLAY(CLAY_ID("DrawerScrim"), {
        .layout = { .sizing = { CLAY_SIZING_FIXED((float)gScreenW),
                                CLAY_SIZING_FIXED((float)gScreenH) } },
        .backgroundColor = (Clay_Color){ 27, 26, 23, 120 },
        .floating = { .attachTo = CLAY_ATTACH_TO_ROOT, .zIndex = 30 },
    }) {
        if (Clay_Hovered() && IsMouseButtonReleased(MOUSE_LEFT_BUTTON))
            gDrawerOpen = false;
    }

    /* drawer panel pinned to the left edge */
    CLAY(CLAY_ID("DrawerPanel"), {
        .layout = {
            .sizing          = { CLAY_SIZING_FIXED(SIDEBAR_W), CLAY_SIZING_FIXED((float)gScreenH) },
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
        },
        .floating = {
            .attachTo     = CLAY_ATTACH_TO_ROOT,
            .attachPoints = { .element = CLAY_ATTACH_POINT_LEFT_TOP,
                              .parent  = CLAY_ATTACH_POINT_LEFT_TOP },
            .zIndex       = 31,
        },
    }) {
        RenderSidebar();
    }
}

/* ═══════════════════════════════════════════════════════════════════════
 *  STAT LEDGER  (one panel, hairline-divided cells — not a card grid)
 * ═══════════════════════════════════════════════════════════════════════ */

/* A single label/value cell. Lives inside a bordered ledger panel that
 * draws the dividers (.betweenChildren), so the cells read as one object. */
static void RenderStatCell(int idx, const char *label,
                           Clay_String val, Clay_Color valColor)
{
    CLAY(CLAY_IDI("StatCell", idx), {
        .layout = {
            .sizing          = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
            .padding         = { 16, 16, SP_SM, SP_SM },
            .childGap        = SP_XS,
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
            .childAlignment  = { .y = CLAY_ALIGN_Y_CENTER },
        },
    }) {
        CLAY_TEXT(CS(label), TC(C_SUBTEXT, 9));
        CLAY_TEXT(val, TC(valColor, 18));
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
#define HDR_CELL(cid, w, lbl, ax) \
        CLAY(CLAY_ID(cid), { \
            .layout = { \
                .sizing         = { w, CLAY_SIZING_GROW(0) }, \
                .padding        = { 12, 12, 0, 0 }, \
                .childAlignment = { .x = (ax), .y = CLAY_ALIGN_Y_CENTER }, \
            }, \
        }) { CLAY_TEXT(CLAY_STRING(lbl), TC(C_SUBTEXT, 10)); }

        if (!gIsMobile) HDR_CELL("HC0", CLAY_SIZING_FIXED(48),  STR_TBL_HDR_NUM,            CLAY_ALIGN_X_LEFT)
        HDR_CELL("HC1", CLAY_SIZING_GROW(0),    STR_TBL_HDR_NAME, CLAY_ALIGN_X_LEFT)
        if (!gIsMobile) HDR_CELL("HC2", CLAY_SIZING_FIXED(84),  STR_TBL_HDR_CODE,         CLAY_ALIGN_X_LEFT)
        HDR_CELL("HC3", CLAY_SIZING_FIXED(56),  STR_TBL_HDR_GRADE,           CLAY_ALIGN_X_RIGHT)
        HDR_CELL("HC4", CLAY_SIZING_FIXED(70),  STR_TBL_HDR_MID,          CLAY_ALIGN_X_RIGHT)
        HDR_CELL("HC5", CLAY_SIZING_FIXED(70),  STR_TBL_HDR_FIN,          CLAY_ALIGN_X_RIGHT)
        HDR_CELL("HC6", CLAY_SIZING_FIXED(64),  STR_TBL_HDR_PASS,         CLAY_ALIGN_X_CENTER)
        if (!gIsMobile) HDR_CELL("HC7", CLAY_SIZING_FIXED(56),  STR_TBL_HDR_CREDIT,           CLAY_ALIGN_X_RIGHT)
        if (!gIsMobile) HDR_CELL("HC8", CLAY_SIZING_FIXED(56),  STR_TBL_HDR_TERM,          CLAY_ALIGN_X_RIGHT)
#undef HDR_CELL
    }
}

/* Score letter → color */
static Clay_Color score_color(const char *letter)
{
    if (!letter || letter[0] == 'X') return C_SUBTEXT;
    if (letter[0] == 'F') return C_RED;
    if (letter[0] == 'A') return C_GREEN;
    if (letter[0] == 'B') return (Clay_Color){ 52, 88, 138, 255};  /* muted navy */
    if (letter[0] == 'C') return C_YELLOW;                          /* ochre      */
    return (Clay_Color){190, 108, 50, 255};  /* D+ / D — burnt orange */
}

static void RenderTableRow(Subject_Node *node, int idx)
{
    bool isOdd = idx % 2 != 0;
    /* studied-but-not-passed (includes auto-F) → red left accent bar */
    bool failed = (node->score_letter != 'X') && (node->status_pass == 0);

    CLAY(CLAY_IDI("Row", idx), {
        .layout = {
            .sizing          = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(ROW_H) },
            .layoutDirection = CLAY_LEFT_TO_RIGHT,
        },
        .backgroundColor = Clay_Hovered() ? C_ROW_HOVER
                         : (isOdd ? C_ROW_ODD : C_ROW_EVEN),
        .border = { .color = C_BORDER, .width = { .bottom = 1 } },
    }) {
        bool hov = Clay_Hovered();
        if (hov) gRowHover = true;  /* main.c switches to pointing-hand cursor */

        /* failed-row left accent bar — floating so it adds no layout shift and
         * keeps its own color independent of the row's hairline border */
        if (failed) {
            CLAY(CLAY_IDI("RowBar", idx), {
                .layout = { .sizing = { CLAY_SIZING_FIXED(3),
                                        CLAY_SIZING_FIXED(ROW_H) } },
                .backgroundColor = C_RED,
                .floating = {
                    .attachTo     = CLAY_ATTACH_TO_PARENT,
                    .attachPoints = { .element = CLAY_ATTACH_POINT_LEFT_TOP,
                                      .parent  = CLAY_ATTACH_POINT_LEFT_TOP },
                    .pointerCaptureMode = CLAY_POINTER_CAPTURE_MODE_PASSTHROUGH,
                    .clipTo       = CLAY_CLIP_TO_ATTACHED_PARENT,
                    .zIndex       = 2,
                },
            }) {}
        }

        /* "Edit ›" affordance revealed on hover (floating → no layout shift) */
        if (hov) {
            CLAY(CLAY_IDI("RowEdit", idx), {
                .layout = {
                    .sizing         = { CLAY_SIZING_FIT(0), CLAY_SIZING_FIXED(20) },
                    .padding        = { 8, 8, 0, 0 },
                    .childAlignment = { .y = CLAY_ALIGN_Y_CENTER },
                },
                .backgroundColor = C_ACCENT_BG,
                .cornerRadius    = CLAY_CORNER_RADIUS(10),
                .floating = {
                    .offset       = { -10.f, 0.f },
                    .attachTo     = CLAY_ATTACH_TO_PARENT,
                    .attachPoints = { .element = CLAY_ATTACH_POINT_RIGHT_CENTER,
                                      .parent  = CLAY_ATTACH_POINT_RIGHT_CENTER },
                    .pointerCaptureMode = CLAY_POINTER_CAPTURE_MODE_PASSTHROUGH,
                    .clipTo       = CLAY_CLIP_TO_ATTACHED_PARENT,
                    .zIndex       = 3,
                },
            }) {
                CLAY_TEXT(CLAY_STRING(STR_ROW_EDIT), TC(C_ACCENT, 10));
            }
        }

        if (hov && IsMouseButtonReleased(MOUSE_LEFT_BUTTON) && !gEditOpen) {
            gEditError[0] = '\0';
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
        /* ── # ── (hidden on mobile) */
        if (!gIsMobile)
        CLAY(CLAY_IDI("RC", idx * 10 + 0), {
            .layout = {
                .sizing         = { CLAY_SIZING_FIXED(48), CLAY_SIZING_GROW(0) },
                .padding        = { 12, 12, 0, 0 },
                .childAlignment = { .y = CLAY_ALIGN_Y_CENTER },
            },
        }) { CLAY_TEXT(DS("%d", idx + 1), TC(C_SUBTEXT, 11)); }

        /* ── Subject Name ── */
        CLAY(CLAY_IDI("RC", idx * 10 + 1), {
            .layout = {
                .sizing         = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
                .padding        = { 12, 2, 0, 0 },
                .childAlignment = { .y = CLAY_ALIGN_Y_CENTER },
            },
        }) { CLAY_TEXT(CS(node->name), TC(C_TEXT, 11)); }

        /* ── Code ── (hidden on mobile) */
        if (!gIsMobile)
        CLAY(CLAY_IDI("RC", idx * 10 + 2), {
            .layout = {
                .sizing         = { CLAY_SIZING_FIXED(84), CLAY_SIZING_GROW(0) },
                .padding        = { 12, 12, 0, 0 },
                .childAlignment = { .y = CLAY_ALIGN_Y_CENTER },
            },
        }) { CLAY_TEXT(CS(node->ID), TC(C_ACCENT, 11)); }

        /* ── Grade letter (right-aligned) ── */
        CLAY(CLAY_IDI("RC", idx * 10 + 3), {
            .layout = {
                .sizing         = { CLAY_SIZING_FIXED(56), CLAY_SIZING_GROW(0) },
                .padding        = { 12, 12, 0, 0 },
                .childAlignment = { .x = CLAY_ALIGN_X_RIGHT, .y = CLAY_ALIGN_Y_CENTER },
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

        /* ── Midterm (right-aligned) ── */
        CLAY(CLAY_IDI("RC", idx * 10 + 4), {
            .layout = {
                .sizing         = { CLAY_SIZING_FIXED(70), CLAY_SIZING_GROW(0) },
                .padding        = { 12, 12, 0, 0 },
                .childAlignment = { .x = CLAY_ALIGN_X_RIGHT, .y = CLAY_ALIGN_Y_CENTER },
            },
        }) {
            Clay_String ms = (node->score_number_mid > 0.001f)
                             ? DS("%.2f", node->score_number_mid)
                             : CLAY_STRING(STR_GRADE_MINUS);
            CLAY_TEXT(ms, TC(node->score_number_mid > 0.001f ? C_TEXT : C_SUBTEXT, 11));
        }

        /* ── Final (right-aligned) ── */
        CLAY(CLAY_IDI("RC", idx * 10 + 5), {
            .layout = {
                .sizing         = { CLAY_SIZING_FIXED(70), CLAY_SIZING_GROW(0) },
                .padding        = { 12, 12, 0, 0 },
                .childAlignment = { .x = CLAY_ALIGN_X_RIGHT, .y = CLAY_ALIGN_Y_CENTER },
            },
        }) {
            Clay_String fs = (node->score_number_final > 0.001f)
                             ? DS("%.2f", node->score_number_final)
                             : CLAY_STRING(STR_GRADE_MINUS);
            CLAY_TEXT(fs, TC(node->score_number_final > 0.001f ? C_TEXT : C_SUBTEXT, 11));
        }

        /* ── Pass badge (centered) ── */
        CLAY(CLAY_IDI("RC", idx * 10 + 6), {
            .layout = {
                .sizing         = { CLAY_SIZING_FIXED(64), CLAY_SIZING_GROW(0) },
                .padding        = { 8, 8, 0, 0 },
                .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER },
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
                CLAY_TEXT(no_score ? CLAY_STRING(STR_PASS_EMPTY)
                                   : (pass_flag ? CLAY_STRING(STR_PASS_YES) : CLAY_STRING(STR_PASS_NO)),
                          TC(fg, 10));
            }
        }

        /* ── Credits (right-aligned) ── (hidden on mobile) */
        if (!gIsMobile)
        CLAY(CLAY_IDI("RC", idx * 10 + 7), {
            .layout = {
                .sizing         = { CLAY_SIZING_FIXED(56), CLAY_SIZING_GROW(0) },
                .padding        = { 12, 12, 0, 0 },
                .childAlignment = { .x = CLAY_ALIGN_X_RIGHT, .y = CLAY_ALIGN_Y_CENTER },
            },
        }) { CLAY_TEXT(DS("%u", node->credit), TC(C_TEXT, 11)); }

        /* ── Term / semester (right-aligned) ── (hidden on mobile) */
        if (!gIsMobile)
        CLAY(CLAY_IDI("RC", idx * 10 + 8), {
            .layout = {
                .sizing         = { CLAY_SIZING_FIXED(56), CLAY_SIZING_GROW(0) },
                .padding        = { 12, 12, 0, 0 },
                .childAlignment = { .x = CLAY_ALIGN_X_RIGHT, .y = CLAY_ALIGN_Y_CENTER },
            },
        }) {
            unsigned int t = node->term_recomment_to_studie;
            CLAY_TEXT(t > 0 ? DS("%u", t) : CLAY_STRING(STR_GRADE_MINUS),
                      TC(C_SUBTEXT, 11));
        }
    }
}

/* ═══════════════════════════════════════════════════════════════════════
 *  MAIN CONTENT
 * ═══════════════════════════════════════════════════════════════════════ */

void RenderMainContent(void)
{
    Subject_Type *st = &gPlayer.numofSubjectType[gActiveNav];

    float cpa_all  = calc_cpa(&gPlayer, 0);
    float cpa_pass = calc_cpa(&gPlayer, 1);
    int   eff      = calc_effective_credits(&gPlayer);
    int   req      = calc_required_credits(&gPlayer);
    int   sec_want = _sl_resolve_limit(&gPlayer, gActiveNav);
    int   sec_pass = _sl_resolve_pass(&gPlayer, gActiveNav);
    bool  grad_ok  = gPlayer.status_can_grauate;

    CLAY(CLAY_ID("Main"), {
        .layout = {
            .sizing          = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
            .padding         = { 24, 24, 20, 20 },
            .childGap        = SP_LG,                 /* generous between sections */
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
        },
        .backgroundColor = C_BG,
    }) {

        /* ── Summary group: header band + stat ledger (tight pairing) ── */
        CLAY(CLAY_ID("SummaryGroup"), {
            .layout = {
                .sizing          = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
                .childGap        = SP_SM,             /* tight: these belong together */
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
            },
        }) {

        /* ── Header band: dominant title + meta, CPA hero, standing, hint ── */
        CLAY(CLAY_ID("HeaderBand"), {
            .layout = {
                .sizing          = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
                .padding         = { 22, 22, 18, 18 },
                .childGap        = SP_LG,
                .childAlignment  = { .y = CLAY_ALIGN_Y_CENTER },
                .layoutDirection = CLAY_LEFT_TO_RIGHT,
            },
            .backgroundColor = C_CARD,
            .cornerRadius    = CLAY_CORNER_RADIUS(4),
            .border          = { .color = C_BORDER,
                                 .width = { .left=1,.right=1,.top=1,.bottom=1 } },
        }) {
            /* Title + meta — the dominant element on the page */
            CLAY(CLAY_ID("HBTitle"), {
                .layout = {
                    .sizing          = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
                    .layoutDirection = CLAY_TOP_TO_BOTTOM,
                    .childGap        = SP_XS,
                },
            }) {
                CLAY_TEXT(CS(gTypeName[gActiveNav]), TC(C_TEXT, 26));
                CLAY_TEXT(
                    DS(STR_HDR_META_FORMAT,
                       gUserName, st->Total_Subject,
                       st->count_passCredit, st->Total_Credit),
                    TC(C_SUBTEXT, 11));
            }

            /* CPA hero — real data, the single most important academic number */
            CLAY(CLAY_ID("HBCpa"), {
                .layout = {
                    .sizing          = { CLAY_SIZING_FIT(0), CLAY_SIZING_FIT(0) },
                    .layoutDirection = CLAY_TOP_TO_BOTTOM,
                    .childAlignment  = { .x = CLAY_ALIGN_X_RIGHT },
                    .childGap        = 2,
                },
            }) {
                CLAY_TEXT(CLAY_STRING(STR_HDR_CPA_LABEL), TC(C_SUBTEXT, 9));
                CLAY_TEXT(DS("%.2f", cpa_all),           TC(C_ACCENT, 32));
                CLAY_TEXT(CLAY_STRING(STR_HDR_CPA_MAX),         TC(C_SUBTEXT, 9));
            }

            /* Graduation standing — a pill, not another card in a grid */
            CLAY(CLAY_ID("HBStanding"), {
                .layout = {
                    .sizing          = { CLAY_SIZING_FIT(0), CLAY_SIZING_FIT(0) },
                    .padding         = { 14, 14, 10, 10 },
                    .childGap        = 2,
                    .childAlignment  = { .x = CLAY_ALIGN_X_CENTER },
                    .layoutDirection = CLAY_TOP_TO_BOTTOM,
                },
                .backgroundColor = grad_ok ? C_GREEN_BG : C_RED_BG,
                .cornerRadius    = CLAY_CORNER_RADIUS(4),
                .border          = { .color = grad_ok ? C_GREEN : C_RED,
                                     .width = { .left=1,.right=1,.top=1,.bottom=1 } },
            }) {
                CLAY_TEXT(CLAY_STRING(STR_HDR_STANDING), TC(C_SUBTEXT, 9));
                CLAY_TEXT(grad_ok ? CLAY_STRING(STR_HDR_READY) : CLAY_STRING(STR_HDR_NOT_READY),
                          TC(grad_ok ? C_GREEN : C_RED, 16));
                CLAY_TEXT(DS(STR_HDR_CREDITS_FORMAT, eff, req), TC(C_SUBTEXT, 9));
            }

            /* Ctrl+K hint */
            CLAY(CLAY_ID("CmdHint"), {
                .layout = {
                    .sizing         = { CLAY_SIZING_FIT(0), CLAY_SIZING_FIXED(30) },
                    .padding        = { 12, 12, 0, 0 },
                    .childAlignment = { .y = CLAY_ALIGN_Y_CENTER },
                },
                .cornerRadius    = CLAY_CORNER_RADIUS(4),
                .border          = { .color = C_BORDER,
                                     .width = { .left=1,.right=1,.top=1,.bottom=1 } },
            }) {
                CLAY_TEXT(CLAY_STRING(STR_CTRL_K_HINT), TC(C_SUBTEXT, 11));
            }
        }

        /* ── Stat ledger: one bordered panel, hairline-divided cells ── */
        CLAY(CLAY_ID("StatLedger"), {
            .layout = {
                .sizing          = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(60) },
                .layoutDirection = CLAY_LEFT_TO_RIGHT,
            },
            .backgroundColor = C_CARD,
            .cornerRadius    = CLAY_CORNER_RADIUS(4),
            .border          = { .color = C_BORDER,
                                 .width = { .left=1,.right=1,.top=1,.bottom=1,
                                            .betweenChildren=1 } },
        }) {
            RenderStatCell(0, STR_STAT_CREDITS_PASSED,
                DS("%d", gPlayer.ToTal_credit_pass), C_GREEN);
            RenderStatCell(1, STR_STAT_CREDITS_FAILED,
                DS("%d", gPlayer.ToTal_credit_npass),
                gPlayer.ToTal_credit_npass > 0 ? C_RED : C_SUBTEXT);
            RenderStatCell(2, STR_STAT_CPA_PASS_ONLY,
                DS("%.2f", cpa_pass), C_TEXT);
            RenderStatCell(3, STR_STAT_TOTAL_CREDITS,
                DS("%d / %d", eff, req), eff >= req ? C_GREEN : C_TEXT);
            RenderStatCell(4, STR_STAT_THIS_SECTION,
                DS("%d / %d", sec_pass, sec_want),
                sec_pass >= sec_want ? C_GREEN : C_TEXT);
        }

        }  /* SummaryGroup */

        /* ── Academic alert banner (hidden when alert level = 0) ── */
        if (gPlayer.status_alert > 0) {
            static const Clay_Color alert_fg[4] = {
                {  0,  0,  0,  0},            /* 0: unused      */
                {166,124, 36,255},            /* 1: ochre       */
                {140, 47, 42,255},            /* 2: oxblood     */
                {178, 52, 42,255},            /* 3: brick       */
            };
            static const Clay_Color alert_bg[4] = {
                {  0,  0,  0,  0},
                {245,236,212,255},            /* pale cream     */
                {243,228,225,255},            /* pale crimson   */
                {246,225,221,255},            /* pale rose      */
            };
            static const char *alert_label[4] = {
                "", STR_ALERT_CAUTION, STR_ALERT_WARNING, STR_ALERT_CRITICAL
            };
            CLAY(CLAY_ID("AlertBanner"), {
                .layout = {
                    .sizing         = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(32) },
                    .padding        = { 14, 14, 0, 0 },
                    .childAlignment = { .y = CLAY_ALIGN_Y_CENTER },
                },
                .backgroundColor = alert_bg[gPlayer.status_alert],
                .cornerRadius    = CLAY_CORNER_RADIUS(4),
                .border = { .color = alert_fg[gPlayer.status_alert],
                            .width = { .left=1,.right=1,.top=1,.bottom=1 } },
            }) {
                CLAY_TEXT(
                    DS(STR_ALERT_BANNER_FORMAT,
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
                .cornerRadius    = CLAY_CORNER_RADIUS(4),
                .border          = { .color = C_RED,
                                     .width = { .left=1,.right=1,.top=1,.bottom=1 } },
            }) {
                CLAY_TEXT(CLAY_STRING(STR_WARN_BANNER_TITLE), TC(C_RED, 11));
                const char *_w = gDataWarningsBuf;
                for (int _k = 0; _k < gDataWarnCount; _k++) {
                    CLAY_TEXT(DS(STR_WARN_BANNER_ITEM, _w), TC(C_TEXT, 11));
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
            .cornerRadius    = CLAY_CORNER_RADIUS(4),
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
                        CLAY_TEXT(CLAY_STRING(STR_EMPTY_CATEGORY),
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
                    DS(STR_TBL_FOOTER_FORMAT,
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
                    DS(STR_TBL_FOOTER_PASSED_CR, gPlayer.ToTal_credit_pass),
                    TC(C_ACCENT, 10));
            }
        }
    }
}

/* ═══════════════════════════════════════════════════════════════════════
 *  DASHBOARD PAGE
 * ═══════════════════════════════════════════════════════════════════════ */

/* Colors used for each grade band in charts */
#define C_GRADE_A  ((Clay_Color){ 47, 120,  75, 255})  /* forest      */
#define C_GRADE_B  ((Clay_Color){ 52,  88, 138, 255})  /* muted navy  */
#define C_GRADE_C  ((Clay_Color){166, 124,  36, 255})  /* ochre       */
#define C_GRADE_D  ((Clay_Color){190, 108,  50, 255})  /* burnt orange*/
#define C_GRADE_F  ((Clay_Color){178,  52,  42, 255})  /* brick       */
#define C_GRADE_X  ((Clay_Color){180, 172, 158, 255})  /* faint paper */

/* Per-type colors cycle for the progress bars */
/* Muted, paper-readable hues — each distinct, none neon. */
static const Clay_Color kTypeColors[17] = {
    { 140,  47,  42, 255}, /* 1  co_so_nganh       — oxblood   */
    {  47, 120,  75, 255}, /* 2  dai_cuong         — forest    */
    { 190, 108,  50, 255}, /* 3  the_thao          — burnt org */
    { 166, 124,  36, 255}, /* 4  ly_luat_chinh_tri — ochre     */
    {  46, 134, 130, 255}, /* 5  tu_chon           — teal      */
    { 124,  82, 150, 255}, /* 6  modunI            — plum      */
    { 178,  78, 120, 255}, /* 7  modunII           — rose      */
    {  52,  88, 138, 255}, /* 8  modunIII          — navy      */
    { 110, 130,  60, 255}, /* 9  modunIV           — olive     */
    { 200, 150,  60, 255}, /* 10 modunV            — amber     */
    { 150,  74,  64, 255}, /* 11 modunVI           — terracotta*/
    {  64, 132, 110, 255}, /* 12 thuc_tap          — sea green */
    { 120, 100,  92, 255}, /* 13 do_an_tot_nghiep  — taupe     */
    {  78, 108, 160, 255}, /* 14 modunVII          — denim     */
    {  56, 128, 100, 255}, /* 15 modunVIII         — emerald   */
    {  98,  74, 150, 255}, /* 16 modunIX           — indigo    */
    { 166,  92,  47, 255}, /* 17 modunX            — cinnamon  */
};

/* Nine grade buckets, fine-grained (with +/- modifier): A+ A B+ B C+ C D+ D F */
const char *kGradeLabels[9] = { "A+","A","B+","B","C+","C","D+","D","F" };

/* Color band for a bucket index — shared by the breakdown bars and the radar. */
static Clay_Color grade_band_color(int b)
{
    switch (b) {
        case 0: case 1: return C_GREEN;                     /* A+, A  */
        case 2: case 3: return (Clay_Color){ 52, 88,138,255};/* B+, B */
        case 4: case 5: return C_YELLOW;                    /* C+, C  */
        case 6: case 7: return (Clay_Color){190,108, 50,255};/* D+, D */
        default:        return C_RED;                       /* F      */
    }
}

/* Honor tier → accent color (used by the Dashboard line and the Planner). */
static Clay_Color honor_color(HonorTier t)
{
    switch (t) {
        case HONOR_GOD:       return C_ACCENT;                  /* oxblood — top */
        case HONOR_EXCELLENT: return C_GREEN;
        case HONOR_GOOD:      return (Clay_Color){ 52, 88, 138, 255};  /* navy */
        case HONOR_NORMAL:    return C_YELLOW;                  /* ochre         */
        default:              return C_SUBTEXT;                 /* below class.  */
    }
}

/* Count studied subjects into the nine buckets. Visible to main.c (which
 * #includes this file) for drawing the radar polygon. */
void calc_grade_counts(int out[9])
{
    for (int i = 0; i < 9; i++) out[i] = 0;
    for (int t = 1; t < sizeSubjectType; t++) {
        if (t == the_thao) continue;  /* sport scores excluded from grade distribution */
        Subject_Node *n = gPlayer.numofSubjectType[t].head;
        while (n) {
            if (n->status_ever_been_study & 1) {
                bool plus = (n->status_ever_been_study & 2) != 0;
                int b = -1;
                switch (n->score_letter) {
                    case 'A': b = plus ? 0 : 1; break;
                    case 'B': b = plus ? 2 : 3; break;
                    case 'C': b = plus ? 4 : 5; break;
                    case 'D': b = plus ? 6 : 7; break;
                    case 'F': b = 8; break;
                    default:  b = -1; break;
                }
                if (b >= 0) out[b]++;
            }
            n = n->next;
        }
    }
}


void RenderDashboard(void)
{
    /* All subjects regardless of study */
    int total_subjects = 0;
    for (int t = 1; t < sizeSubjectType; t++)
        total_subjects += gPlayer.numofSubjectType[t].Total_Subject;

    float cpa_all  = calc_cpa(&gPlayer, 0);
    int   eff      = calc_effective_credits(&gPlayer);
    int   req      = calc_required_credits(&gPlayer);

    CLAY(CLAY_ID("Dash"), {
        .layout = {
            .sizing          = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
            .padding         = { 24, 24, 20, 20 },
            .childGap        = SP_LG,
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
        },
        .backgroundColor = C_BG,
        .clip = { .vertical = true, .childOffset = Clay_GetScrollOffset() },
    }) {

        /* ── Page title ── */
        CLAY(CLAY_ID("DashHdr"), {
            .layout = {
                .sizing          = { CLAY_SIZING_FIT(0), CLAY_SIZING_FIT(0) },
                .childGap        = SP_XS,
                .childAlignment  = { .y = CLAY_ALIGN_Y_CENTER },
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
            },
        }) {
            CLAY_TEXT(CLAY_STRING(STR_NAV_DASHBOARD), TC(C_TEXT, 26));
            CLAY_TEXT(
                DS(STR_DASH_OVERVIEW_FORMAT,
                   gUserName, total_subjects),
                TC(C_SUBTEXT, 11));
        }

        /* ── Standing (dominant) + supporting stat ledger ── */
        CLAY(CLAY_ID("DashTop"), {
            .layout = {
                .sizing          = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
                .childGap        = SP_SM,
                .layoutDirection = CLAY_LEFT_TO_RIGHT,
            },
        }) {
            float prog = req > 0 ? (float)eff / (float)req : 0.f;
            if (prog > 1.f) prog = 1.f;
            bool grad_ok = gPlayer.status_can_grauate;

            /* Dominant: academic standing + a real progress bar */
            CLAY(CLAY_ID("DashStanding"), {
                .layout = {
                    .sizing          = { CLAY_SIZING_PERCENT(0.42f), CLAY_SIZING_FIT(0) },
                    .padding         = { 20, 20, 18, 18 },
                    .childGap        = SP_SM,
                    .layoutDirection = CLAY_TOP_TO_BOTTOM,
                },
                .backgroundColor = grad_ok ? C_GREEN_BG : C_CARD,
                .cornerRadius    = CLAY_CORNER_RADIUS(4),
                .border          = { .color = grad_ok ? C_GREEN : C_BORDER,
                                     .width = { .left=1,.right=1,.top=1,.bottom=1 } },
            }) {
                CLAY_TEXT(CLAY_STRING(STR_DASH_STANDING_LABEL), TC(C_SUBTEXT, 9));
                CLAY_TEXT(grad_ok ? CLAY_STRING(STR_DASH_GRAD_READY)
                                  : CLAY_STRING(STR_DASH_GRAD_PROGRESS),
                          TC(grad_ok ? C_GREEN : C_TEXT, 24));
                /* progress track (eff / req) */
                CLAY(CLAY_ID("DashProgTrack"), {
                    .layout = {
                        .sizing          = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(10) },
                        .layoutDirection = CLAY_LEFT_TO_RIGHT,
                    },
                    .backgroundColor = C_BORDER,
                    .cornerRadius    = CLAY_CORNER_RADIUS(5),
                }) {
                    CLAY(CLAY_ID("DashProgFill"), {
                        .layout = { .sizing = {
                            CLAY_SIZING_PERCENT(prog > 0.f ? prog : 0.01f),
                            CLAY_SIZING_GROW(0) } },
                        .backgroundColor = grad_ok ? C_GREEN : C_ACCENT,
                        .cornerRadius    = CLAY_CORNER_RADIUS(5),
                    }) {}
                }
                CLAY_TEXT(DS(STR_DASH_PROG_FORMAT,
                             eff, req, (int)(prog * 100.f + 0.5f)),
                          TC(C_SUBTEXT, 10));
                /* projected honor tier — ties the Planner forecast into the
                 * main screen (click "Planner" to plan toward a target) */
                {
                    HonorProjection _hp = honor_project(&gPlayer);
                    CLAY_TEXT(DS(STR_DASH_HONOR_FORMAT,
                                 honor_name(_hp.projected), _hp.rate),
                              TC(honor_color(_hp.projected), 10));
                }
            }

            /* Supporting: stat ledger, one panel with hairline dividers */
            CLAY(CLAY_ID("DashLedger"), {
                .layout = {
                    .sizing          = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
                    .layoutDirection = CLAY_LEFT_TO_RIGHT,
                },
                .backgroundColor = C_CARD,
                .cornerRadius    = CLAY_CORNER_RADIUS(4),
                .border          = { .color = C_BORDER,
                                     .width = { .left=1,.right=1,.top=1,.bottom=1,
                                                .betweenChildren=1 } },
            }) {
                RenderStatCell(0, STR_STAT_CREDITS_PASSED,
                    DS("%d", gPlayer.ToTal_credit_pass), C_GREEN);
                RenderStatCell(1, STR_STAT_CREDITS_FAILED,
                    DS("%d", gPlayer.ToTal_credit_npass),
                    gPlayer.ToTal_credit_npass > 0 ? C_RED : C_SUBTEXT);
                RenderStatCell(2, STR_HDR_CPA_LABEL,
                    DS("%.2f", cpa_all), C_ACCENT);
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
                .cornerRadius    = CLAY_CORNER_RADIUS(4),
                .border          = { .color = C_BORDER,
                                     .width = { .left=1,.right=1,.top=1,.bottom=1 } },
            }) {
                CLAY_TEXT(CLAY_STRING(STR_DASH_GRADE_DIST), TC(C_SUBTEXT, 10));
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
                .cornerRadius    = CLAY_CORNER_RADIUS(4),
                .border          = { .color = C_BORDER,
                                     .width = { .left=1,.right=1,.top=1,.bottom=1 } },
            }) {
                CLAY_TEXT(CLAY_STRING(STR_DASH_CPA_GAUGE), TC(C_SUBTEXT, 10));
                CLAY(CLAY_ID(DONUT_CPA_ID), {
                    .layout = {
                        .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
                    },
                    .backgroundColor = C_TRANS,
                }) {}
                CLAY_TEXT(DS(STR_DASH_CPA_FORMAT, cpa_all), TC(C_ACCENT, 10));
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
                .cornerRadius    = CLAY_CORNER_RADIUS(4),
                .border          = { .color = C_BORDER,
                                     .width = { .left=1,.right=1,.top=1,.bottom=1 } },
            }) {
                CLAY_TEXT(CLAY_STRING(STR_DASH_CREDITS_BY_TYPE), TC(C_SUBTEXT, 10));
                for (int t = 1; t < sizeSubjectType; t++) {
                        Subject_Type *st = &gPlayer.numofSubjectType[t];
                        if (gTypeName[t][0] == 0 || st->Total_Subject == 0) continue;

                        int pass = _sl_resolve_pass(&gPlayer, t);
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
                                    DS(STR_DASH_TYPE_BAR_FORMAT, pass, lim),
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

        /* ── Bottom: grade distribution (radar + breakdown) ── */
        int gc[9];
        calc_grade_counts(gc);
        int gc_max = 1, gc_total = 0;
        for (int i = 0; i < 9; i++) {
            gc_total += gc[i];
            if (gc[i] > gc_max) gc_max = gc[i];
        }

        CLAY(CLAY_ID("DashGrades"), {
            .layout = {
                .sizing          = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
                .padding         = { 16, 16, 14, 14 },
                .childGap        = SP_SM,
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
            },
            .backgroundColor = C_CARD,
            .cornerRadius    = CLAY_CORNER_RADIUS(4),
            .border          = { .color = C_BORDER,
                                 .width = { .left=1,.right=1,.top=1,.bottom=1 } },
        }) {
            /* header */
            CLAY(CLAY_ID("DGradeHdr"), {
                .layout = {
                    .sizing          = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(30) },
                    .padding         = { 0, 0, 0, 8 },
                    .childGap        = 12,
                    .childAlignment  = { .y = CLAY_ALIGN_Y_CENTER },
                    .layoutDirection = CLAY_LEFT_TO_RIGHT,
                },
                .border = { .color = C_BORDER, .width = { .bottom = 1 } },
            }) {
                CLAY_TEXT(CLAY_STRING(STR_DASH_GRADE_DIST), TC(C_TEXT, 12));
                CLAY(CLAY_ID("DGradeSp"), {
                    .layout = { .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(1) } },
                }) {}
                CLAY_TEXT(DS(STR_DASH_GRADED_SUBJECTS, gc_total), TC(C_SUBTEXT, 10));
            }

            /* body: radar (left) + breakdown bars (right) */
            CLAY(CLAY_ID("DGradeBody"), {
                .layout = {
                    .sizing          = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(280) },
                    .childGap        = SP_LG,
                    .childAlignment  = { .y = CLAY_ALIGN_Y_CENTER },
                    .layoutDirection = CLAY_LEFT_TO_RIGHT,
                },
            }) {
                /* radar placeholder — Raylib draws the spider chart into this box */
                CLAY(CLAY_ID(RADAR_GRADE_ID), {
                    .layout = { .sizing = { CLAY_SIZING_PERCENT(0.5f),
                                            CLAY_SIZING_GROW(0) } },
                    .backgroundColor = C_TRANS,
                }) {}

                /* breakdown: one bar per grade, exact counts */
                CLAY(CLAY_ID("DGradeBreak"), {
                    .layout = {
                        .sizing          = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
                        .childGap        = SP_SM,
                        .layoutDirection = CLAY_TOP_TO_BOTTOM,
                    },
                }) {
                    for (int b = 0; b < 9; b++) {
                        Clay_Color bc = grade_band_color(b);
                        float pct = (float)gc[b] / (float)gc_max;
                        if (pct < 0.f) pct = 0.f;
                        CLAY(CLAY_IDI("GBRow", b), {
                            .layout = {
                                .sizing          = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(20) },
                                .childGap        = 10,
                                .childAlignment  = { .y = CLAY_ALIGN_Y_CENTER },
                                .layoutDirection = CLAY_LEFT_TO_RIGHT,
                            },
                        }) {
                            /* grade label */
                            CLAY(CLAY_IDI("GBLbl", b), {
                                .layout = {
                                    .sizing         = { CLAY_SIZING_FIXED(32), CLAY_SIZING_GROW(0) },
                                    .childAlignment = { .y = CLAY_ALIGN_Y_CENTER },
                                },
                            }) { CLAY_TEXT(CS(kGradeLabels[b]),
                                           TC(gc[b] > 0 ? bc : C_SUBTEXT, 11)); }
                            /* track + fill */
                            CLAY(CLAY_IDI("GBTrk", b), {
                                .layout = {
                                    .sizing          = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(10) },
                                    .layoutDirection = CLAY_LEFT_TO_RIGHT,
                                },
                                .backgroundColor = C_BORDER,
                                .cornerRadius    = CLAY_CORNER_RADIUS(5),
                            }) {
                                if (gc[b] > 0)
                                    CLAY(CLAY_IDI("GBFill", b), {
                                        .layout = { .sizing = {
                                            CLAY_SIZING_PERCENT(pct > 0.04f ? pct : 0.04f),
                                            CLAY_SIZING_GROW(0) } },
                                        .backgroundColor = bc,
                                        .cornerRadius    = CLAY_CORNER_RADIUS(5),
                                    }) {}
                            }
                            /* count */
                            CLAY(CLAY_IDI("GBCnt", b), {
                                .layout = {
                                    .sizing         = { CLAY_SIZING_FIXED(28), CLAY_SIZING_GROW(0) },
                                    .padding        = { 0, 4, 0, 0 },
                                    .childAlignment = { .x = CLAY_ALIGN_X_RIGHT,
                                                        .y = CLAY_ALIGN_Y_CENTER },
                                },
                            }) { CLAY_TEXT(DS("%d", gc[b]),
                                           TC(gc[b] > 0 ? C_TEXT : C_SUBTEXT, 11)); }
                        }
                    }
                }
            }
        }
    }
}

/* ═══════════════════════════════════════════════════════════════════════
 *  GRADUATION PLANNER
 *  Honor-tier forecast + target feasibility + ranked remaining-subject guide.
 *  All math lives in score_logic.h (honor_project / honor_target_plan).
 * ═══════════════════════════════════════════════════════════════════════ */

/* One remaining (not-yet-passed) subject plus its subject-type id. */
typedef struct { Subject_Node *node; int type; } PlanItem;

/* Ranking: failed retakes first, then earlier recommended term, then higher
 * credit (more CPA leverage). */
static int plan_item_cmp(const void *a, const void *b)
{
    const PlanItem *x = (const PlanItem *)a, *y = (const PlanItem *)b;
    int xf = (x->node->score_letter == 'F');
    int yf = (y->node->score_letter == 'F');
    if (xf != yf) return yf - xf;                                  /* F first   */
    int xt = (int)x->node->term_recomment_to_studie;
    int yt = (int)y->node->term_recomment_to_studie;
    if (xt != yt) return xt - yt;                                  /* term asc  */
    return (int)y->node->credit - (int)x->node->credit;            /* credit ↓  */
}

/* The four selectable honor tiers, low → high. */
static const HonorTier kPlanTiers[4] =
    { HONOR_NORMAL, HONOR_GOOD, HONOR_EXCELLENT, HONOR_GOD };

/* Ambition levels within a tier's band. */
static const FlexLevel kPlanFlex[3]  = { FLEX_LOW, FLEX_MED, FLEX_HIGH };
static const char     *kFlexName[3]  = { "Low", "Medium", "High" };
static const char     *kFlexDesc[3]  = { "just reach it", "mid-band", "every possible" };

/* Is subject type `t` still an OPEN graduation requirement whose remaining
 * subjects should appear in the planner?  Driven entirely by gGradRules so the
 * list matches the credit math — a satisfied requirement contributes nothing:
 *   - sport (counted by subjects, no CPA effect)  → never
 *   - type not offered in this major (no subjects)→ no
 *   - pick-best module group                      → only the recommended
 *       member (chosen[gid]), and only while the group is still unmet
 *   - standalone type / elective pool             → only while its passed
 *       credits are below its required limit
 * `chosen[gid]` must already hold the recommended member of each group. */
static bool plan_type_open(int t, const int *chosen)
{
    const GradRule     *r  = &gGradRules[t];
    const Subject_Type *st = &gPlayer.numofSubjectType[t];
    if (r->mode == GRAD_SUBJECT_COUNT) return false;
    if (st->Total_Subject == 0)        return false;

    if (r->group_id != 0) {
        if (chosen[r->group_id] != t) return false;   /* not the picked module */
        int best_pass = -1, best_limit = 0;
        for (int j = 1; j < sizeSubjectType; j++) {
            if (gGradRules[j].group_id != r->group_id) continue;
            int pc  = (int)gPlayer.numofSubjectType[j].count_passCredit;
            int lim = _sl_resolve_limit(&gPlayer, j);
            if (pc > best_pass) { best_pass = pc; best_limit = lim; }
        }
        return best_pass < best_limit;                /* group still unmet     */
    }
    return (int)st->count_passCredit < _sl_resolve_limit(&gPlayer, t);
}

static SandboxOverride *GetSandboxOverride(const char *subject_id)
{
    for (int i = 0; i < gApp.sandbox_override_count; i++) {
        if (strcmp(gApp.sandbox_overrides[i].subject_id, subject_id) == 0) {
            return &gApp.sandbox_overrides[i];
        }
    }
    return NULL;
}

static void SetSandboxOverride(const char *subject_id, char letter, int plus)
{
    SandboxOverride *opt = GetSandboxOverride(subject_id);
    if (opt) {
        if (letter == 0) {
            int idx = (int)(opt - gApp.sandbox_overrides);
            gApp.sandbox_overrides[idx] = gApp.sandbox_overrides[gApp.sandbox_override_count - 1];
            gApp.sandbox_override_count--;
        } else {
            opt->grade_letter = letter;
            opt->plus = plus;
        }
    } else if (letter != 0) {
        if (gApp.sandbox_override_count < 128) {
            SandboxOverride *new_opt = &gApp.sandbox_overrides[gApp.sandbox_override_count++];
            snprintf(new_opt->subject_id, sizeof(new_opt->subject_id), "%s", subject_id);
            new_opt->grade_letter = letter;
            new_opt->plus = plus;
        }
    }
}

static float calc_cpa_sandbox(Player *p, int pass_only)
{
    float total   = 0.0f;
    int   credits = 0;
    for (int i = 0; i < sizeSubjectType; i++) {
        Subject_Node *cur = p->numofSubjectType[i].head;
        while (cur) {
            SandboxOverride *ov = GetSandboxOverride(cur->ID);
            int use = 0;
            char letter = cur->score_letter;
            int plus = (cur->status_ever_been_study & 2) != 0;
            if (ov && ov->grade_letter != 0) {
                int is_pass = (ov->grade_letter != 'F');
                use = pass_only ? is_pass : 1;
                letter = ov->grade_letter;
                plus = ov->plus;
            } else {
                use = pass_only ? (int)cur->status_pass
                                : (int)(cur->status_ever_been_study & 1);
            }
            if (use) {
                total   += score_to_gpa(letter, plus) * (float)cur->credit;
                credits += (int)cur->credit;
            }
            cur = cur->next;
        }
    }
    return (credits == 0) ? 0.0f : total / (float)credits;
}

static int calc_pass_credits_type_sandbox(Player *p, int t)
{
    if (t < 0 || t >= sizeSubjectType) return 0;
    int credits = 0;
    Subject_Node *cur = p->numofSubjectType[t].head;
    while (cur) {
        SandboxOverride *ov = GetSandboxOverride(cur->ID);
        int is_pass = 0;
        if (ov && ov->grade_letter != 0) {
            is_pass = (ov->grade_letter != 'F');
        } else {
            is_pass = (int)cur->status_pass;
        }
        if (is_pass) {
            credits += (int)cur->credit;
        }
        cur = cur->next;
    }
    return credits;
}

static int calc_effective_credits_sandbox(Player *p)
{
    int count = 0;
    int done[sizeSubjectType];
    for (int k = 0; k < sizeSubjectType; k++) done[k] = 0;
    for (int i = 0; i < sizeSubjectType; i++) {
        if (done[i]) continue;
        const GradRule *r = &gGradRules[i];
        if (r->mode == GRAD_SUBJECT_COUNT) { done[i] = 1; continue; }
        if (r->group_id != 0) {
            int best = 0;
            for (int j = 0; j < sizeSubjectType; j++) {
                if (gGradRules[j].group_id == r->group_id) {
                    int pc = calc_pass_credits_type_sandbox(p, j);
                    if (pc > best) best = pc;
                    done[j] = 1;
                }
            }
            count += best;
        } else {
            count += calc_pass_credits_type_sandbox(p, i);
            done[i] = 1;
        }
    }
    return count;
}

static HonorProjection honor_project_sandbox(Player *p)
{
    HonorProjection hp;
    hp.eff       = calc_effective_credits_sandbox(p);
    hp.req       = calc_required_credits(p);
    hp.remaining = hp.req - hp.eff;
    if (hp.remaining < 0) hp.remaining = 0;
    hp.rate      = calc_cpa_sandbox(p, 1);
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

static void RenderSandboxGradeStepper(int uid, const char *subject_id)
{
    int idx = 0;
    SandboxOverride *ov = GetSandboxOverride(subject_id);
    if (ov && ov->grade_letter != 0) {
        if (ov->grade_letter == 'F') idx = 1;
        else if (ov->grade_letter == 'D' && !ov->plus) idx = 2;
        else if (ov->grade_letter == 'D' && ov->plus) idx = 3;
        else if (ov->grade_letter == 'C' && !ov->plus) idx = 4;
        else if (ov->grade_letter == 'C' && ov->plus) idx = 5;
        else if (ov->grade_letter == 'B' && !ov->plus) idx = 6;
        else if (ov->grade_letter == 'B' && ov->plus) idx = 7;
        else if (ov->grade_letter == 'A') idx = 8;
    }

    CLAY(CLAY_IDI("SandboxStep", uid), {
        .layout = { .sizing          = { CLAY_SIZING_FIT(0), CLAY_SIZING_FIT(0) },
                    .childGap        = SP_XS,
                    .childAlignment  = { .y = CLAY_ALIGN_Y_CENTER },
                    .layoutDirection = CLAY_LEFT_TO_RIGHT },
    }) {
        bool dec_clicked = false;
        int btn_sz = 22;
        CLAY(CLAY_IDI("SandDecBtn", uid), {
            .layout = {
                .sizing         = { CLAY_SIZING_FIXED(btn_sz), CLAY_SIZING_FIXED(btn_sz) },
                .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER },
            },
            .backgroundColor = Clay_Hovered() ? C_ACCENT : C_CARD,
            .cornerRadius    = CLAY_CORNER_RADIUS(4),
            .border          = { .color = C_BORDER, .width = { .left=1,.right=1,.top=1,.bottom=1 } },
        }) {
            if (Clay_Hovered() && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) dec_clicked = true;
            CLAY_TEXT(CLAY_STRING("-"), TC(Clay_Hovered() ? C_WHITE : C_ACCENT, 11));
        }

        char val_buf[8];
        if (idx == 0) strcpy(val_buf, "-");
        else if (idx == 1) strcpy(val_buf, "F");
        else if (idx == 2) strcpy(val_buf, "D");
        else if (idx == 3) strcpy(val_buf, "D+");
        else if (idx == 4) strcpy(val_buf, "C");
        else if (idx == 5) strcpy(val_buf, "C+");
        else if (idx == 6) strcpy(val_buf, "B");
        else if (idx == 7) strcpy(val_buf, "B+");
        else if (idx == 8) strcpy(val_buf, "A");

        CLAY(CLAY_IDI("SandValBox", uid), {
            .layout = {
                .sizing         = { CLAY_SIZING_FIXED(36), CLAY_SIZING_FIXED(btn_sz) },
                .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER },
            },
            .backgroundColor = (idx > 0) ? C_ACCENT_BG : C_TBL_HDR,
            .cornerRadius    = CLAY_CORNER_RADIUS(4),
            .border          = { .color = C_BORDER, .width = { .left=1,.right=1,.top=1,.bottom=1 } },
        }) {
            CLAY_TEXT(CS(val_buf), TC((idx > 0) ? C_ACCENT : C_SUBTEXT, 10));
        }

        bool inc_clicked = false;
        CLAY(CLAY_IDI("SandIncBtn", uid), {
            .layout = {
                .sizing         = { CLAY_SIZING_FIXED(btn_sz), CLAY_SIZING_FIXED(btn_sz) },
                .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER },
            },
            .backgroundColor = Clay_Hovered() ? C_ACCENT : C_CARD,
            .cornerRadius    = CLAY_CORNER_RADIUS(4),
            .border          = { .color = C_BORDER, .width = { .left=1,.right=1,.top=1,.bottom=1 } },
        }) {
            if (Clay_Hovered() && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) inc_clicked = true;
            CLAY_TEXT(CLAY_STRING("+"), TC(Clay_Hovered() ? C_WHITE : C_ACCENT, 11));
        }

        if (dec_clicked || inc_clicked) {
            int new_idx = idx;
            if (dec_clicked) new_idx--;
            if (inc_clicked) new_idx++;
            if (new_idx < 0) new_idx = 0;
            if (new_idx > 8) new_idx = 8;

            if (new_idx != idx) {
                if (new_idx == 0) {
                    SetSandboxOverride(subject_id, 0, 0);
                } else {
                    char letter = 'F';
                    int plus = 0;
                    if (new_idx == 1) { letter = 'F'; plus = 0; }
                    else if (new_idx == 2) { letter = 'D'; plus = 0; }
                    else if (new_idx == 3) { letter = 'D'; plus = 1; }
                    else if (new_idx == 4) { letter = 'C'; plus = 0; }
                    else if (new_idx == 5) { letter = 'C'; plus = 1; }
                    else if (new_idx == 6) { letter = 'B'; plus = 0; }
                    else if (new_idx == 7) { letter = 'B'; plus = 1; }
                    else if (new_idx == 8) { letter = 'A'; plus = 0; }
                    SetSandboxOverride(subject_id, letter, plus);
                }
            }
        }
    }
}

void RenderPlanner(void)
{
    HonorProjection hp = honor_project_sandbox(&gPlayer);

    /* ── pick the recommended member of each pick-best module group ──
     * (the one with the most passed credits so far; ties keep the first). */
    int chosen[sizeSubjectType];
    memset(chosen, 0, sizeof(chosen));
    for (int t = 1; t < sizeSubjectType; t++) {
        int gid = gGradRules[t].group_id;
        if (gid <= 0 || gid >= sizeSubjectType) continue;
        if (gPlayer.numofSubjectType[t].Total_Subject == 0) continue;
        if (chosen[gid] == 0 ||
            gPlayer.numofSubjectType[t].count_passCredit >
            gPlayer.numofSubjectType[chosen[gid]].count_passCredit)
            chosen[gid] = t;
    }

    /* ── collect remaining subjects from OPEN requirements only, then rank ── */
    static PlanItem items[160];
    int nItems = 0;
    for (int t = 1; t < sizeSubjectType; t++) {
        if (!plan_type_open(t, chosen)) continue;   /* satisfied/irrelevant → skip */
        for (Subject_Node *n = gPlayer.numofSubjectType[t].head; n; n = n->next) {
            if (n->status_pass) continue;
            if (nItems < (int)(sizeof(items) / sizeof(items[0]))) {
                items[nItems].node = n;
                items[nItems].type = t;
                nItems++;
            }
        }
    }
    qsort(items, (size_t)nItems, sizeof(items[0]), plan_item_cmp);

    bool  haveTarget = (gPlanTarget != HONOR_NONE);
    /* the tier is reachable at all only if its floor (low ambition) is */
    bool  tierReachable = haveTarget &&
        honor_target_plan(&hp, gPlanTarget).status != TARGET_IMPOSSIBLE;
    float flexT = haveTarget ? honor_flex_target(gPlanTarget, gPlanFlex) : 0.f;
    TargetPlan tp = haveTarget ? honor_target_plan_at(&hp, flexT)
                               : (TargetPlan){0};

    CLAY(CLAY_ID("Planner"), {
        .layout = {
            .sizing          = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
            .padding         = { 24, 24, 20, 20 },
            .childGap        = SP_LG,
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
        },
        .backgroundColor = C_BG,
        .clip = { .vertical = true, .childOffset = Clay_GetScrollOffset() },
    }) {

        /* ── Title ── */
        CLAY(CLAY_ID("PlanHdr"), {
            .layout = { .sizing = { CLAY_SIZING_FIT(0), CLAY_SIZING_FIT(0) },
                        .childGap = SP_XS, .layoutDirection = CLAY_TOP_TO_BOTTOM },
        }) {
            CLAY_TEXT(CLAY_STRING(STR_PLAN_TITLE), TC(C_TEXT, 26));
            CLAY_TEXT(AutoWrapString(STR_PLAN_SUBTITLE),
                      TC(C_SUBTEXT, 11));
        }

        /* ── Standing card: projected tier + reachable range + credits ── */
        CLAY(CLAY_ID("PlanStanding"), {
            .layout = {
                .sizing          = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
                .padding         = { 20, 20, 18, 18 },
                .childGap        = SP_SM,
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
            },
            .backgroundColor = C_CARD,
            .cornerRadius    = CLAY_CORNER_RADIUS(4),
            .border          = { .color = honor_color(hp.projected),
                                 .width = { .left = 3, .top = 1, .right = 1, .bottom = 1 } },
        }) {
            CLAY(CLAY_ID("PlanStandingHeader"), {
                .layout = { .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
                            .childGap = SP_SM, .layoutDirection = CLAY_LEFT_TO_RIGHT,
                            .childAlignment = { .y = CLAY_ALIGN_Y_CENTER } },
            }) {
                CLAY_TEXT(CLAY_STRING(STR_PLAN_PROJECTED_LABEL),
                          TC(C_SUBTEXT, 9));
                if (gApp.sandbox_override_count > 0) {
                    CLAY(CLAY_ID("PlanSandboxBadge"), {
                        .layout = { .padding = { 6, 6, 2, 2 } },
                        .backgroundColor = C_ACCENT_BG,
                        .cornerRadius    = CLAY_CORNER_RADIUS(3),
                    }) {
                        CLAY_TEXT(CLAY_STRING("SANDBOX ACTIVE (SIMULATED)"), TC(C_ACCENT, 8));
                    }
                }
            }
            CLAY_TEXT(CS(honor_name(hp.projected)), TC(honor_color(hp.projected), 24));
            CLAY_TEXT(DS(STR_PLAN_STATS_FORMAT,
                         hp.rate, hp.eff, hp.req, hp.remaining),
                      TC(C_SUBTEXT, 10));
            if (hp.remaining > 0)
                CLAY_TEXT(DS(STR_PLAN_REACHABLE_FORMAT,
                             honor_name(hp.worst), honor_name(hp.best)),
                          TC(C_TEXT, 10));
            else
                CLAY_TEXT(CLAY_STRING(STR_PLAN_FINAL_STANDING),
                          TC(C_TEXT, 10));
        }

        /* ── Target picker ── */
        CLAY(CLAY_ID("PlanPickLbl"), {
            .layout = { .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) } },
        }) { CLAY_TEXT(CLAY_STRING(STR_PLAN_GRADUATE_AS), TC(C_SUBTEXT, 9)); }

        /* Deterministic widths: a row of equal GROW children collapses in this
         * Clay build (only one keeps width), so size the four buttons FIXED off
         * the content width. Mobile stacks them, where single-column GROW is safe. */
        float pickAvail = (float)(gIsMobile ? gScreenW : gScreenW - SIDEBAR_W) - 48.f;
        if (pickAvail < 80.f) pickAvail = 80.f;
        float pickBtnW = (pickAvail - 3.f * SP_SM) / 4.f;

        CLAY(CLAY_ID("PlanPick"), {
            .layout = {
                .sizing          = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
                .childGap        = SP_SM,
                .layoutDirection = gIsMobile ? CLAY_TOP_TO_BOTTOM : CLAY_LEFT_TO_RIGHT,
            },
        }) {
            for (int b = 0; b < 4; b++) {
                HonorTier tier = kPlanTiers[b];
                bool   active  = (gPlanTarget == tier);
                bool   imposs  = (honor_target_plan(&hp, tier).status == TARGET_IMPOSSIBLE);
                Clay_Color col = honor_color(tier);

                CLAY(CLAY_IDI("PlanTgt", b), {
                    .layout = {
                        .sizing          = { gIsMobile ? CLAY_SIZING_GROW(0)
                                                       : CLAY_SIZING_FIXED(pickBtnW),
                                             CLAY_SIZING_FIXED(54) },
                        .padding         = { 12, 12, 8, 8 },
                        .childGap        = 2,
                        .childAlignment  = { .x = CLAY_ALIGN_X_CENTER },
                        .layoutDirection = CLAY_TOP_TO_BOTTOM,
                    },
                    .backgroundColor = active ? col
                                     : (Clay_Hovered() && !imposs ? C_ROW_HOVER : C_CARD),
                    .cornerRadius    = CLAY_CORNER_RADIUS(4),
                    .border          = { .color = imposs ? C_BORDER : col,
                                         .width = { .left=1,.right=1,.top=1,.bottom=1 } },
                }) {
                    if (Clay_Hovered() && !imposs && IsMouseButtonReleased(MOUSE_LEFT_BUTTON))
                        gPlanTarget = tier;
                    Clay_Color tcol = active ? C_WHITE : (imposs ? C_SUBTEXT : col);
                    CLAY_TEXT(CS(honor_name(tier)), TC(tcol, 12));
                    CLAY_TEXT(DS(STR_PLAN_CPA_MIN_FORMAT, kHonorFloor[tier]),
                              TC(active ? C_WHITE : C_SUBTEXT, 8));
                    if (imposs)
                        CLAY_TEXT(CLAY_STRING(STR_PLAN_OUT_OF_REACH),
                                  TC(active ? C_WHITE : C_SUBTEXT, 8));
                }
            }
        }

        /* ── Ambition (flex) selector — how hard to push within the tier ── */
        if (haveTarget && tierReachable) {
            CLAY(CLAY_ID("PlanFlexLbl"), {
                .layout = { .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) } },
            }) { CLAY_TEXT(DS(STR_PLAN_FLEX_LABEL, honor_name(gPlanTarget)),
                           TC(C_SUBTEXT, 9)); }

            float flexBtnW = (pickAvail - 2.f * SP_SM) / 3.f;
            Clay_Color tcol = honor_color(gPlanTarget);

            CLAY(CLAY_ID("PlanFlex"), {
                .layout = {
                    .sizing          = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
                    .childGap        = SP_SM,
                    .layoutDirection = gIsMobile ? CLAY_TOP_TO_BOTTOM : CLAY_LEFT_TO_RIGHT,
                },
            }) {
                for (int f = 0; f < 3; f++) {
                    bool  fa = (gPlanFlex == kPlanFlex[f]);
                    float ft = honor_flex_target(gPlanTarget, kPlanFlex[f]);
                    CLAY(CLAY_IDI("PlanFlexBtn", f), {
                        .layout = {
                            .sizing          = { gIsMobile ? CLAY_SIZING_GROW(0)
                                                           : CLAY_SIZING_FIXED(flexBtnW),
                                                 CLAY_SIZING_FIXED(50) },
                            .padding         = { 10, 10, 6, 6 },
                            .childGap        = 2,
                            .childAlignment  = { .x = CLAY_ALIGN_X_CENTER },
                            .layoutDirection = CLAY_TOP_TO_BOTTOM,
                        },
                        .backgroundColor = fa ? tcol
                                         : (Clay_Hovered() ? C_ROW_HOVER : C_CARD),
                        .cornerRadius    = CLAY_CORNER_RADIUS(4),
                        .border          = { .color = tcol,
                                             .width = { .left=1,.right=1,.top=1,.bottom=1 } },
                    }) {
                        if (Clay_Hovered() && IsMouseButtonReleased(MOUSE_LEFT_BUTTON))
                            gPlanFlex = kPlanFlex[f];
                        CLAY_TEXT(DS(STR_PLAN_FLEX_FORMAT, kFlexName[f], ft),
                                  TC(fa ? C_WHITE : tcol, 11));
                        CLAY_TEXT(CS(kFlexDesc[f]), TC(fa ? C_WHITE : C_SUBTEXT, 8));
                    }
                }
            }
        }

        /* ── Verdict ── */
        {
            Clay_Color vbg = C_CARD; Clay_Color vbd = C_BORDER;
            Clay_String vtxt;
            if (!haveTarget) {
                vtxt = CLAY_STRING(STR_PLAN_VERDICT_NONE);
            } else if (tp.status == TARGET_SECURED) {
                vbg = C_GREEN_BG; vbd = C_GREEN;
                vtxt = (hp.remaining > 0)
                     ? DS(STR_PLAN_VERDICT_SECURE_R,
                          honor_name(gPlanTarget), hp.remaining)
                     : DS(STR_PLAN_VERDICT_SECURE_L, honor_name(gPlanTarget));
            } else if (tp.status == TARGET_REACHABLE) {
                vbd = honor_color(gPlanTarget);
                vtxt = DS(STR_PLAN_VERDICT_REACH,
                          honor_name(gPlanTarget), tp.needed_avg,
                          tp.need_letter, tp.need_plus ? "+" : "", hp.remaining);
            } else { /* IMPOSSIBLE */
                vbg = C_RED_BG; vbd = C_RED;
                vtxt = DS(STR_PLAN_VERDICT_IMPOSS,
                          honor_name(gPlanTarget), honor_name(hp.best));
            }
            CLAY(CLAY_ID("PlanVerdict"), {
                .layout = { .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
                            .padding = { 16, 16, 12, 12 } },
                .backgroundColor = vbg,
                .cornerRadius    = CLAY_CORNER_RADIUS(4),
                .border          = { .color = vbd, .width = { .left=3,.top=1,.right=1,.bottom=1 } },
            }) { CLAY_TEXT(vtxt, TCW(C_TEXT, 12)); }
        }

        /* ── Remaining-subject list ── */
        CLAY(CLAY_ID("PlanListLbl"), {
            .layout = { .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) } },
        }) {
            CLAY_TEXT(DS(STR_PLAN_LIST_LABEL,
                         nItems), TC(C_SUBTEXT, 9));
        }

        if (nItems == 0) {
            CLAY(CLAY_ID("PlanEmpty"), {
                .layout = { .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
                            .padding = { 16, 16, 16, 16 } },
                .backgroundColor = C_CARD,
                .cornerRadius    = CLAY_CORNER_RADIUS(4),
                .border          = { .color = C_BORDER, .width = { .left=1,.right=1,.top=1,.bottom=1 } },
            }) {
                CLAY_TEXT(CLAY_STRING(STR_PLAN_LIST_EMPTY),
                          TC(C_GREEN, 12));
            }
        }

        for (int i = 0; i < nItems; i++) {
            Subject_Node *n = items[i].node;
            int  ty      = items[i].type;
            bool failed  = (n->score_letter == 'F');
            bool isOdd   = (i % 2 != 0);
            /* genuine free choice = elective pool; the picked module is committed */
            bool pool    = (gGradRules[ty].mode == GRAD_FIXED);
            bool sport   = (gGradRules[ty].mode == GRAD_SUBJECT_COUNT);
            bool impact  = !sport && n->credit >= 3;

            CLAY(CLAY_IDI("PlanRow", i), {
                .layout = {
                    .sizing          = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(ROW_H) },
                    .padding         = { 12, 12, 0, 0 },
                    .childGap        = SP_SM,
                    .childAlignment  = { .y = CLAY_ALIGN_Y_CENTER },
                    .layoutDirection = CLAY_LEFT_TO_RIGHT,
                },
                .backgroundColor = isOdd ? C_ROW_ODD : C_ROW_EVEN,
                .border = { .color = failed ? C_RED : C_BORDER,
                            .width = { .left = failed ? 3 : 0, .bottom = 1 } },
            }) {
                /* recommended term (desktop only) */
                if (!gIsMobile)
                    CLAY_TEXT(DS(STR_PLAN_ROW_TERM_FORMAT, n->term_recomment_to_studie), TC(C_SUBTEXT, 10));
                /* code (desktop only) */
                if (!gIsMobile)
                    CLAY_TEXT(DS("%s", n->ID), TC(C_SUBTEXT, 10));
                /* name (grows) */
                CLAY(CLAY_IDI("PlanRowName", i), {
                    .layout = { .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) } },
                }) { CLAY_TEXT(DS("%s", n->name), TC(C_TEXT, 11)); }

                /* high-impact tag */
                if (impact && haveTarget && tp.status == TARGET_REACHABLE) {
                    CLAY(CLAY_IDI("PlanImpact", i), {
                        .layout = { .padding = { 6, 6, 2, 2 } },
                        .backgroundColor = C_ACCENT_BG,
                        .cornerRadius    = CLAY_CORNER_RADIUS(3),
                    }) { CLAY_TEXT(CLAY_STRING(STR_PLAN_HIGH_IMPACT), TC(C_ACCENT, 8)); }
                }
                /* elective/module hint (desktop only) */
                if (pool && !gIsMobile)
                    CLAY_TEXT(CLAY_STRING(STR_PLAN_OPTIONAL), TC(C_SUBTEXT, 8));

                /* credit (desktop only) */
                if (!gIsMobile)
                    CLAY_TEXT(DS(STR_PLAN_ROW_CR_FORMAT, n->credit), TC(C_SUBTEXT, 10));

                /* status chip */
                CLAY(CLAY_IDI("PlanStat", i), {
                    .layout = { .padding = { 6, 6, 2, 2 } },
                    .backgroundColor = failed ? C_RED_BG : C_TBL_HDR,
                    .cornerRadius    = CLAY_CORNER_RADIUS(3),
                }) {
                    CLAY_TEXT(failed ? CLAY_STRING(STR_PLAN_TAG_RETAKE) : CLAY_STRING(STR_PLAN_TAG_NOT_STARTED),
                              TC(failed ? C_RED : C_SUBTEXT, 9));
                }

                /* target-grade chip — only when a reachable/secured target is set */
                if (haveTarget &&
                    (tp.status == TARGET_REACHABLE || tp.status == TARGET_SECURED)) {
                    Clay_String g = sport ? CLAY_STRING(STR_PLAN_TAG_PASS)
                                          : DS("%c%s", tp.need_letter, tp.need_plus ? "+" : "");
                    char gl[2] = { tp.need_letter, 0 };
                    CLAY(CLAY_IDI("PlanGoal", i), {
                        .layout = { .padding = { 6, 6, 2, 2 } },
                        .backgroundColor = C_CARD,
                        .cornerRadius    = CLAY_CORNER_RADIUS(3),
                        .border = { .color = C_BORDER, .width = { .left=1,.right=1,.top=1,.bottom=1 } },
                    }) {
                        CLAY_TEXT(g, TC(sport ? C_GREEN : score_color(gl), 10));
                    }
                }

                /* Sandbox Grade Simulator stepper */
                if (!gIsMobile) {
                    CLAY_TEXT(CLAY_STRING("Simulate:"), TC(C_SUBTEXT, 9));
                }
                RenderSandboxGradeStepper(i, n->ID);
            }
        }
    }
}

/* ═══════════════════════════════════════════════════════════════════════
 *  SETTINGS
 * ═══════════════════════════════════════════════════════════════════════ */

/* Persist the live ui.cfg values (comments preserved by AppConfig_Save),
 * re-apply the frame rate, and confirm with a toast. font_scale needs no
 * extra step — the TC/TCW macros read gFontScale fresh every frame. */
static void UiCfgApplyAndSave(void)
{
    AppConfig_Save("assets/ui.cfg", gApp.font_scale, gApp.target_fps, gApp.theme_id);
    SetTargetFPS(gApp.target_fps);
    Theme_Apply(gApp.theme_id);
    snprintf(gResultMsg, sizeof(gResultMsg), STR_CFG_UI_SAVED);
    ShowToastFor(2.5f);
}

/* Scale a base pixel size by the current font scale so controls stay sized
 * to their text at any font_scale (otherwise fixed boxes clip at 2.0). */
static int Scaled(int px) { return (int)((float)px * gFontScale + 0.5f); }

/* A square +/- button. Returns true on the frame it is clicked. */
static bool CfgStepBtn(int id, Clay_String glyph)
{
    bool clicked = false;
    int sz = Scaled(26);
    CLAY(CLAY_IDI("CfgStepBtn", id), {
        .layout = {
            .sizing         = { CLAY_SIZING_FIXED(sz), CLAY_SIZING_FIXED(sz) },
            .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER },
        },
        .backgroundColor = Clay_Hovered() ? C_ACCENT : C_CARD,
        .cornerRadius    = CLAY_CORNER_RADIUS(6),
        .border          = { .color = C_BORDER,
                             .width = { .left=1,.right=1,.top=1,.bottom=1 } },
    }) {
        if (Clay_Hovered() && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) clicked = true;
        CLAY_TEXT(glyph, TC(Clay_Hovered() ? C_WHITE : C_ACCENT, 15));
    }
    return clicked;
}

/* Label + description on the left, [-] value [+] stepper on the right.
 * Returns -1 / 0 / +1 for the decrement / no-op / increment this frame. */
static int RenderStepperRow(int row, Clay_String label,
                            Clay_String value, Clay_String desc)
{
    int delta = 0;
    CLAY(CLAY_IDI("CfgRow", row), {
        .layout = {
            .sizing          = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
            .childGap        = SP_SM,
            .childAlignment  = { .y = CLAY_ALIGN_Y_CENTER },
            .layoutDirection = CLAY_LEFT_TO_RIGHT,
        },
    }) {
        CLAY(CLAY_IDI("CfgRowLbl", row), {
            .layout = {
                .sizing          = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
                .childGap        = 2,
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
            },
        }) {
            CLAY_TEXT(label, TC(C_TEXT, 13));
            CLAY_TEXT(desc,  TCW(C_SUBTEXT, 9));
        }
        if (CfgStepBtn(row * 2 + 0, CLAY_STRING(STR_STEP_DEC))) delta = -1;
        CLAY(CLAY_IDI("CfgRowVal", row), {
            .layout = {
                .sizing         = { CLAY_SIZING_FIXED(Scaled(96)), CLAY_SIZING_FIXED(Scaled(28)) },
                .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER },
            },
        }) {
            CLAY_TEXT(value, TC(C_ACCENT, 14));
        }
        if (CfgStepBtn(row * 2 + 1, CLAY_STRING(STR_STEP_INC))) delta = +1;
    }
    return delta;
}

/* ── Graduation-rule editor (one card per subject type) ─────────────────── */

static const char *grad_mode_label(int mode)
{
    switch (mode) {
        case GRAD_FIXED:         return STR_GRAD_MODE_FIXED;
        case GRAD_SUBJECT_COUNT: return STR_GRAD_MODE_COUNT;
        default:                 return STR_GRAD_MODE_ALL;
    }
}

/* Persist one rule (RAM already updated by the caller): write the user DB and
 * the cfg file (comments preserved by DB_SaveGradConfig), recompute standings,
 * and confirm. The planner/dashboard read gGradRules directly, so the change
 * is live the moment RAM is updated. */
static void GradRuleApplyAndSave(int t)
{
    DB_UpdateGradRule(t, gGradRules[t].mode, gGradRules[t].limit_val,
                      gGradRules[t].group_id);
    DB_SaveGradConfig("assets/grad_config.cfg");
    RefreshPlayer();
    snprintf(gResultMsg, sizeof(gResultMsg), STR_CFG_GRAD_SAVED);
    ShowToastFor(2.5f);
}



/* A compact +/- stepper without label, aligned horizontally. */
static int RenderCompactStepper(int uid, Clay_String value, int valW)
{
    int delta = 0;
    CLAY(CLAY_IDI("CompStep", uid), {
        .layout = { .sizing          = { CLAY_SIZING_FIT(0), CLAY_SIZING_FIT(0) },
                    .childGap        = SP_XS,
                    .childAlignment  = { .y = CLAY_ALIGN_Y_CENTER },
                    .layoutDirection = CLAY_LEFT_TO_RIGHT },
    }) {
        if (CfgStepBtn(uid * 2, CLAY_STRING(STR_STEP_DEC))) delta = -1;
        CLAY(CLAY_IDI("CompStepVal", uid), {
            .layout = { .sizing         = { CLAY_SIZING_FIXED(Scaled(valW)), CLAY_SIZING_FIXED(Scaled(28)) },
                        .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER } },
            .backgroundColor = C_BG,
            .cornerRadius    = CLAY_CORNER_RADIUS(5),
            .border          = { .color = C_BORDER,
                                 .width = { .left=1,.right=1,.top=1,.bottom=1 } },
        }) { CLAY_TEXT(value, TC(C_ACCENT, 11)); }
        if (CfgStepBtn(uid * 2 + 1, CLAY_STRING(STR_STEP_INC))) delta = +1;
    }
    return delta;
}

/* Render an editable card/table for configured subject types. */
static void RenderGradRuleEditor(void)
{
    /* Find last valid type index to avoid trailing border */
    int lastValidT = 0;
    for (int t = 1; t < sizeSubjectType; t++) {
        if (gTypeName[t][0] != 0) lastValidT = t;
    }

    if (!gIsMobile) {
        /* Desktop layout: Unified responsive table */
        CLAY(CLAY_ID("GradTableContainer"), {
            .layout = {
                .sizing          = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
            },
            .backgroundColor = C_BG,
            .cornerRadius    = CLAY_CORNER_RADIUS(6),
            .border          = { .color = C_BORDER,
                                 .width = { .left=1,.right=1,.top=1,.bottom=1 } },
        }) {
            /* Header Row */
            CLAY(CLAY_ID("GradHeader"), {
                .layout = {
                    .sizing          = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(Scaled(36)) },
                    .layoutDirection = CLAY_LEFT_TO_RIGHT,
                    .childAlignment  = { .y = CLAY_ALIGN_Y_CENTER },
                    .padding         = { Scaled(12), Scaled(12), 0, 0 },
                },
                .backgroundColor = C_TBL_HDR,
                .border          = { .color = C_BORDER, .width = { .bottom = 1 } },
                .cornerRadius    = { .topLeft = 5.0f, .topRight = 5.0f, .bottomLeft = 0.0f, .bottomRight = 0.0f },
            }) {
                CLAY(CLAY_ID("GradHeaderCol1"), {
                    .layout = { .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) } }
                }) {
                    CLAY_TEXT(CLAY_STRING("Subject Type"), TC(C_SUBTEXT, 10));
                }
                CLAY(CLAY_ID("GradHeaderCol2"), {
                    .layout = { .sizing = { CLAY_SIZING_FIXED(Scaled(174)), CLAY_SIZING_FIT(0) },
                                .childAlignment = { .x = CLAY_ALIGN_X_CENTER } }
                }) {
                    CLAY_TEXT(CLAY_STRING(STR_CFG_GRAD_MODE_CAP), TC(C_SUBTEXT, 10));
                }
                CLAY(CLAY_ID("GradHeaderCol3"), {
                    .layout = { .sizing = { CLAY_SIZING_FIXED(Scaled(112)), CLAY_SIZING_FIT(0) },
                                .childAlignment = { .x = CLAY_ALIGN_X_CENTER } }
                }) {
                    CLAY_TEXT(CLAY_STRING(STR_CFG_GRAD_LIMIT_CAP), TC(C_SUBTEXT, 10));
                }
                CLAY(CLAY_ID("GradHeaderCol4"), {
                    .layout = { .sizing = { CLAY_SIZING_FIXED(Scaled(112)), CLAY_SIZING_FIT(0) },
                                .childAlignment = { .x = CLAY_ALIGN_X_CENTER } }
                }) {
                    CLAY_TEXT(CLAY_STRING(STR_CFG_GRAD_GROUP_CAP), TC(C_SUBTEXT, 10));
                }
            }

            int rowIdx = 0;
            for (int t = 1; t < sizeSubjectType; t++) {
                if (gTypeName[t][0] == 0) continue;          /* only types that exist */
                GradRule *r = &gGradRules[t];
                bool isOdd = (rowIdx % 2 != 0);
                rowIdx++;

                CLAY(CLAY_IDI("GradRow", t), {
                    .layout = {
                        .sizing          = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(Scaled(46)) },
                        .layoutDirection = CLAY_LEFT_TO_RIGHT,
                        .childAlignment  = { .y = CLAY_ALIGN_Y_CENTER },
                        .padding         = { Scaled(12), Scaled(12), 0, 0 },
                    },
                    .backgroundColor = Clay_Hovered() ? C_ROW_HOVER : (isOdd ? C_ROW_ODD : C_ROW_EVEN),
                    .border          = { .color = C_BORDER, .width = { .bottom = (t == lastValidT) ? 0 : 1 } },
                }) {
                    /* Column 1: Subject Type badge + name + ID */
                    CLAY(CLAY_IDI("GradCol1", t), {
                        .layout = {
                            .sizing          = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
                            .layoutDirection = CLAY_LEFT_TO_RIGHT,
                            .childAlignment  = { .y = CLAY_ALIGN_Y_CENTER },
                            .childGap        = SP_SM,
                        }
                    }) {
                        CLAY(CLAY_IDI("GradBadge", t), {
                            .layout = { .sizing = { CLAY_SIZING_FIXED(Scaled(6)), CLAY_SIZING_FIXED(Scaled(18)) } },
                            .backgroundColor = kTypeColors[t - 1],
                            .cornerRadius    = CLAY_CORNER_RADIUS(2),
                        }) {}

                        CLAY_TEXT(CS(gTypeName[t]), TC(C_TEXT, 13));
                        CLAY_TEXT(DS(STR_CFG_GRAD_ID_FMT, t), TC(C_SUBTEXT, 9));
                    }

                    /* Column 2: Rule Stepper */
                    CLAY(CLAY_IDI("GradCol2", t), {
                        .layout = {
                            .sizing         = { CLAY_SIZING_FIXED(Scaled(174)), CLAY_SIZING_GROW(0) },
                            .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER },
                        }
                    }) {
                        int dM = RenderCompactStepper(t * 4 + 0, CS(grad_mode_label(r->mode)), 110);
                        if (dM != 0) {
                            r->mode = (r->mode + dM + 3) % 3;
                            GradRuleApplyAndSave(t);
                        }
                    }

                    /* Column 3: Limit Stepper */
                    CLAY(CLAY_IDI("GradCol3", t), {
                        .layout = {
                            .sizing         = { CLAY_SIZING_FIXED(Scaled(112)), CLAY_SIZING_GROW(0) },
                            .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER },
                        }
                    }) {
                        int dL = RenderCompactStepper(t * 4 + 1, DS("%d", r->limit_val), 44);
                        if (dL != 0) {
                            int v = r->limit_val + dL;
                            if (v < 0)   v = 0;
                            if (v > 200) v = 200;
                            r->limit_val = v;
                            GradRuleApplyAndSave(t);
                        }
                    }

                    /* Column 4: Group Stepper */
                    CLAY(CLAY_IDI("GradCol4", t), {
                        .layout = {
                            .sizing         = { CLAY_SIZING_FIXED(Scaled(112)), CLAY_SIZING_GROW(0) },
                            .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER },
                        }
                    }) {
                        int dG = RenderCompactStepper(t * 4 + 2, DS("%d", r->group_id), 44);
                        if (dG != 0) {
                            int v = r->group_id + dG;
                            if (v < 0)                  v = 0;
                            if (v > sizeSubjectType - 1) v = sizeSubjectType - 1;
                            r->group_id = v;
                            GradRuleApplyAndSave(t);
                        }
                    }
                }
            }
        }
    } else {
        /* Mobile Layout: Compact card stack with side-by-side secondary controls */
        int rowIdx = 0;
        for (int t = 1; t < sizeSubjectType; t++) {
            if (gTypeName[t][0] == 0) continue;          /* only types that exist */
            GradRule *r = &gGradRules[t];
            bool isOdd = (rowIdx % 2 != 0);
            rowIdx++;

            CLAY(CLAY_IDI("GradCardM", t), {
                .layout = {
                    .sizing          = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
                    .padding         = { Scaled(12), Scaled(12), Scaled(10), Scaled(10) },
                    .childGap        = SP_SM,
                    .layoutDirection = CLAY_TOP_TO_BOTTOM,
                },
                .backgroundColor = isOdd ? C_ROW_ODD : C_ROW_EVEN,
                .cornerRadius    = CLAY_CORNER_RADIUS(6),
                .border          = { .color = C_BORDER,
                                     .width = { .left=1,.right=1,.top=1,.bottom=1 } },
            }) {
                /* Title row */
                CLAY(CLAY_IDI("GradTitleM", t), {
                    .layout = { .sizing          = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
                                .childAlignment  = { .y = CLAY_ALIGN_Y_CENTER },
                                .childGap        = SP_SM,
                                .layoutDirection = CLAY_LEFT_TO_RIGHT },
                }) {
                    CLAY(CLAY_IDI("GradBadgeM", t), {
                        .layout = { .sizing = { CLAY_SIZING_FIXED(Scaled(6)), CLAY_SIZING_FIXED(Scaled(16)) } },
                        .backgroundColor = kTypeColors[t - 1],
                        .cornerRadius    = CLAY_CORNER_RADIUS(2),
                    }) {}
                    CLAY_TEXT(CS(gTypeName[t]), TC(C_TEXT, 14));
                    CLAY(CLAY_IDI("GradSpM", t), {
                        .layout = { .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(1) } },
                    }) {}
                    CLAY_TEXT(DS(STR_CFG_GRAD_ID_FMT, t), TC(C_SUBTEXT, 9));
                }

                /* Rule Row:   Rule  [- All credits +] */
                CLAY(CLAY_IDI("GradRuleRowM", t), {
                    .layout = { .sizing          = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
                                .childAlignment  = { .y = CLAY_ALIGN_Y_CENTER },
                                .childGap        = SP_SM,
                                .layoutDirection = CLAY_LEFT_TO_RIGHT }
                }) {
                    CLAY(CLAY_IDI("GradRuleLblM", t), {
                        .layout = { .sizing = { CLAY_SIZING_FIXED(Scaled(40)), CLAY_SIZING_FIT(0) } }
                    }) {
                        CLAY_TEXT(CLAY_STRING("Rule"), TC(C_SUBTEXT, 11));
                    }
                    int dM = RenderCompactStepper(t * 4 + 0, CS(grad_mode_label(r->mode)), 110);
                    if (dM != 0) {
                        r->mode = (r->mode + dM + 3) % 3;
                        GradRuleApplyAndSave(t);
                    }
                }

                /* Limit & Group Row side-by-side */
                CLAY(CLAY_IDI("GradValsRowM", t), {
                    .layout = { .sizing          = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
                                .childGap        = SP_MD,
                                .layoutDirection = CLAY_LEFT_TO_RIGHT }
                }) {
                    // Limit
                    CLAY(CLAY_IDI("GradLimColM", t), {
                        .layout = { .sizing          = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
                                    .childAlignment  = { .y = CLAY_ALIGN_Y_CENTER },
                                    .childGap        = SP_SM,
                                    .layoutDirection = CLAY_LEFT_TO_RIGHT }
                    }) {
                        CLAY(CLAY_IDI("GradLimLblM", t), {
                            .layout = { .sizing = { CLAY_SIZING_FIXED(Scaled(40)), CLAY_SIZING_FIT(0) } }
                        }) {
                            CLAY_TEXT(CLAY_STRING("Limit"), TC(C_SUBTEXT, 11));
                        }
                        int dL = RenderCompactStepper(t * 4 + 1, DS("%d", r->limit_val), 44);
                        if (dL != 0) {
                            int v = r->limit_val + dL;
                            if (v < 0)   v = 0;
                            if (v > 200) v = 200;
                            r->limit_val = v;
                            GradRuleApplyAndSave(t);
                        }
                    }

                    // Group
                    CLAY(CLAY_IDI("GradGrpColM", t), {
                        .layout = { .sizing          = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
                                    .childAlignment  = { .y = CLAY_ALIGN_Y_CENTER },
                                    .childGap        = SP_SM,
                                    .layoutDirection = CLAY_LEFT_TO_RIGHT }
                      }) {
                        CLAY(CLAY_IDI("GradGrpLblM", t), {
                            .layout = { .sizing = { CLAY_SIZING_FIXED(Scaled(40)), CLAY_SIZING_FIT(0) } }
                        }) {
                            CLAY_TEXT(CLAY_STRING("Group"), TC(C_SUBTEXT, 11));
                        }
                        int dG = RenderCompactStepper(t * 4 + 2, DS("%d", r->group_id), 44);
                        if (dG != 0) {
                            int v = r->group_id + dG;
                            if (v < 0)                  v = 0;
                            if (v > sizeSubjectType - 1) v = sizeSubjectType - 1;
                            r->group_id = v;
                            GradRuleApplyAndSave(t);
                        }
                    }
                }
            }
        }
    }
}

/* Card shell for one settings group: a heading + path subtitle. The caller
 * fills in the controls inside the block and closes it with a brace. */
#define CFG_CARD_BEGIN(id, title, path)                                       \
    CLAY(CLAY_ID(id), {                                                       \
        .layout = {                                                           \
            .sizing          = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },   \
            .padding         = { 20, 20, 16, 16 },                            \
            .childGap        = SP_MD,                                         \
            .layoutDirection = CLAY_TOP_TO_BOTTOM,                            \
        },                                                                    \
        .backgroundColor = C_CARD,                                            \
        .cornerRadius    = CLAY_CORNER_RADIUS(6),                             \
        .border          = { .color = C_BORDER,                               \
                             .width = { .left=1,.right=1,.top=1,.bottom=1 } },\
    }) {                                                                      \
        CLAY_TEXT(CLAY_STRING(title), TC(C_TEXT, 15));                        \
        CLAY_TEXT(CLAY_STRING(path),  TC(C_SUBTEXT, 9));

void RenderSettings(void)
{
    static const int kFpsPresets[] = { 60, 90, 120, 144, 165, 240 };
    const int nFps = (int)(sizeof(kFpsPresets) / sizeof(kFpsPresets[0]));

    CLAY(CLAY_ID("Settings"), {
        .layout = {
            .sizing          = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
            .padding         = { 24, 24, 20, 20 },
            .childGap        = SP_LG,
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
        },
        .backgroundColor = C_BG,
        .clip = { .vertical = true, .childOffset = Clay_GetScrollOffset() },
    }) {
        /* ── Title ── */
        CLAY(CLAY_ID("SetHdr"), {
            .layout = { .sizing = { CLAY_SIZING_FIT(0), CLAY_SIZING_FIT(0) },
                        .childGap = SP_XS, .layoutDirection = CLAY_TOP_TO_BOTTOM },
        }) {
            CLAY_TEXT(CLAY_STRING(STR_SETTINGS_TITLE), TC(C_TEXT, 26));
            CLAY_TEXT(CLAY_STRING(STR_SETTINGS_SUBTITLE), TC(C_SUBTEXT, 11));
        }

        /* ── Display (ui.cfg) — live, auto-saved controls ── */
        CFG_CARD_BEGIN("SetUiCfg", STR_CFG_UI_TITLE, STR_CFG_UI_PATH)
            /* Text size — font_scale, clamped 1.0 .. 2.5 in 0.1 steps */
            int dFont = RenderStepperRow(0, CLAY_STRING(STR_CFG_UI_FONT_LABEL),
                            DS(STR_CFG_UI_FONT_VAL, gApp.font_scale),
                            CLAY_STRING(STR_CFG_UI_FONT_D));
            if (dFont != 0) {
                float fs = gApp.font_scale + (float)dFont * 0.1f;
                if (fs < 1.0f) fs = 1.0f;
                if (fs > 2.5f) fs = 2.5f;
                gApp.font_scale = fs;
                UiCfgApplyAndSave();
            }

            /* Frame rate — target_fps, stepping through preset values */
            int dFps = RenderStepperRow(1, CLAY_STRING(STR_CFG_UI_FPS_LABEL),
                           DS(STR_CFG_UI_FPS_VAL, gApp.target_fps),
                           CLAY_STRING(STR_CFG_UI_FPS_D));
            if (dFps != 0) {
                int idx = 0;
                for (int i = 0; i < nFps; i++)
                    if (kFpsPresets[i] <= gApp.target_fps) idx = i;
                idx += dFps;
                if (idx < 0) idx = 0;
                if (idx >= nFps) idx = nFps - 1;
                gApp.target_fps = kFpsPresets[idx];
                UiCfgApplyAndSave();
            }

            /* Theme — theme_id, cycling 0..2 */
            static const char *kThemeLabels[] = { "Paper", "Midnight Slate", "Nordic Forest" };
            int dTheme = RenderStepperRow(2, CLAY_STRING("Visual theme"),
                             CS(kThemeLabels[gApp.theme_id]),
                             CLAY_STRING("theme_id - 0 = Paper, 1 = Midnight Slate (Dark), 2 = Nordic Forest."));
            if (dTheme != 0) {
                gApp.theme_id = (gApp.theme_id + dTheme + 3) % 3;
                UiCfgApplyAndSave();
            }
        }

        /* ── Graduation rules (grad_config.cfg) — edit file, then reload ── */
        CFG_CARD_BEGIN("SetGradCfg", STR_CFG_GRAD_TITLE, STR_CFG_GRAD_PATH)
            CLAY_TEXT(CLAY_STRING(STR_CFG_GRAD_BODY), TCW(C_SUBTEXT, 10));
            RenderGradRuleEditor();
            CLAY(CLAY_ID("SetGradReload"), {
                .layout = {
                    .sizing         = { CLAY_SIZING_FIT(0), CLAY_SIZING_FIXED(32) },
                    .padding        = { 16, 16, 0, 0 },
                    .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER },
                },
                .backgroundColor = Clay_Hovered() ? C_ACCENT_DIM : C_CARD,
                .cornerRadius    = CLAY_CORNER_RADIUS(5),
                .border          = { .color = C_BORDER,
                                     .width = { .left=1,.right=1,.top=1,.bottom=1 } },
            }) {
                if (gDBReady && Clay_Hovered() && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
                    int prev_warns = gDataWarnCount;
                    DB_ReloadData();
                    RefreshPlayer();
                    if (gDataWarnCount == 0)
                        snprintf(gResultMsg, sizeof(gResultMsg),
                                 STR_DB_RELOAD_SUCCESS, prev_warns);
                    else
                        snprintf(gResultMsg, sizeof(gResultMsg),
                                 STR_DB_RELOAD_WARN,
                                 gDataWarnCount, gDataWarnCount == 1 ? "" : "s");
                    ShowToastFor(4.f);
                }
                CLAY_TEXT(CLAY_STRING(STR_CFG_GRAD_RELOAD_BTN),
                          TC(Clay_Hovered() ? C_WHITE : C_TEXT, 11));
            }
        }

        /* ── Portal copy-paste import (new feature) ── */
        CFG_CARD_BEGIN("SetImportCfg", "Portal Grades Import", "Import grades from university portal")
            CLAY_TEXT(CLAY_STRING("Copy your grade table from the HUST SIS portal and paste it using the tool to instantly populate your transcript."), TCW(C_SUBTEXT, 10));
            CLAY(CLAY_ID("SetImportOpen"), {
                .layout = {
                    .sizing         = { CLAY_SIZING_FIT(0), CLAY_SIZING_FIXED(32) },
                    .padding        = { 16, 16, 0, 0 },
                    .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER },
                },
                .backgroundColor = Clay_Hovered() ? C_ACCENT_DIM : C_CARD,
                .cornerRadius    = CLAY_CORNER_RADIUS(5),
                .border          = { .color = C_BORDER,
                                     .width = { .left=1,.right=1,.top=1,.bottom=1 } },
            }) {
                if (Clay_Hovered() && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
                    gApp.import_open = true;
                    gApp.import_buf[0] = '\0';
                    gApp.import_len = 0;
                }
                CLAY_TEXT(CLAY_STRING("Open Import Tool"),
                          TC(Clay_Hovered() ? C_WHITE : C_TEXT, 11));
            }
        }

        /* ── Manual-edit warning (footer) ── */
        CLAY(CLAY_ID("SetWarn"), {
            .layout = {
                .sizing          = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
                .padding         = { 16, 16, 14, 14 },
                .childGap        = SP_XS,
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
            },
            .backgroundColor = C_ACCENT_BG,
            .cornerRadius    = CLAY_CORNER_RADIUS(6),
            .border          = { .color = C_ACCENT,
                                 .width = { .left=3,.top=1,.right=1,.bottom=1 } },
        }) {
            CLAY_TEXT(CLAY_STRING(STR_SETTINGS_WARN_TITLE), TC(C_ACCENT, 10));
            CLAY_TEXT(CLAY_STRING(STR_SETTINGS_WARN_BODY),  TCW(C_TEXT, 11));
        }
    }
}

/* ═══════════════════════════════════════════════════════════════════════
 *  ROW EDIT POPUP
 * ═══════════════════════════════════════════════════════════════════════ */

/* ── "Final needed" planner helpers ──────────────────────────────────────
 * Inverts db_compute_letter_r(): given the midterm and the mid/final weight
 * ratio, what final score reaches each grade band?  needed = (thr - rm*mid)/rf.
 * Pass bands only, ordered high → low so the first secured band found is the
 * best one locked in. Band index here matches grade_band_color() 0..7. */
static const struct { const char *name; float threshold; } kPlanBands[8] = {
    { "A+", 9.0f }, { "A", 8.5f }, { "B+", 8.0f }, { "B", 7.0f },
    { "C+", 6.5f }, { "C", 5.5f }, { "D+", 5.0f }, { "D", 4.0f },
};

static void plan_ratio_weights(int ratio, float *rm, float *rf)
{
    switch (ratio) {
        case 1:  *rm = 0.5f; *rf = 0.5f; break;   /* 50 / 50 */
        case 2:  *rm = 0.4f; *rf = 0.6f; break;   /* 40 / 60 */
        default: *rm = 0.3f; *rf = 0.7f; break;   /* 30 / 70 */
    }
}

static const char *kRatioLabels[4] = { "", "50 / 50", "40 / 60", "30 / 70" };

/* ── Shared edit-popup helpers (also called from main.c HandleKeyboard) ──── */

/* Append one keystroke to the active score field, mirroring the keyboard rules:
 * digits and a single '.', max 6 chars. ch == 8 (backspace) deletes one char. */
void EditKeyInput(int ch)
{
    char *buf = (gEditField == 0) ? gEditMidBuf : gEditFinBuf;
    int  *len = (gEditField == 0) ? &gEditMidLen : &gEditFinLen;
    gEditError[0] = '\0';
    if (ch == 8) {                       /* backspace */
        if (*len > 0) buf[--(*len)] = '\0';
        return;
    }
    bool isDot   = (ch == '.');
    bool isDigit = (ch >= '0' && ch <= '9');
    if ((isDigit || isDot) && *len < 6) {
        if (isDot && strchr(buf, '.')) return;   /* only one dot */
        buf[(*len)++] = (char)ch;
        buf[*len]     = '\0';
    }
}

/* Validate + persist the current edit. Returns true on success; on failure
 * leaves the popup open and writes a message into gEditError. */
bool EditTrySave(void)
{
    float mid = (float)atof(gEditMidBuf);
    float fin = (float)atof(gEditFinBuf);
    if (gEditMidLen == 0 || gEditFinLen == 0) {
        snprintf(gEditError, sizeof(gEditError), STR_EDIT_ERR_BOTH_SCORES);
        return false;
    }
    if (mid < 0.f || mid > 10.f || fin < 0.f || fin > 10.f) {
        snprintf(gEditError, sizeof(gEditError), STR_EDIT_ERR_RANGE);
        return false;
    }
    if (!DB_SubjectExists(gEditCode)) {
        snprintf(gEditError, sizeof(gEditError), STR_EDIT_ERR_NOT_FOUND, gEditCode);
        return false;
    }
    DB_UpdateScoreRatio(gEditCode, mid, fin, gEditRatio);
    RefreshPlayer();
    snprintf(gResultMsg, sizeof(gResultMsg),
             STR_EDIT_SUCCESS_FORMAT,
             gEditCode, mid, fin, kRatioLabels[gEditRatio]);
    ShowToastFor(5.f);
    gEditError[0]    = '\0';
    gEditOpen        = false;
    return true;
}

/* On-canvas number pad — taps feed EditKeyInput() into the active field.
 * Always rendered (works with a mouse on desktop, touch on mobile web). */
static void RenderKeypad(float w)
{
    static const char *const keys[12] = {
        "7","8","9", "4","5","6", "1","2","3", "0",".","DEL"
    };
    static const int codes[12] = {
        '7','8','9', '4','5','6', '1','2','3', '0','.', 8 /* backspace */
    };
    float keyW = (w - 12.f) / 3.f;   /* 3 keys per row, 6px gap ×2 */
    CLAY(CLAY_ID("EditKeypad"), {
        .layout = {
            .sizing          = { CLAY_SIZING_FIXED(w), CLAY_SIZING_FIT(0) },
            .childGap        = 6,
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
        },
    }) {
        for (int row = 0; row < 4; row++) {
            CLAY(CLAY_IDI("KpRow", row), {
                .layout = {
                    .sizing          = { CLAY_SIZING_FIXED(w), CLAY_SIZING_FIXED(38) },
                    .childGap        = 6,
                    .layoutDirection = CLAY_LEFT_TO_RIGHT,
                },
            }) {
                for (int col = 0; col < 3; col++) {
                    int k = row * 3 + col;
                    CLAY(CLAY_IDI("KpKey", k), {
                        .layout = {
                            .sizing         = { CLAY_SIZING_FIXED(keyW), CLAY_SIZING_GROW(0) },
                            .childAlignment = { .x = CLAY_ALIGN_X_CENTER,
                                                .y = CLAY_ALIGN_Y_CENTER },
                        },
                        .backgroundColor = Clay_Hovered() ? C_ROW_HOVER : C_TBL_HDR,
                        .cornerRadius    = CLAY_CORNER_RADIUS(6),
                        .border          = { .color = C_BORDER,
                                             .width = { .left=1,.right=1,.top=1,.bottom=1 } },
                    }) {
                        if (Clay_Hovered() && IsMouseButtonReleased(MOUSE_LEFT_BUTTON))
                            EditKeyInput(codes[k]);
                        CLAY_TEXT(CS(keys[k]),
                                  TC(codes[k] == 8 ? C_ACCENT : C_TEXT, 14));
                    }
                }
            }
        }
    }
}

void RenderEditPopup(void)
{
    const char *const *ratio_labels = kRatioLabels;

    /* full-screen backdrop */
    CLAY(CLAY_ID("EditBackdrop"), {
        .layout = {
            .sizing = { CLAY_SIZING_FIXED((float)gScreenW),
                        CLAY_SIZING_FIXED((float)gScreenH) },
        },
        .backgroundColor = (Clay_Color){ 27, 26, 23, 120 },
        .floating = {
            .attachTo = CLAY_ATTACH_TO_ROOT,
            .zIndex   = 15,
        },
    }) {}

    /* popup card — fluid width so it fits narrow / phone screens */
    float editCardW = fminf(520.f, (float)gScreenW - 32.f);
    float innerW    = editCardW - 36.f;   /* body content width (18px padding ×2) */
    CLAY(CLAY_ID("EditCard"), {
        .layout = {
            .sizing          = { CLAY_SIZING_FIXED(editCardW), CLAY_SIZING_FIT(0) },
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

        /* ── Header: title + code (left, wraps) + live grade chip (right) ── */
        CLAY(CLAY_ID("EditHdr"), {
            .layout = {
                .sizing          = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
                .padding         = { 18, 14, 14, 14 },
                .childGap        = 12,
                .childAlignment  = { .y = CLAY_ALIGN_Y_CENTER },
                .layoutDirection = CLAY_LEFT_TO_RIGHT,
            },
            .backgroundColor = C_TBL_HDR,
            .border          = { .color = C_BORDER, .width = { .bottom = 1 } },
        }) {
            CLAY(CLAY_ID("EditHdrText"), {
                .layout = {
                    .sizing          = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
                    .childGap        = 3,
                    .layoutDirection = CLAY_TOP_TO_BOTTOM,
                },
            }) {
                /* wraps within the remaining width → never collides with chip */
                CLAY_TEXT(CS(gEditSubjectName), TCW(C_TEXT, 13));
                CLAY_TEXT(DS(STR_EDIT_CODE_FORMAT, gEditCode),  TC(C_ACCENT, 10));
            }

            /* Live result: grade letter + average + PASS/FAIL as you type */
            float midv = (gEditMidLen > 0) ? (float)atof(gEditMidBuf) : -1.f;
            float finv = (gEditFinLen > 0) ? (float)atof(gEditFinBuf) : -1.f;
            bool  ok   = midv >= 0.f && midv <= 10.f && finv >= 0.f && finv <= 10.f;
            float rmh, rfh;
            plan_ratio_weights(gEditRatio, &rmh, &rfh);
            const char *L = ok ? db_compute_letter_r(midv, finv, rmh, rfh) : "-";
            bool  pass = ok && (L[0] != 'F');
            float avg  = ok ? (rmh * midv + rfh * finv) : 0.f;
            Clay_Color accent = !ok ? C_SUBTEXT : (pass ? C_GREEN : C_RED);

            /* Chip: big grade letter | hairline | AVERAGE value + PASS/FAIL */
            CLAY(CLAY_ID("EditHdrChip"), {
                .layout = {
                    .sizing          = { CLAY_SIZING_FIT(0), CLAY_SIZING_FIT(0) },
                    .padding         = { 14, 14, 10, 10 },
                    .childGap        = 12,
                    .childAlignment  = { .y = CLAY_ALIGN_Y_CENTER },
                    .layoutDirection = CLAY_LEFT_TO_RIGHT,
                },
                .backgroundColor = !ok ? C_BG : (pass ? C_GREEN_BG : C_RED_BG),
                .cornerRadius    = CLAY_CORNER_RADIUS(8),
                .border          = { .color = accent,
                                     .width = { .left=1,.right=1,.top=1,.bottom=1 } },
            }) {
                /* grade letter, dominant */
                CLAY(CLAY_ID("EditHdrGrade"), {
                    .layout = {
                        .sizing         = { CLAY_SIZING_FIT(0), CLAY_SIZING_FIT(0) },
                        .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER },
                    },
                }) {
                    CLAY_TEXT(DS("%s", L), TC(ok ? score_color(L) : C_SUBTEXT, 28));
                }
                /* hairline divider */
                CLAY(CLAY_ID("EditHdrDiv"), {
                    .layout          = { .sizing = { CLAY_SIZING_FIXED(1), CLAY_SIZING_FIXED(34) } },
                    .backgroundColor = accent,
                }) {}
                /* average + pass/fail, stacked */
                CLAY(CLAY_ID("EditHdrMeta"), {
                    .layout = {
                        .sizing          = { CLAY_SIZING_FIT(0), CLAY_SIZING_FIT(0) },
                        .childGap        = 2,
                        .layoutDirection = CLAY_TOP_TO_BOTTOM,
                    },
                }) {
                    CLAY_TEXT(CLAY_STRING(STR_EDIT_LABEL_AVERAGE), TC(C_SUBTEXT, 8));
                    CLAY_TEXT(ok ? DS("%.2f", avg) : CLAY_STRING(STR_PASS_EMPTY),
                              TC(C_TEXT, 15));
                    CLAY_TEXT(ok ? (pass ? CLAY_STRING(STR_EDIT_STATUS_PASS) : CLAY_STRING(STR_EDIT_STATUS_FAIL))
                                 : CLAY_STRING(STR_EDIT_STATUS_ENTER),
                              TC(accent, 10));
                }
            }
        }

        /* ── Body ── */
        CLAY(CLAY_ID("EditBody"), {
            .layout = {
                .sizing          = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
                .padding         = { 18, 18, 14, 14 },
                .childGap        = 12,
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
            },
        }) {

            /* Score input row — deterministic FIXED column widths (split-GROW /
             * PERCENT children collapse in this Clay build, so avoid them) */
            float scoreColW = (innerW - 12.f) / 2.f;
            CLAY(CLAY_ID("EditScoreRow"), {
                .layout = {
                    .sizing          = { CLAY_SIZING_FIXED(innerW), CLAY_SIZING_FIT(0) },
                    .childGap        = 12,
                    .layoutDirection = CLAY_LEFT_TO_RIGHT,
                },
            }) {
                /* Midterm field */
                CLAY(CLAY_ID("EditMidCol"), {
                    .layout = {
                        .sizing          = { CLAY_SIZING_FIXED(scoreColW), CLAY_SIZING_FIT(0) },
                        .childGap        = 6,
                        .layoutDirection = CLAY_TOP_TO_BOTTOM,
                    },
                }) {
                    CLAY_TEXT(CLAY_STRING(STR_EDIT_LABEL_MIDTERM), TC(C_SUBTEXT, 10));
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
                                        : DS("%s ",  gEditMidBuf),
                                      TC(C_TEXT, 13));
                        else
                            CLAY_TEXT(gEditField==0 && cursorOn
                                        ? CLAY_STRING(STR_INPUT_CURSOR) : CLAY_STRING(STR_INPUT_PLACEHOLDER),
                                      TC(C_SUBTEXT, 13));
                        /* click to focus */
                        if (Clay_Hovered() && IsMouseButtonReleased(MOUSE_LEFT_BUTTON))
                            gEditField = 0;
                    }
                }

                /* Final field */
                CLAY(CLAY_ID("EditFinCol"), {
                    .layout = {
                        .sizing          = { CLAY_SIZING_FIXED(scoreColW), CLAY_SIZING_FIT(0) },
                        .childGap        = 6,
                        .layoutDirection = CLAY_TOP_TO_BOTTOM,
                    },
                }) {
                    CLAY_TEXT(CLAY_STRING(STR_EDIT_LABEL_FINAL), TC(C_SUBTEXT, 10));
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
                                        : DS("%s ",  gEditFinBuf),
                                      TC(C_TEXT, 13));
                        else
                            CLAY_TEXT(gEditField==1 && cursorOn
                                        ? CLAY_STRING(STR_INPUT_CURSOR) : CLAY_STRING(STR_INPUT_PLACEHOLDER),
                                      TC(C_SUBTEXT, 13));
                        /* click to focus */
                        if (Clay_Hovered() && IsMouseButtonReleased(MOUSE_LEFT_BUTTON))
                            gEditField = 1;
                    }
                }
            }

            /* Tab hint */
            CLAY_TEXT(CLAY_STRING(STR_EDIT_HINT_KEYS), TC(C_SUBTEXT, 9));

            /* Inline validation message (set by EditTrySave) */
            if (gEditError[0])
                CLAY_TEXT(DS("%s", gEditError), TC(C_RED, 11));

            /* On-canvas number pad */
            RenderKeypad(innerW);

            /* Ratio selector */
            CLAY(CLAY_ID("EditRatioRow"), {
                .layout = {
                    .sizing          = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
                    .childGap        = 8,
                    .childAlignment  = { .y = CLAY_ALIGN_Y_CENTER },
                    .layoutDirection = CLAY_LEFT_TO_RIGHT,
                },
            }) {
                CLAY_TEXT(CLAY_STRING(STR_EDIT_LABEL_RATIO), TC(C_SUBTEXT, 10));
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

            /* ── "What you need on the final" planner ──────────────────── */
            CLAY(CLAY_ID("EditPlan"), {
                .layout = {
                    .sizing          = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
                    .padding         = { 14, 14, 12, 12 },
                    .childGap        = 7,
                    .layoutDirection = CLAY_TOP_TO_BOTTOM,
                },
                .backgroundColor = C_TBL_HDR,
                .cornerRadius    = CLAY_CORNER_RADIUS(8),
                .border          = { .color = C_BORDER,
                                     .width = { .left=1,.right=1,.top=1,.bottom=1 } },
            }) {
                CLAY_TEXT(CLAY_STRING(STR_EDIT_PLAN_TITLE), TC(C_SUBTEXT, 9));

                float midv = (gEditMidLen > 0) ? (float)atof(gEditMidBuf) : -1.f;
                float rm, rf;
                plan_ratio_weights(gEditRatio, &rm, &rf);

                if (midv < 0.f || midv > 10.f) {
                    CLAY_TEXT(CLAY_STRING(STR_EDIT_PLAN_PROMPT),
                              TC(C_SUBTEXT, 10));
                } else if (midv < MIN_PASS_SCORE) {
                    /* mid below the per-component floor → unpassable */
                    CLAY_TEXT(DS(STR_EDIT_PLAN_AUTO_F,
                                 midv, MIN_PASS_SCORE),
                              TC(C_RED, 11));
                    CLAY_TEXT(CLAY_STRING(STR_EDIT_PLAN_UNPASSABLE),
                              TC(C_SUBTEXT, 10));
                } else {
                    int shown = 0;
                    for (int b = 0; b < 8; b++) {
                        float need = (kPlanBands[b].threshold - rm * midv) / rf;
                        /* the final must also clear the per-component floor */
                        if (need < MIN_PASS_SCORE) need = MIN_PASS_SCORE;
                        if (need > 10.f) continue;     /* out of reach */
                        float disp = ceilf(need * 10.f) / 10.f;  /* round up to 0.1 */

                        CLAY(CLAY_IDI("PlanRow", b), {
                            .layout = {
                                .sizing          = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(24) },
                                .childGap        = 10,
                                .childAlignment  = { .y = CLAY_ALIGN_Y_CENTER },
                                .layoutDirection = CLAY_LEFT_TO_RIGHT,
                            },
                        }) {
                            CLAY(CLAY_IDI("PlanChip", b), {
                                .layout = {
                                    .sizing         = { CLAY_SIZING_FIXED(32), CLAY_SIZING_FIXED(18) },
                                    .childAlignment = { .x = CLAY_ALIGN_X_CENTER,
                                                        .y = CLAY_ALIGN_Y_CENTER },
                                },
                                .backgroundColor = grade_band_color(b),
                                .cornerRadius    = CLAY_CORNER_RADIUS(4),
                            }) {
                                CLAY_TEXT(DS("%s", kPlanBands[b].name), TC(C_WHITE, 9));
                            }
                            CLAY_TEXT(DS(STR_EDIT_PLAN_NEED_FORMAT, disp), TC(C_TEXT, 11));
                        }
                        shown++;
                    }

                    if (shown == 0)
                        CLAY_TEXT(CLAY_STRING(STR_EDIT_PLAN_IMPOSSIBLE),
                                  TC(C_SUBTEXT, 10));
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

            /* Save — primary (filled green) */
            CLAY(CLAY_ID("EditSave"), {
                .layout = {
                    .sizing         = { CLAY_SIZING_FIT(0), CLAY_SIZING_FIXED(34) },
                    .padding        = { 18, 18, 6, 6 },
                    .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER },
                },
                .backgroundColor = Clay_Hovered() ? (Clay_Color){38,98,61,255} : C_GREEN,
                .cornerRadius    = CLAY_CORNER_RADIUS(4),
                .border          = { .color = C_GREEN,
                                     .width = { .left=1,.right=1,.top=1,.bottom=1 } },
            }) {
                if (Clay_Hovered() && IsMouseButtonReleased(MOUSE_LEFT_BUTTON))
                    EditTrySave();
                CLAY_TEXT(CLAY_STRING(STR_EDIT_SAVE_BTN), TC(C_WHITE, 11));
            }

            /* spacer pushes the secondary/destructive actions to the right */
            CLAY(CLAY_ID("EditBtnSp"), {
                .layout = { .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(1) } },
            }) {}

            /* Reset — destructive, de-emphasized (ghost outline) */
            CLAY(CLAY_ID("EditReset"), {
                .layout = {
                    .sizing         = { CLAY_SIZING_FIT(0), CLAY_SIZING_FIXED(34) },
                    .padding        = { 16, 16, 6, 6 },
                    .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER },
                },
                .backgroundColor = Clay_Hovered() ? C_RED_BG : C_TRANS,
                .cornerRadius    = CLAY_CORNER_RADIUS(4),
                .border          = { .color = C_RED,
                                     .width = { .left=1,.right=1,.top=1,.bottom=1 } },
            }) {
                if (Clay_Hovered() && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
                    DB_ClearScore(gEditCode);
                    RefreshPlayer();
                    snprintf(gResultMsg, sizeof(gResultMsg),
                             STR_EDIT_RESET_SUCCESS, gEditCode);
                    ShowToastFor(5.f);
                    gEditError[0]    = '\0';
                    gEditOpen = false;
                }
                CLAY_TEXT(CLAY_STRING(STR_EDIT_RESET_BTN), TC(C_RED, 11));
            }

            /* Cancel — neutral */
            CLAY(CLAY_ID("EditCancel"), {
                .layout = {
                    .sizing         = { CLAY_SIZING_FIT(0), CLAY_SIZING_FIXED(34) },
                    .padding        = { 18, 18, 6, 6 },
                    .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER },
                },
                .backgroundColor = Clay_Hovered() ? C_ROW_HOVER : C_BORDER,
                .cornerRadius    = CLAY_CORNER_RADIUS(6),
            }) {
                if (Clay_Hovered() && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
                    gEditError[0] = '\0';
                    gEditOpen = false;
                }
                CLAY_TEXT(CLAY_STRING(STR_EDIT_CANCEL_BTN), TC(C_TEXT, 11));
            }
        }
    }
}

/* ═══════════════════════════════════════════════════════════════════════
 *  NAME INPUT SCREEN  (shown on startup before DB is opened)
 * ═══════════════════════════════════════════════════════════════════════ */

void RenderNameInput(void)
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
            CLAY_TEXT(CLAY_STRING(STR_NAME_INPUT_TITLE), TC(C_TEXT, 15));
            CLAY_TEXT(CLAY_STRING(STR_NAME_INPUT_SUBTITLE),
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
            CLAY_TEXT(CLAY_STRING(STR_NAME_INPUT_LABEL), TC(C_SUBTEXT, 10));
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
                    CLAY_TEXT(cursorOn ? CLAY_STRING(STR_INPUT_CURSOR) : CLAY_STRING(STR_INPUT_BLANK),
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
                    CLAY_TEXT(DS(STR_NAME_INPUT_DB_FOUND, gUserName),
                              TC(C_GREEN, 10));
                else
                    CLAY_TEXT(DS(STR_NAME_INPUT_DB_NEW, gUserName),
                              TC(C_ACCENT, 10));
            } else {
                CLAY_TEXT(CLAY_STRING(STR_NAME_INPUT_PROMPT),
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
            CLAY_TEXT(CLAY_STRING(STR_NAME_INPUT_ENTER_CFM), TC(C_SUBTEXT, 10));
            CLAY_TEXT(CLAY_STRING(STR_NAME_INPUT_ESC_QUIT),      TC(C_SUBTEXT, 10));
        }
    }
}

/* ═══════════════════════════════════════════════════════════════════════
 *  COMMAND PALETTE OVERLAY
 * ═══════════════════════════════════════════════════════════════════════ */

void RenderCommandPopup(void)
{
    /* full-screen backdrop */
    CLAY(CLAY_ID("PopupBackdrop"), {
        .layout = {
            .sizing = { CLAY_SIZING_FIXED((float)gScreenW),
                        CLAY_SIZING_FIXED((float)gScreenH) },
        },
        .backgroundColor = (Clay_Color){ 27, 26, 23, 110 },
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
            }) { CLAY_TEXT(CLAY_STRING(STR_CMD_BADGE_K), TC(C_WHITE, 10)); }
            CLAY_TEXT(CLAY_STRING(STR_CMD_TITLE), TC(C_TEXT, 12));
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
            }) { CLAY_TEXT(CLAY_STRING(STR_CTRL_K_HINT), TC(C_SUBTEXT, 10)); }
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
            CLAY_TEXT(CLAY_STRING(STR_CMD_PROMPT_ICON), TC(C_ACCENT, 17));
            if (gCmdLen > 0) {
                bool cursorOn = ((int)(GetTime() * 2) % 2) == 0;
                CLAY_TEXT(cursorOn ? DS("%s|", gCmdBuf) : DS("%s ", gCmdBuf),
                          TC(C_TEXT, 14));
            } else {
                CLAY_TEXT(CLAY_STRING(STR_CMD_PROMPT),
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
            CLAY_TEXT(CLAY_STRING(STR_CMD_HINT_ENTER),    TC(C_SUBTEXT, 10));
            CLAY_TEXT(CLAY_STRING(STR_CMD_HINT_BACKSPACE), TC(C_SUBTEXT, 10));
            CLAY_TEXT(CLAY_STRING(STR_CMD_HINT_ESC), TC(C_SUBTEXT, 10));
        }
    }
}

/* ═══════════════════════════════════════════════════════════════════════
 *  RESULT TOAST
 * ═══════════════════════════════════════════════════════════════════════ */

void RenderResultToast(void)
{
    float remaining = gResultShowUntil - (float)GetTime();
    if (remaining <= 0.f) return;

    uint8_t alpha = (remaining < 1.f) ? (uint8_t)(remaining * 255.f) : 255;

    Clay_Color toastBg  = (Clay_Color){250, 248, 243, alpha };
    Clay_Color toastBdr = (Clay_Color){140,  47,  42, alpha };
    Clay_Color textClr  = (Clay_Color){ 27,  26,  23, alpha };

    CLAY(CLAY_ID("ResultToast"), {
        .layout = {
            .sizing          = { CLAY_SIZING_FIT(0), CLAY_SIZING_FIXED(44) },
            .padding         = { 18, 18, 0, 0 },
            .childGap        = 10,
            .childAlignment  = { .y = CLAY_ALIGN_Y_CENTER },
            .layoutDirection = CLAY_LEFT_TO_RIGHT,
        },
        .backgroundColor = toastBg,
        .cornerRadius    = CLAY_CORNER_RADIUS(4),
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
        CLAY_TEXT(CLAY_STRING(STR_TOAST_ARROW),    TC(toastBdr, 12));
        CLAY_TEXT(DS("%s", gResultMsg), TC(textClr,  13));
    }
}

/* ═══════════════════════════════════════════════════════════════════════
 *  PORTAL IMPORT POPUP
 * ═══════════════════════════════════════════════════════════════════════ */

static int ImportPortalGrades(const char *text)
{
    if (!text || strlen(text) == 0) return 0;
    
    int imported_count = 0;
    char *copy = strdup(text);
    if (!copy) return 0;
    
    char *line = copy;
    char *line_saveptr = NULL;
    char *line_token = strtok_r(line, "\n\r", &line_saveptr);
    
    db_exec("BEGIN;");
    while (line_token) {
        char *line_copy = strdup(line_token);
        if (line_copy) {
            char *word_saveptr = NULL;
            char *word = strtok_r(line_copy, " \t", &word_saveptr);
            
            char code[32] = {0};
            bool found_code = false;
            
            float vals[16];
            int n_vals = 0;
            
            while (word) {
                int wl = (int)strlen(word);
                while (wl > 0 && (word[wl-1] == ',' || word[wl-1] == ';' || word[wl-1] == ':')) {
                    word[--wl] = '\0';
                }
                
                if (!found_code && wl >= 5 && wl <= 10) {
                    int alpha_count = 0;
                    int digit_count = 0;
                    for (int i = 0; i < wl; i++) {
                        if (word[i] >= 'A' && word[i] <= 'Z') alpha_count++;
                        else if (word[i] >= '0' && word[i] <= '9') digit_count++;
                    }
                    if (alpha_count >= 2 && digit_count >= 3) {
                        strncpy(code, word, sizeof(code) - 1);
                        for (int i = 0; code[i]; i++) {
                            if (code[i] >= 'a' && code[i] <= 'z') code[i] -= 32;
                        }
                        if (DB_SubjectExists(code)) {
                            found_code = true;
                        }
                    }
                } else if (found_code) {
                    char *endptr = NULL;
                    float v = (float)strtod(word, &endptr);
                    if (endptr != word && *endptr == '\0') {
                        if (n_vals < 16) {
                            vals[n_vals++] = v;
                        }
                    }
                }
                word = strtok_r(NULL, " \t", &word_saveptr);
            }
            
            if (found_code && n_vals > 0) {
                int credits = DB_GetSubjectCredits(code);
                int score_start_idx = 0;
                
                if (score_start_idx < n_vals && (int)vals[score_start_idx] == credits) {
                    score_start_idx++;
                }
                
                if (score_start_idx < n_vals && vals[score_start_idx] > 100.f) {
                    score_start_idx++;
                }
                
                float mid = -1.f;
                float fin = -1.f;
                
                if (score_start_idx < n_vals) {
                    float v = vals[score_start_idx++];
                    if (v >= 0.f && v <= 10.f) mid = v;
                }
                if (score_start_idx < n_vals) {
                    float v = vals[score_start_idx++];
                    if (v >= 0.f && v <= 10.f) fin = v;
                }
                
                if (mid >= 0.f || fin >= 0.f) {
                    if (mid < 0.f) mid = 0.f;
                    if (fin < 0.f) fin = 0.f;
                    
                    DB_UpdateScore(code, mid, fin);
                    imported_count++;
                }
            }
            free(line_copy);
        }
        line_token = strtok_r(NULL, "\n\r", &line_saveptr);
    }
    db_exec("COMMIT;");
    free(copy);
    return imported_count;
}

void RenderImportPopup(void)
{
    /* full-screen backdrop */
    CLAY(CLAY_ID("ImportBackdrop"), {
        .layout = {
            .sizing = { CLAY_SIZING_FIXED((float)gScreenW),
                        CLAY_SIZING_FIXED((float)gScreenH) },
        },
        .backgroundColor = (Clay_Color){ 27, 26, 23, 110 },
        .floating = {
            .attachTo = CLAY_ATTACH_TO_ROOT,
            .zIndex   = 5,
        },
    }) {}

    /* popup card */
    CLAY(CLAY_ID("ImportCard"), {
        .layout = {
            .sizing          = { CLAY_SIZING_FIXED(580), CLAY_SIZING_FIT(0) },
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
            .childGap        = SP_MD,
            .padding         = { 20, 20, 20, 20 },
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
        /* Title */
        CLAY(CLAY_ID("ImpTitleRow"), {
            .layout = { .sizing          = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
                        .layoutDirection = CLAY_LEFT_TO_RIGHT,
                        .childAlignment  = { .y = CLAY_ALIGN_Y_CENTER },
                        .childGap        = SP_SM },
        }) {
            CLAY(CLAY_ID("ImpBadge"), {
                .layout = { .sizing = { CLAY_SIZING_FIXED(6), CLAY_SIZING_FIXED(18) } },
                .backgroundColor = C_ACCENT,
                .cornerRadius = CLAY_CORNER_RADIUS(2)
            }) {}
            CLAY_TEXT(CLAY_STRING("Portal Grades Import"), TC(C_TEXT, 16));
        }

        CLAY_TEXT(CLAY_STRING("Copy your grades table from HUST SIS or any other portal, then click \"Paste from Clipboard\" to parse and update your scores. (Ctrl+V works as a shortcut)"), TCW(C_SUBTEXT, 10));

        /* Preview box */
        CLAY(CLAY_ID("ImpPreviewContainer"), {
            .layout = {
                .sizing          = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(140) },
                .padding         = { 12, 12, 10, 10 },
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
            },
            .backgroundColor = C_BG,
            .cornerRadius    = CLAY_CORNER_RADIUS(6),
            .border          = { .color = C_BORDER, .width = { .left=1,.right=1,.top=1,.bottom=1 } }
        }) {
            if (gApp.import_len > 0) {
                char preview[120];
                int preview_len = gApp.import_len;
                if (preview_len > 100) preview_len = 100;
                strncpy(preview, gApp.import_buf, preview_len);
                preview[preview_len] = '\0';
                if (gApp.import_len > 100) strcat(preview, "...");

                CLAY_TEXT(CS(preview), TCW(C_TEXT, 11));
            } else {
                CLAY_TEXT(CLAY_STRING("No text pasted yet. Please copy from portal and click Paste below."), TCW(C_SUBTEXT, 11));
            }
        }

        /* Buttons Row */
        CLAY(CLAY_ID("ImpBtnRow"), {
            .layout = {
                .sizing          = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
                .childGap        = SP_MD,
                .layoutDirection = CLAY_LEFT_TO_RIGHT,
                .childAlignment  = { .x = CLAY_ALIGN_X_RIGHT }
            }
        }) {
            /* Paste Button */
            CLAY(CLAY_ID("ImpBtnPaste"), {
                .layout = {
                    .sizing         = { CLAY_SIZING_FIT(0), CLAY_SIZING_FIXED(32) },
                    .padding        = { 14, 14, 0, 0 },
                    .childAlignment = { .y = CLAY_ALIGN_Y_CENTER },
                },
                .backgroundColor = Clay_Hovered() ? C_ACCENT_DIM : C_ACCENT,
                .cornerRadius    = CLAY_CORNER_RADIUS(6),
            }) {
                if (Clay_Hovered() && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
                    const char *clipText = GetClipboardText();
                    if (clipText) {
                        strncpy(gApp.import_buf, clipText, sizeof(gApp.import_buf) - 1);
                        gApp.import_buf[sizeof(gApp.import_buf) - 1] = '\0';
                        gApp.import_len = (int)strlen(gApp.import_buf);
                    }
                }
                CLAY_TEXT(CLAY_STRING("Paste from Clipboard"), TC(C_WHITE, 11));
            }

            /* Import button */
            bool can_import = gApp.import_len > 0;
            CLAY(CLAY_ID("ImpBtnApply"), {
                .layout = {
                    .sizing         = { CLAY_SIZING_FIT(0), CLAY_SIZING_FIXED(32) },
                    .padding        = { 14, 14, 0, 0 },
                    .childAlignment = { .y = CLAY_ALIGN_Y_CENTER },
                },
                .backgroundColor = !can_import ? C_BORDER : (Clay_Hovered() ? C_GREEN_BG : C_CARD),
                .cornerRadius    = CLAY_CORNER_RADIUS(6),
                .border          = { .color = can_import ? C_GREEN : C_BORDER, .width = { .left=1,.right=1,.top=1,.bottom=1 } }
            }) {
                if (can_import && Clay_Hovered() && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
                    int count = ImportPortalGrades(gApp.import_buf);
                    snprintf(gResultMsg, sizeof(gResultMsg), "Successfully imported %d grades!", count);
                    ShowToastFor(5.f);
                    gApp.import_open = false;
                    RefreshPlayer();
                }
                CLAY_TEXT(CLAY_STRING("Import & Apply"), TC(!can_import ? C_SUBTEXT : C_GREEN, 11));
            }

            /* Cancel button */
            CLAY(CLAY_ID("ImpBtnCancel"), {
                .layout = {
                    .sizing         = { CLAY_SIZING_FIT(0), CLAY_SIZING_FIXED(32) },
                    .padding        = { 14, 14, 0, 0 },
                    .childAlignment = { .y = CLAY_ALIGN_Y_CENTER },
                },
                .backgroundColor = Clay_Hovered() ? C_ROW_HOVER : C_CARD,
                .cornerRadius    = CLAY_CORNER_RADIUS(6),
                .border          = { .color = C_BORDER, .width = { .left=1,.right=1,.top=1,.bottom=1 } }
            }) {
                if (Clay_Hovered() && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
                    gApp.import_open = false;
                }
                CLAY_TEXT(CLAY_STRING("Cancel"), TC(C_TEXT, 11));
            }
        }
    }
}
