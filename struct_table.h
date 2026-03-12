#ifndef STRUCT_TABLE_H
#define STRUCT_TABLE_H

#define MAXSIZENAME 256
#define MAXSIZEID 26
#define MAX_COLUMNS 10
#define MAX_COLUMN_WIDTH 56

typedef struct Subject_Node {
    char name[MAXSIZENAME];
    char ID[MAXSIZEID];
    char score_letter;
    float score_number_mid;
    float  score_number_final;
    unsigned int status_pass : 1;
    unsigned int status_ever_been_study: 2;
    unsigned int credit : 4;
    unsigned int term_recomment_to_studie: 4;
    struct Subject_Node *next;
} Subject_Node;

typedef struct _Subject_Type {
    int Total_Subject;
    unsigned int count_passSubject;
    unsigned int count_passCredit; // ADD NEW
    int Total_Credit;
    char nameoftype[MAXSIZENAME];
    Subject_Node* head;
    Subject_Node* tail;
} Subject_Type;

typedef enum {
    _slot_unused = 0,     /* reserved — IDs start at 1                         */
    co_so_nganh = 1,      /* core major subjects                               */
    dai_cuong = 2,        /* general education subjects                        */
    the_thao = 3,         /* sport/physical education (counted by subject)     */
    ly_luat_chinh_tri = 4,/* law & politics                                    */
    tu_chon = 5,          /* supplementary/elective knowledge                 */
    modunI   = 6,         /* module slots 6-11: flexible per major             */
    modunII  = 7,
    modunIII = 8,
    modunIV  = 9,
    modunV   = 10,
    modunVI  = 11,        /* extra slot; unused modules stay empty             */
    thuc_tap = 12,        /* internship                                        */
    do_an_tot_nghiep = 13,/* thesis / graduation project                       */
    sizeSubjectType = 14  /* total array size (index 0 unused)                 */
} index_subject_type;

typedef struct Player {
    char name_player[MAXSIZENAME];
    int ToTal_credit_pass; // ADD NEW
    int ToTal_credit_npass;
    Subject_Type numofSubjectType[sizeSubjectType];
    unsigned int status_can_grauate : 1;
    unsigned int status_alert : 2;
} Player;

/* ── Graduation rule for one subject type ────────────────────────────────
 *  Loaded from the user DB (grad_rules table), originally seeded from
 *  assets/grad_config.cfg.  Used exclusively by score_logic.h.
 * ──────────────────────────────────────────────────────────────────────── */
typedef enum {
    GRAD_TOTAL_CREDIT  = 0,  /* must pass all credits defined for this type */
    GRAD_FIXED         = 1,  /* must pass exactly limit_val credits          */
    GRAD_SUBJECT_COUNT = 2,  /* must pass limit_val subjects (not credits)   */
} GradMode;

typedef struct {
    int mode;      /* GradMode value                                          */
    int limit_val; /* explicit limit for FIXED/SUBJECT_COUNT; 0 = auto       */
    int group_id;  /* 0=standalone; same nonzero id = "pick best of group"   */
} GradRule;

#endif