/*
 * db.h — SQLite backend for the Student Transcript Viewer
 *
 * Included ONCE from main.c after the globals are declared.
 *
 * Database: db_<username>.db  (file in working directory)
 *
 * Schema
 * ──────
 *   subject_types  (id INTEGER PK, name TEXT)
 *   subjects       (id PK, code TEXT, name TEXT,
 *                   type_id INT, credit INT, term INT)
 *   subject_scores (subject_id PK, score_letter TEXT,
 *                   mid REAL, final REAL, pass INT, ever_studied INT)
 *
 * Public API
 * ──────────
 *   DB_Exists(username)        — check if db_<username>.db exists
 *   DB_Open(username)          — open (create new or reopen existing)
 *   DB_SeedFromDat()           — seed from assets/subjects.dat (new DB only)
 *   DB_SeedGradRules()         — seed grad_rules from assets/grad_config.cfg
 *   DB_LoadGradRules()         — fill gGradRules[] from DB (call after schema)
 *   DB_LoadScores()            — load from assets/tablejerry.txt (optional import)
 *   DB_Query(player)           — fill *player from DB
 *   DB_Close()                 — close handle
 *   DB_UpdateScore(code,m,f)   — UPDATE score row, return rows changed
 *   DB_ClearScore(code)        — reset to X / 0.0
 *   DB_SubjectExists(code)     — 1 if code is in subjects table
 */

#ifndef DB_H
#define DB_H

#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "struct_table.h"

static sqlite3 *gDB = NULL;

/* ── helpers ─────────────────────────────────────────────────────────── */
static void db_exec(const char *sql)
{
    char *err = NULL;
    if (sqlite3_exec(gDB, sql, NULL, NULL, &err) != SQLITE_OK) {
        fprintf(stderr, "[db] SQL: %s\n  %.80s\n", err, sql);
        sqlite3_free(err);
    }
}

/* Compute letter grade with explicit mid/final weight ratio */
static const char *db_compute_letter_r(float mid, float final_, float rm, float rf)
{
    if (mid == 0.f && final_ == 0.f) return "X";
    float g = rm * mid + rf * final_;
    if (g >= 9.0f) return "A+";
    if (g >= 8.5f) return "A";
    if (g >= 8.0f) return "B+";
    if (g >= 7.0f) return "B";
    if (g >= 6.5f) return "C+";
    if (g >= 5.5f) return "C";
    if (g >= 5.0f) return "D+";
    if (g >= 4.0f) return "D";
    return "F";
}

static int db_is_pass(const char *l)
{
    if (!l || l[0]=='X' || l[0]=='F') return 0;
    return 1;
}

/* Build path "db_<username>.db" into buf (max bufsz) */
static void db_path(const char *username, char *buf, int bufsz)
{
    snprintf(buf, bufsz, "db_%s.db", username);
}

/* ─────────────────────────────────────────────────────────────────────
 * DB_Exists — returns 1 if db_<username>.db file exists
 * ───────────────────────────────────────────────────────────────────── */
int DB_Exists(const char *username)
{
    char path[128];
    db_path(username, path, sizeof(path));
    struct stat st;
    return (stat(path, &st) == 0) ? 1 : 0;
}

/* ─────────────────────────────────────────────────────────────────────
 * DB_Open — open (or create) db_<username>.db
 *           Returns 1 on success.
 * ───────────────────────────────────────────────────────────────────── */
int DB_Open(const char *username)
{
    char path[128];
    db_path(username, path, sizeof(path));

    if (sqlite3_open(path, &gDB) != SQLITE_OK) {
        fprintf(stderr, "[db] Cannot open '%s': %s\n",
                path, sqlite3_errmsg(gDB));
        return 0;
    }
    db_exec("PRAGMA journal_mode=WAL;");
    db_exec("PRAGMA foreign_keys=ON;");
    return 1;
}

/* ─────────────────────────────────────────────────────────────────────
 * DB_CreateSchema — create tables if they don't exist yet
 * ───────────────────────────────────────────────────────────────────── */
void DB_CreateSchema(void)
{
    db_exec(
        "CREATE TABLE IF NOT EXISTS subject_types("
        "  id   INTEGER PRIMARY KEY,"
        "  name TEXT NOT NULL"
        ");"
    );
    db_exec(
        "CREATE TABLE IF NOT EXISTS subjects("
        "  id      INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  code    TEXT NOT NULL,"
        "  name    TEXT NOT NULL,"
        "  type_id INTEGER NOT NULL REFERENCES subject_types(id),"
        "  credit  INTEGER NOT NULL DEFAULT 0,"
        "  term    INTEGER NOT NULL DEFAULT 0,"
        "  UNIQUE(code, type_id)"
        ");"
    );
    db_exec(
        "CREATE TABLE IF NOT EXISTS subject_scores("
        "  subject_id   INTEGER PRIMARY KEY REFERENCES subjects(id),"
        "  score_letter TEXT    NOT NULL DEFAULT 'X',"
        "  mid          REAL    NOT NULL DEFAULT 0.0,"
        "  final        REAL    NOT NULL DEFAULT 0.0,"
        "  pass         INTEGER NOT NULL DEFAULT 0,"
        "  ever_studied INTEGER NOT NULL DEFAULT 0"
        ");"
    );
    db_exec(
        "CREATE TABLE IF NOT EXISTS grad_rules("
        "  type_id   INTEGER PRIMARY KEY,"
        "  mode      INTEGER NOT NULL DEFAULT 0,"
        "  limit_val INTEGER NOT NULL DEFAULT 0,"
        "  group_id  INTEGER NOT NULL DEFAULT 0"
        ");"
    );
}

/* ─────────────────────────────────────────────────────────────────────
 * DB_SeedFromDat — parse assets/subjects.dat and insert subjects.
 *   Called once when creating a new DB.
 *   Format per data line:  CODE  TERM  CREDIT  Subject Name
 *   Section headers:       [N] type_name
 *   Comment/blank lines:   # or empty — skip
 * ───────────────────────────────────────────────────────────────────── */
void DB_SeedFromDat(void)
{
    /* open subjects.dat — type names are read from [N] headers inline */
    FILE *f = fopen(gSubjectsDatPath, "r");
    if (!f) { fprintf(stderr,"[db] Cannot open %s\n", gSubjectsDatPath); return; }

    sqlite3_stmt *stType = NULL;
    sqlite3_prepare_v2(gDB,
        "INSERT OR IGNORE INTO subject_types(id,name) VALUES(?,?);",
        -1, &stType, NULL);

    sqlite3_stmt *stSubj = NULL, *stScore = NULL;
    sqlite3_prepare_v2(gDB,
        "INSERT OR IGNORE INTO subjects(code,name,type_id,credit,term)"
        " VALUES(?,?,?,?,?);",
        -1, &stSubj, NULL);
    sqlite3_prepare_v2(gDB,
        "INSERT OR IGNORE INTO subject_scores(subject_id,score_letter,mid,final,pass,ever_studied)"
        " SELECT id,'X',0.0,0.0,0,0 FROM subjects WHERE code=?;",
        -1, &stScore, NULL);

    db_exec("BEGIN;");

    char line[512];
    int  cur_type = -1;
    while (fgets(line, sizeof(line), f)) {
        /* trim newline */
        int len = (int)strlen(line);
        while (len > 0 && (line[len-1]=='\n'||line[len-1]=='\r')) line[--len]='\0';

        /* skip blank and comment lines */
        if (len == 0 || line[0] == '#') continue;

        /* section header: "[N] Display Name" — seed subject_types from here */
        if (line[0] == '[') {
            char type_display[64] = {0};
            if (sscanf(line, "[%d] %63[^\n]", &cur_type, type_display) >= 1) {
                if (type_display[0] != '\0') {
                    sqlite3_bind_int (stType, 1, cur_type);
                    sqlite3_bind_text(stType, 2, type_display, -1, SQLITE_TRANSIENT);
                    sqlite3_step(stType);
                    sqlite3_reset(stType);
                }
            }
            continue;
        }

        if (cur_type < 0) continue;

        /* data line: CODE  TERM  CREDIT  Subject Name */
        char code[MAXSIZEID]   = {0};
        int  term = 0, credit = 0;
        char name[MAXSIZENAME] = {0};

        /* read CODE */
        int si = 0;
        int ci = 0;
        while (line[si] && line[si] != ' ' && line[si] != '\t' && ci < MAXSIZEID-1)
            code[ci++] = line[si++];
        if (ci == 0) continue;

        /* skip whitespace */
        while (line[si]==' '||line[si]=='\t') si++;
        /* read TERM */
        term = (int)strtol(line + si, NULL, 10);
        while (line[si] && line[si]!=' ' && line[si]!='\t') si++;
        while (line[si]==' '||line[si]=='\t') si++;
        /* read CREDIT */
        credit = (int)strtol(line + si, NULL, 10);
        while (line[si] && line[si]!=' ' && line[si]!='\t') si++;
        while (line[si]==' '||line[si]=='\t') si++;
        /* rest = name */
        strncpy(name, line + si, MAXSIZENAME-1);

        sqlite3_bind_text(stSubj, 1, code,   -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stSubj, 2, name,   -1, SQLITE_TRANSIENT);
        sqlite3_bind_int (stSubj, 3, cur_type);
        sqlite3_bind_int (stSubj, 4, credit);
        sqlite3_bind_int (stSubj, 5, term);
        sqlite3_step(stSubj);
        sqlite3_reset(stSubj);

        sqlite3_bind_text(stScore, 1, code, -1, SQLITE_TRANSIENT);
        sqlite3_step(stScore);
        sqlite3_reset(stScore);
    }

    db_exec("COMMIT;");
    sqlite3_finalize(stType);
    sqlite3_finalize(stSubj);
    sqlite3_finalize(stScore);
    fclose(f);
}

/* ─────────────────────────────────────────────────────────────────────
 * DB_LoadScores — parse assets/tabletest.txt pipe-table and seed scores.
 *   Optional: called when importing an existing score sheet.
 * ───────────────────────────────────────────────────────────────────── */
void DB_LoadScores_Test(void)
{
    FILE *f = fopen("assets/tabletest.txt", "r");
    if (!f) return;

    sqlite3_stmt *stmt = NULL;
    sqlite3_prepare_v2(gDB,
        "UPDATE subject_scores SET score_letter=?,mid=?,final=?,pass=?,ever_studied=?"
        " WHERE subject_id IN (SELECT id FROM subjects WHERE code=?);",
        -1, &stmt, NULL);

    char line[512];
    while (fgets(line, sizeof(line), f)) {
        int ll=(int)strlen(line);
        while(ll>0&&(line[ll-1]=='\n'||line[ll-1]=='\r')) line[--ll]='\0';
        if(line[0]!='|') continue;

        /* tokenise on '|' */
        char *tok[12]; int nt=0;
        char *p=line;
        while(*p && nt<12) {
            if(*p=='|') {
                p++;
                while(*p==' ') p++;
                tok[nt++]=p;
                char *e=strchr(p,'|');
                if(e){ char *e2=e-1; while(e2>=p&&*e2==' ') e2--; *(e2+1)='\0'; p=e; }
                else break;
            } else p++;
        }
        if(nt<7) continue;

        const char *code=tok[1], *letter=tok[2], *smid=tok[3],
                   *sfin=tok[4], *spass=tok[5];

        if(!code||code[0]=='-'||code[0]=='\0') continue;
        if(strncmp(code,"ID",2)==0) continue;
        if(strcmp(letter,"Score")==0) continue;

        float mid_=(float)atof(smid), fin_=(float)atof(sfin);
        int   pass=(spass&&strcmp(spass,"YES")==0)?1:0;
        int   ever=(mid_>0||fin_>0)?1:0;
        if(letter&&letter[0]=='X'){ever=0;pass=0;}

        sqlite3_bind_text  (stmt,1,letter?letter:"X",-1,SQLITE_TRANSIENT);
        sqlite3_bind_double(stmt,2,(double)mid_);
        sqlite3_bind_double(stmt,3,(double)fin_);
        sqlite3_bind_int   (stmt,4,pass);
        sqlite3_bind_int   (stmt,5,ever);
        sqlite3_bind_text  (stmt,6,code,-1,SQLITE_TRANSIENT);
        sqlite3_step(stmt);
        sqlite3_reset(stmt);
    }

    sqlite3_finalize(stmt);
    fclose(f);
}

/* ───────────────────────────────────────────────────────────────────── * DB_SeedGradRules — parse assets/grad_config.cfg and insert into grad_rules.
 *   Uses INSERT OR IGNORE, so safe to call on existing DBs.
 *   Falls back to hardcoded defaults if the cfg file is missing.
 *   Format per data line:  type_id  mode  limit_val  group_id
 * ───────────────────────────────────────────────────────────────────────── */
void DB_SeedGradRules(void)
{
    /* Fallback defaults matching the IT major layout (sizeSubjectType=14, IDs 1-13)
     * Index 0 is the unused reserved slot. */
    static const int def_mode [sizeSubjectType] = {
        0,           /* [0] unused                  */
        0,0,2,0,1,   /* [1-5]  fixed required types */
        0,0,0,0,0,0, /* [6-11] module slots         */
        0,0          /* [12-13] thuc_tap, do_an     */
    };
    static const int def_limit[sizeSubjectType] = {
        0,
        0,0,4,0,9,
        0,0,0,0,0,0,
        0,0
    };
    static const int def_group[sizeSubjectType] = {
        0,
        0,0,0,0,0,
        1,1,1,2,2,0,
        0,0
    };

    sqlite3_stmt *stmt = NULL;
    sqlite3_prepare_v2(gDB,
        "INSERT OR IGNORE INTO grad_rules(type_id,mode,limit_val,group_id)"
        " VALUES(?,?,?,?);",
        -1, &stmt, NULL);

    FILE *f = fopen(gGradCfgPath, "r");
    db_exec("BEGIN;");
    if (!f) {
        /* no cfg — seed defaults */
        for (int i = 0; i < sizeSubjectType; i++) {
            sqlite3_bind_int(stmt, 1, i);
            sqlite3_bind_int(stmt, 2, def_mode[i]);
            sqlite3_bind_int(stmt, 3, def_limit[i]);
            sqlite3_bind_int(stmt, 4, def_group[i]);
            sqlite3_step(stmt);
            sqlite3_reset(stmt);
        }
    } else {
        char line[256];
        while (fgets(line, sizeof(line), f)) {
            int len = (int)strlen(line);
            while (len > 0 && (line[len-1]=='\n'||line[len-1]=='\r')) line[--len]='\0';
            /* skip leading whitespace, then comments and blanks */
            int si = 0;
            while (line[si]==' '||line[si]=='\t') si++;
            if (line[si]=='#' || line[si]=='\0') continue;
            int type_id=0, mode=0, limit_val=0, group_id=0;
            if (sscanf(line+si, "%d %d %d %d",
                       &type_id, &mode, &limit_val, &group_id) < 2) continue;
            if (type_id < 0 || type_id >= sizeSubjectType) continue;
            sqlite3_bind_int(stmt, 1, type_id);
            sqlite3_bind_int(stmt, 2, mode);
            sqlite3_bind_int(stmt, 3, limit_val);
            sqlite3_bind_int(stmt, 4, group_id);
            sqlite3_step(stmt);
            sqlite3_reset(stmt);
        }
        fclose(f);
    }
    db_exec("COMMIT;");
    sqlite3_finalize(stmt);
}

/* ─────────────────────────────────────────────────────────────────────────
 * DB_LoadGradRules — read grad_rules table into gGradRules[].
 *   Call after DB_CreateSchema() (and DB_SeedGradRules()) on every DB open.
 * ───────────────────────────────────────────────────────────────────────── */
void DB_LoadGradRules(void)
{
    memset(gGradRules, 0, sizeof(gGradRules));
    sqlite3_stmt *stmt = NULL;
    sqlite3_prepare_v2(gDB,
        "SELECT type_id, mode, limit_val, group_id FROM grad_rules;",
        -1, &stmt, NULL);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int tid = sqlite3_column_int(stmt, 0);
        if (tid < 0 || tid >= sizeSubjectType) continue;
        gGradRules[tid].mode      = sqlite3_column_int(stmt, 1);
        gGradRules[tid].limit_val = sqlite3_column_int(stmt, 2);
        gGradRules[tid].group_id  = sqlite3_column_int(stmt, 3);
    }
    sqlite3_finalize(stmt);
}

/* forward declaration — defined later in this file */
void DB_ValidateData(void);

/* ─────────────────────────────────────────────────────────────────────────
 * DB_ReloadData — full wipe-and-reseed of subject + rule tables.
 *
 *   Strategy (preserves existing scores):
 *     1. Snapshot current scores to a temp table keyed by subject code.
 *     2. DELETE all rows from subject_scores, subjects, subject_types,
 *        grad_rules — tables are now empty.
 *     3. Run DB_SeedFromDat() and DB_SeedGradRules() as if it were a
 *        brand-new DB.  Every row is inserted fresh from the asset files.
 *     4. Restore saved scores into the newly created subject_scores rows
 *        (matched by subject code).  Subjects that were removed from
 *        subjects.dat simply lose their snapshot — they no longer exist.
 *     5. Reload in-RAM state: gTypeName[], gGradRules[], gDataWarnings.
 *
 *   Net effect: the DB always exactly mirrors the asset files after a
 *   reload, with no stale/orphaned rows, while retaining every grade the
 *   user has recorded.
 * ───────────────────────────────────────────────────────────────────────── */
void DB_ReloadData(void)
{
    /* ── 1. Snapshot scores by subject code ─────────────────────────── */
    db_exec(
        "CREATE TEMP TABLE IF NOT EXISTS _reload_scores("
        "  code         TEXT PRIMARY KEY,"
        "  score_letter TEXT,"
        "  mid          REAL,"
        "  final        REAL,"
        "  pass         INTEGER,"
        "  ever_studied INTEGER"
        ");"
    );
    db_exec("DELETE FROM _reload_scores;");
    db_exec(
        "INSERT INTO _reload_scores "
        "SELECT s.code, sc.score_letter, sc.mid, sc.final, sc.pass, sc.ever_studied "
        "  FROM subjects s JOIN subject_scores sc ON sc.subject_id = s.id;"
    );

    /* ── 2. Wipe all data tables ─────────────────────────────────────── */
    db_exec("DELETE FROM subject_scores;");
    db_exec("DELETE FROM subjects;");
    db_exec("DELETE FROM subject_types;");
    db_exec("DELETE FROM grad_rules;");

    /* ── 3. Re-seed fresh from asset files ───────────────────────────── */
    DB_SeedFromDat();
    DB_SeedGradRules();

    /* ── 4. Restore saved scores onto the freshly seeded subjects ────── */
    {
        sqlite3_stmt *sel = NULL, *upd = NULL;
        sqlite3_prepare_v2(gDB,
            "SELECT code, score_letter, mid, final, pass, ever_studied"
            "  FROM _reload_scores;",
            -1, &sel, NULL);
        sqlite3_prepare_v2(gDB,
            "UPDATE subject_scores"
            "  SET score_letter=?, mid=?, final=?, pass=?, ever_studied=?"
            "  WHERE subject_id IN (SELECT id FROM subjects WHERE code=?);",
            -1, &upd, NULL);
        db_exec("BEGIN;");
        while (sqlite3_step(sel) == SQLITE_ROW) {
            const char *code = (const char*)sqlite3_column_text(sel, 0);
            const char *ltr  = (const char*)sqlite3_column_text(sel, 1);
            double      mid  = sqlite3_column_double(sel, 2);
            double      fin  = sqlite3_column_double(sel, 3);
            int         pass = sqlite3_column_int  (sel, 4);
            int         ever = sqlite3_column_int  (sel, 5);
            /* skip subjects that were never studied — keep fresh default */
            if (ever == 0 && pass == 0 && (!ltr || strcmp(ltr, "X") == 0)) continue;
            sqlite3_bind_text  (upd, 1, ltr,  -1, SQLITE_TRANSIENT);
            sqlite3_bind_double(upd, 2, mid);
            sqlite3_bind_double(upd, 3, fin);
            sqlite3_bind_int   (upd, 4, pass);
            sqlite3_bind_int   (upd, 5, ever);
            sqlite3_bind_text  (upd, 6, code, -1, SQLITE_TRANSIENT);
            sqlite3_step(upd);
            sqlite3_reset(upd);
        }
        db_exec("COMMIT;");
        sqlite3_finalize(sel);
        sqlite3_finalize(upd);
    }
    db_exec("DROP TABLE IF EXISTS _reload_scores;");

    /* ── 5. Refresh in-RAM state ─────────────────────────────────────── */
    memset(gTypeName, 0, sizeof(gTypeName));
    DB_LoadGradRules();
    DB_ValidateData();
}

/* ─────────────────────────────────────────────────────────────────────────
 * DB_ValidateData — cross-check subjects.dat vs grad_config.cfg.
 *   Checks:  1. All required IDs {1,2,3,4,5,12,13} exist in subject_types.
 *            2. All required IDs exist in grad_rules.
 *            3. Every ID in grad_rules also exists in subject_types.
 *            4. Every ID in subject_types also exists in grad_rules.
 *   Results are packed as NUL-terminated strings into gDataWarningsBuf.
 *   gDataWarnCount is set to the number of warnings (0 = all OK).
 * ───────────────────────────────────────────────────────────────────────── */
void DB_ValidateData(void)
{
    static const int required[] = {1, 2, 3, 4, 5, 12, 13};
    static const int nreq = (int)(sizeof(required) / sizeof(required[0]));
    static const char *req_names[] = {
        "co_so_nganh", "dai_cuong", "the_thao",
        "ly_luat_chinh_tri", "tu_chon", "thuc_tap", "do_an_tot_nghiep"
    };

    memset(gDataWarningsBuf, 0, sizeof(gDataWarningsBuf));
    gDataWarnCount = 0;
    int pos = 0;

#define _DV_WARN(fmt, ...) do { \
    int _n = snprintf(gDataWarningsBuf + pos, DATA_WARN_BUF - pos - 1, fmt, ##__VA_ARGS__); \
    if (_n > 0 && pos + _n < DATA_WARN_BUF - 1) { pos += _n + 1; gDataWarnCount++; } \
} while(0)

    int in_types[sizeSubjectType], in_rules[sizeSubjectType];
    memset(in_types, 0, sizeof(in_types));
    memset(in_rules, 0, sizeof(in_rules));

    sqlite3_stmt *st = NULL;
    sqlite3_prepare_v2(gDB, "SELECT id FROM subject_types;", -1, &st, NULL);
    while (sqlite3_step(st) == SQLITE_ROW) {
        int id = sqlite3_column_int(st, 0);
        if (id > 0 && id < sizeSubjectType) in_types[id] = 1;
    }
    sqlite3_finalize(st);

    sqlite3_prepare_v2(gDB, "SELECT type_id FROM grad_rules;", -1, &st, NULL);
    while (sqlite3_step(st) == SQLITE_ROW) {
        int id = sqlite3_column_int(st, 0);
        if (id > 0 && id < sizeSubjectType) in_rules[id] = 1;
    }
    sqlite3_finalize(st);

    /* 1+2. Required IDs must exist in both tables */
    for (int k = 0; k < nreq; k++) {
        int id = required[k];
        if (!in_types[id])
            _DV_WARN("subjects.dat: missing required section [%d] (%s)", id, req_names[k]);
        if (!in_rules[id])
            _DV_WARN("grad_config.cfg: missing required entry for id %d (%s)", id, req_names[k]);
    }

    /* 3. grad_rules has a rule for a type not in subject_types */
    for (int i = 1; i < sizeSubjectType; i++) {
        if (in_rules[i] && !in_types[i])
            _DV_WARN("grad_config.cfg id %d has no matching section in subjects.dat", i);
    }

    /* 4. subject_types has a type not covered by grad_rules */
    for (int i = 1; i < sizeSubjectType; i++) {
        if (in_types[i] && !in_rules[i])
            _DV_WARN("subjects.dat [%d] has no matching rule in grad_config.cfg", i);
    }

#undef _DV_WARN
}

/* ───────────────────────────────────────────────────────────────────────── * DB_Query — populate *player from the database
 * ───────────────────────────────────────────────────────────────────── */
void DB_Query(Player *player)
{
    if (!gDB || !player) return;

    /* free old linked lists */
    for (int t=0; t<sizeSubjectType; t++) {
        Subject_Node *cur=player->numofSubjectType[t].head;
        while(cur){ Subject_Node *nx=cur->next; free(cur); cur=nx; }
        memset(&player->numofSubjectType[t], 0, sizeof(Subject_Type));
    }
    player->ToTal_credit_pass  = 0;
    player->ToTal_credit_npass = 0;
    player->status_can_grauate = 0;
    player->status_alert       = 0;

    /* Load type names from subject_types table → fills gTypeName[] and nameoftype */
    {
        sqlite3_stmt *stTN = NULL;
        sqlite3_prepare_v2(gDB,
            "SELECT id, name FROM subject_types ORDER BY id;",
            -1, &stTN, NULL);
        while (sqlite3_step(stTN) == SQLITE_ROW) {
            int         tid  = sqlite3_column_int (stTN, 0);
            const char *tnam = (const char*)sqlite3_column_text(stTN, 1);
            if (tid >= 0 && tid < sizeSubjectType && tnam) {
                snprintf(gTypeName[tid], 64, "%s", tnam);
                strncpy(player->numofSubjectType[tid].nameoftype, tnam, MAXSIZENAME-1);
            }
        }
        sqlite3_finalize(stTN);
    }

    sqlite3_stmt *stmt=NULL;
    sqlite3_prepare_v2(gDB,
        "SELECT s.type_id,s.code,s.name,s.credit,s.term,"
        "       sc.score_letter,sc.mid,sc.final,sc.pass,sc.ever_studied"
        "  FROM subjects s JOIN subject_scores sc ON sc.subject_id=s.id"
        " ORDER BY s.type_id, s.id;",
        -1, &stmt, NULL);

    while(sqlite3_step(stmt)==SQLITE_ROW) {
        int         tid    = sqlite3_column_int   (stmt,0);
        const char *code   = (const char*)sqlite3_column_text(stmt,1);
        const char *name   = (const char*)sqlite3_column_text(stmt,2);
        int         credit = sqlite3_column_int   (stmt,3);
        int         term   = sqlite3_column_int   (stmt,4);
        const char *letter = (const char*)sqlite3_column_text(stmt,5);
        float       mid    = (float)sqlite3_column_double(stmt,6);
        float       fin    = (float)sqlite3_column_double(stmt,7);
        int         pass   = sqlite3_column_int   (stmt,8);
        int         studied= sqlite3_column_int   (stmt,9);

        if(tid<0||tid>=sizeSubjectType) continue;
        Subject_Type *st = &player->numofSubjectType[tid];

        Subject_Node *node=(Subject_Node*)calloc(1,sizeof(Subject_Node));
        if(!node) continue;
        strncpy(node->name, name?name:"", MAXSIZENAME-1);
        strncpy(node->ID,   code?code:"", MAXSIZEID-1);
        node->score_letter           = letter&&letter[0]?letter[0]:'X';
        node->score_number_mid       = mid;
        node->score_number_final     = fin;
        node->status_pass            = (unsigned)pass;
        /* bit0=ever_studied, bit1=has '+' modifier */
        node->status_ever_been_study = (unsigned)(studied&1)
                                     | (unsigned)((letter&&letter[1]=='+')?2:0);
        node->credit                 = (unsigned)credit;
        node->term_recomment_to_studie=(unsigned)term;
        node->next=NULL;

        if(!st->head){st->head=st->tail=node;}
        else{st->tail->next=node;st->tail=node;}

        st->Total_Subject++;
        st->Total_Credit+=credit;
        if(pass){
            st->count_passSubject++;
            st->count_passCredit+=(unsigned)credit;
            player->ToTal_credit_pass+=credit;
        } else if (studied & 1) {
            /* only count non-pass credits for subjects that were actually studied */
            player->ToTal_credit_npass+=credit;
        }
    }
    sqlite3_finalize(stmt);
    /* Note: status_can_grauate and status_alert are computed by
       update_player_status() in score_logic.h — call it after DB_Query(). */
}

/* ─────────────────────────────────────────────────────────────────────
 * DB_UpdateScoreRatio — update score with explicit mid/final weight ratio
 *   ratio_sel: 1 = 50/50,  2 = 40/60,  3 (or default) = 30/70
 * ───────────────────────────────────────────────────────────────────── */
int DB_UpdateScoreRatio(const char *code, float mid_, float final_, int ratio_sel)
{
    if (!gDB || !code) return 0;
    float rm, rf;
    switch (ratio_sel) {
        case 1:  rm = 0.5f; rf = 0.5f; break;  /* 50 / 50 */
        case 2:  rm = 0.4f; rf = 0.6f; break;  /* 40 / 60 */
        default: rm = 0.3f; rf = 0.7f; break;  /* 30 / 70 */
    }
    const char *letter = db_compute_letter_r(mid_, final_, rm, rf);
    int pass = db_is_pass(letter);
    int ever = (mid_ > 0 || final_ > 0) ? 1 : 0;

    sqlite3_stmt *stmt = NULL;
    sqlite3_prepare_v2(gDB,
        "UPDATE subject_scores SET score_letter=?,mid=?,final=?,pass=?,ever_studied=?"
        " WHERE subject_id IN (SELECT id FROM subjects WHERE code=?);",
        -1, &stmt, NULL);
    sqlite3_bind_text  (stmt, 1, letter, -1, SQLITE_STATIC);
    sqlite3_bind_double(stmt, 2, (double)mid_);
    sqlite3_bind_double(stmt, 3, (double)final_);
    sqlite3_bind_int   (stmt, 4, pass);
    sqlite3_bind_int   (stmt, 5, ever);
    sqlite3_bind_text  (stmt, 6, code, -1, SQLITE_TRANSIENT);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return (rc == SQLITE_DONE) ? sqlite3_changes(gDB) : 0;
}

/* ─────────────────────────────────────────────────────────────────────
 * DB_UpdateScore — convenience wrapper using default 30/70 ratio
 * ───────────────────────────────────────────────────────────────────── */
int DB_UpdateScore(const char *code, float mid_, float final_)
{
    return DB_UpdateScoreRatio(code, mid_, final_, 3);
}

/* ─────────────────────────────────────────────────────────────────────
 * DB_ClearScore
 * ───────────────────────────────────────────────────────────────────── */
int DB_ClearScore(const char *code) { return DB_UpdateScoreRatio(code, 0.f, 0.f, 3); }

/* ─────────────────────────────────────────────────────────────────────
 * DB_SubjectExists
 * ───────────────────────────────────────────────────────────────────── */
int DB_SubjectExists(const char *code)
{
    if(!gDB||!code) return 0;
    sqlite3_stmt *stmt=NULL;
    sqlite3_prepare_v2(gDB,
        "SELECT COUNT(*) FROM subjects WHERE code=?;",
        -1, &stmt, NULL);
    sqlite3_bind_text(stmt,1,code,-1,SQLITE_TRANSIENT);
    int n=0;
    if(sqlite3_step(stmt)==SQLITE_ROW) n=sqlite3_column_int(stmt,0);
    sqlite3_finalize(stmt);
    return n>0;
}

/* ─────────────────────────────────────────────────────────────────────
 * DB_Close
 * ───────────────────────────────────────────────────────────────────── */
void DB_Close(void)
{
    if(gDB){ sqlite3_close(gDB); gDB=NULL; }
}

#endif /* DB_H */
