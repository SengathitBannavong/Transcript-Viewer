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
 *   subjects       (id PK, code TEXT UNIQUE, name TEXT,
 *                   type_id INT, credit INT, term INT)
 *   subject_scores (subject_id PK, score_letter TEXT,
 *                   mid REAL, final REAL, pass INT, ever_studied INT)
 *
 * Public API
 * ──────────
 *   DB_Exists(username)        — check if db_<username>.db exists
 *   DB_Open(username)          — open (create new or reopen existing)
 *   DB_SeedFromDat()           — seed from assets/subjects.dat (new DB only)
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

/* Compute letter grade from 0.3*mid + 0.7*final */
static const char *db_compute_letter(float mid, float final_)
{
    if (mid == 0.f && final_ == 0.f) return "X";
    float g = 0.3f * mid + 0.7f * final_;
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
        "  code    TEXT NOT NULL UNIQUE,"
        "  name    TEXT NOT NULL,"
        "  type_id INTEGER NOT NULL REFERENCES subject_types(id),"
        "  credit  INTEGER NOT NULL DEFAULT 0,"
        "  term    INTEGER NOT NULL DEFAULT 0"
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
    FILE *f = fopen("assets/subjects.dat", "r");
    if (!f) { fprintf(stderr,"[db] Cannot open assets/subjects.dat\n"); return; }

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
        " WHERE subject_id=(SELECT id FROM subjects WHERE code=?);",
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

/* ─────────────────────────────────────────────────────────────────────
 * DB_Query — populate *player from the database
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
        } else {
            player->ToTal_credit_npass+=credit;
        }
    }
    sqlite3_finalize(stmt);

    player->status_can_grauate=(player->ToTal_credit_pass>=130)?1:0;
}

/* ─────────────────────────────────────────────────────────────────────
 * DB_UpdateScore
 * ───────────────────────────────────────────────────────────────────── */
int DB_UpdateScore(const char *code, float mid_, float final_)
{
    if(!gDB||!code) return 0;
    const char *letter=db_compute_letter(mid_,final_);
    int pass=db_is_pass(letter);
    int ever=(mid_>0||final_>0)?1:0;

    sqlite3_stmt *stmt=NULL;
    sqlite3_prepare_v2(gDB,
        "UPDATE subject_scores SET score_letter=?,mid=?,final=?,pass=?,ever_studied=?"
        " WHERE subject_id=(SELECT id FROM subjects WHERE code=?);",
        -1, &stmt, NULL);
    sqlite3_bind_text  (stmt,1,letter,-1,SQLITE_STATIC);
    sqlite3_bind_double(stmt,2,(double)mid_);
    sqlite3_bind_double(stmt,3,(double)final_);
    sqlite3_bind_int   (stmt,4,pass);
    sqlite3_bind_int   (stmt,5,ever);
    sqlite3_bind_text  (stmt,6,code,-1,SQLITE_TRANSIENT);
    int rc=sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return (rc==SQLITE_DONE)?sqlite3_changes(gDB):0;
}

/* ─────────────────────────────────────────────────────────────────────
 * DB_ClearScore
 * ───────────────────────────────────────────────────────────────────── */
int DB_ClearScore(const char *code){ return DB_UpdateScore(code,0.f,0.f); }

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
