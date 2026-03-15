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

#include "utf8_vn.c"

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

/* startup import overlay (CTT-SIS curriculum table) */
#define IMPORT_RAW_MAX       262144
#define IMPORT_TYPES_MAX     32
#define IMPORT_ROWS_MAX      2048
#define IMPORT_SUMMARY_LINES 32

typedef struct {
    int  type_id;
    char type_name[64];
    char code[MAXSIZEID];
    int  term;
    int  credit;
    char name[MAXSIZENAME];
    int  studied;
    int  pass;
    char letter[4];
    float score_num;
} ImportSubjectRow;

typedef struct {
    int type_id;
    int expected_count;
    int expected_total_credit;
    int expected_pass_credit;
    int parsed_count;
    char display_name[96];
} ImportTypeSummary;

static bool gImportOpen = false;
static int  gImportStage = 0; /* 0=paste+parse, 1=review+confirm, 2=enter username */
static bool gImportPasted = false;
static bool gImportLastSuccess = false;
static int  gImportStatusKind = 0; /* -1 error, 0 neutral, 1 success */
static char gImportRaw[IMPORT_RAW_MAX] = {0};
static int  gImportRawLen = 0;
static char gImportStatusMsg[512] = "Press Ctrl+V to paste CTT-SIS table.";
static char gImportUserBuf[26] = {0};
static int  gImportUserLen = 0;
static ImportTypeSummary gImportTypes[IMPORT_TYPES_MAX];
static int  gImportTypeCount = 0;
static ImportSubjectRow gImportRows[IMPORT_ROWS_MAX];
static int  gImportRowCount = 0;
static char gImportSummaryLines[IMPORT_SUMMARY_LINES][160];
static int  gImportSummaryCount = 0;

/* forward declarations for runtime UI config values */
static float gFontScale;
static int   gTargetFPS;
static const char *type_name_for_id(int tid);

/* forward declarations used by startup import helpers */
static void RefreshPlayer(void);
void DB_ReloadData(void);

static int utf8_char_len(unsigned char c)
{
    if ((c & 0xE0) == 0xC0) return 2;
    if ((c & 0xF0) == 0xE0) return 3;
    if ((c & 0xF8) == 0xF0) return 4;
    return 1;
}

static void trim_copy(char *dst, int dstsz, const char *src)
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

static void to_ascii_normalized(const char *src, char *dst, int dstsz, int space_to_underscore, int lower)
{
    if (!dst || dstsz <= 0) return;
    if (!src) { dst[0] = '\0'; return; }
    int di = 0;
    for (int si = 0; src[si] && di < dstsz - 1; ) {
        unsigned char c = (unsigned char)src[si];
        if (c < 128) {
            char out = (char)c;
            if (out == ' ' && space_to_underscore) out = '_';
            if (out == '\t') {
                dst[di++] = '\t';
                si++;
                continue;
            }
            if (lower) out = (char)tolower((unsigned char)out);
            dst[di++] = out;
            si++;
            continue;
        }

        int clen = utf8_char_len(c);
        if (clen <= 0) clen = 1;
        if (src[si + clen - 1] == '\0') {
            si++;
            continue;
        }
        char mapped = covert_to_eng(src + si);
        if (mapped) {
            if (lower) mapped = (char)tolower((unsigned char)mapped);
            dst[di++] = mapped;
        }
        si += clen;
    }
    dst[di] = '\0';
}

static int looks_like_subject_code(const char *s)
{
    if (!s || !s[0]) return 0;
    int i = 0;
    int alpha = 0;
    while (s[i] && ((s[i] >= 'A' && s[i] <= 'Z') || (s[i] >= 'a' && s[i] <= 'z'))) {
        alpha++;
        i++;
    }
    if (alpha < 2 || alpha > 4) return 0;
    int digits = 0;
    while (s[i] && isdigit((unsigned char)s[i])) {
        digits++;
        i++;
    }
    if (digits < 3) return 0;
    return s[i] == '\0';
}

static int parse_first_int(const char *s)
{
    if (!s) return 0;
    while (*s && !isdigit((unsigned char)*s) && *s != '-') s++;
    return (int)strtol(s, NULL, 10);
}

static int token_is_plain_int(const char *s)
{
    if (!s || !s[0]) return 0;
    int i = 0;
    if (s[i] == '+' || s[i] == '-') i++;
    int digits = 0;
    for (; s[i]; i++) {
        if (!isdigit((unsigned char)s[i])) return 0;
        digits++;
    }
    return digits > 0;
}

static int token_is_grade(const char *s)
{
    if (!s || !s[0]) return 0;
    if (!(s[0] == 'A' || s[0] == 'B' || s[0] == 'C' || s[0] == 'D' || s[0] == 'F' || s[0] == 'R' || s[0] == 'X'))
        return 0;
    if (s[1] == '\0') return 1;
    if (s[1] == '+' && s[2] == '\0') return 1;
    return 0;
}

static int token_is_number(const char *s)
{
    if (!s || !s[0]) return 0;
    int i = 0;
    if (s[i] == '+' || s[i] == '-') i++;
    int digits = 0;
    int dot_seen = 0;
    for (; s[i]; i++) {
        if (isdigit((unsigned char)s[i])) {
            digits++;
            continue;
        }
        if (s[i] == '.' && !dot_seen) {
            dot_seen = 1;
            continue;
        }
        return 0;
    }
    return digits > 0;
}

static int map_raw_type_to_internal(int raw_code)
{
    switch (raw_code) {
        case 7:   return 13;
        case 210: return 4;
        case 220: return 3;
        case 250: return 2;
        case 300: return 1;
        case 320: return 5;
        case 360: return 12;
        case 400: return 6;
        case 401: return 7;
        case 402: return 8;
        case 403: return 9;
        case 404: return 10;
        case 405: return 11;
        default:  return 0;
    }
}

static int map_type_name_fallback(const char *norm)
{
    if (!norm) return 0;
    if (strstr(norm, "co_so_va_cot_loi_nganh")) return 1;
    if (strstr(norm, "toan_va_khoa_hoc_co_ban")) return 2;
    if (strstr(norm, "giao_duc_the_chat")) return 3;
    if (strstr(norm, "ly_luan_chinh_tri") || strstr(norm, "phap_luat_dai_cuong")) return 4;
    if (strstr(norm, "kien_thuc_bo_tro") || strstr(norm, "tu_chon")) return 5;
    if (strstr(norm, "mo_dun_1")) return 6;
    if (strstr(norm, "mo_dun_2")) return 7;
    if (strstr(norm, "mo_dun_3")) return 8;
    if (strstr(norm, "mo_dun_4")) return 9;
    if (strstr(norm, "mo_dun_5")) return 10;
    if (strstr(norm, "mo_dun_6")) return 11;
    if (strstr(norm, "thuc_tap")) return 12;
    if (strstr(norm, "do_an") || strstr(norm, "khoa_luan")) return 13;
    return 0;
}

static ImportTypeSummary *import_get_or_add_type(int tid)
{
    for (int i = 0; i < gImportTypeCount; i++)
        if (gImportTypes[i].type_id == tid) return &gImportTypes[i];
    if (gImportTypeCount >= IMPORT_TYPES_MAX) return NULL;
    ImportTypeSummary *t = &gImportTypes[gImportTypeCount++];
    memset(t, 0, sizeof(*t));
    t->type_id = tid;
    snprintf(t->display_name, sizeof(t->display_name), "%s", type_name_for_id(tid));
    return t;
}

static void import_add_summary_line(const char *fmt, ...)
{
    if (gImportSummaryCount >= IMPORT_SUMMARY_LINES) return;
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(gImportSummaryLines[gImportSummaryCount],
              sizeof(gImportSummaryLines[gImportSummaryCount]), fmt, ap);
    va_end(ap);
    gImportSummaryCount++;
}

static int split_tab_fields(const char *line, char fields[][512], int max_fields)
{
    int count = 0;
    int start = 0;
    int len = (int)strlen(line);
    for (int i = 0; i <= len && count < max_fields; i++) {
        if (line[i] == '\t' || line[i] == '\0') {
            int n = i - start;
            if (n >= 511) n = 511;
            memcpy(fields[count], line + start, (size_t)n);
            fields[count][n] = '\0';
            count++;
            start = i + 1;
        }
    }
    return count;
}

static void import_reset_runtime_state(void)
{
    gImportTypeCount = 0;
    gImportRowCount = 0;
    gImportSummaryCount = 0;
}

static int write_subjects_from_import(char *out_msg, int out_msg_sz)
{
    FILE *f = fopen(gSubjectsDatPath, "w");
    if (!f) {
        snprintf(out_msg, out_msg_sz, "Import: cannot write subjects.dat file");
        return 0;
    }
    fprintf(f,
            "# subjects.dat - generated from CTT-SIS import\n"
            "# Format: CODE TERM CREDIT Subject Name\n\n");

    for (int tid = 1; tid < sizeSubjectType; tid++) {
        int has = 0;
        for (int i = 0; i < gImportRowCount; i++) {
            if (gImportRows[i].type_id == tid) { has = 1; break; }
        }
        if (!has) continue;
        fprintf(f, "[%d] %s\n", tid, type_name_for_id(tid));
        for (int i = 0; i < gImportRowCount; i++) {
            if (gImportRows[i].type_id != tid) continue;
            fprintf(f, "%-9s %-5d %-6d %s\n",
                    gImportRows[i].code,
                    gImportRows[i].term,
                    gImportRows[i].credit,
                    gImportRows[i].name);
        }
        fprintf(f, "\n");
    }
    fclose(f);
    return 1;
}

static int write_grad_modules_from_import(char *out_msg, int out_msg_sz)
{
    char lines[512][512];
    int line_count = 0;
    FILE *f = fopen(gGradCfgPath, "r");
    if (!f) {
        snprintf(out_msg, out_msg_sz, "Import: cannot read grad_config.cfg file");
        return 0;
    }
    while (line_count < 512 && fgets(lines[line_count], sizeof(lines[line_count]), f))
        line_count++;
    fclose(f);

    int module_count = 0;
    for (int tid = 6; tid <= 11; tid++) {
        for (int i = 0; i < gImportRowCount; i++) {
            if (gImportRows[i].type_id == tid) { module_count++; break; }
        }
    }

    char out_lines[640][512];
    int out_count = 0;
    int insert_at = -1;
    for (int i = 0; i < line_count; i++) {
        int tid = -1;
        if (sscanf(lines[i], " %d", &tid) == 1 && tid >= 6 && tid <= 11) {
            if (insert_at < 0) insert_at = out_count;
            continue;
        }
        size_t cp = strnlen(lines[i], sizeof(lines[i]) - 1);
        memcpy(out_lines[out_count], lines[i], cp);
        out_lines[out_count][cp] = '\0';
        out_count++;
    }
    if (insert_at < 0) insert_at = out_count;

    char module_rows[32][512];
    int module_rows_count = 0;
    for (int k = 0; k < module_count && module_rows_count < 32; k++) {
        int tid = 6 + k;
        int gid = (k < 3) ? 1 : 2;
        snprintf(module_rows[module_rows_count], sizeof(module_rows[module_rows_count]),
                 "%-12d %-6d %-7d %-8d # %s\n",
                 tid, 0, 0, gid, type_name_for_id(tid));
        module_rows_count++;
    }

    for (int i = out_count - 1; i >= insert_at; i--) {
        strncpy(out_lines[i + module_rows_count], out_lines[i], sizeof(out_lines[i + module_rows_count]) - 1);
        out_lines[i + module_rows_count][sizeof(out_lines[i + module_rows_count]) - 1] = '\0';
    }
    for (int i = 0; i < module_rows_count; i++)
        strncpy(out_lines[insert_at + i], module_rows[i], sizeof(out_lines[insert_at + i]) - 1);
    for (int i = 0; i < module_rows_count; i++)
        out_lines[insert_at + i][sizeof(out_lines[insert_at + i]) - 1] = '\0';
    out_count += module_rows_count;

    f = fopen(gGradCfgPath, "w");
    if (!f) {
        snprintf(out_msg, out_msg_sz, "Import: cannot write grad_config.cfg file");
        return 0;
    }
    for (int i = 0; i < out_count; i++) fputs(out_lines[i], f);
    fclose(f);
    return 1;
}

static int import_parse_only(char *out_msg, int out_msg_sz)
{
    import_reset_runtime_state();
    if (!gImportPasted || gImportRawLen <= 0) {
        snprintf(out_msg, out_msg_sz, "No pasted data. Press Ctrl+V first.");
        return 0;
    }

    int current_raw_type = 0;
    int current_tid = 0;

    const char *cursor = gImportRaw;
    while (*cursor) {
        const char *nl = strchr(cursor, '\n');
        int ll = nl ? (int)(nl - cursor) : (int)strlen(cursor);
        if (ll <= 0) {
            if (!nl) break;
            cursor = nl + 1;
            continue;
        }

        char line[2048];
        if (ll >= (int)sizeof(line)) ll = (int)sizeof(line) - 1;
        memcpy(line, cursor, (size_t)ll);
        line[ll] = '\0';
        if (ll > 0 && line[ll - 1] == '\r') line[ll - 1] = '\0';

        char norm[2048];
        to_ascii_normalized(line, norm, (int)sizeof(norm), 1, 1);

        if (strstr(norm, "ma_loai_hp:")) {
            const char *p = strstr(norm, "ma_loai_hp:");
            current_raw_type = parse_first_int(p + 11);
        } else if (strstr(norm, "collapse\tloai_hp:")) {
            const char *p = strstr(norm, "loai_hp:");
            char type_norm[256] = {0};
            if (p) {
                p += 8;
                int i = 0;
                while (p[i] && strncmp(p + i, "(count=", 7) != 0 && i < (int)sizeof(type_norm) - 1) {
                    type_norm[i] = p[i];
                    i++;
                }
                type_norm[i] = '\0';
            }
            int exp_count = 0, exp_credit = 0, exp_pass = 0;
            char *pc = strstr(norm, "count=");
            if (pc) exp_count = parse_first_int(pc + 6);
            pc = strstr(norm, "tong_tc:");
            if (pc) exp_credit = parse_first_int(pc + 8);
            pc = strstr(norm, "tong_dat:");
            if (pc) exp_pass = parse_first_int(pc + 9);

            current_tid = map_raw_type_to_internal(current_raw_type);
            if (current_tid == 0) current_tid = map_type_name_fallback(type_norm);

            if (current_tid > 0) {
                ImportTypeSummary *t = import_get_or_add_type(current_tid);
                if (t) {
                    if (exp_count > 0) t->expected_count = exp_count;
                    if (exp_credit > 0) t->expected_total_credit = exp_credit;
                    if (exp_pass >= 0) t->expected_pass_credit = exp_pass;
                    if (type_norm[0]) {
                        char pretty[96];
                        to_ascii_normalized(type_norm, pretty, (int)sizeof(pretty), 0, 0);
                        for (int i = 0; pretty[i]; i++) if (pretty[i] == '_') pretty[i] = ' ';
                        trim_copy(t->display_name, (int)sizeof(t->display_name), pretty);
                    }
                }
            }
        } else {
            char fields[20][512];
            int nf = split_tab_fields(line, fields, 20);
            int code_idx = -1;
            for (int i = 0; i < nf; i++) {
                char token_trim[128] = {0};
                trim_copy(token_trim, (int)sizeof(token_trim), fields[i]);
                if (looks_like_subject_code(token_trim)) {
                    code_idx = i;
                    break;
                }
            }
            if (code_idx >= 0 && current_tid > 0 && gImportRowCount < IMPORT_ROWS_MAX) {
                ImportSubjectRow *r = &gImportRows[gImportRowCount++];
                memset(r, 0, sizeof(*r));
                r->type_id = current_tid;
                snprintf(r->type_name, sizeof(r->type_name), "%s", type_name_for_id(current_tid));

                char code[64] = {0};
                trim_copy(code, (int)sizeof(code), fields[code_idx]);
                snprintf(r->code, sizeof(r->code), "%s", code);

                char name_raw[MAXSIZENAME] = {0};
                int name_idx = code_idx + 1;
                while (name_idx < nf && name_raw[0] == '\0') {
                    trim_copy(name_raw, (int)sizeof(name_raw), fields[name_idx]);
                    name_idx++;
                }

                char term_tok[64] = {0};
                int term_idx = name_idx;
                while (term_idx < nf && term_tok[0] == '\0') {
                    trim_copy(term_tok, (int)sizeof(term_tok), fields[term_idx]);
                    term_idx++;
                }

                char credit_tok[64] = {0};
                int cred_idx = term_idx;
                while (cred_idx < nf && credit_tok[0] == '\0') {
                    trim_copy(credit_tok, (int)sizeof(credit_tok), fields[cred_idx]);
                    cred_idx++;
                }

                r->term = parse_first_int(term_tok);
                if (r->term < 0) r->term = 0;
                r->credit = parse_first_int(credit_tok);
                if (r->credit < 0) r->credit = 0;

                int learned_credit = 0;
                char letter_tok[8] = {0};
                float score_num = 0.f;
                int after_credit_idx = cred_idx;
                for (int i = after_credit_idx; i < nf; i++) {
                    char tok[64] = {0};
                    trim_copy(tok, (int)sizeof(tok), fields[i]);
                    if (tok[0] == '\0') continue;

                    if (learned_credit == 0 && token_is_plain_int(tok)) {
                        learned_credit = atoi(tok);
                        continue;
                    }
                    if (letter_tok[0] == '\0' && token_is_grade(tok)) {
                        snprintf(letter_tok, sizeof(letter_tok), "%s", tok);
                        continue;
                    }
                    if (score_num <= 0.f && token_is_number(tok)) {
                        score_num = (float)atof(tok);
                    }
                }

                char name_ascii[MAXSIZENAME] = {0};
                to_ascii_normalized(name_raw, name_ascii, (int)sizeof(name_ascii), 0, 0);
                trim_copy(r->name, (int)sizeof(r->name), name_ascii);
                if (r->name[0] == '\0') {
                    strncpy(r->name, r->code, sizeof(r->name) - 1);
                    r->name[sizeof(r->name) - 1] = '\0';
                }

                r->studied = 0;
                r->pass = 0;
                r->score_num = score_num;
                r->letter[0] = '\0';
                if (letter_tok[0]) snprintf(r->letter, sizeof(r->letter), "%s", letter_tok);

                if (r->letter[0]) {
                    r->studied = 1;
                    if (!(r->letter[0] == 'F' || r->letter[0] == 'R' || r->letter[0] == 'X')) r->pass = 1;
                } else if (learned_credit > 0 || r->score_num > 0.f) {
                    r->studied = 1;
                    if (r->credit > 0 && learned_credit >= r->credit) r->pass = 1;
                }

                ImportTypeSummary *t = import_get_or_add_type(current_tid);
                if (t) t->parsed_count++;
            }
        }

        if (!nl) break;
        cursor = nl + 1;
    }

    if (gImportRowCount <= 0) {
        snprintf(out_msg, out_msg_sz, "Import failed: no subject rows detected. Please copy full CTT-SIS table again.");
        return 0;
    }

    int ok = 1;
    static const int required_ids[] = {1, 2, 3, 4, 5, 12, 13};
    for (int k = 0; k < (int)(sizeof(required_ids) / sizeof(required_ids[0])); k++) {
        int tid = required_ids[k];
        int found = 0;
        for (int i = 0; i < gImportTypeCount; i++) {
            if (gImportTypes[i].type_id == tid && gImportTypes[i].parsed_count > 0) {
                found = 1;
                break;
            }
        }
        if (!found) {
            ok = 0;
            import_add_summary_line("Missing required type [%d] %s", tid, type_name_for_id(tid));
        }
    }

    for (int i = 0; i < gImportTypeCount; i++) {
        ImportTypeSummary *t = &gImportTypes[i];
        if (t->expected_count > 0 && t->parsed_count != t->expected_count) {
            ok = 0;
            import_add_summary_line("[%d] %s: parsed %d / expected %d",
                                    t->type_id, t->display_name,
                                    t->parsed_count, t->expected_count);
        }
    }

    if (!ok) {
        snprintf(out_msg, out_msg_sz, "Imported with warnings. Please re-copy table (missing/mismatch detected).");
        return 0;
    }

    gImportSummaryCount = 0;
    for (int i = 0; i < gImportTypeCount && gImportSummaryCount < IMPORT_SUMMARY_LINES; i++) {
        ImportTypeSummary *t = &gImportTypes[i];
        import_add_summary_line("[%d] %s | subjects=%d | total_tc=%d | passed_tc=%d",
                                t->type_id,
                                t->display_name,
                                t->parsed_count,
                                t->expected_total_credit,
                                t->expected_pass_credit);
    }
    snprintf(out_msg, out_msg_sz, "Import success: wrote %d subjects, %d types.", gImportRowCount, gImportTypeCount);
    return 1;
}

static int import_finalize_for_user(char *out_msg, int out_msg_sz);

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

static int import_finalize_for_user(char *out_msg, int out_msg_sz)
{
    if (gImportRowCount <= 0 || gImportTypeCount <= 0) {
        snprintf(out_msg, out_msg_sz, "Nothing to import. Parse data first.");
        return 0;
    }
    if (gImportUserLen <= 0) {
        snprintf(out_msg, out_msg_sz, "Please enter username.");
        return 0;
    }

    if (!write_subjects_from_import(out_msg, out_msg_sz)) return 0;
    if (!write_grad_modules_from_import(out_msg, out_msg_sz)) return 0;

    char db_path_local[128];
    snprintf(db_path_local, sizeof(db_path_local), "db_%s.db", gImportUserBuf);

    if (gDB) DB_Close();
    remove(db_path_local); /* overwrite if exists */

    snprintf(gUserName, sizeof(gUserName), "%s", gImportUserBuf);
    gNameLen = (int)strlen(gUserName);
    InitPlayerDB();
    if (!gDBReady) {
        snprintf(out_msg, out_msg_sz, "Failed to initialize DB for user %s", gImportUserBuf);
        return 0;
    }

    RefreshPlayer();

    gImportOpen = false;
    gImportStage = 0;
    snprintf(out_msg, out_msg_sz, "Import completed for user %s (types/subjects only, db overwritten).", gImportUserBuf);
    return 1;
}

/* --- Keyboard handler -------------------------------------------------- */
static void HandleKeyboard(void)
{
    if (gImportOpen) {
        if (IsKeyPressed(KEY_F2)) {
            gImportOpen = false;
            gImportStage = 0;
            return;
        }

        if (gImportStage == 0) {
            if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_V)) {
                const char *clip = GetClipboardText();
                if (clip && clip[0]) {
                    snprintf(gImportRaw, sizeof(gImportRaw), "%s", clip);
                    gImportRawLen = (int)strlen(gImportRaw);
                    gImportPasted = true;
                    gImportLastSuccess = false;
                    gImportStatusKind = 0;
                    snprintf(gImportStatusMsg, sizeof(gImportStatusMsg),
                             "Table pasted (%d chars). Press Enter to parse.", gImportRawLen);
                } else {
                    gImportStatusKind = -1;
                    snprintf(gImportStatusMsg, sizeof(gImportStatusMsg),
                             "Clipboard is empty. Copy full CTT-SIS table then press Ctrl+V.");
                }
                return;
            }

            if (IsKeyPressed(KEY_ENTER)) {
                int ok = import_parse_only(gImportStatusMsg, (int)sizeof(gImportStatusMsg));
                gImportStatusKind = ok ? 1 : -1;
                gImportLastSuccess = ok ? true : false;
                if (ok) gImportStage = 1;
                return;
            }

            if (IsKeyPressed(KEY_BACKSPACE)) {
                gImportPasted = false;
                gImportRaw[0] = '\0';
                gImportRawLen = 0;
                gImportLastSuccess = false;
                gImportStatusKind = 0;
                gImportSummaryCount = 0;
                snprintf(gImportStatusMsg, sizeof(gImportStatusMsg), "Paste cleared. Press Ctrl+V to paste again.");
                return;
            }
        } else if (gImportStage == 1) {
            if (IsKeyPressed(KEY_ENTER)) {
                gImportStage = 2;
                gImportUserLen = 0;
                gImportUserBuf[0] = '\0';
                gImportStatusKind = 0;
                snprintf(gImportStatusMsg, sizeof(gImportStatusMsg), "Enter username then press Enter to import types/subjects and overwrite DB if exists.");
                return;
            }
            if (IsKeyPressed(KEY_BACKSPACE)) {
                gImportStage = 0;
                gImportStatusKind = 0;
                snprintf(gImportStatusMsg, sizeof(gImportStatusMsg), "Back to paste step. Press Ctrl+V to re-copy.");
                return;
            }
        } else {
            if ((IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE)) && gImportUserLen > 0)
                gImportUserBuf[--gImportUserLen] = '\0';

            int ch;
            while ((ch = GetCharPressed()) != 0) {
                if (ch >= 32 && ch < 127 && gImportUserLen < 25) {
                    gImportUserBuf[gImportUserLen++] = (char)ch;
                    gImportUserBuf[gImportUserLen] = '\0';
                }
            }

            if (IsKeyPressed(KEY_ENTER)) {
                int ok = import_finalize_for_user(gImportStatusMsg, (int)sizeof(gImportStatusMsg));
                gImportStatusKind = ok ? 1 : -1;
                gImportLastSuccess = ok ? true : false;
                if (ok) {
                    strncpy(gResultMsg, gImportStatusMsg, sizeof(gResultMsg) - 1);
                    gResultMsg[sizeof(gResultMsg) - 1] = '\0';
                    gHasResult = true;
                    gResultShowUntil = (float)GetTime() + 5.f;
                }
                return;
            }
        }
        return;
    }

    /* Name-input phase: capture username before anything else */
    if (gNameInput) {
        if (IsKeyPressed(KEY_F2)) {
            gImportOpen = true;
            gImportStage = 0;
            gImportStatusKind = 0;
            gImportSummaryCount = 0;
            snprintf(gImportStatusMsg, sizeof(gImportStatusMsg),
                     "Copy table at CTT-SIS, then press Ctrl+V.");
            return;
        }
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
        if (gImportOpen) RenderImportSetupInput();
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
