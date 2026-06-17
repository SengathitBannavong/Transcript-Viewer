#pragma once

#include "clay.h"

/* ─── Layout constants ───────────────────────────────────────────────── */
#define SIDEBAR_W   248
#define ROW_H        44
#define HDR_H        42

#define BP_MOBILE   900
#define NAV_PLANNER (sizeSubjectType)
#define NAV_SETTINGS (sizeSubjectType + 1)

/* Spacing scale */
#define SP_XS   4
#define SP_SM   8
#define SP_MD  14
#define SP_LG  22
#define SP_XL  32

/* Element IDs for canvas drawing */
#define DONUT_GRADE_ID  "DashDonutGrade"
#define DONUT_CPA_ID    "DashDonutCPA"
#define RADAR_GRADE_ID  "DashRadarGrade"

/* Themes */
typedef struct {
    Clay_Color bg;
    Clay_Color sidebar;
    Clay_Color card;
    Clay_Color tbl_hdr;
    Clay_Color row_odd;
    Clay_Color row_even;
    Clay_Color row_hover;
    Clay_Color border;
    Clay_Color accent;
    Clay_Color accent_dim;
    Clay_Color accent_bg;
    Clay_Color text;
    Clay_Color subtext;
    Clay_Color white;
    Clay_Color green;
    Clay_Color green_bg;
    Clay_Color red;
    Clay_Color red_bg;
    Clay_Color yellow;
    Clay_Color yellow_bg;
} AppTheme;

extern AppTheme gTheme;
void Theme_Apply(int theme_id);

#define C_BG         (gTheme.bg)
#define C_SIDEBAR    (gTheme.sidebar)
#define C_CARD       (gTheme.card)
#define C_TBL_HDR    (gTheme.tbl_hdr)
#define C_ROW_ODD    (gTheme.row_odd)
#define C_ROW_EVEN   (gTheme.row_even)
#define C_ROW_HOVER  (gTheme.row_hover)
#define C_BORDER     (gTheme.border)
#define C_ACCENT     (gTheme.accent)
#define C_ACCENT_DIM (gTheme.accent_dim)
#define C_ACCENT_BG  (gTheme.accent_bg)
#define C_TEXT       (gTheme.text)
#define C_SUBTEXT    (gTheme.subtext)
#define C_WHITE      (gTheme.white)
#define C_GREEN      (gTheme.green)
#define C_GREEN_BG   (gTheme.green_bg)
#define C_RED        (gTheme.red)
#define C_RED_BG     (gTheme.red_bg)
#define C_YELLOW     (gTheme.yellow)
#define C_YELLOW_BG  (gTheme.yellow_bg)
#define C_TRANS      ((Clay_Color){  0,   0,   0,   0})

extern const char *kGradeLabels[9];

/* UI render function prototypes */
void RenderTopBar(void);
void RenderSidebar(void);
void RenderDashboard(void);
void RenderPlanner(void);
void RenderSettings(void);
void RenderMainContent(void);
void RenderNameInput(void);
void RenderDrawer(void);
void RenderEditPopup(void);
void RenderCommandPopup(void);
void RenderImportPopup(void);
void RenderResultToast(void);

void calc_grade_counts(int out[9]);
bool EditTrySave(void);
void EditKeyInput(int ch);
