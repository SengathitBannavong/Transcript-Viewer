/*
 * main.c -- Student Transcript Viewer (entry point + orchestration)
 *
 * Build:   make
 *
 * File layout
 * -----------
 *   Struct_Table.h  -- Player / Subject_Node / Subject_Type structs
 *   app_data.h      -- gPlayer global + gTypeName[]
 *   db.h            -- SQLite backend (DB_Open/Close/Query/Update)
 *   cmd.h           -- command palette dispatcher
 *   ui.c            -- all Clay rendering functions
 *   main.c          -- globals, keyboard handler, layout root, main()
 *
 * ui.c is NOT compiled separately; it is #include'd here after all
 * globals are defined so it can reference them directly.
 */

#define CLAY_IMPLEMENTATION
#include "clay.h"
#include "raylib.h"
#include "clay_renderer_raylib.c"   /* Raylib rendering backend  */

#include "app_data.h"               /* gPlayer + gTypeName                */
#include "score_logic.h"            /* CPA / graduation / alert logic     */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>
#include <ctype.h>

/* --- Window constants ------------------------------------------------- */
#define WIN_W  1400
#define WIN_H   820

/* --- Globals (read/written by ui.c) ----------------------------------- */
static Font gFonts[1];
static bool gCustomFont = false;

/* active sidebar item: index into gPlayer.numofSubjectType */
static int  gActiveNav  = 1;

/* current window size; updated every frame for correct backdrop sizing */
static int  gScreenW    = WIN_W;
static int  gScreenH    = WIN_H;

/* per-frame dynamic string arena */
#define DYN_BUF_SIZE 262144
static char gDynBuf[DYN_BUF_SIZE];
static int  gDynPos;

/* command palette state */
static bool  gPopupOpen       = false;
static char  gCmdBuf[256]     = {0};  /* text being typed            */
static int   gCmdLen          = 0;

/* settings popup state */
static bool  gSettingsOpen = false;
static int   gSettingsEditTarget = 0; /* 0=ui.cfg 1=grad_config.cfg 2=subjects.dat */
static int   gSettingsField = 0; /* input focus index, per active target */

/* ui.cfg form state */
static char  gUiScaleBuf[32] = {0};
static int   gUiScaleLen = 0;
static char  gUiFpsBuf[32] = {0};
static int   gUiFpsLen = 0;

/* grad_config.cfg table state */
typedef struct {
    int type_id;
    int mode;
    int limit;
    int group_id;
    int raw_line_index;
    char inline_comment[256];
} GradRow;
static GradRow gGradRows[64];
static int     gGradRowCount = 0;
static int     gGradSelected = -1;
static char    gGradRawLines[256][512];
static int     gGradRawCount = 0;
static int     gGradRawRowForLine[256];
static char    gGradModeBuf[16] = {0};
static int     gGradModeLen = 0;
static char    gGradLimitBuf[16] = {0};
static int     gGradLimitLen = 0;
static char    gGradGroupBuf[16] = {0};
static int     gGradGroupLen = 0;

/* subjects.dat table state */
typedef struct {
    int  type_id;
    char type_name[64];
    char code[MAXSIZEID];
    int  term;
    int  credit;
    char name[MAXSIZENAME];
} SubjectRow;
static SubjectRow gSubjectRows[1024];
static int        gSubjectRowCount = 0;
static int        gSubjectSelected = -1;
static int        gSubjectSection = 1;
static char       gSubCodeBuf[MAXSIZEID] = {0};
static int        gSubCodeLen = 0;
static char       gSubTermBuf[16] = {0};
static int        gSubTermLen = 0;
static char       gSubCreditBuf[16] = {0};
static int        gSubCreditLen = 0;
static char       gSubNameBuf[MAXSIZENAME] = {0};
static int        gSubNameLen = 0;

/* row-edit popup state */
static bool  gEditOpen        = false;
static char  gEditCode[MAXSIZEID] = {0};  /* subject code being edited     */
static char  gEditMidBuf[32]  = {0};
static int   gEditMidLen      = 0;
static char  gEditFinBuf[32]  = {0};
static int   gEditFinLen      = 0;
static int   gEditField       = 0;    /* 0 = mid, 1 = final            */
static int   gEditRatio       = 3;    /* 1=50/50  2=40/60  3=30/70     */
static char  gEditSubjectName[MAXSIZENAME] = {0};
static bool  gHasResult       = false;
static float gResultShowUntil = -1.f; /* GetTime() expiry for toast  */

/* command execution results (written by ExecuteCommand in cmd.h) */
static char  gFilterDept[64]  = {0};  /* kept for compat; not used in student view */
static char  gResultMsg[256]  = {0};  /* message shown in toast                    */

/* subject-table filter state */
static char  gTableFilterQuery[128] = {0};
static int   gTableFilterLen = 0;
static int   gTableFilterMode = 0; /* 0=all 1=pass 2=fail 3=no-score */
static bool  gFilterInputFocus = false;

/* runtime asset paths (configurable from settings UI) */
static char  gAssetsDir[256] = "assets";
static char  gSubjectsDatPath[512] = "assets/subjects.dat";
static char  gUiCfgPath[512] = "assets/ui.cfg";
static char  gFontsCfgPath[512] = "assets/fonts.cfg";
static char  gGradCfgPath[512] = "assets/grad_config.cfg";
static const char *kAssetPathConfigFile = "assets_path.cfg";

/* forward declarations for runtime UI config values */
static float gFontScale;
static int   gTargetFPS;

static const char *settings_target_path(int t)
{
    if (t == 0) return gUiCfgPath;
    if (t == 1) return gGradCfgPath;
    return gSubjectsDatPath;
}

static const char *type_name_for_id(int tid)
{
    static const char *fallback[sizeSubjectType] = {
        "unused", "Co So Nganh", "Dai Cuong", "The Thao", "Ly Luan Chinh Tri",
        "Tu Chon", "Modul I", "Modul II", "Modul III", "Modul IV", "Modul V",
        "Modul VI", "Thuc Tap", "Do An Tot Nghiep"
    };
    if (tid > 0 && tid < sizeSubjectType) {
        if (gTypeName[tid][0] != '\0') return gTypeName[tid];
        return fallback[tid];
    }
    return "Unknown";
}

static void fill_from_int(char *buf, int bufsz, int *len, int value)
{
    snprintf(buf, bufsz, "%d", value);
    *len = (int)strlen(buf);
}

static void settings_load_ui_cfg(void)
{
    float scale = gFontScale;
    int fps = gTargetFPS;
    FILE *f = fopen(gUiCfgPath, "r");
    if (f) {
        char line[256];
        while (fgets(line, sizeof(line), f)) {
            if (line[0] == '#' || line[0] == '\n' || line[0] == '\r') continue;
            char key[64];
            float val;
            if (sscanf(line, "%63s %f", key, &val) == 2) {
                if (strcmp(key, "font_scale") == 0) scale = val;
                if (strcmp(key, "target_fps") == 0) fps = (int)val;
            }
        }
        fclose(f);
    }
    snprintf(gUiScaleBuf, sizeof(gUiScaleBuf), "%.2f", scale);
    gUiScaleLen = (int)strlen(gUiScaleBuf);
    fill_from_int(gUiFpsBuf, sizeof(gUiFpsBuf), &gUiFpsLen, fps);
}

static int settings_save_ui_cfg(char *out_msg, int out_msg_size)
{
    float scale = (float)atof(gUiScaleBuf);
    int fps = atoi(gUiFpsBuf);
    if (scale < 0.1f || scale > 10.f) {
        snprintf(out_msg, out_msg_size, "ui.cfg: font_scale must be 0.1..10.0");
        return 0;
    }
    if (fps < 60 || fps > 240) {
        snprintf(out_msg, out_msg_size, "ui.cfg: target_fps must be 60..240");
        return 0;
    }

    FILE *f = fopen(gUiCfgPath, "w");
    if (!f) {
        snprintf(out_msg, out_msg_size, "ui.cfg: cannot write file");
        return 0;
    }
    fprintf(f,
            "# ui.cfg - UI display settings for Transcript Viewer\n"
            "# Changes take effect immediately for scale/fps.\n\n"
            "font_scale %.2f\n"
            "target_fps %d\n",
            scale, fps);
    fclose(f);

    gFontScale = scale;
    gTargetFPS = fps;
    SetTargetFPS(gTargetFPS);
    snprintf(out_msg, out_msg_size, "ui.cfg saved");
    return 1;
}

static void settings_load_grad_cfg(void)
{
    gGradRowCount = 0;
    gGradSelected = -1;
    gGradRawCount = 0;
    for (int i = 0; i < (int)(sizeof(gGradRawRowForLine)/sizeof(gGradRawRowForLine[0])); i++)
        gGradRawRowForLine[i] = -1;

    FILE *f = fopen(gGradCfgPath, "r");
    if (!f) return;
    char line[512];
    while (fgets(line, sizeof(line), f)) {
        int raw_idx = -1;
        if (gGradRawCount < (int)(sizeof(gGradRawLines)/sizeof(gGradRawLines[0]))) {
            raw_idx = gGradRawCount;
            snprintf(gGradRawLines[gGradRawCount], sizeof(gGradRawLines[gGradRawCount]), "%s", line);
            gGradRawRowForLine[gGradRawCount] = -1;
            gGradRawCount++;
        }

        if (gGradRowCount >= (int)(sizeof(gGradRows)/sizeof(gGradRows[0]))) continue;

        int tid, mode, lim, gid;
        if (sscanf(line, " %d %d %d %d", &tid, &mode, &lim, &gid) == 4) {
            GradRow *r = &gGradRows[gGradRowCount];
            *r = (GradRow){ tid, mode, lim, gid, raw_idx, {0} };

            char *hash = strchr(line, '#');
            if (hash) {
                while (*hash == ' ' || *hash == '\t') hash++;
                snprintf(r->inline_comment, sizeof(r->inline_comment), "%s", hash);
                int ll = (int)strlen(r->inline_comment);
                while (ll > 0 && (r->inline_comment[ll - 1] == '\n' || r->inline_comment[ll - 1] == '\r'))
                    r->inline_comment[--ll] = '\0';
            }

            if (raw_idx >= 0) gGradRawRowForLine[raw_idx] = gGradRowCount;
            gGradRowCount++;
        }
    }
    fclose(f);
    if (gGradRowCount > 0) gGradSelected = 0;
}

static void settings_select_grad_row(int idx)
{
    if (idx < 0 || idx >= gGradRowCount) return;
    gGradSelected = idx;
    fill_from_int(gGradModeBuf, sizeof(gGradModeBuf), &gGradModeLen, gGradRows[idx].mode);
    fill_from_int(gGradLimitBuf, sizeof(gGradLimitBuf), &gGradLimitLen, gGradRows[idx].limit);
    fill_from_int(gGradGroupBuf, sizeof(gGradGroupBuf), &gGradGroupLen, gGradRows[idx].group_id);
}

static int settings_save_grad_cfg(char *out_msg, int out_msg_size)
{
    if (gGradSelected >= 0 && gGradSelected < gGradRowCount) {
        int mode = atoi(gGradModeBuf);
        int lim = atoi(gGradLimitBuf);
        int gid = atoi(gGradGroupBuf);
        if (mode < 0 || mode > 2) {
            snprintf(out_msg, out_msg_size, "grad_config: mode must be 0..2");
            return 0;
        }
        if (lim < 0) lim = 0;
        if (gid < 0) gid = 0;
        gGradRows[gGradSelected].mode = mode;
        gGradRows[gGradSelected].limit = lim;
        gGradRows[gGradSelected].group_id = gid;
    }

    FILE *f = fopen(gGradCfgPath, "w");
    if (!f) {
        snprintf(out_msg, out_msg_size, "grad_config: cannot write file");
        return 0;
    }

    if (gGradRawCount > 0) {
        for (int i = 0; i < gGradRawCount; i++) {
            int row_idx = gGradRawRowForLine[i];
            if (row_idx >= 0 && row_idx < gGradRowCount) {
                GradRow *r = &gGradRows[row_idx];
                if (r->inline_comment[0] != '\0') {
                    fprintf(f, "%-12d %-6d %-7d %-8d %s\n",
                            r->type_id, r->mode, r->limit, r->group_id, r->inline_comment);
                } else {
                    fprintf(f, "%-12d %-6d %-7d %-8d\n",
                            r->type_id, r->mode, r->limit, r->group_id);
                }
            } else {
                fputs(gGradRawLines[i], f);
            }
        }
    } else {
        for (int i = 0; i < gGradRowCount; i++) {
            fprintf(f, "%d %d %d %d\n",
                    gGradRows[i].type_id,
                    gGradRows[i].mode,
                    gGradRows[i].limit,
                    gGradRows[i].group_id);
        }
    }
    fclose(f);
    snprintf(out_msg, out_msg_size, "grad_config.cfg saved");
    return 1;
}

static void settings_load_subjects_dat(void)
{
    gSubjectRowCount = 0;
    gSubjectSelected = -1;
    gSubjectSection = 1;

    FILE *f = fopen(gSubjectsDatPath, "r");
    if (!f) return;
    char line[1024];
    int cur_type = 0;
    char cur_name[64] = {0};
    while (fgets(line, sizeof(line), f) && gSubjectRowCount < (int)(sizeof(gSubjectRows)/sizeof(gSubjectRows[0]))) {
        int ll = (int)strlen(line);
        while (ll > 0 && (line[ll - 1] == '\n' || line[ll - 1] == '\r')) line[--ll] = '\0';
        if (ll == 0 || line[0] == '#') continue;

        if (line[0] == '[') {
            if (sscanf(line, "[%d] %63[^\n]", &cur_type, cur_name) >= 1) {
                if (cur_name[0] == '\0') snprintf(cur_name, sizeof(cur_name), "Type %d", cur_type);
            }
            continue;
        }
        if (cur_type <= 0) continue;

        char code[MAXSIZEID] = {0};
        int term = 0;
        int credit = 0;
        char name[MAXSIZENAME] = {0};
        if (sscanf(line, " %25s %d %d %255[^\n]", code, &term, &credit, name) == 4) {
            SubjectRow *r = &gSubjectRows[gSubjectRowCount++];
            r->type_id = cur_type;
            snprintf(r->type_name, sizeof(r->type_name), "%s", cur_name);
            snprintf(r->code, sizeof(r->code), "%s", code);
            r->term = term;
            r->credit = credit;
            snprintf(r->name, sizeof(r->name), "%s", name);
        }
    }
    fclose(f);
}

static void settings_select_subject_row(int idx)
{
    if (idx < 0 || idx >= gSubjectRowCount) return;
    gSubjectSelected = idx;
    gSubjectSection = gSubjectRows[idx].type_id;
    snprintf(gSubCodeBuf, sizeof(gSubCodeBuf), "%s", gSubjectRows[idx].code);
    gSubCodeLen = (int)strlen(gSubCodeBuf);
    fill_from_int(gSubTermBuf, sizeof(gSubTermBuf), &gSubTermLen, gSubjectRows[idx].term);
    fill_from_int(gSubCreditBuf, sizeof(gSubCreditBuf), &gSubCreditLen, gSubjectRows[idx].credit);
    snprintf(gSubNameBuf, sizeof(gSubNameBuf), "%s", gSubjectRows[idx].name);
    gSubNameLen = (int)strlen(gSubNameBuf);
}

static int settings_save_subjects_dat(char *out_msg, int out_msg_size)
{
    if (gSubjectSelected >= 0 && gSubjectSelected < gSubjectRowCount) {
        SubjectRow *r = &gSubjectRows[gSubjectSelected];
        snprintf(r->code, sizeof(r->code), "%s", gSubCodeBuf);
        r->term = atoi(gSubTermBuf);
        r->credit = atoi(gSubCreditBuf);
        snprintf(r->name, sizeof(r->name), "%s", gSubNameBuf);
    }

    FILE *f = fopen(gSubjectsDatPath, "w");
    if (!f) {
        snprintf(out_msg, out_msg_size, "subjects.dat: cannot write file");
        return 0;
    }
    fprintf(f,
            "# subjects.dat - generated by settings UI\n"
            "# Format: CODE TERM CREDIT Subject Name\n\n");

    for (int t = 1; t < sizeSubjectType; t++) {
        int first = -1;
        const char *nm = NULL;
        for (int i = 0; i < gSubjectRowCount; i++) {
            if (gSubjectRows[i].type_id == t) {
                first = i;
                nm = gSubjectRows[i].type_name;
                break;
            }
        }
        if (first < 0) continue;
        fprintf(f, "[%d] %s\n", t, nm ? nm : "Type");
        for (int i = 0; i < gSubjectRowCount; i++) {
            if (gSubjectRows[i].type_id != t) continue;
            fprintf(f, "%-9s %-5d %-6d %s\n",
                    gSubjectRows[i].code,
                    gSubjectRows[i].term,
                    gSubjectRows[i].credit,
                    gSubjectRows[i].name);
        }
        fprintf(f, "\n");
    }
    fclose(f);
    snprintf(out_msg, out_msg_size, "subjects.dat saved");
    return 1;
}

/* UI scale (loaded from assets/ui.cfg) */
static float gFontScale  = 1.8f;          /* multiplier for all font sizes         */
static int   gTargetFPS  = 60;            /* target FPS (60–240, set in ui.cfg)    */

/* name-input screen state */
static bool  gNameInput  = true;      /* true = show name-input screen             */
static bool  gDBReady    = false;     /* true = DB loaded, show main UI            */
static char  gUserName[26] = {0};    /* entered username (max 25 chars + NUL)     */
static int   gNameLen    = 0;

#include "db.h"                     /* SQLite backend                     */

/* forward decl: used by settings apply helper */
static void RefreshPlayer(void);

/* --- Asset path helpers --------------------------------------------- */
static void set_trimmed_copy(char *dst, int dstsz, const char *src)
{
    if (!dst || dstsz <= 0) return;
    if (!src) { dst[0] = '\0'; return; }
    while (*src && isspace((unsigned char)*src)) src++;
    int n = (int)strlen(src);
    while (n > 0 && isspace((unsigned char)src[n - 1])) n--;
    if (n >= dstsz) n = dstsz - 1;
    memcpy(dst, src, (size_t)n);
    dst[n] = '\0';
}

static void rebuild_cfg_paths_from_assets_dir(void)
{
    snprintf(gUiCfgPath, sizeof(gUiCfgPath), "%s/ui.cfg", gAssetsDir);
    snprintf(gFontsCfgPath, sizeof(gFontsCfgPath), "%s/fonts.cfg", gAssetsDir);
    snprintf(gGradCfgPath, sizeof(gGradCfgPath), "%s/grad_config.cfg", gAssetsDir);
}

static void load_asset_path_config(void)
{
    snprintf(gAssetsDir, sizeof(gAssetsDir), "%s", "assets");
    snprintf(gSubjectsDatPath, sizeof(gSubjectsDatPath), "%s", "assets/subjects.dat");
    rebuild_cfg_paths_from_assets_dir();

    FILE *f = fopen(kAssetPathConfigFile, "r");
    if (!f) return;

    char line[768];
    char key[64];
    char val[640];
    while (fgets(line, sizeof(line), f)) {
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r') continue;
        if (sscanf(line, "%63s %639[^\n\r]", key, val) != 2) continue;
        if (strcmp(key, "assets_dir") == 0) {
            set_trimmed_copy(gAssetsDir, sizeof(gAssetsDir), val);
        } else if (strcmp(key, "subjects_dat") == 0) {
            set_trimmed_copy(gSubjectsDatPath, sizeof(gSubjectsDatPath), val);
        }
    }
    fclose(f);

    if (gAssetsDir[0] == '\0') snprintf(gAssetsDir, sizeof(gAssetsDir), "%s", "assets");
    rebuild_cfg_paths_from_assets_dir();
    if (gSubjectsDatPath[0] == '\0')
        snprintf(gSubjectsDatPath, sizeof(gSubjectsDatPath), "%s/subjects.dat", gAssetsDir);
}

static void open_settings_popup(void)
{
    gSettingsOpen = true;
    gSettingsField = 0;

    settings_load_ui_cfg();
    settings_load_grad_cfg();
    settings_load_subjects_dat();

    gSettingsEditTarget = 0;
    if (gGradRowCount > 0) settings_select_grad_row(0);
    for (int i = 0; i < gSubjectRowCount; i++) {
        if (gSubjectRows[i].type_id == gSubjectSection) {
            settings_select_subject_row(i);
            break;
        }
    }
}

/* --- Score/player refresh helper ------------------------------------- */
/* Calls DB_Query then recomputes graduation status + alert level.       */
static void RefreshPlayer(void)
{
    DB_Query(&gPlayer);
    update_player_status(&gPlayer);
}

/* --- Command dispatcher ----------------------------------------------- */
#include "cmd.h"

/* --- UI rendering layer (sees all globals above) ---------------------- */
#include "ui.c"

/* --- DB initializer (called when user confirms name on startup) -------- */
static void InitPlayerDB(void)
{
    for(int i = 0; i < sizeSubjectType; i++)
      gTypeName[i][0] = 0;
    bool isNew = !DB_Exists(gUserName);
    if (!DB_Open(gUserName)) {
        snprintf(gResultMsg, sizeof(gResultMsg),
                 "Error: cannot open db_%s.db", gUserName);
        gHasResult       = true;
        gResultShowUntil = (float)GetTime() + 5.f;
        return;
    }
    DB_CreateSchema();
    if (isNew) {
        DB_SeedFromDat();
        if (strcmp(gUserName, "test") == 0) DB_LoadScores_Test();
    }
    DB_SeedGradRules();   /* INSERT OR IGNORE — safe for existing DBs too */
    DB_LoadGradRules();
    DB_ValidateData();
    /* Auto-reload: if validation found problems, re-parse the asset files so
     * that any fixes the user made since the last run are picked up.       */
    if (gDataWarnCount > 0) DB_ReloadData();
    memset(&gPlayer, 0, sizeof(gPlayer));
    strncpy(gPlayer.name_player, gUserName, MAXSIZENAME - 1);
    RefreshPlayer();
    gDBReady   = true;
    gNameInput = false;
    char title[64];
    snprintf(title, sizeof(title), "Transcript Viewer  --  %s", gUserName);
    SetWindowTitle(title);
}

/* --- Keyboard handler -------------------------------------------------- */
static void HandleKeyboard(void)
{
    /* Name-input phase: capture username before anything else */
    if (gNameInput) {
        if (IsKeyPressed(KEY_ESCAPE)) { CloseWindow(); return; }
        if (IsKeyPressed(KEY_ENTER) && gNameLen > 0) { InitPlayerDB(); return; }
        if ((IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE))
                && gNameLen > 0)
            gUserName[--gNameLen] = '\0';
        int ch;
        while ((ch = GetCharPressed()) != 0)
            if (ch >= 32 && ch < 127 && gNameLen < 25) {
                gUserName[gNameLen++] = (char)ch;
                gUserName[gNameLen]   = '\0';
            }
        return;
    }

    /* F1 -- toggle Clay debug overlay */
    if (IsKeyPressed(KEY_F1)) {
        Clay_SetDebugModeEnabled(!Clay_IsDebugModeEnabled());
        return;
    }

    /* Ctrl+, -- open settings popup */
    if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_COMMA)) {
        open_settings_popup();
        return;
    }

    /* Settings popup takes priority */
    if (gSettingsOpen) {
        if (IsKeyPressed(KEY_ESCAPE)) {
            gSettingsOpen = false;
            return;
        }
        if (IsKeyPressed(KEY_TAB)) {
            int maxField = (gSettingsEditTarget == 0) ? 1 : (gSettingsEditTarget == 1 ? 2 : 3);
            gSettingsField = (gSettingsField + 1) % (maxField + 1);
            return;
        }

        char *activeBuf = NULL;
        int *activeLen = NULL;
        int maxLen = 0;
        bool allow_dot = false;

        if (gSettingsEditTarget == 0) {
            if (gSettingsField == 0) {
                activeBuf = gUiScaleBuf; activeLen = &gUiScaleLen; maxLen = (int)sizeof(gUiScaleBuf) - 1; allow_dot = true;
            } else {
                activeBuf = gUiFpsBuf; activeLen = &gUiFpsLen; maxLen = (int)sizeof(gUiFpsBuf) - 1;
            }
        } else if (gSettingsEditTarget == 1) {
            if (gSettingsField == 0) {
                activeBuf = gGradModeBuf; activeLen = &gGradModeLen; maxLen = (int)sizeof(gGradModeBuf) - 1;
            } else if (gSettingsField == 1) {
                activeBuf = gGradLimitBuf; activeLen = &gGradLimitLen; maxLen = (int)sizeof(gGradLimitBuf) - 1;
            } else {
                activeBuf = gGradGroupBuf; activeLen = &gGradGroupLen; maxLen = (int)sizeof(gGradGroupBuf) - 1;
            }
        } else {
            if (gSettingsField == 0) {
                activeBuf = gSubCodeBuf; activeLen = &gSubCodeLen; maxLen = (int)sizeof(gSubCodeBuf) - 1;
            } else if (gSettingsField == 1) {
                activeBuf = gSubTermBuf; activeLen = &gSubTermLen; maxLen = (int)sizeof(gSubTermBuf) - 1;
            } else if (gSettingsField == 2) {
                activeBuf = gSubCreditBuf; activeLen = &gSubCreditLen; maxLen = (int)sizeof(gSubCreditBuf) - 1;
            } else {
                activeBuf = gSubNameBuf; activeLen = &gSubNameLen; maxLen = (int)sizeof(gSubNameBuf) - 1;
            }
        }

        if (activeBuf && activeLen) {
            if ((IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE)) && *activeLen > 0) {
                activeBuf[--(*activeLen)] = '\0';
            }
            int ch;
            while ((ch = GetCharPressed()) != 0) {
                if (*activeLen >= maxLen) break;
                if (gSettingsEditTarget == 2 && gSettingsField == 3) {
                    if (ch >= 32 && ch < 127) {
                        activeBuf[(*activeLen)++] = (char)ch;
                        activeBuf[*activeLen] = '\0';
                    }
                } else {
                    bool ok = (ch >= '0' && ch <= '9');
                    if (!ok && allow_dot && ch == '.' && !strchr(activeBuf, '.')) ok = true;
                    if (!ok && gSettingsEditTarget == 2 && gSettingsField == 0
                            && ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || ch == '_')) ok = true;
                    if (ok) {
                        activeBuf[(*activeLen)++] = (char)ch;
                        activeBuf[*activeLen] = '\0';
                    }
                }
            }
        }
        return;
    }

    /* Edit popup takes input priority after F1 */
    if (gEditOpen) {
        if (IsKeyPressed(KEY_ESCAPE)) {
            gEditOpen = false;
            return;
        }
        /* Tab / Shift+Tab cycle between fields */
        if (IsKeyPressed(KEY_TAB)) {
            gEditField = (gEditField + 1) % 2;
            return;
        }
        /* Backspace */
        char *activeBuf = gEditField == 0 ? gEditMidBuf : gEditFinBuf;
        int  *activeLen = gEditField == 0 ? &gEditMidLen : &gEditFinLen;
        if ((IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE))
                && *activeLen > 0) {
            activeBuf[--(*activeLen)] = '\0';
        }
        /* Printable: digits and single '.' */
        int ch;
        while ((ch = GetCharPressed()) != 0) {
            bool isDot   = (ch == '.');
            bool isDigit = (ch >= '0' && ch <= '9');
            if ((isDigit || isDot) && *activeLen < 6) {
                /* only one dot allowed */
                if (isDot && strchr(activeBuf, '.')) continue;
                activeBuf[(*activeLen)++] = (char)ch;
                activeBuf[*activeLen]     = '\0';
            }
        }
        return;
    }

    /* table filter text input (when search box focused) */
    if (gFilterInputFocus && !gPopupOpen) {
        if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_ENTER)) {
            gFilterInputFocus = false;
            return;
        }
        if ((IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE))
                && gTableFilterLen > 0) {
            gTableFilterQuery[--gTableFilterLen] = '\0';
        }
        int ch;
        while ((ch = GetCharPressed()) != 0) {
            if (ch >= 32 && ch < 127 && gTableFilterLen < (int)sizeof(gTableFilterQuery) - 1) {
                gTableFilterQuery[gTableFilterLen++] = (char)ch;
                gTableFilterQuery[gTableFilterLen] = '\0';
            }
        }
        return;
    }

    /* Ctrl+K -- toggle palette */
    if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_K)) {
        gFilterInputFocus = false;
        gPopupOpen = !gPopupOpen;
        if (!gPopupOpen) { gCmdLen = 0; gCmdBuf[0] = '\0'; }
        return;
    }

    if (gPopupOpen && gEditOpen) gPopupOpen = false;
    if (gPopupOpen && gSettingsOpen) gPopupOpen = false;
    if (!gPopupOpen) return;

    /* Escape -- dismiss */
    if (IsKeyPressed(KEY_ESCAPE)) {
        gPopupOpen = false;
        gCmdLen    = 0;
        gCmdBuf[0] = '\0';
        return;
    }

    /* Enter -- submit */
    if (IsKeyPressed(KEY_ENTER) && gCmdLen > 0) {
        ExecuteCommand(gCmdBuf, &gActiveNav, gFilterDept, gResultMsg, sizeof(gResultMsg));
        gHasResult       = true;
        gResultShowUntil = (float)GetTime() + 5.f;
        gPopupOpen       = false;
        gCmdLen          = 0;
        gCmdBuf[0]       = '\0';
        return;
    }

    /* Backspace (with key-repeat) */
    if ((IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE))
            && gCmdLen > 0) {
        gCmdBuf[--gCmdLen] = '\0';
    }

    /* Printable ASCII */
    int ch;
    while ((ch = GetCharPressed()) != 0) {
        if (ch >= 32 && ch < 127 && gCmdLen < 253) {
            gCmdBuf[gCmdLen++] = (char)ch;
            gCmdBuf[gCmdLen]   = '\0';
        }
    }
}

/* --- Root layout ------------------------------------------------------- */
static void BuildLayout(void)
{
    CLAY(CLAY_ID("Root"), {
        .layout = {
            .sizing          = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
            .layoutDirection = CLAY_LEFT_TO_RIGHT,
        },
        .backgroundColor = C_BG,
    }) {
        if (!gNameInput) {
            RenderSidebar();
            if (gActiveNav == 0)
                RenderDashboard();
            else
                RenderMainContent();
        }
    }

    if (gNameInput) {
        RenderNameInput();
    } else {
        /* Floating overlays rendered on top via zIndex */
        if (gEditOpen)   RenderEditPopup();
        if (gSettingsOpen) RenderSettingsPopup();
        if (gPopupOpen)  RenderCommandPopup();
        if (gHasResult)  RenderResultToast();
    }
}

/* --- Clay error handler ------------------------------------------------ */
static void HandleClayError(Clay_ErrorData err)
{
    fprintf(stderr, "[Clay error %d] %.*s\n",
            err.errorType, err.errorText.length, err.errorText.chars);
}

/* --- Entry point ------------------------------------------------------- */
int main(void)
{
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
    InitWindow(WIN_W, WIN_H, "Transcript Viewer");
    SetTargetFPS(gTargetFPS);  /* will be updated after ui.cfg loads */

    load_asset_path_config();

    /* Font loading: read paths from selected fonts.cfg, fall back to Raylib default */
#define MAX_FONT_ENTRIES 32
#define MAX_FONT_PATH    512
    static char fontPathBuf[MAX_FONT_ENTRIES][MAX_FONT_PATH];
    const char *fontCandidates[MAX_FONT_ENTRIES + 1];
    int numFontCandidates = 0;

    FILE *fontCfg = fopen(gFontsCfgPath, "r");
    if (fontCfg) {
        char line[MAX_FONT_PATH];
        while (fgets(line, sizeof(line), fontCfg) && numFontCandidates < MAX_FONT_ENTRIES) {
            /* strip newline */
            int ll = (int)strlen(line);
            while (ll > 0 && (line[ll-1] == '\n' || line[ll-1] == '\r' || line[ll-1] == ' '))
                line[--ll] = '\0';
            if (ll == 0 || line[0] == '#') continue;  /* blank / comment */
            snprintf(fontPathBuf[numFontCandidates], MAX_FONT_PATH, "%s", line);
            fontCandidates[numFontCandidates] = fontPathBuf[numFontCandidates];
            numFontCandidates++;
        }
        fclose(fontCfg);
    }
    // if no font found: show error on screen and exit
    if (numFontCandidates == 0) {
        fprintf(stderr,
            "[font] No font paths found in %s.\n"
            "       Add at least one valid .ttf path to that file and restart.\n",
            gFontsCfgPath);
        const char *msg1 = "No font found — cannot start.";
        const char *msg2 = "Add a .ttf path to fonts.cfg and restart.";
        const char *msg3 = "Press any key or close this window to exit.";
        SetTargetFPS(60);
        while (!WindowShouldClose() && !GetKeyPressed()) {
            BeginDrawing();
            ClearBackground((Color){ 11, 11, 20, 255 });
            DrawText(msg1, 40, 120, 28, (Color){ 239, 68, 68, 255 });
            DrawText(msg2, 40, 165, 20, (Color){ 224, 224, 242, 255 });
            DrawText(msg3, 40, 210, 16, (Color){ 110, 110, 155, 255 });
            EndDrawing();
        }
        CloseWindow();
        return 1;
    }
    fontCandidates[numFontCandidates] = NULL;  /* sentinel */

    gFonts[0] = (Font){0};
    for (int i = 0; fontCandidates[i]; i++) {
        if (FileExists(fontCandidates[i])) {
            Font f = LoadFontEx(fontCandidates[i], 72, NULL, 250);
            if (f.texture.id != 0) {
                gFonts[0]   = f;
                gCustomFont = true;
                SetTextureFilter(gFonts[0].texture, TEXTURE_FILTER_BILINEAR);
                break;
            }
        }
    }
    if (!gCustomFont) gFonts[0] = GetFontDefault();

    /* ── UI config: load font_scale from selected ui.cfg ───────────── */
    {
        FILE *uiCfg = fopen(gUiCfgPath, "r");
        if (uiCfg) {
            char line[256];
            while (fgets(line, sizeof(line), uiCfg)) {
                if (line[0] == '#' || line[0] == '\n' || line[0] == '\r') continue;
                char key[64]; float val;
                if (sscanf(line, "%63s %f", key, &val) == 2) {
                    if (strcmp(key, "font_scale") == 0 && val > 0.1f && val < 10.f)
                        gFontScale = val;
                    if (strcmp(key, "target_fps") == 0) {
                        int fps = (int)val;
                        if (fps < 60)  fps = 60;
                        if (fps > 240) fps = 240;
                        gTargetFPS = fps;
                    }
                }
            }
            fclose(uiCfg);
        }
        SetTargetFPS(gTargetFPS);  /* apply after ui.cfg parsed */
    }

    /* ── SQLite: opened via InitPlayerDB() after user enters name ───── */
    memset(&gPlayer, 0, sizeof(gPlayer));

    /* Clay initialisation */
    Clay_SetMaxElementCount(32768);
    size_t memSize = Clay_MinMemorySize();
    void  *mem     = malloc(memSize);
    Clay_Arena arena = Clay_CreateArenaWithCapacityAndMemory(memSize, mem);
    Clay_Initialize(arena,
                    (Clay_Dimensions){ (float)WIN_W, (float)WIN_H },
                    (Clay_ErrorHandler){ HandleClayError, NULL });
    Clay_SetMeasureTextFunction(Raylib_MeasureText, gFonts);
    Clay_SetDebugModeEnabled(false);  /* F1 toggles at runtime */

    /* Main loop */
    while (!WindowShouldClose()) {
        gScreenW = GetScreenWidth();
        gScreenH = GetScreenHeight();

        Clay_SetLayoutDimensions((Clay_Dimensions){ (float)gScreenW, (float)gScreenH });
        Clay_SetPointerState(
            (Clay_Vector2){ (float)GetMouseX(), (float)GetMouseY() },
            IsMouseButtonDown(MOUSE_LEFT_BUTTON));
        Clay_UpdateScrollContainers(
            true,
            (Clay_Vector2){ 0.f, -GetMouseWheelMove() * 50.f },
            GetFrameTime());

        HandleKeyboard();

        /* expire toast */
        if (gHasResult && (float)GetTime() >= gResultShowUntil)
            gHasResult = false;

        gDynPos = 0;  /* reset per-frame string arena */

        Clay_BeginLayout();
        BuildLayout();
        Clay_RenderCommandArray cmds = Clay_EndLayout();

        BeginDrawing();
        ClearBackground((Color){ 11, 11, 20, 255 });
        Clay_Raylib_Render(cmds, gFonts);

        /* ── Draw donut charts on top of Clay output (dashboard only) ── */
        if (gDBReady && !gNameInput && gActiveNav == 0) {
            /* Fetch bounding boxes from last frame */
            Clay_ElementData gd = Clay_GetElementData(
                Clay_GetElementId(CLAY_STRING(DONUT_GRADE_ID)));
            Clay_ElementData cd = Clay_GetElementData(
                Clay_GetElementId(CLAY_STRING(DONUT_CPA_ID)));

            /* ── Grade distribution donut ── */
            if (gd.found) {
                float cx = gd.boundingBox.x + gd.boundingBox.width  * 0.5f;
                float cy = gd.boundingBox.y + gd.boundingBox.height * 0.5f;
                float r  = (gd.boundingBox.width  < gd.boundingBox.height
                            ? gd.boundingBox.width : gd.boundingBox.height) * 0.5f - 6.f;
                if (r < 10.f) r = 10.f;
                float inner = r * 0.58f;

                /* count grades */
                int cA=0,cB=0,cC=0,cD=0,cF=0,cX=0, tot=0;
                for (int t=1; t<sizeSubjectType; t++) {
                    Subject_Node *nd = gPlayer.numofSubjectType[t].head;
                    while (nd) {
                        if (nd->status_ever_been_study & 1) {
                            switch(nd->score_letter){
                                case 'A': cA++; break; case 'B': cB++; break;
                                case 'C': cC++; break; case 'D': cD++; break;
                                case 'F': cF++; break; default: cX++; break;
                            }
                            tot++;
                        }  //else cX++;
                        nd = nd->next;
                    }
                }
                if (tot == 0) tot = 1;

                typedef struct { int cnt; Color col; } Seg;
                Seg segs[6] = {
                    {cA, (Color){ 34,197, 94,255}},
                    {cB, (Color){ 99,102,241,255}},
                    {cC, (Color){234,179,  8,255}},
                    {cD, (Color){249,115, 22,255}},
                    {cF, (Color){239, 68, 68,255}},
                    {cX, (Color){ 60, 60, 90,180}},
                };
                float angle = -90.f;
                for (int s=0; s<6; s++) {
                    if (segs[s].cnt == 0) continue;
                    float sweep = 360.f * segs[s].cnt / (float)tot;
                    DrawRing((Vector2){cx,cy}, inner, r, angle, angle+sweep, 36,
                             segs[s].col);
                    angle += sweep;
                }
                /* centre text */
                char ctxt[16];
                snprintf(ctxt, sizeof(ctxt), "%d", tot);
                float tw = MeasureTextEx(gFonts[0], ctxt, 14.f, 1.f).x;
                DrawTextEx(gFonts[0], ctxt,
                           (Vector2){cx - tw*0.5f, cy - 10.f},
                           12.f*gFontScale, 1.f, (Color){224,224,242,255});
                DrawTextEx(gFonts[0], "subj",
                           (Vector2){cx - MeasureTextEx(gFonts[0],"subj",10.f,1.f).x*0.5f,
                                     cy + 2.f},
                           12.f*gFontScale, 1.f, (Color){110,110,155,255});
            }

            /* ── CPA gauge (arc from -135° to 135°, 0..4 scale) ── */
            if (cd.found) {
                float cx = cd.boundingBox.x + cd.boundingBox.width  * 0.5f;
                float cy = cd.boundingBox.y + cd.boundingBox.height * 0.5f;
                float r  = (cd.boundingBox.width  < cd.boundingBox.height
                            ? cd.boundingBox.width : cd.boundingBox.height) * 0.5f - 6.f;
                if (r < 10.f) r = 10.f;
                float inner = r * 0.58f;

                float cpa_v = calc_cpa(&gPlayer, 0);
                if (cpa_v > 4.f) cpa_v = 4.f;

                /* background arc */
                DrawRing((Vector2){cx,cy}, inner, r, -135.f, 135.f, 36,
                         (Color){36,36,68,255});
                /* filled arc */
                float fillEnd = -135.f + 270.f * (cpa_v / 4.f);
                Color fillCol = cpa_v >= 3.5f ? (Color){ 34,197, 94,255}
                              : cpa_v >= 2.5f ? (Color){ 99,102,241,255}
                              : cpa_v >= 2.0f ? (Color){234,179,  8,255}
                                              : (Color){239, 68, 68,255};
                if (cpa_v > 0.01f)
                    DrawRing((Vector2){cx,cy}, inner, r, -135.f, fillEnd, 36,
                             fillCol);
                /* centre label */
                char cpatxt[16];
                snprintf(cpatxt, sizeof(cpatxt), "%.2f", cpa_v);
                float tw2 = MeasureTextEx(gFonts[0], cpatxt, 16.f, 1.f).x;
                DrawTextEx(gFonts[0], cpatxt,
                           (Vector2){cx - tw2*0.5f, cy - 10.f},
                           12.f*gFontScale, 1.f, fillCol);
                DrawTextEx(gFonts[0], "/ 4.00",
                           (Vector2){cx - MeasureTextEx(gFonts[0],"/ 4.00",10.f,1.f).x*0.5f,
                                     cy + 8.f},
                           12.f*gFontScale, 1.f, (Color){110,110,155,255});
            }
        }

        /* FPS counter */
        char fps[32];
        snprintf(fps, sizeof(fps), "FPS: %d", GetFPS());
        DrawTextEx(gFonts[0], fps,
                   (Vector2){ (float)(gScreenW - 80), 6.f }, 14.f, 1.f,
                   (Color){ 80, 80, 120, 180 });
        EndDrawing();
    }

    if (gCustomFont) UnloadFont(gFonts[0]);
    free(mem);
    DB_Close();
    Clay_Raylib_Close();
    return 0;
}
