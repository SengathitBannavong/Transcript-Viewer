/*
 * test_logic.c — Unit tests for score_logic.h and DB_ValidateData
 *
 * Build:  make test
 * Run:    ./bin/test_logic
 *
 * No Raylib/Clay dependency — compiles with only -lsqlite3 -lm.
 *
 * Test groups
 * ───────────
 *   score_to_gpa          — letter+modifier → 4-scale GPA
 *   calc_status_alert     — failed credits → alert level 0-3
 *   calc_cpa              — cumulative GPA, pass-only vs all-studied
 *   calc_missing_types    — required types with no subjects flagged;
 *                           module slots never flagged
 *   calc_effective_credits — module pick-best + sport exclusion
 *   calc_required_credits  — module group counted once
 *   update_player_status  — graduation pass/fail matrix
 *   DB_ValidateData       — data-file cross-check warnings
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* Pull in the same header stack as main.c (no Raylib needed here) */
#include "app_data.h"     /* gPlayer, gTypeName, gGradRules, gDataWarnings… */

/* db.h expects runtime-configurable asset paths from main.c; provide test defaults. */
static char gSubjectsDatPath[512] = "assets/subjects.dat";
static char gGradCfgPath[512] = "assets/grad_config.cfg";

#include "db.h"           /* gDB, DB_CreateSchema, DB_ValidateData, … */
#include "score_logic.h"  /* all score / graduation calculation functions   */

/* ══════════════════════════════════════════════════════════════════════
 * Tiny test framework
 * ══════════════════════════════════════════════════════════════════════ */
static int t_pass = 0, t_fail = 0;
static const char *t_group = "";

#define GROUP(name)  do { t_group = (name); \
    printf("\n[%s]\n", t_group); } while(0)

#define CHECK(cond, msg) do { \
    if (cond) { printf("  PASS  %s\n", msg); t_pass++; } \
    else      { printf("  FAIL  %s\n", msg); t_fail++; } \
} while(0)

#define FCHECK(a, b, msg) \
    CHECK(fabsf((float)(a) - (float)(b)) < 0.001f, msg)

/* ══════════════════════════════════════════════════════════════════════
 * Helpers
 * ══════════════════════════════════════════════════════════════════════ */

/* Pool of reusable Subject_Node instances for building linked lists */
static Subject_Node node_pool[128];
static int          node_pool_used = 0;

static void node_pool_reset(void) { node_pool_used = 0; }

/* Allocate a Subject_Node from the pool.
 *   letter  — score letter ('A','B','C','D','F')
 *   plus    — 1 if grade has '+' modifier
 *   credit  — credit weight of this subject
 *   studied — 1 if ever studied (affects CPA denominator)
 *   passed  — 1 if subject was passed */
static Subject_Node *make_node(char letter, int plus, int credit,
                                int studied, int passed)
{
    Subject_Node *n = &node_pool[node_pool_used++];
    memset(n, 0, sizeof(*n));
    n->score_letter          = letter;
    n->credit                = (unsigned)credit;
    n->status_pass           = passed ? 1u : 0u;
    /* bit 0 = ever_studied, bit 1 = plus modifier */
    n->status_ever_been_study = (unsigned)(studied ? 1 : 0) |
                                (unsigned)(plus    ? 2 : 0);
    n->next = NULL;
    return n;
}

/* Append node to a Subject_Type's linked list and keep counters in sync */
static void type_add(Subject_Type *st, Subject_Node *n)
{
    if (!st->head) st->head = n;
    else           st->tail->next = n;
    st->tail = n;
    st->Total_Subject++;
    if (n->status_pass) {
        st->count_passSubject++;
        st->count_passCredit += (unsigned)n->credit;
    }
    st->Total_Credit += (int)n->credit;
}

/* ── Set up the "real IT major" graduation rules in gGradRules[] ──────
 *  Mirrors what grad_config.cfg + DB_LoadGradRules() would produce.    */
static void setup_real_grad_rules(void)
{
    memset(gGradRules, 0, sizeof(gGradRules));
    /* [1] co_so_nganh   — all credits */
    gGradRules[co_so_nganh].mode = GRAD_TOTAL_CREDIT;
    /* [2] dai_cuong      — all credits */
    gGradRules[dai_cuong].mode = GRAD_TOTAL_CREDIT;
    /* [3] the_thao       — 4 subjects */
    gGradRules[the_thao].mode      = GRAD_SUBJECT_COUNT;
    gGradRules[the_thao].limit_val = 4;
    /* [4] ly_luat_chinh_tri — all credits */
    gGradRules[ly_luat_chinh_tri].mode = GRAD_TOTAL_CREDIT;
    /* [5] tu_chon        — 9 credits (fixed) */
    gGradRules[tu_chon].mode      = GRAD_FIXED;
    gGradRules[tu_chon].limit_val = 9;
    /* [6-8] module group 1 (pick best) — all credits of that module */
    gGradRules[modunI  ].mode = GRAD_TOTAL_CREDIT; gGradRules[modunI  ].group_id = 1;
    gGradRules[modunII ].mode = GRAD_TOTAL_CREDIT; gGradRules[modunII ].group_id = 1;
    gGradRules[modunIII].mode = GRAD_TOTAL_CREDIT; gGradRules[modunIII].group_id = 1;
    /* [9-10] module group 2 (pick best) */
    gGradRules[modunIV].mode = GRAD_TOTAL_CREDIT; gGradRules[modunIV].group_id = 2;
    gGradRules[modunV ].mode = GRAD_TOTAL_CREDIT; gGradRules[modunV ].group_id = 2;
    /* [12] thuc_tap       — all credits */
    gGradRules[thuc_tap].mode = GRAD_TOTAL_CREDIT;
    /* [13] do_an_tot_nghiep — all credits */
    gGradRules[do_an_tot_nghiep].mode = GRAD_TOTAL_CREDIT;
}

/* Build a player that meets ALL graduation requirements.
 *  All required types have subjects, CPA = 4.0.
 *  Must call setup_real_grad_rules() first.                             */
static void setup_passing_player(Player *p)
{
    node_pool_reset();
    memset(p, 0, sizeof(*p));

    /* [1] co_so_nganh: 3 subjects × 3c, all passed, Total_Credit = 9 */
    for (int i = 0; i < 3; i++) {
        type_add(&p->numofSubjectType[co_so_nganh],
                 make_node('A', 0, 3, 1, 1));
    }
    p->numofSubjectType[co_so_nganh].Total_Credit = 9;

    /* [2] dai_cuong: 6 subjects × 3c = 18c, all passed */
    for (int i = 0; i < 6; i++) {
        type_add(&p->numofSubjectType[dai_cuong],
                 make_node('B', 0, 3, 1, 1));
    }
    p->numofSubjectType[dai_cuong].Total_Credit = 18;

    /* [3] the_thao: 4 subjects (0c each), all passed */
    for (int i = 0; i < 4; i++) {
        type_add(&p->numofSubjectType[the_thao],
                 make_node('A', 0, 0, 1, 1));
    }

    /* [4] ly_luat_chinh_tri: 5 subjects × 2c = 10c, all passed */
    for (int i = 0; i < 5; i++) {
        type_add(&p->numofSubjectType[ly_luat_chinh_tri],
                 make_node('A', 0, 2, 1, 1));
    }
    p->numofSubjectType[ly_luat_chinh_tri].Total_Credit = 10;

    /* [5] tu_chon: 3 × 3c = 9c, all passed (exactly the GRAD_FIXED limit) */
    for (int i = 0; i < 3; i++) {
        type_add(&p->numofSubjectType[tu_chon],
                 make_node('A', 0, 3, 1, 1));
    }
    p->numofSubjectType[tu_chon].Total_Credit = 9;

    /* [6] modunI (group 1): 5 × 4c = 20c, all passed */
    for (int i = 0; i < 5; i++) {
        type_add(&p->numofSubjectType[modunI],
                 make_node('A', 0, 4, 1, 1));
    }
    p->numofSubjectType[modunI].Total_Credit = 20;

    /* [7],[8] modunII/III: present but no passes (group picks best = modunI) */
    type_add(&p->numofSubjectType[modunII],
             make_node('F', 0, 4, 1, 0));
    p->numofSubjectType[modunII].Total_Credit = 20;
    type_add(&p->numofSubjectType[modunIII],
             make_node('F', 0, 4, 1, 0));
    p->numofSubjectType[modunIII].Total_Credit = 20;

    /* [9] modunIV (group 2): 3 × 3c = 9c, all passed */
    for (int i = 0; i < 3; i++) {
        type_add(&p->numofSubjectType[modunIV],
                 make_node('A', 0, 3, 1, 1));
    }
    p->numofSubjectType[modunIV].Total_Credit = 9;

    /* [10] modunV: present but no passes */
    type_add(&p->numofSubjectType[modunV],
             make_node('F', 0, 3, 1, 0));
    p->numofSubjectType[modunV].Total_Credit = 9;

    /* [12] thuc_tap: 1 × 4c, passed */
    type_add(&p->numofSubjectType[thuc_tap],
             make_node('A', 0, 4, 1, 1));
    p->numofSubjectType[thuc_tap].Total_Credit = 4;

    /* [13] do_an_tot_nghiep: 1 × 10c, passed */
    type_add(&p->numofSubjectType[do_an_tot_nghiep],
             make_node('A', 0, 10, 1, 1));
    p->numofSubjectType[do_an_tot_nghiep].Total_Credit = 10;
}

/* ── In-memory DB helpers for DB_ValidateData tests ─────────────────── */
static void db_test_open(void)
{
    if (gDB) { sqlite3_close(gDB); gDB = NULL; }
    sqlite3_open(":memory:", &gDB);
    DB_CreateSchema();
}

static void db_insert_type(int id)
{
    char sql[128];
    snprintf(sql, sizeof(sql),
             "INSERT INTO subject_types(id,name) VALUES(%d,'type%d');", id, id);
    db_exec(sql);
}

static void db_insert_rule(int type_id)
{
    char sql[128];
    snprintf(sql, sizeof(sql),
             "INSERT INTO grad_rules(type_id,mode,limit_val,group_id)"
             " VALUES(%d,0,0,0);", type_id);
    db_exec(sql);
}

static int db_scalar_int(const char *sql)
{
    sqlite3_stmt *st = NULL;
    int v = 0;
    if (sqlite3_prepare_v2(gDB, sql, -1, &st, NULL) == SQLITE_OK) {
        if (sqlite3_step(st) == SQLITE_ROW) v = sqlite3_column_int(st, 0);
    }
    if (st) sqlite3_finalize(st);
    return v;
}

/* ══════════════════════════════════════════════════════════════════════
 * Test groups
 * ══════════════════════════════════════════════════════════════════════ */

/* ── 1. score_to_gpa ──────────────────────────────────────────────── */
static void test_score_to_gpa(void)
{
    GROUP("score_to_gpa");
    FCHECK(score_to_gpa('A', 0), 4.0f, "A   = 4.0");
    FCHECK(score_to_gpa('A', 1), 4.5f, "A+  = 4.5");
    FCHECK(score_to_gpa('B', 0), 3.0f, "B   = 3.0");
    FCHECK(score_to_gpa('B', 1), 3.5f, "B+  = 3.5");
    FCHECK(score_to_gpa('C', 0), 2.0f, "C   = 2.0");
    FCHECK(score_to_gpa('C', 1), 2.5f, "C+  = 2.5");
    FCHECK(score_to_gpa('D', 0), 1.0f, "D   = 1.0");
    FCHECK(score_to_gpa('D', 1), 1.5f, "D+  = 1.5");
    FCHECK(score_to_gpa('F', 0), 0.0f, "F   = 0.0");
    FCHECK(score_to_gpa('X', 0), 0.0f, "X (unknown) = 0.0");
}

/* ── 2. calc_status_alert ─────────────────────────────────────────── */
static void test_calc_status_alert(void)
{
    GROUP("calc_status_alert  (thresholds: 0-7 OK, 8-15 Caution, 16-26 Warning, 27+ Critical)");
    CHECK(calc_status_alert(0)  == 0, "0 failed credits   → alert 0 (OK)");
    CHECK(calc_status_alert(7)  == 0, "7 failed credits   → alert 0 (OK, boundary)");
    CHECK(calc_status_alert(8)  == 1, "8 failed credits   → alert 1 (Caution, boundary)");
    CHECK(calc_status_alert(15) == 1, "15 failed credits  → alert 1 (Caution)");
    CHECK(calc_status_alert(16) == 2, "16 failed credits  → alert 2 (Warning, boundary)");
    CHECK(calc_status_alert(26) == 2, "26 failed credits  → alert 2 (Warning)");
    CHECK(calc_status_alert(27) == 3, "27 failed credits  → alert 3 (Critical, boundary)");
    CHECK(calc_status_alert(99) == 3, "99 failed credits  → alert 3 (Critical)");
}

/* ── 3. calc_cpa ──────────────────────────────────────────────────── */
static void test_calc_cpa(void)
{
    GROUP("calc_cpa");
    Player p;
    memset(&p, 0, sizeof(p));
    node_pool_reset();

    /* empty player → 0.0 */
    FCHECK(calc_cpa(&p, 0), 0.0f, "no subjects → CPA 0.0");

    /* single A (4c, studied, passed) */
    type_add(&p.numofSubjectType[co_so_nganh], make_node('A', 0, 4, 1, 1));
    FCHECK(calc_cpa(&p, 0), 4.0f, "single A (4c) → CPA 4.0");

    /* add a C (2c, studied, NOT passed) */
    type_add(&p.numofSubjectType[co_so_nganh], make_node('C', 0, 2, 1, 0));
    /* pass_only=0: (4*4 + 2*2) / 6 = 20/6 ≈ 3.333 */
    FCHECK(calc_cpa(&p, 0), 20.0f/6.0f, "A(4c)+C(2c) all-studied CPA ≈ 3.33");
    /* pass_only=1: only A counted  → 4*4/4 = 4.0 */
    FCHECK(calc_cpa(&p, 1), 4.0f,        "A(4c)+C(2c) pass-only   CPA = 4.0");
}

/* ── 4. calc_missing_types ─────────────────────────────────────────── */
static void test_calc_missing_types(void)
{
    GROUP("calc_missing_types");
    Player p;
    int out[sizeSubjectType];

    /* Good case: all required types have subjects */
    memset(&p, 0, sizeof(p));
    p.numofSubjectType[co_so_nganh      ].Total_Subject = 5;
    p.numofSubjectType[dai_cuong        ].Total_Subject = 8;
    p.numofSubjectType[the_thao         ].Total_Subject = 4;
    p.numofSubjectType[ly_luat_chinh_tri].Total_Subject = 5;
    p.numofSubjectType[tu_chon          ].Total_Subject = 3;
    p.numofSubjectType[thuc_tap         ].Total_Subject = 1;
    p.numofSubjectType[do_an_tot_nghiep ].Total_Subject = 1;
    CHECK(calc_missing_types(&p, out) == 0,
          "all required types present → 0 missing (GOOD)");
    CHECK(out[co_so_nganh] == 0, "  co_so_nganh not flagged");
    CHECK(out[thuc_tap]    == 0, "  thuc_tap not flagged");

    /* Bad: co_so_nganh has no subjects */
    p.numofSubjectType[co_so_nganh].Total_Subject = 0;
    CHECK(calc_missing_types(&p, out) == 1,
          "co_so_nganh missing → 1 warning (BAD)");
    CHECK(out[co_so_nganh] == 1, "  co_so_nganh flagged");

    /* Bad: multiple required missing */
    p.numofSubjectType[thuc_tap        ].Total_Subject = 0;
    p.numofSubjectType[do_an_tot_nghiep].Total_Subject = 0;
    CHECK(calc_missing_types(&p, out) == 3,
          "co_so_nganh + thuc_tap + do_an missing → 3 warnings (BAD)");
    CHECK(out[do_an_tot_nghiep] == 1, "  do_an_tot_nghiep flagged");

    /* Module slots (6-11) missing → NOT flagged (flexible) */
    memset(&p, 0, sizeof(p));
    p.numofSubjectType[co_so_nganh      ].Total_Subject = 5;
    p.numofSubjectType[dai_cuong        ].Total_Subject = 8;
    p.numofSubjectType[the_thao         ].Total_Subject = 4;
    p.numofSubjectType[ly_luat_chinh_tri].Total_Subject = 5;
    p.numofSubjectType[tu_chon          ].Total_Subject = 3;
    p.numofSubjectType[thuc_tap         ].Total_Subject = 1;
    p.numofSubjectType[do_an_tot_nghiep ].Total_Subject = 1;
    /* modules 6-11 all zero (slots unused in this major) */
    CHECK(calc_missing_types(&p, out) == 0,
          "empty module slots 6-11 → not flagged (flexible by design)");
    CHECK(out[modunI] == 0 && out[modunVI] == 0,
          "  modunI and modunVI both unflagged");
}

/* ── 5. calc_effective_credits ────────────────────────────────────── */
static void test_calc_effective_credits(void)
{
    GROUP("calc_effective_credits");
    setup_real_grad_rules();
    Player p;
    memset(&p, 0, sizeof(p));

    /* Simple standalone types only, no module groups yet */
    p.numofSubjectType[co_so_nganh      ].count_passCredit = 30;
    p.numofSubjectType[dai_cuong        ].count_passCredit = 24;
    p.numofSubjectType[ly_luat_chinh_tri].count_passCredit = 10;
    p.numofSubjectType[tu_chon          ].count_passCredit =  9;
    p.numofSubjectType[thuc_tap         ].count_passCredit =  4;
    p.numofSubjectType[do_an_tot_nghiep ].count_passCredit = 10;
    /* modules: only modunI has credits */
    p.numofSubjectType[modunI ].count_passCredit = 20;
    p.numofSubjectType[modunII].count_passCredit =  0;
    /* group2: modunIV has credits */
    p.numofSubjectType[modunIV].count_passCredit = 9;
    /* the_thao is SUBJECT_COUNT → excluded */
    p.numofSubjectType[the_thao].count_passCredit = 0;
    /* expected = 30+24+10+9+4+10 + best(20,0,0) + best(9,0) = 116 */
    int eff = calc_effective_credits(&p);
    CHECK(eff == 116, "correct effective credits (GOOD) — sport excluded, best module picked");

    /* Module group pick-best: modunII > modunI */
    p.numofSubjectType[modunI ].count_passCredit =  5;
    p.numofSubjectType[modunII].count_passCredit = 25;
    /* expected = 30+24+10+9+4+10 + best(5,25,0) + best(9,0) = 87+25+9 = 121 */
    eff = calc_effective_credits(&p);
    CHECK(eff == 121, "module group picks modunII (25) over modunI (5) (GOOD)");

    /* All module passes zero → group contributes 0 */
    p.numofSubjectType[modunI  ].count_passCredit = 0;
    p.numofSubjectType[modunII ].count_passCredit = 0;
    p.numofSubjectType[modunIII].count_passCredit = 0;
    p.numofSubjectType[modunIV ].count_passCredit = 0;
    p.numofSubjectType[modunV  ].count_passCredit = 0;
    eff = calc_effective_credits(&p);
    CHECK(eff == 87, "zero module credits → groups contribute 0 (BAD scenario)");
}

/* ── 6. calc_required_credits ─────────────────────────────────────── */
static void test_calc_required_credits(void)
{
    GROUP("calc_required_credits");
    setup_real_grad_rules();
    Player p;
    memset(&p, 0, sizeof(p));

    /* Give Total_Credit values that GRAD_TOTAL_CREDIT will resolve to */
    p.numofSubjectType[co_so_nganh      ].Total_Credit = 30;
    p.numofSubjectType[dai_cuong        ].Total_Credit = 24;
    p.numofSubjectType[ly_luat_chinh_tri].Total_Credit = 10;
    /* tu_chon is GRAD_FIXED(9) → uses limit_val=9, not Total_Credit */
    p.numofSubjectType[tu_chon          ].Total_Credit = 99; /* irrelevant */
    p.numofSubjectType[modunI           ].Total_Credit = 20; /* group 1 limit taken from first member */
    p.numofSubjectType[modunII          ].Total_Credit = 20;
    p.numofSubjectType[modunIII         ].Total_Credit = 20;
    p.numofSubjectType[modunIV          ].Total_Credit =  9; /* group 2 */
    p.numofSubjectType[modunV           ].Total_Credit =  9;
    p.numofSubjectType[thuc_tap         ].Total_Credit =  4;
    p.numofSubjectType[do_an_tot_nghiep ].Total_Credit = 10;
    /* the_thao is SUBJECT_COUNT → excluded from credit requirement */

    /* Required = 30 + 24 + 10 + 9(fixed) + 20(grp1,first) + 9(grp2,first) + 4 + 10 = 116 */
    int req = calc_required_credits(&p);
    CHECK(req == 116,
          "required credits = 116 (group counted once, sport excluded, tu_chon fixed=9)");

    /* Halve co_so_nganh: required drops by 15 */
    p.numofSubjectType[co_so_nganh].Total_Credit = 15;
    CHECK(calc_required_credits(&p) == 101,
          "reduced co_so_nganh Total_Credit → required drops accordingly");
}

/* ── 7. update_player_status ──────────────────────────────────────── */
static void test_update_player_status(void)
{
    GROUP("update_player_status");
    setup_real_grad_rules();

    Player p;

    /* Good: meets all requirements */
    setup_passing_player(&p);
    update_player_status(&p);
    CHECK(p.status_can_grauate == 1,
          "player meeting all requirements → can_graduate=1 (GOOD)");

    /* Bad: co_so_nganh credits insufficient */
    setup_passing_player(&p);
    p.numofSubjectType[co_so_nganh].count_passCredit = 0; /* has 9 total but passed 0 */
    update_player_status(&p);
    CHECK(p.status_can_grauate == 0,
          "co_so_nganh passCredits=0 < Total_Credit=9 → can_graduate=0 (BAD)");

    /* Bad: the_thao subject count insufficient */
    setup_passing_player(&p);
    p.numofSubjectType[the_thao].count_passSubject = 3; /* need 4 */
    update_player_status(&p);
    CHECK(p.status_can_grauate == 0,
          "the_thao passSubjects=3 < limit=4 → can_graduate=0 (BAD)");

    /* Bad: tu_chon credits below GRAD_FIXED limit */
    setup_passing_player(&p);
    p.numofSubjectType[tu_chon].count_passCredit = 8; /* need 9 */
    update_player_status(&p);
    CHECK(p.status_can_grauate == 0,
          "tu_chon passCredits=8 < fixed_limit=9 → can_graduate=0 (BAD)");

    /* Bad: module group — no member passes */
    setup_passing_player(&p);
    p.numofSubjectType[modunI  ].count_passCredit = 0;
    p.numofSubjectType[modunII ].count_passCredit = 0;
    p.numofSubjectType[modunIII].count_passCredit = 0;
    update_player_status(&p);
    CHECK(p.status_can_grauate == 0,
          "all module group-1 members have 0 passCredits → can_graduate=0 (BAD)");

    /* Bad: CPA < 2.0 — replace all nodes with D grades (1.0 GPA) */
    setup_passing_player(&p);
    /* Rebuild co_so_nganh with all D grades so overall CPA < 2.0 */
    node_pool_reset();
    p.numofSubjectType[co_so_nganh].head  = NULL;
    p.numofSubjectType[co_so_nganh].tail  = NULL;
    /* Add many D-grade subjects (GPA 1.0) to drag CPA below 2.0        */
    /* Other types still have A grades (GPA 4.0)                        */
    /* To get CPA < 2.0, we need (1.0*weight + 4.0*rest)/(weight+rest) < 2.0
     * => 1.0*W + 4.0*R < 2.0*(W+R) => 2R < W  => W > 2R               */
    /* other types total studied: ~(18+0+10+9+20+20+9+4+10)=100 credits */
    /* add 201 credits of D to co_so_nganh to guarantee CPA < 2.0       */
    for (int i = 0; i < 67; i++) {   /* 67 × 3c = 201c of D */
        Subject_Node *n = make_node('D', 0, 3, 1, 1);
        if (!p.numofSubjectType[co_so_nganh].head)
            p.numofSubjectType[co_so_nganh].head = n;
        else
            p.numofSubjectType[co_so_nganh].tail->next = n;
        p.numofSubjectType[co_so_nganh].tail = n;
    }
    p.numofSubjectType[co_so_nganh].count_passCredit = 201;
    p.numofSubjectType[co_so_nganh].Total_Credit     = 201;
    update_player_status(&p);
    CHECK(p.status_can_grauate == 0,
          "CPA < 2.0 (mass D grades) → can_graduate=0 (BAD)");

    /* Good: alert level from ToTal_credit_npass */
    setup_passing_player(&p);
    p.ToTal_credit_npass = 20;
    update_player_status(&p);
    CHECK(p.status_alert == 2, "20 failed credits → alert 2 (Warning)");
}

/* ── 8. DB_ValidateData ───────────────────────────────────────────── */
static void test_db_validate(void)
{
    GROUP("DB_ValidateData");
    static const int required_ids[] = {1, 2, 3, 4, 5, 12, 13};
    int nreq = 7;

    /* GOOD: all required IDs in both tables → 0 warnings */
    db_test_open();
    for (int k = 0; k < nreq; k++) {
        db_insert_type(required_ids[k]);
        db_insert_rule(required_ids[k]);
    }
    /* also add a module (id=6) in both — should be fine */
    db_insert_type(6);
    db_insert_rule(6);
    DB_ValidateData();
    CHECK(gDataWarnCount == 0,
          "all required IDs in both tables → 0 warnings (GOOD)");

    /* BAD: required ID=1 missing from subject_types */
    db_test_open();
    for (int k = 0; k < nreq; k++) db_insert_rule(required_ids[k]);
    for (int k = 1; k < nreq; k++) db_insert_type(required_ids[k]); /* skip id=1 */
    DB_ValidateData();
    CHECK(gDataWarnCount >= 2,
          "id=1 missing from subject_types → at least 2 warnings (BAD)");

    /* BAD: required ID=12 missing from grad_rules */
    db_test_open();
    for (int k = 0; k < nreq; k++) db_insert_type(required_ids[k]);
    for (int k = 0; k < nreq - 1; k++) db_insert_rule(required_ids[k]); /* skip id=13 */
    DB_ValidateData();
    int w1 = gDataWarnCount;
    CHECK(w1 >= 1,
          "id=13 missing from grad_rules → at least 1 warning (BAD)");

    /* BAD: grad_rules has id=7 (module) with no subject_types entry */
    db_test_open();
    for (int k = 0; k < nreq; k++) {
        db_insert_type(required_ids[k]);
        db_insert_rule(required_ids[k]);
    }
    db_insert_rule(7); /* orphaned rule — no matching subject_type */
    DB_ValidateData();
    CHECK(gDataWarnCount >= 1,
          "orphaned rule for id=7 (no subject_types entry) → warning (BAD)");

    /* BAD: subject_types has id=8 with no grad_rules entry */
    db_test_open();
    for (int k = 0; k < nreq; k++) {
        db_insert_type(required_ids[k]);
        db_insert_rule(required_ids[k]);
    }
    db_insert_type(8); /* type with no rule */
    DB_ValidateData();
    CHECK(gDataWarnCount >= 1,
          "subject_types id=8 has no grad_rules entry → warning (BAD)");

    /* BAD: multiple required IDs missing — all reported */
    db_test_open();
    /* only put ids 1,2 in subject_types; only ids 1,2 in grad_rules */
    db_insert_type(1); db_insert_type(2);
    db_insert_rule(1); db_insert_rule(2);
    DB_ValidateData();
    /* missing from subject_types: 3,4,5,12,13 = 5 warnings
       missing from grad_rules:    3,4,5,12,13 = 5 warnings
       total >= 10                                           */
    CHECK(gDataWarnCount >= 10,
          "5 required IDs missing from both tables → >=10 warnings (BAD)");

    /* Clean up */
    if (gDB) { sqlite3_close(gDB); gDB = NULL; }
}

    /* ── 9. duplicate subject code across types ───────────────────────── */
    static void test_db_duplicate_code_across_types(void)
    {
        GROUP("DB duplicate code across types");
        db_test_open();

        db_insert_type(6);
        db_insert_type(7);

        db_exec("INSERT INTO subjects(code,name,type_id,credit,term) VALUES('IT4244','M1',6,2,6);");
        db_exec("INSERT INTO subjects(code,name,type_id,credit,term) VALUES('IT4244','M2',7,2,6);");
        db_exec("INSERT OR IGNORE INTO subjects(code,name,type_id,credit,term) VALUES('IT4244','M1dup',6,2,6);");

        CHECK(db_scalar_int("SELECT COUNT(*) FROM subjects WHERE code='IT4244';") == 2,
            "same code in different types is preserved; same code+type is deduped");

        db_exec("INSERT OR IGNORE INTO subject_scores(subject_id,score_letter,mid,final,pass,ever_studied)"
            " SELECT id,'X',0.0,0.0,0,0 FROM subjects WHERE code='IT4244';");

        int changed = DB_UpdateScore("IT4244", 8.0f, 8.0f);
        CHECK(changed == 2,
            "DB_UpdateScore updates all duplicated rows sharing same code");

        CHECK(db_scalar_int("SELECT COUNT(*) FROM subject_scores sc"
                    " JOIN subjects s ON s.id=sc.subject_id"
                    " WHERE s.code='IT4244' AND sc.pass=1 AND sc.ever_studied=1;") == 2,
            "both duplicate rows have updated pass/studied flags");

        Player p;
        memset(&p, 0, sizeof(p));
        DB_Query(&p);
        CHECK(p.numofSubjectType[6].Total_Subject == 1 && p.numofSubjectType[7].Total_Subject == 1,
            "DB_Query keeps duplicates in separate type sections for rendering");

        if (gDB) { sqlite3_close(gDB); gDB = NULL; }
    }

/* ══════════════════════════════════════════════════════════════════════
 * main
 * ══════════════════════════════════════════════════════════════════════ */
int main(void)
{
    printf("==========================================================\n");
    printf("  Student Transcript Viewer — Logic Unit Tests\n");
    printf("==========================================================\n");

    test_score_to_gpa();
    test_calc_status_alert();
    test_calc_cpa();
    test_calc_missing_types();
    test_calc_effective_credits();
    test_calc_required_credits();
    test_update_player_status();
    test_db_validate();
    test_db_duplicate_code_across_types();

    printf("\n==========================================================\n");
    printf("  Results:  %d passed,  %d failed\n", t_pass, t_fail);
    printf("==========================================================\n");

    return t_fail > 0 ? 1 : 0;
}
