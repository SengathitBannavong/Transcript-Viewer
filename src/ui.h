#pragma once

#include "clay.h"

/* ─── Layout constants ───────────────────────────────────────────────── */
#define SIDEBAR_W   248
#define ROW_H        44
#define HDR_H        42

#define BP_MOBILE   900
#define NAV_PLANNER (sizeSubjectType)

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

/* Colors — Academic "paper" theme */
#define C_BG         ((Clay_Color){241, 237, 229, 255})  /* warm paper page   */
#define C_SIDEBAR    ((Clay_Color){234, 229, 219, 255})  /* deeper paper edge */
#define C_CARD       ((Clay_Color){250, 248, 243, 255})  /* raised panel      */
#define C_TBL_HDR    ((Clay_Color){236, 231, 221, 255})  /* header band       */
#define C_ROW_ODD    ((Clay_Color){247, 244, 238, 255})  /* ledger stripe     */
#define C_ROW_EVEN   ((Clay_Color){252, 250, 246, 255})
#define C_ROW_HOVER  ((Clay_Color){244, 230, 227, 255})  /* pale crimson wash */
#define C_BORDER     ((Clay_Color){221, 214, 201, 255})  /* hairline rule     */
#define C_ACCENT     ((Clay_Color){140,  47,  42, 255})  /* oxblood crimson   */
#define C_ACCENT_DIM ((Clay_Color){176,  92,  86, 255})  /* faded crimson     */
#define C_ACCENT_BG  ((Clay_Color){243, 228, 225, 255})  /* pale crimson wash */
#define C_TEXT       ((Clay_Color){ 27,  26,  23, 255})  /* ink               */
#define C_SUBTEXT    ((Clay_Color){122, 116, 104, 255})  /* muted ink         */
#define C_WHITE      ((Clay_Color){250, 248, 244, 255})  /* paper-white (on accent) */
#define C_GREEN      ((Clay_Color){ 47, 120,  75, 255})  /* forest (pass)     */
#define C_GREEN_BG   ((Clay_Color){222, 236, 224, 255})  /* pale sage         */
#define C_RED        ((Clay_Color){178,  52,  42, 255})  /* brick (fail)      */
#define C_RED_BG     ((Clay_Color){246, 225, 221, 255})  /* pale rose         */
#define C_YELLOW     ((Clay_Color){166, 124,  36, 255})  /* ochre             */
#define C_YELLOW_BG  ((Clay_Color){245, 236, 212, 255})  /* pale cream        */
#define C_TRANS      ((Clay_Color){  0,   0,   0,   0})

extern const char *kGradeLabels[9];

/* UI render function prototypes */
void RenderTopBar(void);
void RenderSidebar(void);
void RenderDashboard(void);
void RenderPlanner(void);
void RenderMainContent(void);
void RenderNameInput(void);
void RenderDrawer(void);
void RenderEditPopup(void);
void RenderCommandPopup(void);
void RenderResultToast(void);

void calc_grade_counts(int out[9]);
bool EditTrySave(void);
void EditKeyInput(int ch);
